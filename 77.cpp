#include "78.h"
#include "86.h"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <stack>
#include <functional>

namespace slime::pass {

using namespace ir;

void MemoryToRegisterPass::runOnFunction(Function *target) {
    std::set<Value *>       promotableVarSet;
    std::set<Instruction *> deleteLater;

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

    //! simplify inblock memory operations
    for (auto block : sortedBlocks) {
        if (dfn[block] == -1) { continue; }

        auto &instrs   = block->instructions();
        auto  instIter = instrs.begin();

        std::map<Value *, StoreInst *> def;
        while (instIter != instrs.end()) {
            auto inst = *instIter++;

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
                    auto ok = lastStore->removeFromBlock();
                    assert(ok);
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
                    //! value is from preds or is a non-def
                    continue;
                }
                std::vector<Use *> uses(
                    load->uses().begin(), load->uses().end());
                for (auto use : uses) { use->reset(lastStore->rhs()); }
                auto ok = load->removeFromBlock();
                assert(ok);
                continue;
            }
        }
    }

    //! remove unused alloca
    for (auto var : promotableVarSet) {
        auto alloca  = var->asInstruction()->asAlloca();
        bool hasUser = false;
        for (auto use : alloca->uses()) {
            auto inst = use->owner()->asInstruction();
            if (inst->id() == InstructionID::Load) {
                auto load = inst->asLoad();
                if (load->uses().size() > 0) {
                    hasUser = true;
                    break;
                }
            }
        }
        if (!hasUser) {
            std::vector<Use *> uses(
                alloca->uses().begin(), alloca->uses().end());
            for (auto use : uses) {
                auto ok = use->owner()->asInstruction()->removeFromBlock();
                assert(ok);
            }
            deleteLater.insert(alloca);
        }
    }
    for (auto var : deleteLater) {
        auto ok = var->removeFromBlock();
        assert(ok);
        promotableVarSet.erase(var->unwrap());
    }
    deleteLater.clear();

    //! compute dom tree and frontiers
    BlockMap    idom;
    BlockSetMap domfr;
    BlockSetMap idomSuccs;
    computeDomFrontier(idom, domfr, target);
    for (auto &[succ, dominator] : idom) { idomSuccs[dominator].insert(succ); }

    //! place phi node
    std::map<PhiInst *, AllocaInst *> phiSource;
    std::set<PhiInst *>               phiSet;
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
                phiSet.insert(phi);
                visted.insert(frontier);
                worklist.push(frontier);
            }
        }
    }

    //! place non-def load
    std::set<LoadInst *> nondefSet;
    for (auto var : promotableVarSet) {
        auto nondef = LoadInst::create(var);
        nondefSet.insert(nondef);
        nondef->insertAfter(var->asInstruction());
    }

    //! compute reach defines
    using DefineListTable = std::map<Value *, std::set<Instruction *>>;
    std::map<BasicBlock *, DefineListTable> reachDefines;
    std::set<StoreInst *>                   storeSet;
    for (auto block : sortedBlocks) {
        if (dfn[block] == -1) { continue; }
        std::map<Value *, Instruction *> defineTable;

        auto &incomingValues = reachDefines[block];
        auto &instrs         = block->instructions();
        auto  instIter       = instrs.begin();
        while (instIter != instrs.end()) {
            auto inst = *instIter++;
            //! define from phi
            if (inst->id() == InstructionID::Phi) {
                auto phi    = inst->asPhi();
                auto target = phiSource[phi];
                assert(promotableVarSet.count(target));
                defineTable[target] = phi;
                continue;
            }
            //! define from store
            if (inst->id() == InstructionID::Store) {
                auto store  = inst->asStore();
                auto target = store->lhs();
                if (!promotableVarSet.count(target)) { continue; }
                defineTable[target] = store;
                storeSet.insert(store);
                continue;
            }
            //! nondef from load
            if (inst->id() == InstructionID::Load) {
                auto load   = inst->asLoad();
                auto target = load->operand();
                if (!nondefSet.count(load)) { continue; }
                defineTable[target] = load;
                continue;
            }
        }

        std::vector<BasicBlock *> succs;
        if (block->isBranched()) {
            succs.push_back(block->branch());
            succs.push_back(block->branchElse());
        } else if (block->isLinear() && !block->isTerminal()) {
            succs.push_back(block->branch());
        }
        for (auto succ : succs) {
            auto &incomings = reachDefines[succ];
            for (auto &[var, def] : defineTable) { incomings[var].insert(def); }
            for (auto &[var, defs] : incomingValues) {
                if (!defineTable.count(var)) {
                    auto &incomings = reachDefines[succ];
                    incomings[var].insert(defs.begin(), defs.end());
                }
            }
        }
    }

    //! apply reach defines
    for (auto block : sortedBlocks) {
        if (dfn[block] == -1) { continue; }
        std::map<Value *, Instruction *> defineTable;

        auto &incomingValues = reachDefines[block];
        auto &instrs         = block->instructions();
        auto  instIter       = instrs.begin();
        while (instIter != instrs.end()) {
            auto inst = *instIter++;

            if (inst->id() == InstructionID::Phi) {
                auto phi    = inst->asPhi();
                auto target = phiSource[phi];
                for (auto value : incomingValues[target]) {
                    auto source = value->parent();
                    auto define = value->unwrap();
                    if (value->id() == InstructionID::Store) {
                        define = value->asStore()->rhs();
                    }
                    phi->addIncomingValue(define, source);
                }
                defineTable[target] = phi;
                continue;
            }

            if (inst->id() == InstructionID::Store) {
                auto store  = inst->asStore();
                auto target = store->lhs();
                if (!promotableVarSet.count(target)) { continue; }
                defineTable[target] = store;
                continue;
            }

            if (inst->id() == InstructionID::Load) {
                auto load   = inst->asLoad();
                auto target = load->operand();
                if (!promotableVarSet.count(target)) { continue; }
                if (nondefSet.count(load)) {
                    defineTable[target] = load;
                    continue;
                }

                Value *value = nullptr;
                if (defineTable.count(target)) {
                    value = defineTable[target]->unwrap();
                } else if (
                    incomingValues.count(target)
                    && incomingValues[target].size() == 1) {
                    value = (*incomingValues[target].begin())->unwrap();
                }
                if (value->asInstruction()->id() == InstructionID::Store) {
                    value = value->asInstruction()->asStore()->rhs();
                }
                assert(value != nullptr);

                std::vector<Use *> uses(
                    load->uses().begin(), load->uses().end());
                for (auto use : uses) { use->reset(value); }

                auto ok = load->removeFromBlock();
                assert(ok);

                continue;
            }
        }
    }

    //! clean up insts
    for (auto phi : phiSet) {
        bool shouldRemove = phi->uses().size() == 0;
        if (auto singleIncoming = phi->totalOperands() == 2) {
            auto source = static_cast<BasicBlock *>(phi->op()[1].value());
            auto value  = phi->op()[0];
            std::vector<Use *> uses(phi->uses().begin(), phi->uses().end());
            for (auto use : uses) {
                auto owner = use->owner()->asInstruction();
                use->reset(value);
                if (owner->id() == InstructionID::Phi) {
                    (use + 1)->reset(source);
                }
            }
            shouldRemove = true;
        }
        if (shouldRemove) {
            auto ok = phi->removeFromBlock();
            assert(ok);
        }
    }

    for (auto load : nondefSet) {
        if (load->uses().size() == 0) {
            auto ok = load->removeFromBlock();
            assert(ok);
            deleteLater.insert(load);
        }
    }
    for (auto load : deleteLater) { nondefSet.erase(load->asLoad()); }
    deleteLater.clear();

    for (auto store : storeSet) {
        auto ok = store->removeFromBlock();
        assert(ok);
    }

    for (auto alloca : promotableVarSet) {
        assert(alloca->uses().size() <= 1);
        if (alloca->uses().size() == 0) {
            auto ok = alloca->asInstruction()->asAlloca()->removeFromBlock();
            assert(ok);
        }
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