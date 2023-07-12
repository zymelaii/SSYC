#include "Mem2Reg.h"

#include <list>

namespace slime::pass {

using namespace ir;

void Mem2RegPass::runOnFunction(ir::Function *target) {
    //! compute dominance frontiers
    solveDominanceFrontier(target);

    //! forward in-block store/load value
    for (auto block : target->basicBlocks()) {
        std::map<Value *, StoreInst *> defineList;

        auto it = block->instructions().begin();
        while (it != block->instructions().end()) {
            auto inst = *it++;

            //! remove previous defines since fncall may cause it
            //! invliad
            if (inst->id() == InstructionID::Call) { continue; }

            //! update store value
            if (inst->id() == InstructionID::Store) {
                auto store   = inst->asStore();
                auto address = store->lhs();
                if (defineList.count(address)) {
                    defineList.at(address)->removeFromBlock();
                }
                defineList[address] = store;
                continue;
            }

            //! forward stored value
            if (inst->id() == InstructionID::Load) {
                auto load    = inst->asLoad();
                auto address = load->operand();
                if (defineList.count(address)) {
                    auto value = defineList.at(address)->rhs();
                    for (int i = 0; i < load->totalUse(); ++i) {
                        load->op()[i].reset(value);
                    }
                    load->removeFromBlock();
                }
                continue;
            }
        }
    }

    //! insert phi nodes
    std::set<LoadInst *>                      useSet;
    std::map<Value *, std::set<BasicBlock *>> definedIn;
    std::map<BasicBlock *, std::set<Value *>> usesInserted;

    for (auto block : target->basicBlocks()) {
        for (auto inst : block->instructions()) {
            if (inst->id() == InstructionID::Load) {
                useSet.insert(inst->asLoad());
                continue;
            }
            if (inst->id() == InstructionID::Store) {
                auto store = inst->asStore();
                definedIn[store->lhs()].insert(block);
                continue;
            }
        }
    }

    for (auto use : useSet) {
        auto load    = use->asLoad();
        auto address = load->operand();

        std::list<BasicBlock *> blocks(
            definedIn[address].begin(), definedIn[address].end());
        while (!blocks.empty()) {
            auto block = blocks.back();
            blocks.pop_back();
            auto dfSet = dfSetList_[id_[block]];
            for (auto frontier : *dfSet) {
                auto &uses = usesInserted[frontier];
                if (uses.count(address)) { continue; }
                uses.insert(address);
                auto phi = PhiInst::create(load->type());
                phi->addIncomingValue(load->unwrap(), frontier);
                phi->insertToHead(frontier);
                if (!definedIn[use].count(frontier)) {
                    blocks.push_front(frontier);
                }
            }
        }
    }

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

Mem2RegPass::~Mem2RegPass() {
    for (auto &domSet : domSetList_) { delete domSet; }
    for (auto &dfSet : dfSetList_) { delete dfSet; }
    domSetList_.clear();
    dfSetList_.clear();
}

void Mem2RegPass::solveDominanceFrontier(Function *target) {
    domSetList_.resize(target->size());
    dfSetList_.resize(target->size());

    for (auto &domSet : domSetList_) { domSet = new DomSet{}; }
    for (auto &dfSet : dfSetList_) { dfSet = new DomSet{}; }

    int index = 0;
    for (auto block : target->basicBlocks()) {
        id_[block] = index++;
        domSetList_[id_[block]]->insert(block);
    }

    bool changed = false;
    do {
        changed = false;
        for (auto block : target->basicBlocks()) {
            if (block->isTerminal()) { continue; }
            if (block->branch() != nullptr
                && !domSetList_[id_[block->branch()]]->count(block)) {
                bool dom = true;
                for (auto t : block->branch()->inBlocks()) {
                    if (!domSetList_[id_[t]]->count(block)) {
                        dom = false;
                        break;
                    }
                }
                if (dom) {
                    changed = true;
                    domSetList_[id_[block->branch()]]->insert(block);
                }
            }
            if (block->branchElse() != nullptr
                && !domSetList_[id_[block->branchElse()]]->count(block)) {
                bool dom = true;
                for (auto t : block->branchElse()->inBlocks()) {
                    if (!domSetList_[id_[t]]->count(block)) {
                        dom = false;
                        break;
                    }
                }
                if (dom) {
                    changed = true;
                    domSetList_[id_[block->branchElse()]]->insert(block);
                }
            }
        }
    } while (changed);

    for (auto block : target->basicBlocks()) {
        auto  index = id_[block];
        auto &fr    = dfSetList_[index];
        fr->insert(block);
        for (auto t : *domSetList_[index]) {
            if (t->isTerminal()) { continue; }
            if (t->branch() && domSetList_[index]->count(t->branch()) == 0) {
                fr->insert(t->branch());
            }
            if (t->branchElse()
                && domSetList_[index]->count(t->branchElse()) == 0) {
                fr->insert(t->branchElse());
            }
        }
    }

    // for (auto block : target->basicBlocks()) {
    //     std::cout << "[" << id[block] << "]"
    //               << " DOMS: { ";
    //     for (auto e : *domSetList_[id[block]]) { std::cout << id[e] << ", ";
    //     } std::cout << "} DF: { "; for (auto e : *dfSetList_[id[block]]) {
    //     std::cout << id[e] << ", "; } std::cout << "}\n";
    // }
}

} // namespace slime::pass
