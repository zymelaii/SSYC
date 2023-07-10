#include "gencode.h"
#include "slime/ir/instruction.def"
#include "slime/ir/value.h"
#include "slime/ir/user.h"
#include "slime/ir/instruction.h"

namespace slime::backend {

Generator *Generator::generate() {
    return new Generator();
}

void Generator::genCode(FILE *fp, Module *module) {
    generator_.asmFile = fp;
    generator_.allocator = Allocator::create();
    for (auto e : *module) {
        if (e->type()->isFunction()) { genAssembly(static_cast<Function *>(e)); }
    }
}

void Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    for (auto block : func->basicBlocks()) {
        genInstList(&block->instructions());
    }
}

void Generator::genInstList(InstructionList *instlist) {
    for (auto inst : *instlist) { genInst(inst); }
}

void Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    // generator_.allocator->updateAllocation(inst);
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
    fprintf(generator_.asmFile, "sub sp, 4\n");
}

void Generator::genLoadInst(LoadInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genStoreInst(StoreInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genRetInst(RetInst *inst) {
    assert(0 && "unfinished yet!\n");
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

} // namespace slime::backend