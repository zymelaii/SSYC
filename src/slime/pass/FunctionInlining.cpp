#include "FunctionInlining.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace slime::ir;

void FunctionInliningPass::run(Module *module) {
    for (auto function : module->globalObjects()) {
        if (function->isFunction()) {
            testAndSetRecursionFlag(function->asFunction());
        }
    }
    UniversalIRPass::run(module);
}

void FunctionInliningPass::runOnFunction(Function *target) {
    auto blockIter = target->basicBlocks().begin();
    while (blockIter != target->basicBlocks().end()) {
        auto block    = *blockIter;
        auto instIter = block->instructions().begin();
        while (instIter != block->instructions().end()) {
            auto inst = *instIter++;
            if (inst->id() != InstructionID::Call) { continue; }
            auto instAfterCall = *instIter;
            bool done = tryExecuteFunctionInlining(block, inst->asCall());
            if (done) { break; }
        }
        //! WARNING: delay the iter increment due to iterBlock invliadation
        //! caused by function inlining
        ++blockIter;
    }
}

bool FunctionInliningPass::tryExecuteFunctionInlining(
    BasicBlock *block, CallInst *call) {
    auto target = block->parent();
    auto fn     = call->callee()->asFunction();
    if (isRecursiveFunction(fn)) { return false; }

    //! FIXME: function with no blocks can be either external or
    //! empty
    //! here assume a 0-block function is an external one
    if (fn->size() == 0) { return false; }

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

    auto inlineEntry = BasicBlock::create(target);
    auto inlineExit  = insertNewBlockAfter(call);
    inlineEntry->insertOrMoveAfter(block);
    block->reset(inlineEntry);

    //! block mapping table for inlining
    std::map<ir::BasicBlock *, ir::BasicBlock *> blockMappings;

    //! value mapping table for inlining
    std::map<ir::Value *, ir::Value *> instValueMappings;

    //! prepare retval store
    AllocaInst *retvalAddress = nullptr;
    Value      *retval        = nullptr;
    if (!fn->proto()->returnType()->isVoid()) {
        retvalAddress = Instruction::createAlloca(call->type());
        retvalAddress->insertToHead(inlineEntry);
        auto load = Instruction::createLoad(retvalAddress);
        load->insertToHead(inlineExit);
        retval = load->unwrap();
    }
    instValueMappings[call] = retval;

    auto mapping = [&](Value *value) {
        if (value->isLabel()) {
            value = blockMappings[static_cast<BasicBlock *>(value)];
        } else if (value->isInstruction()) {
            assert(instValueMappings.count(value));
            value = instValueMappings[value];
        } else if (value->isParameter()) {
            value = call->paramAt(static_cast<Parameter *>(value)->index());
        }
        assert(value != nullptr);
        return value;
    };

    auto currentBlock = inlineEntry;
    for (auto block : fn->basicBlocks()) {
        auto thisBlock = BasicBlock::create(target);
        thisBlock->insertOrMoveAfter(currentBlock);
        currentBlock         = thisBlock;
        blockMappings[block] = thisBlock;
    }
    inlineEntry->reset(static_cast<BasicBlock *>(mapping(fn->front())));

    auto inlineBlockIter = fn->basicBlocks().begin();
    while (inlineBlockIter != fn->basicBlocks().end()) {
        auto inlineBlock = *inlineBlockIter++;
        currentBlock     = blockMappings[inlineBlock];
        auto instIter    = inlineBlock->instructions().begin();
        while (instIter != inlineBlock->instructions().end()) {
            auto inst   = *instIter++;
            auto cloned = cloneInstruction(inst, mapping);
            if (inst->id() == InstructionID::Br) {
                assert(instIter == inlineBlock->instructions().end());
                if (inlineBlock->isBranched()) {
                    currentBlock->reset(
                        mapping(inlineBlock->control()),
                        static_cast<BasicBlock *>(
                            mapping(inlineBlock->branch())),
                        static_cast<BasicBlock *>(
                            mapping(inlineBlock->branchElse())));
                } else {
                    assert(!inlineBlock->isTerminal());
                    currentBlock->reset(static_cast<BasicBlock *>(
                        mapping(inlineBlock->branch())));
                }
            } else if (inst->id() == InstructionID::Ret) {
                assert(instIter == inlineBlock->instructions().end());
                if (retvalAddress != nullptr) {
                    Instruction::createStore(
                        retvalAddress, mapping(inst->asRet()->operand()))
                        ->insertToTail(currentBlock);
                }
                currentBlock->reset(inlineExit);
            } else {
                assert(cloned != nullptr);
                instValueMappings[inst->unwrap()] = cloned->unwrap();
                cloned->insertToTail(currentBlock);
            }
        }
        assert(!currentBlock->isIncomplete() && currentBlock->size() > 0);
    }

    //! replace retval
    if (retvalAddress != nullptr) {
        std::vector uses(call->uses().begin(), call->uses().end());
        for (auto &use : uses) { use->reset(retval); }
        assert(call->uses().size() == 0);
    }
    call->removeFromBlock();

    return true;
}

BasicBlock *FunctionInliningPass::insertNewBlockAfter(Instruction *inst) {
    assert(inst->id() != InstructionID::Br);
    assert(inst->id() != InstructionID::Ret);

    auto thisBlock = inst->parent();
    auto fn        = thisBlock->parent();
    auto block     = BasicBlock::create(fn);
    block->insertOrMoveAfter(thisBlock);

    auto iter = inst->intoIter();
    ++iter;
    while (iter != thisBlock->instructions().end()) {
        auto inst = *iter++;
        inst->insertToTail(block);
    }

    block->syncFlowWithInstUnsafe();
    thisBlock->reset(block);

    return block;
}

void FunctionInliningPass::testAndSetRecursionFlag(Function *function) {
    if (depsMap.count(function)) { return; }
    auto &deps = depsMap[function];
    for (auto block : function->basicBlocks()) {
        for (auto inst : block->instructions()) {
            if (inst->id() == InstructionID::Call) {
                deps.insert(inst->asCall()->callee()->asFunction());
            }
        }
    }
}

Instruction *FunctionInliningPass::cloneInstruction(
    Instruction                            *instruction,
    std::function<ir::Value *(ir::Value *)> mappingStrategy) {
    Instruction *value = nullptr;
    switch (instruction->id()) {
        case InstructionID::Alloca: {
            value = Instruction::createAlloca(
                instruction->asAlloca()->type()->tryGetElementType());
        } break;
        case InstructionID::Load: {
            auto load = instruction->asLoad();
            value = Instruction::createLoad(mappingStrategy(load->operand()));
        } break;
        case InstructionID::Store: {
            auto store = instruction->asStore();
            value      = Instruction::createStore(
                mappingStrategy(store->lhs()), mappingStrategy(store->rhs()));
        } break;
        case InstructionID::GetElementPtr: {
            auto getelementptr = instruction->asGetElementPtr();
            if (getelementptr->op<2>() == nullptr) {
                value = Instruction::createGetElementPtr(
                    mappingStrategy(getelementptr->op<0>()),
                    mappingStrategy(getelementptr->op<1>()));
            } else {
                value = Instruction::createGetElementPtr(
                    mappingStrategy(getelementptr->op<0>()),
                    mappingStrategy(getelementptr->op<1>()),
                    mappingStrategy(getelementptr->op<2>()));
            }
        } break;
        case InstructionID::Add: {
            auto add = instruction->asAdd();
            value    = Instruction::createAdd(
                mappingStrategy(add->lhs()), mappingStrategy(add->rhs()));
        } break;
        case InstructionID::Sub: {
            auto sub = instruction->asSub();
            value    = Instruction::createSub(
                mappingStrategy(sub->lhs()), mappingStrategy(sub->rhs()));
        } break;
        case InstructionID::Mul: {
            auto mul = instruction->asMul();
            value    = Instruction::createMul(
                mappingStrategy(mul->lhs()), mappingStrategy(mul->rhs()));
        } break;
        case InstructionID::UDiv: {
            auto udiv = instruction->asUDiv();
            value     = Instruction::createUDiv(
                mappingStrategy(udiv->lhs()), mappingStrategy(udiv->rhs()));
        } break;
        case InstructionID::SDiv: {
            auto sdiv = instruction->asSDiv();
            value     = Instruction::createSDiv(
                mappingStrategy(sdiv->lhs()), mappingStrategy(sdiv->rhs()));
        } break;
        case InstructionID::URem: {
            auto urem = instruction->asURem();
            value     = Instruction::createURem(
                mappingStrategy(urem->lhs()), mappingStrategy(urem->rhs()));
        } break;
        case InstructionID::SRem: {
            auto srem = instruction->asSRem();
            value     = Instruction::createSRem(
                mappingStrategy(srem->lhs()), mappingStrategy(srem->rhs()));
        } break;
        case InstructionID::FNeg: {
            auto fneg = instruction->asFNeg();
            value = Instruction::createFNeg(mappingStrategy(fneg->operand()));
        } break;
        case InstructionID::FAdd: {
            auto fadd = instruction->asFAdd();
            value     = Instruction::createFAdd(
                mappingStrategy(fadd->lhs()), mappingStrategy(fadd->rhs()));
        } break;
        case InstructionID::FSub: {
            auto fsub = instruction->asFSub();
            value     = Instruction::createFSub(
                mappingStrategy(fsub->lhs()), mappingStrategy(fsub->rhs()));
        } break;
        case InstructionID::FMul: {
            auto fmul = instruction->asFMul();
            value     = Instruction::createFMul(
                mappingStrategy(fmul->lhs()), mappingStrategy(fmul->rhs()));
        } break;
        case InstructionID::FDiv: {
            auto fdiv = instruction->asFDiv();
            value     = Instruction::createFDiv(
                mappingStrategy(fdiv->lhs()), mappingStrategy(fdiv->rhs()));
        } break;
        case InstructionID::FRem: {
            auto frem = instruction->asFRem();
            value     = Instruction::createFRem(
                mappingStrategy(frem->lhs()), mappingStrategy(frem->rhs()));
        } break;
        case InstructionID::Shl: {
            auto shl = instruction->asShl();
            value    = Instruction::createShl(
                mappingStrategy(shl->lhs()), mappingStrategy(shl->rhs()));
        } break;
        case InstructionID::LShr: {
            auto lshr = instruction->asLShr();
            value     = Instruction::createLShr(
                mappingStrategy(lshr->lhs()), mappingStrategy(lshr->rhs()));
        } break;
        case InstructionID::AShr: {
            auto ashr = instruction->asAShr();
            value     = Instruction::createAShr(
                mappingStrategy(ashr->lhs()), mappingStrategy(ashr->rhs()));
        } break;
        case InstructionID::And: {
            auto instAnd = instruction->asAnd();
            value        = Instruction::createAnd(
                mappingStrategy(instAnd->lhs()),
                mappingStrategy(instAnd->rhs()));
        } break;
        case InstructionID::Or: {
            auto instOr = instruction->asOr();
            value       = Instruction::createOr(
                mappingStrategy(instOr->lhs()), mappingStrategy(instOr->rhs()));
        } break;
        case InstructionID::Xor: {
            auto instXor = instruction->asXor();
            value        = Instruction::createXor(
                mappingStrategy(instXor->lhs()),
                mappingStrategy(instXor->rhs()));
        } break;
        case InstructionID::FPToUI: {
            auto fptoui = instruction->asFPToUI();
            value =
                Instruction::createFPToUI(mappingStrategy(fptoui->operand()));
        } break;
        case InstructionID::FPToSI: {
            auto fptosi = instruction->asFPToSI();
            value =
                Instruction::createFPToSI(mappingStrategy(fptosi->operand()));
        } break;
        case InstructionID::UIToFP: {
            auto uitofp = instruction->asUIToFP();
            value =
                Instruction::createUIToFP(mappingStrategy(uitofp->operand()));
        } break;
        case InstructionID::SIToFP: {
            auto sitofp = instruction->asSIToFP();
            value =
                Instruction::createSIToFP(mappingStrategy(sitofp->operand()));
        } break;
        case InstructionID::ZExt: {
            auto zext = instruction->asZExt();
            value = Instruction::createZExt(mappingStrategy(zext->operand()));
        } break;
        case InstructionID::ICmp: {
            auto icmp = instruction->asICmp();
            value     = Instruction::createICmp(
                icmp->predicate(),
                mappingStrategy(icmp->lhs()),
                mappingStrategy(icmp->rhs()));
        } break;
        case InstructionID::FCmp: {
            auto fcmp = instruction->asFCmp();
            value     = Instruction::createFCmp(
                fcmp->predicate(),
                mappingStrategy(fcmp->lhs()),
                mappingStrategy(fcmp->rhs()));
        } break;
        case InstructionID::Phi: {
            auto phi = Instruction::createPhi(instruction->unwrap()->type());
            for (int i = 0; i < instruction->totalOperands(); i += 2) {
                if (instruction->useAt(i) == nullptr) { break; }
                phi->addIncomingValue(
                    mappingStrategy(instruction->useAt(i)),
                    static_cast<BasicBlock *>(
                        mappingStrategy(instruction->useAt(i + 1))));
            }
            value = phi;
        } break;
        case InstructionID::Call: {
            auto call = Instruction::createCall(
                instruction->asCall()->callee()->asFunction());
            for (int i = 1; i < instruction->totalOperands(); ++i) {
                call->op()[i] = mappingStrategy(instruction->useAt(i));
            }
            value = call;
        } break;
        default: {
        } break;
    }
    return value;
}

} // namespace slime::pass
