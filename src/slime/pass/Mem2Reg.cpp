#include "Mem2Reg.h"

#include <list>
#include <queue>
#include <stack>
#include <set>
#include <algorithm>

namespace slime::pass {

using namespace ir;

void Mem2RegPass::runOnFunction(ir::Function *target) {
    if (target->size() == 0) { return; }

    std::set<Value *>                         allocas;
    std::map<Value *, std::set<StoreInst *>>  defines;
    std::map<Value *, std::set<BasicBlock *>> definedIn;
    std::map<Value *, std::set<LoadInst *>>   usedIn;

    //! 1. eliminate needless load
    //! 2. get sources of use-def
    //! 3. get allocas
    for (auto block : target->basicBlocks()) {
        std::map<Value *, StoreInst *> defineList;
        auto                           it = block->instructions().begin();
        while (it != block->instructions().end()) {
            auto inst = *it++;

            if (inst->id() == InstructionID::Alloca) {
                allocas.insert(inst->asAlloca());
                continue;
            }

            //! call may cause define to be invalid
            if (inst->id() == InstructionID::Call) {
                auto call = inst->asCall();
                for (int i = 0; i < call->totalParams(); ++i) {
                    if (auto address = call->paramAt(i).value();
                        allocas.count(address)) {
                        defineList.erase(address);
                    }
                }
                continue;
            }

            if (inst->id() == InstructionID::Store) {
                auto  store     = inst->asStore();
                auto &defBlocks = definedIn[store->lhs()];
                defBlocks.insert(block);
                if (defineList.count(store)) {
                    //! remove out-dated in-block store
                    auto ok = defineList.at(store->lhs())->removeFromBlock();
                    assert(ok);
                }
                defineList[store->lhs()] = store;
                continue;
            }

            if (inst->id() == InstructionID::Load) {
                auto load = inst->asLoad();
                if (defineList.count(load->operand())) {
                    auto value = defineList.at(load->operand())->rhs();
                    std::vector<Use *> uses(
                        load->uses().begin(), load->uses().end());
                    for (auto use : uses) { use->reset(value); }
                    auto ok = load->removeFromBlock();
                    assert(ok);
                } else {
                    auto &usedBlocks = usedIn[load->operand()];
                    usedBlocks.insert(load);
                }
                continue;
            }
        }
        for (auto [address, store] : defineList) {
            if (!address->isInstruction()) { continue; }
            auto alloca = address->asInstruction();
            if (alloca->id() != InstructionID::Alloca) { continue; }
            defines[address].insert(store);
        }
    }

    //! compute dominance frontiers
    solveDominanceFrontier(target);

    //! phi insertion & renaming algorithm
    //! ref: https://szp15.com/post/how-to-construct-ssa/

    phiParent_.clear();
    for (auto value : allocas) {
        if (defines[value].empty()) { continue; }

        auto alloca = value->asInstruction()->asAlloca();
        // auto phi =
        // Instruction::createPhi(value->type()->tryGetElementType()); for (auto
        // inst : defines[value]) {
        //     auto store = inst->asStore();
        //     assert(store->lhs() == value);
        //     phi->addIncomingValue(store->rhs(), store->parent());
        //     auto ok = store->removeFromBlock();
        //     assert(ok);
        // }

        std::queue<BasicBlock *> worklist;
        std::set<BasicBlock *>   visited;
        std::set<BasicBlock *>   placed;
        for (auto block : definedIn[alloca]) { worklist.push(block); }

        int i = 0;
        while (!worklist.empty()) {
            auto block = worklist.front();
            worklist.pop();
            for (auto df : domfrSetList_[block]) {
                if (placed.count(df)) { continue; }
                placed.insert(df);
                auto phi =
                    Instruction::createPhi(alloca->type()->tryGetElementType());
                for (auto inst : defines[alloca]) {
                    auto store = inst->asStore();
                    assert(store->lhs() == alloca);
                    phi->addIncomingValue(store->rhs(), store->parent());
                }
                phi->insertToHead(df);
                phiParent_[phi] = alloca;
                if (!visited.count(df)) {
                    visited.insert(df);
                    worklist.push(df);
                }
            }
        }
    }

    for (auto alloca : allocas) {
        valueStacks_[alloca->asInstruction()->asAlloca()] = {};
    }
    blockVisited_.clear();
    searchAndRename(target->front());

    for (auto alloca : allocas) {
        if (alloca->uses().size() == 0) {
            alloca->asInstruction()->asAlloca()->removeFromBlock();
        }
    }

    return;

    //! eliminate phi instruction
    for (auto block : target->basicBlocks()) {
        auto &instructions = block->instructions();
        auto  it           = instructions.begin();
        while (it != instructions.end()) {
            auto inst = *it++;
            if (inst->id() != InstructionID::Phi) { continue; }
            auto phi = inst->asPhi();
            assert(phi->totalUse() > 0);
            auto address = Instruction::createAlloca(phi->type());
            address->insertToHead(target->front());
            for (int i = 0; i < phi->totalUse(); i += 2) {
                auto user   = phi->op()[i];
                auto source = phi->op()[i + 1];
                auto inst   = Instruction::createStore(address->unwrap(), user);
                if (user->isInstruction()) {
                    inst->insertAfter(user->asInstruction());
                } else {
                    inst->insertToTail(
                        static_cast<BasicBlock *>(source.value()));
                }
            }
            auto load = Instruction::createLoad(address->unwrap());
            load->insertAfter(phi);
            auto value = load->unwrap();
            for (auto use : value->uses()) {}
            std::vector<Use *> v(phi->uses().begin(), phi->uses().end());
            for (auto use : v) { use->reset(value); }
            phi->removeFromBlock();
        }
    }
}

void Mem2RegPass::searchAndRename(ir::BasicBlock *block) {
    blockVisited_.insert(block);

    std::vector<Instruction *> instRemoveLater;
    std::vector<Value *>       defs;
    for (auto inst : block->instructions()) {
        if (inst->id() == InstructionID::Phi) {
            auto address = phiParent_[inst->asPhi()];
            defs.push_back(address);
            valueStacks_[address].push(inst->unwrap());
        } else if (inst->id() == InstructionID::Load) {
            instRemoveLater.push_back(inst);
            auto  address = inst->asLoad()->operand();
            auto &values  = valueStacks_[address];
            auto  value   = !values.empty() ? values.top() : nullptr;
            if (value != nullptr) {
                std::vector<Use *> uses(
                    inst->unwrap()->uses().begin(),
                    inst->unwrap()->uses().end());
                for (auto use : uses) { use->reset(value); }
            }
        } else if (inst->id() == InstructionID::Store) {
            instRemoveLater.push_back(inst);
            auto store = inst->asStore();
            defs.push_back(store->lhs());
            valueStacks_[store->lhs()].push(store->rhs());
        }
    }

    for (auto inst : instRemoveLater) {
        auto ok = inst->removeFromBlock();
        // assert(ok);
    }

    std::vector<BasicBlock *> succBlocks;
    if (block->isBranched()) {
        succBlocks.push_back(block->branch());
        succBlocks.push_back(block->branchElse());
    } else if (block->isLinear() && !block->isTerminal()) {
        succBlocks.push_back(block->branch());
    }

    for (auto succ : succBlocks) {
        for (auto inst : succ->instructions()) {
            if (inst->id() != InstructionID::Phi) { continue; }
            auto  phi     = inst->asPhi();
            auto  address = phiParent_[phi];
            auto &values  = valueStacks_[address];
            if (!values.empty()) { phi->addIncomingValue(values.top(), block); }
        }
    }

    for (auto succ : succBlocks) {
        if (!blockVisited_.count(succ)) { searchAndRename(succ); }
    }

    for (auto def : defs) { valueStacks_[def].pop(); }
}

void Mem2RegPass::solveDominanceFrontier(Function *target) {
    const int n = target->size();

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

    std::vector<int> idom(n);

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
            idom[succ] = t == semi[minNode[succ]] ? t : minNode[succ];
        }

        //! reset semi tree
        semiTree[t] = -1;
    }

    //! post process to finalize idom
    for (int i = 1; i < nodeAt.size(); ++i) {
        auto t = nodeAt[i];
        if (idom[t] != semi[t]) { idom[t] = idom[idom[t]]; }
    }

    //! compute dom frontier by idom
    std::map<BasicBlock *, BasicBlock *> idom_;
    for (auto block : target->basicBlocks()) {
        idom_[block] = revIndex[idom[index[block]]];
    }

    domfrSetList_.clear();
    for (auto block : target->basicBlocks()) { domfrSetList_[block].clear(); }

    for (auto block : target->basicBlocks()) {
        if (block->inBlocks().size() <= 1) { continue; }
        for (auto pred : block->inBlocks()) {
            auto runner = pred;
            while (runner != idom_[block]) {
                domfrSetList_[runner].insert(block);
                runner = idom_[runner];
            }
        }
    }

    for (auto block : target->basicBlocks()) {
        std::cout << "; " << target->name() << "#" << block->id() << ": "
                  << "dfn=" << dfn[index[block]] << "; idom="
                  << (idom_[block] != nullptr ? idom_[block]->id() : -1)
                  << "; DF={";
        for (auto df : domfrSetList_[block]) { std::cout << " " << df->id(); }
        std::cout << (domfrSetList_[block].empty() ? "}" : " }") << std::endl;
    }

//! WARNING: the following algorithm can only compute the idom of
//! a dag but not a graph with circles
#if 0
    std::map<BasicBlock *, int>                       depth;
    std::queue<std::pair<int, BasicBlock *>>          queue;
    std::stack<std::pair<BasicBlock *, BasicBlock *>> edges;
    std::set<std::pair<BasicBlock *, BasicBlock *>>   circles;
    std::map<BasicBlock *, BasicBlock *>              idom;

    //! mark circle edge
    edges.push({nullptr, target->front()});
    while (!edges.empty()) {
        auto [pred, block] = edges.top();
        edges.pop();
        if (depth.count(block)) {
            circles.insert({pred, block});
            continue;
        }
        depth[block] = -1;
        if (block->isLinear() && !block->isTerminal()) {
            edges.push({block, block->branch()});
        } else if (block->isBranched()) {
            edges.push({block, block->branch()});
            edges.push({block, block->branchElse()});
        }
    }

    //! compute max depth for each node
    queue.push({0, target->front()});
    while (!queue.empty()) {
        auto [depth_, block] = queue.front();
        queue.pop();
        depth[block] = std::max(depth[block], depth_);
        if (block->isLinear() && !block->isTerminal()) {
            if (!circles.count({block, block->branch()})) {
                queue.push({depth_ + 1, block->branch()});
            }
        } else if (block->isBranched()) {
            if (!circles.count({block, block->branch()})) {
                queue.push({depth_ + 1, block->branch()});
            }
            if (!circles.count({block, block->branchElse()})) {
                queue.push({depth_ + 1, block->branchElse()});
            }
        }
    }
    assert(depth.size() == target->size());

    //! compute idom by lca
    idom[target->front()] = nullptr;
    auto       it         = target->basicBlocks().begin();
    const auto end        = target->basicBlocks().end();
    it++;
    while (it != end) {
        auto                     block = *it++;
        std::stack<BasicBlock *> stack;
        std::stack<BasicBlock *> preds;
        for (auto pred : block->inBlocks()) {
            if (pred == block) { continue; }
            preds.push(pred);
        }
        assert(preds.size() >= 1);
        while (preds.size() != 1) {
            int minDepth = depth[preds.top()];
            int maxDepth = depth[preds.top()];
            stack.push(preds.top());
            preds.pop();
            while (!preds.empty()) {
                auto pred = preds.top();
                minDepth  = std::min(minDepth, depth[pred]);
                maxDepth  = std::max(maxDepth, depth[pred]);
                stack.push(pred);
                preds.pop();
            }
            assert(minDepth <= maxDepth);

            std::set<BasicBlock *> flag;
            while (!stack.empty()) {
                auto block = stack.top();
                stack.pop();
                if (minDepth < maxDepth && depth[block] == minDepth
                    && !flag.count(block)) {
                    preds.push(block);
                    flag.insert(block);
                }
                for (auto pred : block->inBlocks()) {
                    if (flag.count(pred)) { continue; }
                    preds.push(pred);
                    flag.insert(pred);
                }
            }
        }
        idom[block] = preds.top();
    }

    //! compute dom frontier by idom
    domfrSetList_.clear();
    for (auto block : target->basicBlocks()) { domfrSetList_[block].clear(); }

    for (auto block : target->basicBlocks()) {
        if (block->inBlocks().size() <= 1) { continue; }
        for (auto pred : block->inBlocks()) {
            auto runner = pred;
            while (runner != idom[block]) {
                domfrSetList_[runner].insert(block);
                runner = idom[runner];
            }
        }
    }

    for (auto block : target->basicBlocks()) {
        std::cout << "; " << target->name() << "#" << block->id() << ": "
                  << "depth=" << depth[block] << "; idom="
                  << (idom[block] != nullptr ? idom[block]->id() : -1)
                  << "; DF={";
        for (auto df : domfrSetList_[block]) { std::cout << " " << df->id(); }
        std::cout << (domfrSetList_[block].empty() ? "}" : " }") << std::endl;
    }
#endif
}

} // namespace slime::pass
