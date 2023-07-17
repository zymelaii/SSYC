#include "DeadCodeElimination.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void DeadCodeEliminationPass::run(Module *module) {
    for (auto fn : module->globalObjects()) {
        if (fn->isFunction()) {
            useList.clear();
            defList.clear();
            runOnFunction(fn->asFunction());
        }
    }
}

void DeadCodeEliminationPass::runOnFunction(Function *target) {
    auto blockIter = target->basicBlocks().begin();
    if (blockIter == target->basicBlocks().end()) { return; }

    std::set<StoreInst *> storeSet;

    auto entry = *blockIter;
    while (blockIter != target->basicBlocks().end()) {
        auto block = *blockIter++;
        if (block->uses().size() == 0 && block != entry) {
            block->remove();
            continue;
        } else if (block->isIncomplete()) {
            auto ok = block->tryMarkAsTerminal();
            assert(ok);
        }
        std::list<ir::AllocaInst *> allocas;
        auto                        instIter = block->instructions().begin();
        while (instIter != block->instructions().end()) {
            auto inst = *instIter++;
            if (inst->id() == ir::InstructionID::Store) {
                auto store = inst->asStore();
                if (auto &lastStore = defList[store->lhs()]) {
                    if (lastStore != nullptr) {
                        auto lastLoad = useList[lastStore];
                        if (!lastLoad
                            && lastLoad->parent() == lastStore->parent()
                            && lastStore->parent() == store->parent()
                            && lastLoad->uses().size() == 0) {
                            auto ok = lastStore->removeFromBlock();
                            assert(ok);
                            storeSet.erase(lastStore);
                        }
                    }
                    lastStore = store;
                }
                storeSet.insert(store);
            }
            if (inst->id() == ir::InstructionID::Load) {
                auto load = inst->asLoad();
                if (load->uses().size() == 0) {
                    load->removeFromBlock();
                    continue;
                }
                auto address = load->operand();
                if (defList.count(address) != 0) {
                    auto  store    = defList[address];
                    auto &lastLoad = useList[store];
                    if (lastLoad) { lastLoad->removeFromBlock(); }
                    lastLoad = load;
                }
            }
            if (inst->id() == ir::InstructionID::Br) {
                auto br = inst->asBr();
                if (block->isBranched() && br->op<0>()->isImmediate()) {
                    auto imm =
                        static_cast<ir::ConstantInt *>(br->op<0>().value());
                    auto branchToSecond = !imm->value;
                    if (branchToSecond) {
                        block->reset(block->branchElse());
                    } else {
                        block->reset(block->branch());
                    }
                }
                continue;
            }
            if (inst->id() == ir::InstructionID::Alloca) {
                allocas.push_front(inst->asAlloca());
                continue;
            }
        }
        for (auto alloca : allocas) {
            if (alloca->uses().size() == 0) {
                alloca->removeFromBlock();
            } else {
                alloca->insertToHead(entry);
            }
        }
    }

    //! remove no-effect store
    for (auto store : storeSet) {
        auto address = store->lhs();
        //! alloca is only used by store itself
        if (address->uses().size() == 1) {
            auto ok = store->removeFromBlock();
            assert(ok);
            if (address->isInstruction()
                && address->asInstruction()->id() == InstructionID::Alloca) {
                auto alloca = address->asInstruction()->asAlloca();
                if (alloca->uses().size() == 0) {
                    auto ok = alloca->removeFromBlock();
                    assert(ok);
                }
            }
        }
    }

    //! FIXME: this block combination cannot solve empty loop issues
    //! completely
    blockIter = target->basicBlocks().begin();
    while (blockIter != target->basicBlocks().end()) {
        const auto MAX_COMBO = 5;
        int        combo     = 0;
        auto       block     = *blockIter++;
        while (block->isLinear() && !block->isTerminal()) {
            if (combo++ < MAX_COMBO) { break; }
            auto follow = block->branch();
            if (follow->inBlocks().size() == 1) {
                if (blockIter != target->basicBlocks().end()
                    && *blockIter == follow) {
                    ++blockIter;
                }
                block->reset();
                auto instIter = follow->instructions().begin();
                while (instIter != follow->instructions().end()) {
                    (*instIter++)->insertToTail(block);
                }
                block->syncFlowWithInstUnsafe();
                follow->remove();
                break;
            }
            if (!follow->isLinear() || follow->size() != 1) { break; }
            if (follow->isTerminal()) {
                auto ret = follow->instructions().tail()->value()->asRet();
                block->reset();
                bool ok = block->tryMarkAsTerminal(ret->operand());
                assert(ok);
            } else {
                block->reset(follow->branch());
            }
        }
    }

    blockIter = target->basicBlocks().begin();
    assert(blockIter != target->basicBlocks().end());
    ++blockIter;
    while (blockIter != target->basicBlocks().end()) {
        if (auto block = *blockIter++; block->uses().size() == 0) {
            block->remove();
        }
    }
}

} // namespace slime::pass
