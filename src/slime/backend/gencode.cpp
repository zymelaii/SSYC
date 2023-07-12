#include "gencode.h"
#include "regalloc.h"

#include "slime/ir/instruction.def"
#include "slime/ir/value.h"
#include "slime/ir/user.h"
#include "slime/ir/instruction.h"
#include <cstddef>
#include <cstdio>

namespace slime::backend {

Generator *Generator::generate() {
    return new Generator();
}

void Generator::genCode(FILE *fp, Module *module) {
    generator_.asmFile   = fp;
    generator_.allocator = Allocator::create();
    generator_.stack     = generator_.allocator->stack;
    for (auto e : *module) {
        if (e->type()->isFunction()) {
            if (!strcmp(e->name().data(), "memset")) continue;
            genAssembly(static_cast<Function *>(e));
        }
    }
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

void Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    println("%s:", func->name().data());
    int cnt = 0;
    for (auto block : func->basicBlocks()) {
        println(".block%d:", cnt++); // label
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

void Generator::genInstList(InstructionList *instlist) {
    for (auto inst : *instlist) { genInst(inst); }
}

void Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    generator_.allocator->cur_inst++;
    generator_.allocator->updateAllocation(
        this, generator_.cur_block, generator_.allocator->cur_inst);
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
        case InstructionID::Phi:
            genPhiInst(inst->asPhi());
            break;
        case InstructionID::Call:
            genCallInst(inst->asCall());
            break;
        default:
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            exit(-1);
    }
}

void Generator::genAllocaInst(AllocaInst *inst) {
    auto e    = inst->unwrap()->type()->tryGetElementType();
    int  size = 1;
    while (e != nullptr && e->isArray()) {
        e     = e->tryGetElementType();
        size *= e->asArrayType()->size();
    }
    cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, size * 4);
    auto var = findVariable(inst->unwrap());
    generator_.allocator->releaseRegister(var->reg);
    var->is_alloca = true;
    generator_.stack->spillVar(var, size);
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
            offset    = generator_.stack->stackSize
                   - generator_.stack->lookupOnStackVar(sourceVar);
        };
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
        auto srcVar = findVariable(source);
        if (srcVar->is_spilled) {
            //! TODO: handle spilled variable
            assert(0);
        }
        sourceReg = srcVar->reg;
    }
    auto tarVar = findVariable(target);
    if (tarVar->is_global) {
        //! TODO: global variable
    } else if (tarVar->is_alloca) {
        targetReg = ARMGeneralRegs::SP;
        offset    = generator_.stack->stackSize
               - generator_.stack->lookupOnStackVar(tarVar);
    }
    cgStr(sourceReg, targetReg, offset);
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
            cgMov(ARMGeneralRegs::R0, var->reg);
        }
    }
    cgBx(ARMGeneralRegs::LR);
}

void Generator::genBrInst(BrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genGetElemPtrInst(GetElementPtrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAddInst(AddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSubInst(SubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genMulInst(MulInst *inst) {
    assert(0 && "unfinished yet!\n");
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
    assert(0 && "unfinished yet!\n");
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
    assert(0 && "unfinished yet!\n");
}

void Generator::genFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genPhiInst(PhiInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genCallInst(CallInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::cgMov(ARMGeneralRegs rd, ARMGeneralRegs rs) {
    println("    mov    %s, %s", reg2str(rd), reg2str(rs));
}

void Generator::cgMov(ARMGeneralRegs rd, int32_t imm) {
    println("    mov    %s, #%d", reg2str(rd), imm);
}

void Generator::cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(src), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(src));
    println("    ldr    %s, %s", reg2str(dst), tmpStr);
}

void Generator::cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(dst), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(dst));
    println("    str    %s, %s", reg2str(src), tmpStr);
}

void Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    sub    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    sub    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgBx(ARMGeneralRegs rd) {
    println("    bx     %s", reg2str(rd));
}

void Generator::cgPush(RegList &reglist) {
    printf("    push   [");
    for (auto reg : reglist) {
        printf("%s", reg2str(reg));
        if (reg != reglist.tail()->value()) printf(", ");
    }
    println("]");
}

} // namespace slime::backend