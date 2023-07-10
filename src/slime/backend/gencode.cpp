#include "gencode.h"
#include "slime/ir/instruction.def"
#include "slime/ir/value.h"
#include "slime/ir/instruction.h"

namespace slime::backend {
void Generator::GenAssembly(Function *func) {
    assert(generator_.asmFile != nullptr);
    generator_.allocator = Allocator::create();
    for (auto block : func->basicBlocks()) {
        GenInstList(&block->instructions());
    }
}

void Generator::GenInstList(InstructionList *instlist) {
    for (auto inst : *instlist) { GenInst(inst); }
}

void Generator::GenInst(Instruction *inst) {
    InstructionID instID = inst->id();
    generator_.allocator->updateAllocation(inst);
    switch (inst->id()) {
        case InstructionID::Alloca:
            GenAllocaInst(inst->asAlloca());
            break;
        case InstructionID::Load:
            GenLoadInst(inst->asLoad());
            break;
        case InstructionID::Store:
            GenStoreInst(inst->asStore());
            break;
        case InstructionID::Ret:
            GenRetInst(inst->asRet());
            break;
        case InstructionID::Br:
            GenBrInst(inst->asBr());
        case InstructionID::GetElementPtr:
            GenGetElemPtrInst(inst->asGetElementPtr());
            break;
        case InstructionID::Add:
            GenAddInst(inst->asAdd());
            break;
        case InstructionID::Sub:
            GenSubInst(inst->asSub());
            break;
        case InstructionID::Mul:
            GenMulInst(inst->asMul());
            break;
        case InstructionID::UDiv:
            GenUDivInst(inst->asUDiv());
            break;
        case InstructionID::SDiv:
            GenSDivInst(inst->asSDiv());
            break;
        case InstructionID::URem:
            GenURemInst(inst->asURem());
            break;
        case InstructionID::SRem:
            GenSRemInst(inst->asSRem());
            break;
        case InstructionID::FNeg:
            GenFNegInst(inst->asFNeg());
            break;
        case InstructionID::FAdd:
            GenFAddInst(inst->asFAdd());
            break;
        case InstructionID::FSub:
            GenFSubInst(inst->asFSub());
            break;
        case InstructionID::FMul:
            GenFMulInst(inst->asFMul());
            break;
        case InstructionID::FDiv:
            GenFDivInst(inst->asFDiv());
            break;
        case InstructionID::FRem:
            GenFRemInst(inst->asFRem());
            break;
        case InstructionID::Shl:
            GenShlInst(inst->asShl());
            break;
        case InstructionID::LShr:
            GenLShrInst(inst->asLShr());
            break;
        case InstructionID::AShr:
            GenAShrInst(inst->asAShr());
            break;
        case InstructionID::And:
            GenAndInst(inst->asAnd());
            break;
        case InstructionID::Or:
            GenOrInst(inst->asOr());
            break;
        case InstructionID::Xor:
            GenXorInst(inst->asXor());
            break;
        case InstructionID::FPToUI:
            GenFPToUIInst(inst->asFPToUI());
            break;
        case InstructionID::FPToSI:
            GenFPToSIInst(inst->asFPToSI());
            break;
        case InstructionID::UIToFP:
            GenUIToFPInst(inst->asUIToFP());
            break;
        case InstructionID::SIToFP:
            GenSIToFPInst(inst->asSIToFP());
            break;
        case InstructionID::ICmp:
            GenICmpInst(inst->asICmp());
            break;
        case InstructionID::FCmp:
            GenFCmpInst(inst->asFCmp());
            break;
        case InstructionID::Phi:
            GenPhiInst(inst->asPhi());
            break;
        case InstructionID::Call:
            GenCallInst(inst->asCall());
            break;
        default:
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            exit(-1);
    }
}

void Generator::GenAllocaInst(AllocaInst *inst) {
    fprintf(generator_.asmFile, "sub sp, 4\n");
}

void Generator::GenLoadInst(LoadInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenStoreInst(StoreInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenRetInst(RetInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenBrInst(BrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenGetElemPtrInst(GetElementPtrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenAddInst(AddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenSubInst(SubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenMulInst(MulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenUDivInst(UDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenSDivInst(SDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenURemInst(URemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenSRemInst(SRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFNegInst(FNegInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFAddInst(FAddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFSubInst(FSubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFMulInst(FMulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFDivInst(FDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFRemInst(FRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenShlInst(ShlInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenLShrInst(LShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenAShrInst(AShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenAndInst(AndInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenOrInst(OrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenXorInst(XorInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFPToUIInst(FPToUIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFPToSIInst(FPToSIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenUIToFPInst(UIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenSIToFPInst(SIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenICmpInst(ICmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenPhiInst(PhiInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::GenCallInst(CallInst *inst) {
    assert(0 && "unfinished yet!\n");
}

} // namespace slime::backend