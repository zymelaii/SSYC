#include "pass.h"
#include "Peekhole.h"

#include <slime/ir/module.h>
#include <slime/ir/value.h>
#include <slime/ir/user.h>
#include <slime/ir/instruction.h>
#include <map>
#include <list>

namespace slime::pass {

class OneShotPass : public UniversalIRPass {
public:
    void run(ir::Module *module) override {
        PeekholePass{}.run(module);
        for (auto object : module->globalObjects()) {
            if (object->isFunction()) {
                auto fn = object->asFunction();
                if (fn->size() > 0) { testAndSetRecursionFlag(fn); }
            }
        }
        UniversalIRPass::run(module);
    }

    void runOnFunction(ir::Function *target) override {
        executeDeadCodeElimination(target);
        executeFunctionInlining(target);
        executeDeadCodeElimination(target);
    }

    void executeDeadCodeElimination(ir::Function *target) {
        auto blockIter = target->basicBlocks().begin();
        if (blockIter == target->basicBlocks().end()) { return; }
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
            auto instIter = block->instructions().begin();
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
                                lastStore->removeFromBlock();
                            }
                        }
                        lastStore = store;
                    }
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

        blockIter = target->basicBlocks().begin();
        while (blockIter != target->basicBlocks().end()) {
            auto block = *blockIter++;
            while (block->isLinear() && !block->isTerminal()) {
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

    void executeFunctionInlining(ir::Function *target) {
        auto blockIter = target->basicBlocks().begin();
        while (blockIter != target->basicBlocks().end()) {
            auto block    = *blockIter++;
            auto instIter = block->instructions().begin();
            while (instIter != block->instructions().end()) {
                auto inst = *instIter++;
                if (inst->id() != ir::InstructionID::Call) { continue; }
                auto call = inst->asCall();
                auto fn   = call->callee()->asFunction();
                //! fn is a recursive function
                if (fndeps[fn].count(fn)) { continue; }
                //! FIXME: function with no blocks can be either external or
                //! empty
                if (fn->size() == 0) { continue; }

                //! ================
                //! block:
                //!     ...
                //!     %ret = call ...
                //!     ...rest
                //! ================
                //! block:
                //!     ...
                //!     %ret = call ...
                //!     br %inline.entry
                //! inline.entry:
                //!     %retptr = alloca ...
                //!     br %inline.blocks..., %inline.exit
                //! inline.blocks...:
                //!     ...
                //! inline.exit:
                //!     %retval = load retptr
                //!     ...rest
                //! ================

                //! cut fncall
                auto inlineEntry = ir::BasicBlock::create(target);
                auto inlineExit  = ir::BasicBlock::create(target);
                inlineEntry->insertOrMoveAfter(block);
                inlineExit->insertOrMoveAfter(inlineEntry);
                while (instIter != block->instructions().end()) {
                    auto inst = *instIter++;
                    inst->insertToTail(inlineExit);
                }
                inlineExit->syncFlowWithInstUnsafe();
                block->reset(inlineEntry);

                //! prepare retval store
                ir::AllocaInst *retvalAddress = nullptr;
                ir::Value      *retval        = nullptr;
                if (!fn->proto()->returnType()->isVoid()) {
                    retvalAddress = ir::Instruction::createAlloca(call->type());
                    retvalAddress->insertToHead(inlineEntry);
                    auto load = ir::Instruction::createLoad(retvalAddress);
                    load->insertToHead(inlineExit);
                    retval = load->unwrap();
                }

                //! perform inlining
                std::map<ir::BasicBlock *, ir::BasicBlock *> blockMappings;
                std::map<const ir::Parameter *, ir::Value *> paramMappings;
                std::map<ir::Value *, ir::Value *>           instValueMappings;

                for (int i = 0; i < fn->totalParams(); ++i) {
                    paramMappings[fn->paramAt(i)] = call->paramAt(i);
                }

                instValueMappings[call] = retval;

                auto mapping = [&](ir::Value *value) {
                    if (value->isInstruction()) {
                        value = instValueMappings.count(value)
                                  ? instValueMappings[value]
                                  : value;
                    } else if (value->isParameter()) {
                        value =
                            paramMappings[static_cast<ir::Parameter *>(value)];
                    }
                    assert(value != nullptr);
                    return value;
                };

                auto currentBlock = inlineEntry;
                for (auto block : fn->basicBlocks()) {
                    auto thisBlock = ir::BasicBlock::create(target);
                    thisBlock->insertOrMoveAfter(currentBlock);
                    currentBlock         = thisBlock;
                    blockMappings[block] = thisBlock;
                }
                inlineEntry->reset(blockMappings[fn->front()]);

                auto inlineBlockIter = fn->basicBlocks().begin();
                while (inlineBlockIter != fn->basicBlocks().end()) {
                    auto inlineBlock = *inlineBlockIter++;
                    currentBlock     = blockMappings[inlineBlock];
                    auto instIter    = inlineBlock->instructions().begin();
                    while (instIter != inlineBlock->instructions().end()) {
                        auto       inst        = *instIter++;
                        ir::Value *value       = nullptr;
                        bool       shouldRemap = true;
                        switch (inst->id()) {
                            case ir::InstructionID::Alloca: {
                                value = ir::Instruction::createAlloca(
                                    inst->asAlloca()
                                        ->type()
                                        ->tryGetElementType());
                            } break;
                            case ir::InstructionID::Load: {
                                auto load = inst->asLoad();
                                value     = ir::Instruction::createLoad(
                                    mapping(load->operand()));
                            } break;
                            case ir::InstructionID::Store: {
                                auto store = inst->asStore();
                                value      = ir::Instruction::createStore(
                                    mapping(store->lhs()),
                                    mapping(store->rhs()));
                            } break;
                            case ir::InstructionID::Ret: {
                                assert(
                                    instIter
                                    == inlineBlock->instructions().end());
                                if (retvalAddress != nullptr) {
                                    ir::Instruction::createStore(
                                        retvalAddress,
                                        mapping(inst->asRet()->operand()))
                                        ->insertToTail(currentBlock);
                                }
                                currentBlock->reset(inlineExit);
                            } break;
                            case ir::InstructionID::Br: {
                                assert(
                                    instIter
                                    == inlineBlock->instructions().end());
                                if (inlineBlock->isBranched()) {
                                    currentBlock->reset(
                                        mapping(inlineBlock->control()),
                                        blockMappings[inlineBlock->branch()],
                                        blockMappings[inlineBlock
                                                          ->branchElse()]);
                                } else {
                                    assert(!inlineBlock->isTerminal());
                                    currentBlock->reset(
                                        blockMappings[inlineBlock->branch()]);
                                }
                            } break;
                            case ir::InstructionID::GetElementPtr: {
                                auto getelementptr = inst->asGetElementPtr();
                                if (getelementptr->op<2>() == nullptr) {
                                    value =
                                        ir::Instruction::createGetElementPtr(
                                            mapping(getelementptr->op<0>()),
                                            mapping(getelementptr->op<1>()));
                                } else {
                                    value =
                                        ir::Instruction::createGetElementPtr(
                                            mapping(getelementptr->op<0>()),
                                            mapping(getelementptr->op<1>()),
                                            mapping(getelementptr->op<2>()));
                                }
                            } break;
                            case ir::InstructionID::Add: {
                                auto add = inst->asAdd();
                                value    = ir::Instruction::createAdd(
                                    mapping(add->lhs()), mapping(add->rhs()));
                            } break;
                            case ir::InstructionID::Sub: {
                                auto sub = inst->asSub();
                                value    = ir::Instruction::createSub(
                                    mapping(sub->lhs()), mapping(sub->rhs()));
                            } break;
                            case ir::InstructionID::Mul: {
                                auto mul = inst->asMul();
                                value    = ir::Instruction::createMul(
                                    mapping(mul->lhs()), mapping(mul->rhs()));
                            } break;
                            case ir::InstructionID::UDiv: {
                                auto udiv = inst->asUDiv();
                                value     = ir::Instruction::createUDiv(
                                    mapping(udiv->lhs()), mapping(udiv->rhs()));
                            } break;
                            case ir::InstructionID::SDiv: {
                                auto sdiv = inst->asSDiv();
                                value     = ir::Instruction::createSDiv(
                                    mapping(sdiv->lhs()), mapping(sdiv->rhs()));
                            } break;
                            case ir::InstructionID::URem: {
                                auto urem = inst->asURem();
                                value     = ir::Instruction::createURem(
                                    mapping(urem->lhs()), mapping(urem->rhs()));
                            } break;
                            case ir::InstructionID::SRem: {
                                auto srem = inst->asSRem();
                                value     = ir::Instruction::createSRem(
                                    mapping(srem->lhs()), mapping(srem->rhs()));
                            } break;
                            case ir::InstructionID::FNeg: {
                                auto fneg = inst->asFNeg();
                                value     = ir::Instruction::createFNeg(
                                    mapping(fneg->operand()));
                            } break;
                            case ir::InstructionID::FAdd: {
                                auto fadd = inst->asFAdd();
                                value     = ir::Instruction::createFAdd(
                                    mapping(fadd->lhs()), mapping(fadd->rhs()));
                            } break;
                            case ir::InstructionID::FSub: {
                                auto fsub = inst->asFSub();
                                value     = ir::Instruction::createFSub(
                                    mapping(fsub->lhs()), mapping(fsub->rhs()));
                            } break;
                            case ir::InstructionID::FMul: {
                                auto fmul = inst->asFMul();
                                value     = ir::Instruction::createFMul(
                                    mapping(fmul->lhs()), mapping(fmul->rhs()));
                            } break;
                            case ir::InstructionID::FDiv: {
                                auto fdiv = inst->asFDiv();
                                value     = ir::Instruction::createFDiv(
                                    mapping(fdiv->lhs()), mapping(fdiv->rhs()));
                            } break;
                            case ir::InstructionID::FRem: {
                                auto frem = inst->asFRem();
                                value     = ir::Instruction::createFRem(
                                    mapping(frem->lhs()), mapping(frem->rhs()));
                            } break;
                            case ir::InstructionID::Shl: {
                                auto shl = inst->asShl();
                                value    = ir::Instruction::createShl(
                                    mapping(shl->lhs()), mapping(shl->rhs()));
                            } break;
                            case ir::InstructionID::LShr: {
                                auto lshr = inst->asLShr();
                                value     = ir::Instruction::createLShr(
                                    mapping(lshr->lhs()), mapping(lshr->rhs()));
                            } break;
                            case ir::InstructionID::AShr: {
                                auto ashr = inst->asAShr();
                                value     = ir::Instruction::createAShr(
                                    mapping(ashr->lhs()), mapping(ashr->rhs()));
                            } break;
                            case ir::InstructionID::And: {
                                auto instAnd = inst->asAnd();
                                value        = ir::Instruction::createAnd(
                                    mapping(instAnd->lhs()),
                                    mapping(instAnd->rhs()));
                            } break;
                            case ir::InstructionID::Or: {
                                auto instOr = inst->asOr();
                                value       = ir::Instruction::createOr(
                                    mapping(instOr->lhs()),
                                    mapping(instOr->rhs()));
                            } break;
                            case ir::InstructionID::Xor: {
                                auto instXor = inst->asXor();
                                value        = ir::Instruction::createXor(
                                    mapping(instXor->lhs()),
                                    mapping(instXor->rhs()));
                            } break;
                            case ir::InstructionID::FPToUI: {
                                auto fptoui = inst->asFPToUI();
                                value       = ir::Instruction::createFPToUI(
                                    mapping(fptoui->operand()));
                            } break;
                            case ir::InstructionID::FPToSI: {
                                auto fptosi = inst->asFPToSI();
                                value       = ir::Instruction::createFPToSI(
                                    mapping(fptosi->operand()));
                            } break;
                            case ir::InstructionID::UIToFP: {
                                auto uitofp = inst->asUIToFP();
                                value       = ir::Instruction::createUIToFP(
                                    mapping(uitofp->operand()));
                            } break;
                            case ir::InstructionID::SIToFP: {
                                auto sitofp = inst->asSIToFP();
                                value       = ir::Instruction::createSIToFP(
                                    mapping(sitofp->operand()));
                            } break;
                            case ir::InstructionID::ZExt: {
                                auto zext = inst->asZExt();
                                value     = ir::Instruction::createZExt(
                                    mapping(zext->operand()));
                            } break;
                            case ir::InstructionID::ICmp: {
                                auto icmp = inst->asICmp();
                                value     = ir::Instruction::createICmp(
                                    icmp->predicate(),
                                    mapping(icmp->lhs()),
                                    mapping(icmp->rhs()));
                            } break;
                            case ir::InstructionID::FCmp: {
                                auto fcmp = inst->asFCmp();
                                value     = ir::Instruction::createFCmp(
                                    fcmp->predicate(),
                                    mapping(fcmp->lhs()),
                                    mapping(fcmp->rhs()));
                            } break;
                            case ir::InstructionID::Phi: {
                                auto phi = ir::Instruction::createPhi(
                                    inst->unwrap()->type());
                                for (int i = 0; i < inst->totalOperands();
                                     i     += 2) {
                                    if (inst->useAt(i) == nullptr) { break; }
                                    phi->addIncomingValue(
                                        mapping(inst->useAt(i)),
                                        blockMappings
                                            [static_cast<ir::BasicBlock *>(
                                                inst->useAt(i + 1).value())]);
                                }
                                value = phi->unwrap();
                            } break;
                            case ir::InstructionID::Call: {
                                auto call = ir::Instruction::createCall(
                                    inst->asCall()->callee()->asFunction());
                                for (int i = 1; i < inst->totalOperands();
                                     ++i) {
                                    call->op()[i] = mapping(inst->useAt(i));
                                }
                                value = call->unwrap();
                            } break;
                        }
                        if (value != nullptr) {
                            if (shouldRemap) {
                                instValueMappings[inst->unwrap()] = value;
                            }
                            value->asInstruction()->insertToTail(currentBlock);
                        }
                    }
                    assert(
                        !currentBlock->isIncomplete()
                        && currentBlock->size() > 0);
                }

                //! replace retval
                if (retvalAddress != nullptr) {
                    std::vector uses(call->uses().begin(), call->uses().end());
                    for (auto &use : uses) { use->reset(retval); }
                    assert(call->uses().size() == 0);
                }
                call->removeFromBlock();
            }
        }
    }

    void testAndSetRecursionFlag(ir::Function *function) {
        if (fndeps.count(function)) { return; }
        auto &deps = fndeps[function];
        for (auto block : function->basicBlocks()) {
            for (auto inst : block->instructions()) {
                if (inst->id() == ir::InstructionID::Call) {
                    deps.insert(inst->asCall()->callee()->asFunction());
                }
            }
        }
    }

private:
    //! address -> store
    std::map<ir::Value *, ir::StoreInst *> defList;
    //! store -> load
    std::map<ir::StoreInst *, ir::LoadInst *> useList;
    //! address -> blocks
    std::map<ir::Value *, std::set<ir::BasicBlock *>> usedInBlocks;
    //! function dependency
    std::map<ir::Function *, std::set<ir::Function *>> fndeps;
};

} // namespace slime::pass
