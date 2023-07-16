#include "gencode.h"
#include "regalloc.h"

#include <slime/ir/instruction.def>
#include <slime/ir/module.h>
#include <slime/ir/value.h>
#include <slime/ir/user.h>
#include <slime/ir/instruction.h>
#include <slime/utils/list.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace slime::backend {

Generator *Generator::generate() {
    return new Generator();
}

void Generator::genCode(FILE *fp, Module *module) {
    generator_.asmFile           = fp;
    generator_.allocator         = Allocator::create();
    generator_.stack             = generator_.allocator->stack;
    generator_.usedGlobalVars    = new UsedGlobalVars;
    GlobalObjectList *global_var = new GlobalObjectList;
    println("   .arch armv7a");
    println("   .text");
    for (auto e : *module) {
        if (e->type()->isFunction()) {
            generator_.allocator->initAllocator();
            generator_.cur_func = static_cast<Function *>(e);
            if (!strcmp(e->name().data(), "memset")) continue;
            genAssembly(static_cast<Function *>(e));
            generator_.cur_funcnum++;
            println("   .pool\n");
        } else {
            if (e->tryIntoGlobalVariable() != nullptr)
                global_var->insertToTail(e);
        }
    }
    // generator constant pool
    genUsedGlobVars();
    for (auto e : *global_var) { genGlobalDef(e); }
}

const char *Generator::reg2str(ARMGeneralRegs reg) {
    switch (reg) {
        case ARMGeneralRegs::R0:
            return "r0";
        case ARMGeneralRegs::R1:
            return "r1";
        case ARMGeneralRegs::R2:
            return "r2";
        case ARMGeneralRegs::R3:
            return "r3";
        case ARMGeneralRegs::R4:
            return "r4";
        case ARMGeneralRegs::R5:
            return "r5";
        case ARMGeneralRegs::R6:
            return "r6";
        case ARMGeneralRegs::R7:
            return "r7";
        case ARMGeneralRegs::R8:
            return "r8";
        case ARMGeneralRegs::R9:
            return "r9";
        case ARMGeneralRegs::R10:
            return "r10";
        case ARMGeneralRegs::R11:
            return "r11";
        case ARMGeneralRegs::IP:
            return "ip";
        case ARMGeneralRegs::SP:
            return "sp";
        case ARMGeneralRegs::LR:
            return "lr";
        case ARMGeneralRegs::None:
        default:
            fprintf(stderr, "Invalid Register!");
            exit(-1);
    }
}

void Generator::genGlobalDef(GlobalObject *obj) {
    if (obj->tryIntoFunction() != nullptr) {
        auto func = obj->asFunction();
        println("   .globl %s", func->name().data());
        println("   .p2align 2");
        println("   .type %s, %%function", func->name().data());
        println("%s:", func->name().data());
    } else if (obj->tryIntoGlobalVariable()) {
        auto globvar  = obj->asGlobalVariable();
        auto initData = globvar->type()->tryGetElementType();
        println("   .type %s, %%object", globvar->name().data());
        if (initData->isArray()) {
            uint32_t size = 1;
            auto     e    = initData;
            while (e != nullptr && e->isArray()) {
                size *= e->asArrayType()->size();
                e     = e->tryGetElementType();
            }
            println("   .comm %s, %d, %d", globvar->name().data(), size * 4, 4);
        } else {
            println("   .data");
            println("   .globl %s", globvar->name().data());
            println("   .p2align 2");
            println("%s:", globvar->name().data());
            //! TODO: float global var
            assert(!initData->isFloat());
            if (initData->isInteger()) {
                println(
                    "   .long   %d",
                    static_cast<const ConstantInt *>(globvar->data())->value);
                println("   .size %s, 4\n", globvar->name().data());
            }
        }
    }
}

void Generator::genUsedGlobVars() {
    for (auto e : *generator_.usedGlobalVars) {
        println("%s:", e.second.data());
        println("   .long %s", e.first->val->name().data());
    }
    println("");
}

void Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    genGlobalDef(func);
    for (auto block : func->basicBlocks()) {
        println(
            ".F%dBB.%d:",
            generator_.cur_funcnum,
            getBlockNum(block->id())); // label
        generator_.cur_block = block;
        genInstList(&block->instructions());
    }
}

Variable *Generator::findVariable(Value *val) {
    auto blockVarTable = generator_.allocator->blockVarTable;
    auto valVarTable   = blockVarTable->find(generator_.cur_block)->second;
    auto it            = valVarTable->find(val);
    if (it == valVarTable->end()) { assert(0 && "unexpected error"); }
    return it->second;
}

void Generator::addUsedGlobalVar(Variable *var) {
    assert(var->is_global);
    static uint32_t nrGlobVar = 0;
    static char     str[10];
    if (generator_.usedGlobalVars->find(var)
        != generator_.usedGlobalVars->end())
        return;
    sprintf(str, "GlobAddr%d", nrGlobVar++);
    generator_.usedGlobalVars->insert({var, str});
}

void Generator::saveCallerReg() {
    RegList regList;
    assert(generator_.allocator->usedRegs.empty());
    generator_.allocator->getUsedRegs(generator_.cur_func->basicBlocks());
    auto usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    cgPush(regList);
}

void Generator::restoreCallerReg() {
    RegList regList;
    auto    usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    cgPop(regList);
}

// 将IR中基本块的id转成从0开始的顺序编号
int Generator::getBlockNum(int blockid) {
    BasicBlockList blocklist = generator_.cur_func->basicBlocks();
    int            num       = 0;
    for (auto block : blocklist) {
        if (block->id() == blockid) { return num; }
        num++;
    }
    assert(0 && "Can't reach here");
}

// 获得下一个基本块，如果没有则返回空指针
BasicBlock *Generator::getNextBlock() {
    BasicBlockList blocklist = generator_.cur_func->basicBlocks();
    auto           it        = blocklist.node_begin();
    auto           end       = blocklist.node_end();
    while (it != end) {
        auto block = it->value();
        if (block == generator_.cur_block) {
            ++it;
            if (it != end)
                return it->value();
            else
                return nullptr;
        }
        ++it;
    }
    assert(0 && "Can't reach here");
}

Instruction *Generator::getNextInst(Instruction *inst) {
    auto instlist = generator_.cur_block->instructions();
    auto it       = instlist.node_begin();
    auto end      = instlist.node_end();
    while (it != end) {
        if (it->value() == inst) {
            it++;
            if (it != end)
                return it->value();
            else
                return nullptr;
        }
        ++it;
    }
    assert(0 && "Can't reach here");
}

void Generator::genInstList(InstructionList *instlist) {
    int  allocaSize = 0;
    bool flag       = false;
    for (auto inst : *instlist) {
        generator_.allocator->cur_inst++;
        if (inst->id() == InstructionID::Alloca) {
            allocaSize += genAllocaInst(inst->asAlloca());
            flag        = true;
            continue;
        } else if (
            (flag || generator_.allocator->cur_inst == 1)
            && inst->id() != InstructionID::Alloca) {
            flag = false;
            saveCallerReg();
            cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, allocaSize);
        }
        genInst(inst);
    }
}

void Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    assert(instID != InstructionID::Alloca);
    generator_.allocator->updateAllocation(this, generator_.cur_block, inst);

    switch (inst->id()) {
        case InstructionID::Alloca:
            genAllocaInst(inst->asAlloca());
            break;
        case InstructionID::Load:
            genLoadInst(inst->asLoad());
            break;
        case InstructionID::Store:
            genStoreInst(inst->asStore());
            break;
        case InstructionID::Ret:
            genRetInst(inst->asRet());
            break;
        case InstructionID::Br:
            genBrInst(inst->asBr());
            break;
        case InstructionID::GetElementPtr:
            genGetElemPtrInst(inst->asGetElementPtr());
            break;
        case InstructionID::Add:
            genAddInst(inst->asAdd());
            break;
        case InstructionID::Sub:
            genSubInst(inst->asSub());
            break;
        case InstructionID::Mul:
            genMulInst(inst->asMul());
            break;
        case InstructionID::UDiv:
            genUDivInst(inst->asUDiv());
            break;
        case InstructionID::SDiv:
            genSDivInst(inst->asSDiv());
            break;
        case InstructionID::URem:
            genURemInst(inst->asURem());
            break;
        case InstructionID::SRem:
            genSRemInst(inst->asSRem());
            break;
        case InstructionID::FNeg:
            genFNegInst(inst->asFNeg());
            break;
        case InstructionID::FAdd:
            genFAddInst(inst->asFAdd());
            break;
        case InstructionID::FSub:
            genFSubInst(inst->asFSub());
            break;
        case InstructionID::FMul:
            genFMulInst(inst->asFMul());
            break;
        case InstructionID::FDiv:
            genFDivInst(inst->asFDiv());
            break;
        case InstructionID::FRem:
            genFRemInst(inst->asFRem());
            break;
        case InstructionID::Shl:
            genShlInst(inst->asShl());
            break;
        case InstructionID::LShr:
            genLShrInst(inst->asLShr());
            break;
        case InstructionID::AShr:
            genAShrInst(inst->asAShr());
            break;
        case InstructionID::And:
            genAndInst(inst->asAnd());
            break;
        case InstructionID::Or:
            genOrInst(inst->asOr());
            break;
        case InstructionID::Xor:
            genXorInst(inst->asXor());
            break;
        case InstructionID::FPToUI:
            genFPToUIInst(inst->asFPToUI());
            break;
        case InstructionID::FPToSI:
            genFPToSIInst(inst->asFPToSI());
            break;
        case InstructionID::UIToFP:
            genUIToFPInst(inst->asUIToFP());
            break;
        case InstructionID::SIToFP:
            genSIToFPInst(inst->asSIToFP());
            break;
        case InstructionID::ICmp:
            genICmpInst(inst->asICmp());
            break;
        case InstructionID::FCmp:
            genFCmpInst(inst->asFCmp());
            break;
            genPhiInst(inst->asPhi());
            break;
        case InstructionID::Call:
            genCallInst(inst->asCall());
            break;
        default:
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            exit(-1);
    }
    generator_.allocator->checkLiveInterval();
}

int Generator::genAllocaInst(AllocaInst *inst) {
    Instruction *pinst = inst;
    auto         e     = inst->unwrap()->type()->tryGetElementType();
    int          size  = 1;
    while (e != nullptr && e->isArray()) {
        size *= e->asArrayType()->size();
        e     = e->tryGetElementType();
    }
    auto var       = findVariable(inst->unwrap());
    var->is_alloca = true;
    generator_.stack->spillVar(var, size * 4);
    return size * 4;
}

void Generator::genLoadInst(LoadInst *inst) {
    auto           targetVar = findVariable(inst->unwrap());
    auto           source    = inst->useAt(0);
    int            offset;
    ARMGeneralRegs sourceReg;
    if (source->isLabel()) {
        //! TODO: label
        assert(0);
    } else {
        assert(source->type()->isPointer());
        auto sourceVar = findVariable(source);

        if (sourceVar->is_alloca) {
            sourceReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize - sourceVar->stackpos;
        } else if (sourceVar->is_global) {
            addUsedGlobalVar(sourceVar);
            offset = 0;
            assert(sourceVar->reg != ARMGeneralRegs::None);
            cgLdr(sourceVar->reg, sourceVar);
            sourceReg = sourceVar->reg;
            // if (sourceVar->reg == ARMGeneralRegs::None) {
            //     cgLdr(targetVar->reg, sourceVar);
            //     sourceReg = targetVar->reg;
            // } else
            //     sourceReg = sourceVar->reg;
        } else {
            sourceReg = sourceVar->reg;
            offset    = 0;
        }
        cgLdr(targetVar->reg, sourceReg, offset);
    }
}

void Generator::genStoreInst(StoreInst *inst) {
    auto           target = inst->useAt(0);
    auto           source = inst->useAt(1);
    ARMGeneralRegs sourceReg, targetReg;
    int            offset;
    if (source.value()->isConstant()) {
        if (source.value()->type()->isInteger()) {
            sourceReg   = generator_.allocator->allocateRegister();
            int32_t imm = static_cast<ConstantInt *>(source.value())->value;
            if (sourceReg == ARMGeneralRegs::None) {
                //! TODO: spill something
                assert(0 && "TODO!");
            }
            cgMov(sourceReg, imm);
        } else if (source.value()->type()->isFloat()) {
            //! TODO: float
            assert(0);
        }
    } else {
        auto sourceVar = findVariable(source);
        if (sourceVar->is_spilled) {
            //! TODO: handle spilled variable
            assert(0);
        }
        sourceReg = sourceVar->reg;
    }
    auto targetVar = findVariable(target);
    if (targetVar->is_global) {
        assert(targetVar->reg != ARMGeneralRegs::None);
        targetReg = targetVar->reg;
        addUsedGlobalVar(targetVar);
        cgLdr(targetReg, targetVar);
        cgStr(sourceReg, targetReg, 0);
    } else {
        if (targetVar->is_alloca) {
            targetReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize - targetVar->stackpos;
        } else {
            targetReg = targetVar->reg;
            offset    = 0;
        }
        cgStr(sourceReg, targetReg, offset);
    }
    if (source.value()->isConstant())
        generator_.allocator->releaseRegister(sourceReg);
}

void Generator::genRetInst(RetInst *inst) {
    if (inst->totalOperands() != 0) {
        auto operand = inst->useAt(0).value();
        if (operand->isImmediate()) {
            assert(operand->type()->isInteger());
            cgMov(
                ARMGeneralRegs::R0, static_cast<ConstantInt *>(operand)->value);
        } else {
            assert(Allocator::isVariable(operand));
            auto var = findVariable(operand);
            if (var->reg != ARMGeneralRegs::R0)
                cgMov(ARMGeneralRegs::R0, var->reg);
        }
    }
    if (generator_.stack->stackSize > 0)
        cgAdd(
            ARMGeneralRegs::SP,
            ARMGeneralRegs::SP,
            generator_.stack->stackSize);
    restoreCallerReg();
    cgBx(ARMGeneralRegs::LR);
}

void Generator::genBrInst(BrInst *inst) {
    BasicBlock *nextblock = getNextBlock();
    if (generator_.cur_block->isLinear()) {
        Value *target = inst->useAt(0);
        if (target->id() != nextblock->id()) cgB(inst->useAt(0));
    } else {
        auto control = inst->parent()->control()->asInstruction();
        ComparePredicationType predict;
        if (control->id() == InstructionID::ICmp) {
            predict = control->asICmp()->predicate();
        } else if (control->id() == InstructionID::Load) {
            predict = ComparePredicationType::EQ;
        } else {
            assert(0 && "FCMP");
        }

        Variable *cond    = findVariable(inst->useAt(0));
        Value    *target1 = inst->useAt(1), *target2 = inst->useAt(2);
        if (cond->reg != ARMGeneralRegs::None) {
            cgTst(cond->reg, 1);
            cgB(target2, predict);
            cgB(target1);
        }
        // if (nextblock->id() == target1->id()) {
        //     cgB(target2, ComparePredicationType::EQ);
        // } else if (nextblock->id() == target2->id()) {
        //     cgB(target1, ComparePredicationType::NE);
        // } else {
        cgB(target1, predict);
        cgB(target2);
        // }
    }
}

int Generator::sizeOfType(ir::Type *type) {
    switch (type->kind()) {
        case TypeKind::Integer:
        case TypeKind::Float:
        case TypeKind::Pointer: {
            //! FIXME: depends on 32-bit arch
            return 4;
        } break;
        case TypeKind::Array: {
            return sizeOfType(type->tryGetElementType())
                 * type->asArrayType()->size();
        }
        default: {
            assert(false);
            return -1;
        }
    }
}

void Generator::genGetElemPtrInst(GetElementPtrInst *inst) {
    //! NOTE: 2 more extra registers requried

    //! addr = base + i[0] * sizeof(type) + i[1] * sizeof(*type) + ...
    auto baseType = inst->op<0>()->type()->tryGetElementType();
    println("; %%%d:", inst->unwrap()->id());
    //! dest is initially assign to base addr
    auto dest = findVariable(inst->unwrap())->reg;
    if (auto var = findVariable(inst->op<0>()); var->is_alloca) {
        cgMov(dest, ARMGeneralRegs::SP);
        cgAdd(dest, dest, generator_.stack->stackSize - var->stackpos);
    } else if (var->is_global) {
        cgLdr(dest, var);
    } else {
        assert(!var->is_spilled);
        assert(!inst->op<0>()->isImmediate());
        cgMov(dest, var->reg);
    }

    decltype(dest) *tmp = nullptr;
    decltype(dest)  regPlaceholder{};
    bool            tmpIsAllocated = false;
    for (int i = 1; i < inst->totalOperands(); ++i) {
        auto op = inst->op()[i];
        if (op == nullptr) {
            assert(i > 1);
            break;
        }
        //! tmp <- index
        int multiplier = -1;
        if (op->isImmediate()) {
            auto imm   = static_cast<ConstantInt *>(op.value())->value;
            multiplier = imm;
            if (multiplier >= 1) {
                regPlaceholder = generator_.allocator->allocateRegister();
                tmp            = &regPlaceholder;
                tmpIsAllocated = true;
                if (multiplier > 1) { cgMov(*tmp, imm); }
            }
        } else if (op->isGlobal()) {
            //! FIXME: assume that reg of global variable is free to use
            auto var = findVariable(op);
            tmp      = &var->reg;
            cgLdr(*tmp, var);
            cgLdr(*tmp, *tmp, 0);
        } else {
            auto var = findVariable(op);
            assert(var != nullptr);
            tmp = &var->reg;
        }

        //! offset <- stride * index
        const auto stride = sizeOfType(baseType);

        if (multiplier == -1) {
            auto reg = generator_.allocator->allocateRegister();
            cgMov(reg, stride);
            cgMul(reg, *tmp, reg);
            //! swap register: *tmp <- reg
            if (tmpIsAllocated) { generator_.allocator->releaseRegister(*tmp); }
            regPlaceholder = reg;
        } else if (multiplier > 0) {
            if (multiplier == 1) {
                cgMov(*tmp, stride);
            } else if (multiplier == 2) {
                cgMov(*tmp, stride);
                cgAdd(*tmp, *tmp, *tmp);
            } else {
                auto reg = generator_.allocator->allocateRegister();
                cgMov(reg, stride);
                cgMul(reg, *tmp, reg);
                generator_.allocator->releaseRegister(reg);
            }
        }

        if (multiplier != 0) {
            //! dest <- base + offset
            cgAdd(dest, dest, *tmp);
        }

        if (tmpIsAllocated) {
            generator_.allocator->releaseRegister(*tmp);
            tmp            = nullptr;
            tmpIsAllocated = false;
        }

        if (i + 1 == inst->totalOperands() || inst->op()[i + 1] == nullptr) {
            break;
        }

        baseType = baseType->tryGetElementType();
    }

    // for (int i = 1; i < inst->totalOperands(); ++i) {
    //     if (inst->useAt(i).value() == nullptr) { break; }
    //     if (inst->useAt(i)->isImmediate()) {
    //         reg = generator_.allocator->allocateRegister();
    //         cgMov(
    //             reg,
    //             static_cast<ConstantInt *>(inst->useAt(i)->asConstantData())
    //                 ->value);
    //     } else {
    //         reg = findVariable(inst->useAt(i))->reg;
    //     }
    //     auto reg_imm = generator_.allocator->allocateRegister();
    //     cgMov(reg_imm, sizeOfType(baseType));
    //     cgMul(reg_imm, reg, reg_imm);
    //     generator_.allocator->releaseRegister(reg);
    //     generator_.allocator->releaseRegister(reg_imm);
    //     cgAdd(dest, dest, reg_imm);
    //     //! FIXME: combine with ldr dest, addr, offset_reg
    //     if (i + 1 == inst->totalOperands()) { break; }
    //     cgLdr(dest, dest, 0);
    //     baseType = baseType->tryGetElementType();
    // }

    // auto targetVar         = findVariable(inst->unwrap()),
    //      sourceVar         = findVariable(inst->useAt(0));
    // auto           tmp     = inst->useAt(0)->type()->tryGetElementType();
    // ARMGeneralRegs tmpreg  = generator_.allocator->allocateRegister();
    // ARMGeneralRegs tmpreg2 = generator_.allocator->allocateRegister();
    // assert(tmpreg != ARMGeneralRegs::None);
    // assert(tmpreg2 != ARMGeneralRegs::None);
    // println(";%%%d", inst->unwrap()->id());
    // if (inst->unwrap()->id() == 50) { tmp->isArray(); }
    // cgMov(targetVar->reg, 0);
    // assert(tmp != nullptr && tmp->isArray());
    // for (int i = 1; i < inst->totalOperands() && tmp; i++) {
    //     auto idx  = inst->useAt(i).value();
    //     int  size = tmp->isInteger() ? 1 : tmp->asArrayType()->size();
    //     auto e    = tmp->tryGetElementType();
    //     while (e != nullptr && e->isArray()) {
    //         size *= e->asArrayType()->size();
    //         e     = e->tryGetElementType();
    //     }
    //     if (!Allocator::isVariable(idx)) {
    //         int imm = static_cast<ConstantInt
    //         *>(idx->asConstantData())->value; if (imm != 0) {
    //             cgMov(tmpreg, imm);
    //             cgMov(tmpreg2, size * 4);
    //             cgMul(tmpreg, tmpreg, tmpreg2);
    //             cgAdd(targetVar->reg, targetVar->reg, tmpreg);
    //         }
    //     } else {
    //         auto var = findVariable(idx);
    //         cgLsl(tmpreg, var->reg, size * 2);
    //         cgAdd(targetVar->reg, targetVar->reg, tmpreg);
    //     }
    //     tmp = tmp->tryGetElementType();
    // }
    // generator_.allocator->releaseRegister(tmpreg);
    // generator_.allocator->releaseRegister(tmpreg2);

    // if (sourceVar->is_alloca) {
    //     cgAdd(
    //         targetVar->reg,
    //         targetVar->reg,
    //         generator_.stack->stackSize - sourceVar->stackpos);
    //     cgAdd(targetVar->reg, ARMGeneralRegs::SP, targetVar->reg);
    // } else {
    //     cgAdd(targetVar->reg, targetVar->reg, sourceVar->reg);
    //     // assert(0 && "unfinished yet!\n");
    // }
    // println("");
}

void Generator::genAddInst(AddInst *inst) {
    ARMGeneralRegs rd  = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs  = findVariable(inst->useAt(0))->reg;
    auto           op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        cgAdd(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgAdd(rd, rs, imm);
    }
}

void Generator::genSubInst(SubInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        cgMov(rd, imm);
        rs = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        cgSub(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgSub(rd, rs, imm);
    }
}

void Generator::genMulInst(MulInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        cgMov(rd, imm);
        rs = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        cgMul(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgMul(rd, rs, imm);
    }
}

void Generator::genUDivInst(UDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSDivInst(SDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genURemInst(URemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSRemInst(SRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFNegInst(FNegInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFAddInst(FAddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFSubInst(FSubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFMulInst(FMulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFDivInst(FDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    assert(0 && "unfinished yet!\n");
}

void Generator::genFRemInst(FRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genShlInst(ShlInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        cgMov(rd, imm);
        rs = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        assert(0);
        // cgLsl(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgLsl(rd, rs, imm);
    }
}

void Generator::genLShrInst(LShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAShrInst(AShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAndInst(AndInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genOrInst(OrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genXorInst(XorInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFPToUIInst(FPToUIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFPToSIInst(FPToSIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genUIToFPInst(UIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSIToFPInst(SIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genICmpInst(ICmpInst *inst) {
    auto op1 = inst->useAt(0), op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        Variable *lhs = findVariable(op1), *rhs = findVariable(op2);
        cgCmp(lhs->reg, rhs->reg);
    } else {
        //! TODO: float
        assert(!op2->asConstantData()->type()->isFloat());
        Variable *lhs = findVariable(op1);
        int32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgCmp(lhs->reg, imm);
    }

    auto result = findVariable(inst->unwrap());
    if (result->reg != ARMGeneralRegs::None) {
        cgMov(result->reg, 0);
        cgMov(result->reg, 1, inst->predicate());
        cgAnd(result->reg, result->reg, 1);
    }
}

void Generator::genFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genPhiInst(PhiInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genCallInst(CallInst *inst) {
    for (int i = 0; i < inst->totalParams(); i++) {
        if (i < 4) {
            assert(!generator_.allocator->regAllocatedMap[i]);
            if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
                assert(!inst->paramAt(i)->type()->isFloat());
                auto constant = inst->paramAt(i)->asConstantData();
                cgMov(
                    static_cast<ARMGeneralRegs>(i),
                    static_cast<ConstantInt *>(constant)->value);
            } else {
                assert(Allocator::isVariable(inst->paramAt(i)));
                Variable *var = findVariable(inst->paramAt(i));
                if (var->is_alloca) {
                    cgMov(static_cast<ARMGeneralRegs>(i), ARMGeneralRegs::SP);
                    cgAdd(
                        static_cast<ARMGeneralRegs>(i),
                        static_cast<ARMGeneralRegs>(i),
                        generator_.stack->stackSize - var->stackpos);
                } else if (var->reg != static_cast<ARMGeneralRegs>(i)) {
                    assert(var->reg != ARMGeneralRegs::None);
                    cgMov(static_cast<ARMGeneralRegs>(i), var->reg);
                }
            }
        } else {
            assert(0);
            // 需要修改
            if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
                assert(!inst->paramAt(i)->type()->isFloat());
                generator_.stack->pushVar(
                    Variable::create(inst->paramAt(i)), 4);
            } else {
                assert(Allocator::isVariable(inst->paramAt(i)));
                Variable *var = findVariable(inst->paramAt(i).value());
                generator_.stack->pushVar(var, 4);
            }
        }
    }
    if (inst->unwrap()->uses().size() != 0) {
        generator_.allocator->regAllocatedMap[0] = true;
        auto var                                 = findVariable(inst->unwrap());
        var->reg                                 = ARMGeneralRegs::R0;
    }
    cgBl(inst->callee()->asFunction());
}

void Generator::cgMov(
    ARMGeneralRegs rd, ARMGeneralRegs rs, ComparePredicationType cond) {
    switch (cond) {
        case ComparePredicationType::TRUE:
            println("    mov    %s, %s", reg2str(rd), reg2str(rs));
            break;
        case ComparePredicationType::EQ:
            println("    moveq  %s, %s", reg2str(rd), reg2str(rs));
            break;
        default:
            assert(0 && "unfinished comparative type");
    }
}

void Generator::cgMov(
    ARMGeneralRegs rd, int32_t imm, ComparePredicationType cond) {
    switch (cond) {
        case ComparePredicationType::TRUE:
            println("    mov    %s, #%d", reg2str(rd), imm);
            break;
        case ComparePredicationType::EQ:
            println("    moveq  %s, #%d", reg2str(rd), imm);
            break;
        case ComparePredicationType::SLT:
        default:
            assert(0 && "unfinished comparative type");
    }
}

void Generator::cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(src), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(src));
    println("    ldr    %s, %s", reg2str(dst), tmpStr);
}

void Generator::cgLdr(ARMGeneralRegs dst, Variable *var) {
    assert(var->is_global);
    auto it = generator_.usedGlobalVars->find(var);
    if (it == generator_.usedGlobalVars->end())
        assert(0 && "it must be an error here.");
    println("    ldr    %s, %s", reg2str(dst), it->second.data());
}

void Generator::cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(dst), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(dst));
    println("    str    %s, %s", reg2str(src), tmpStr);
}

void Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    add    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    add    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    sub    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    sub    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgMul(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    mul    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgMul(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    mul    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgAnd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    and    %s, %s, #%d", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgAnd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    and    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgLsl(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    lsl    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgCmp(ARMGeneralRegs op1, ARMGeneralRegs op2) {
    println("    cmp    %s, %s", reg2str(op1), reg2str(op2));
}

void Generator::cgCmp(ARMGeneralRegs op1, int32_t op2) {
    println("    cmp    %s, #%d", reg2str(op1), op2);
}

void Generator::cgTst(ARMGeneralRegs op1, int32_t op2) {
    println("    tst    %s, #%d", reg2str(op1), op2);
}

void Generator::cgB(Value *brTarget, ComparePredicationType cond) {
    assert(brTarget->isLabel());
    size_t blockid = brTarget->id();
    switch (cond) {
        case ComparePredicationType::TRUE:
            println(
                "    b       .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
            break;
        case ComparePredicationType::EQ:
            println(
                "    beq     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
            break;
        case ComparePredicationType::NE:
            println(
                "    bne     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
            break;
        case ComparePredicationType::SLT:
        case ComparePredicationType::ULT:
            println(
                "    blt     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
            break;
        case ComparePredicationType::SGT:
            println(
                "    bgt     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
            break;
        default:
            assert(0);
    }
}

void Generator::cgBx(ARMGeneralRegs rd) {
    println("    bx     %s", reg2str(rd));
}

void Generator::cgBl(Function *callee) {
    println("    bl     %s", callee->name().data());
}

void Generator::cgPush(RegList &reglist) {
    fprintf(generator_.asmFile, "    push   {");
    for (auto reg : reglist) {
        fprintf(generator_.asmFile, "%s", reg2str(reg));
        if (reg != reglist.tail()->value()) printf(", ");
    }
    println("}");
}

void Generator::cgPop(RegList &reglist) {
    fprintf(generator_.asmFile, "    pop    {");
    for (auto reg : reglist) {
        fprintf(generator_.asmFile, "%s", reg2str(reg));
        if (reg != reglist.tail()->value()) printf(", ");
    }
    println("}");
}

} // namespace slime::backend
