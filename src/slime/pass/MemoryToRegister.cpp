#include "MemoryToRegister.h"
#include "ValueNumbering.h"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <stack>

namespace slime::pass {

using namespace ir;

void MemoryToRegisterPass::runOnFunction(Function *target) {
    using DefineMap = std::map<AllocaInst *, std::vector<Instruction *>>;
    std::set<Value *> promotableVarSet;
    std::set<Value *> maybeUseBeforeDef;

    auto &blocks = target->basicBlocks();

    //! compute deep first order
    std::map<BasicBlock *, int> dfn;
    int                         dfOrder = -1;
    for (auto block : blocks) { dfn[block] = dfOrder; }
    std::stack<BasicBlock *> dfStack;
    dfStack.push(target->front());
    while (!dfStack.empty()) {
        auto block = dfStack.top();
        dfStack.pop();
        if (dfn[block] != -1) { continue; }
        dfn[block] = ++dfOrder;
        if (block->isBranched()) {
            dfStack.push(block->branch());
            dfStack.push(block->branchElse());
        } else if (block->isLinear() && !block->isTerminal()) {
            dfStack.push(block->branch());
        }
    }
    std::vector<BasicBlock *> sortedBlocks(blocks.begin(), blocks.end());
    std::sort(
        sortedBlocks.begin(),
        sortedBlocks.end(),
        [&dfn](const auto &lhs, const auto &rhs) {
            return dfn[lhs] < dfn[rhs];
        });

    //! simplify inblock memory operation
    std::map<Value *, StoreInst *> def;
    for (auto block : sortedBlocks) {
        if (dfn[block] == -1) { continue; }
        auto &instrs = block->instructions();
        auto  iter   = instrs.begin();
        while (iter != instrs.end()) {
            auto inst = *iter++;

            //! get promotable alloca
            if (inst->id() == InstructionID::Alloca) {
                auto alloca     = inst->asAlloca();
                bool promotable = true;
                for (auto use : alloca->uses()) {
                    const auto id = use->owner()->asInstruction()->id();
                    if (id != InstructionID::Load
                        && id != InstructionID::Store) {
                        promotable = false;
                        break;
                    }
                }
                if (promotable) { promotableVarSet.insert(alloca); }
                continue;
            }

            //! update most recent store
            if (inst->id() == InstructionID::Store) {
                auto store  = inst->asStore();
                auto target = store->lhs();
                if (!promotableVarSet.count(target)) { continue; }
                auto &lastStore = def[target];
                if (lastStore != nullptr) {
                    if (lastStore->parent() == store->parent()) {
                        auto ok = lastStore->removeFromBlock();
                        assert(ok);
                    }
                }
                lastStore = store;
                continue;
            }

            //! forward use-def
            if (inst->id() == InstructionID::Load) {
                auto load   = inst->asLoad();
                auto target = load->operand();
                if (!promotableVarSet.count(target)) { continue; }
                if (load->uses().size() == 0) {
                    auto ok = load->removeFromBlock();
                    assert(ok);
                    continue;
                }
                auto &lastStore = def[target];
                if (lastStore == nullptr) {
                    maybeUseBeforeDef.insert(target);
                    continue;
                }
                //! FIXME: store may not dom load
                if (lastStore->parent() == load->parent()) {
                    std::vector<Use *> uses(
                        load->uses().begin(), load->uses().end());
                    for (auto use : uses) { use->reset(lastStore->rhs()); }
                    assert(load->uses().size() == 0);
                    auto ok = load->removeFromBlock();
                    assert(ok);
                }
                continue;
            }
        }
    }

    //! remove unused alloca
    std::set<Instruction *> deleteLater;
    for (auto var : promotableVarSet) {
        auto       alloca    = var->asInstruction()->asAlloca();
        bool       hasUser   = false;
        size_t     totalDef  = 0;
        StoreInst *singleDef = nullptr;

        //! collect use-def information
        for (auto use : alloca->uses()) {
            auto user = use->owner()->asInstruction();
            if (user->id() == InstructionID::Store) {
                singleDef = user->asStore();
                ++totalDef;
            } else {
                assert(user->id() == InstructionID::Load);
                hasUser = true;
            }
        }
        bool shouldRemove = !hasUser || totalDef == 1;

        //! forward single store
        if (totalDef == 1) {
            auto value = singleDef->rhs();
            auto ok    = singleDef->removeFromBlock();
            assert(ok);
            std::vector<Use *> uses;
            for (auto use : alloca->uses()) {
                auto load = use->owner()->asInstruction()->asLoad();
                uses.insert(
                    uses.end(), load->uses().begin(), load->uses().end());
            }
            for (auto use : uses) { use->reset(value); }
        }

        //! remove no use store/load
        if (shouldRemove) {
            std::vector<Use *> uses(
                alloca->uses().begin(), alloca->uses().end());
            for (auto use : uses) {
                auto ok = use->owner()->asInstruction()->removeFromBlock();
                assert(ok);
            }
            deleteLater.insert(alloca);
        }
    }
    for (auto inst : deleteLater) {
        auto ok = inst->removeFromBlock();
        assert(ok);
        promotableVarSet.erase(inst->unwrap());
    }
    deleteLater.clear();

    //! compute dom tree and frontiers
    BlockMap    idom;
    BlockSetMap domfr;
    BlockSetMap idomSuccs;
    computeDomFrontier(idom, domfr, target);
    for (auto &[succ, dominator] : idom) { idomSuccs[dominator].insert(succ); }

    //! place phi node
    std::map<PhiInst *, AllocaInst *>           phiSource;
    std::map<BasicBlock *, std::set<PhiInst *>> phiNodes;
    for (auto var : promotableVarSet) {
        std::set<BasicBlock *>   visted;
        std::stack<BasicBlock *> worklist;

        auto alloca = var->asInstruction()->asAlloca();
        for (auto use : alloca->uses()) {
            auto user = use->owner()->asInstruction();
            if (user->id() != InstructionID::Store) { continue; }
            auto store = user->asStore();
            worklist.push(store->parent());
        }

        while (!worklist.empty()) {
            auto df = worklist.top();
            worklist.pop();
            for (auto frontier : domfr[df]) {
                if (visted.count(frontier)) { continue; }
                auto phi = PhiInst::create(alloca->type()->tryGetElementType());
                phi->insertToHead(frontier);
                phiSource[phi] = alloca;
                phiNodes[frontier].insert(phi);
                visted.insert(frontier);
                worklist.push(frontier);
            }
        }
    }

    //! compute reach defines
    std::vector<std::map<Value *, Value *>> reachDefines(blocks.size());
    std::map<Value *, BasicBlock *>         defSource;
    for (auto block : sortedBlocks) {
        auto &reachDefine = reachDefines[dfn[block]];
        for (auto phi : phiNodes[block]) {}
        auto &instrs = block->instructions();
        auto  iter   = instrs.begin();
        while (iter != instrs.end()) {
            auto inst = *iter++;
            if (inst->id() == InstructionID::Phi) {
                auto phi                    = inst->asPhi();
                reachDefine[phiSource[phi]] = phi;
                defSource[phi]              = block;
                continue;
            }
            if (inst->id() == InstructionID::Store) {
                auto store  = inst->asStore();
                auto source = store->lhs();
                if (!promotableVarSet.count(source)) { continue; }
                reachDefine[source]     = store->rhs();
                defSource[store->rhs()] = block;
                auto ok                 = inst->removeFromBlock();
                assert(ok);
                continue;
            }
            if (inst->id() == InstructionID::Load) {
                auto load   = inst->asLoad();
                auto source = load->operand();
                if (!promotableVarSet.count(source)) { continue; }
                auto value = reachDefine[source];
                assert(value != nullptr);
                std::vector<Use *> uses(
                    load->uses().begin(), load->uses().end());
                for (auto use : uses) { use->reset(value); }
                auto ok = inst->removeFromBlock();
                assert(ok);
                continue;
            }
        }
        for (auto [var, def] : reachDefine) {
            if (var->uses().size() == 0) { continue; }
            if (block->isBranched()) {
                reachDefines[dfn[block->branch()]][var]     = def;
                reachDefines[dfn[block->branchElse()]][var] = def;
            } else if (block->isLinear() && !block->isTerminal()) {
                reachDefines[dfn[block->branch()]][var] = def;
            }
        }
    }

    //! add incoming values to phi
    std::stack<PhiInst *> stack;
    for (auto &[block, nodes] : phiNodes) {
        for (auto phi : nodes) {
            auto              source = phiSource[phi];
            std::set<Value *> incomings;
            for (auto pred : block->inBlocks()) {
                //! FIXME: assert: reachDefines[dfn[pred]][source] != nullptr
                if (reachDefines[dfn[pred]].count(source)) {
                    incomings.insert(reachDefines[dfn[pred]][source]);
                }
            }
            for (auto incoming : incomings) {
                if (incoming->isInstruction()
                    && incoming->asInstruction()->id() == InstructionID::Phi) {
                    auto incomingPhi = incoming->asInstruction()->asPhi();
                    for (int i = 0; i < incomingPhi->totalOperands(); i += 2) {
                        phi->addIncomingValue(
                            incomingPhi->operands()[i],
                            static_cast<BasicBlock *>(
                                incomingPhi->operands()[i + 1].value()));
                    }
                } else {
                    phi->addIncomingValue(incoming, defSource[incoming]);
                }
            }
            stack.push(phi);
        }
    }
    while (!stack.empty()) {
        auto phi = stack.top();
        stack.pop();
        if (!phi->parent()) { continue; }
        if (phi->uses().size() == 0) {
            for (int i = 0; i < phi->totalUse(); i += 2) {
                auto op = phi->useAt(i);
                if (op->isInstruction()
                    && op->asInstruction()->id() == InstructionID::Phi) {
                    auto incomingPhi = op->asInstruction()->asPhi();
                    stack.push(incomingPhi);
                }
            }
            auto ok = phi->removeFromBlock();
            assert(ok);
        }
    }

    //! remove allocas
    for (auto alloca : promotableVarSet) {
        auto ok = alloca->asInstruction()->removeFromBlock();
        assert(ok);
    }
}

void MemoryToRegisterPass::computeDomFrontier(
    BlockMap &idom, BlockSetMap &domfr, Function *target) {
    const auto n = target->size();

    //! NOTE: using linked list to represent graph
    using edge_t = struct {
        int succ;
        int nextEdge;
    };

    std::vector<edge_t> edges;
    std::vector<int>    graph(n, -1);
    std::vector<int>    graphInv(n, -1);
    std::vector<int>    semiTree(n, -1);

    //! encode BasicBlock* into int
    std::map<BasicBlock *, int> index;
    std::vector<BasicBlock *>   revIndex(n);
    int                         i = 0;
    for (auto block : target->basicBlocks()) {
        index[block]  = i;
        revIndex[i++] = block;
    }

    //! build graph
    for (auto block : target->basicBlocks()) {
        for (auto pred : block->inBlocks()) {
            auto from = index[pred];
            auto to   = index[block];
            edges.push_back({to, graph[from]});
            graph[from] = edges.size() - 1;
            edges.push_back({from, graphInv[to]});
            graphInv[to] = edges.size() - 1;
        }
    }

    std::vector<int> idom_(n);

    //! Lengauer-Tarjan algorithm
    //! 1. compute deep-first order
    //! 2. compute semi-dom
    //! 3. compute idom

    //! dfn[k] := deep-first order of node k

    //! semi[k] := x with min dfn[x],
    //!     where x->xi...->k, dfn[xi] > dfn[k], i >= 1

    //! if dfn[x] < dfn[k] then semi[k] may be x
    //! if dfn[x] > dfn[k] then semi[k] may be semi[u],
    //!     where u is ancestor of k, dfn[u] > dfn[k],
    //!     where x->k

    //! if x = v then idom[k] = x
    //! if dfn[x] > dfn[v] then idom[k] = idom[u],
    //!     where x = semi[k], k->...->x, v = semi[u],
    //!     where u with min dfn[u], u belongs k->...

    std::vector<int> dfn(n, -1);
    std::vector<int> father(n);
    std::vector<int> unionSet(n);
    std::vector<int> semi(n);
    std::vector<int> minNode(n);
    std::vector<int> nodeAt;

    std::function<void(int)> tarjan = [&](int k) {
        dfn[k] = nodeAt.size();
        nodeAt.push_back(k);
        for (int i = graph[k]; i != -1; i = edges[i].nextEdge) {
            if (dfn[edges[i].succ] == -1) {
                father[edges[i].succ] = k;
                tarjan(edges[i].succ);
            }
        }
    };

    std::function<int(int)> query = [&](int k) {
        if (k == unionSet[k]) { return k; }
        int result = query(unionSet[k]);
        if (dfn[semi[minNode[unionSet[k]]]] < dfn[semi[minNode[k]]]) {
            minNode[k] = minNode[unionSet[k]];
        }
        unionSet[k] = result;
        return result;
    };

    //! compute deep-first order
    tarjan(0);

    //! initialize
    for (int i = 0; i < n; ++i) {
        semi[i]     = i;
        unionSet[i] = i;
        minNode[i]  = i;
    }

    //! process in reverse dfn order
    for (int i = nodeAt.size() - 1; i > 0; --i) {
        //! compute semi-dom
        int t = nodeAt[i];
        for (int i = graphInv[t]; i != -1; i = edges[i].nextEdge) {
            auto succ = edges[i].succ;
            if (dfn[succ] == -1) { continue; }
            query(succ);
            if (dfn[semi[minNode[succ]]] < dfn[semi[t]]) {
                semi[t] = semi[minNode[succ]];
            }
        }
        unionSet[t] = father[t];

        //! update semi tree
        edges.push_back({t, semiTree[semi[t]]});
        semiTree[semi[t]] = edges.size() - 1;

        //! compute idom
        t = father[t];
        for (int i = semiTree[t]; i != -1; i = edges[i].nextEdge) {
            auto succ = edges[i].succ;
            query(succ);
            idom_[succ] = t == semi[minNode[succ]] ? t : minNode[succ];
        }

        //! reset semi tree
        semiTree[t] = -1;
    }

    //! post process to finalize idom
    for (int i = 1; i < nodeAt.size(); ++i) {
        auto t = nodeAt[i];
        if (idom_[t] != semi[t]) { idom_[t] = idom_[idom_[t]]; }
    }

    //! compute dom frontier by idom
    idom.clear();
    for (auto block : target->basicBlocks()) {
        idom[block] = revIndex[idom_[index[block]]];
    }

    domfr.clear();
    for (auto block : target->basicBlocks()) { domfr[block].clear(); }

    for (auto block : target->basicBlocks()) {
        if (block->inBlocks().size() <= 1) { continue; }
        for (auto pred : block->inBlocks()) {
            auto runner = pred;
            while (runner != idom[block]) {
                domfr[runner].insert(block);
                runner = idom[runner];
            }
        }
    }

    std::vector<BasicBlock *> deleteLater;
    for (auto [succ, dominator] : idom) {
        if (succ == dominator) { deleteLater.push_back(succ); }
    }
    for (auto block : deleteLater) { idom.erase(block); }
}

} // namespace slime::pass
