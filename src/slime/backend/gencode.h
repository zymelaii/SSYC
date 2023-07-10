#include "regalloc.h"

#include <slime/ir/type.h>
#include "slime/ir/value.h"
#include <slime/ir/module.h>
#include <slime/utils/list.h>
#include <slime/ir/instruction.h>
#include <cstdint>
#include <cstddef>

namespace slime::backend {

using namespace ir;

class Generator {
    Generator(){};

public:
    void GenAssembly(Function *func);

protected:
    void GenInstList(InstructionList *instlist);
    void GenInst(Instruction *inst);
    void GenAllocaInst(AllocaInst *inst);
    void GenLoadInst(LoadInst *inst);
    void GenStoreInst(StoreInst *inst);
    void GenRetInst(RetInst *inst);
    void GenBrInst(BrInst *inst);
    void GenGetElemPtrInst(GetElementPtrInst *inst);
    void GenAddInst(AddInst *inst);
    void GenSubInst(SubInst *inst);
    void GenMulInst(MulInst *inst);
    void GenUDivInst(UDivInst *inst);
    void GenSDivInst(SDivInst *inst);
    void GenURemInst(URemInst *inst);
    void GenSRemInst(SRemInst *inst);
    void GenFNegInst(FNegInst *inst);
    void GenFAddInst(FAddInst *inst);
    void GenFSubInst(FSubInst *inst);
    void GenFMulInst(FMulInst *inst);
    void GenFDivInst(FDivInst *inst);
    void GenFRemInst(FRemInst *inst);
    void GenShlInst(ShlInst *inst);
    void GenLShrInst(LShrInst *inst);
    void GenAShrInst(AShrInst *inst);
    void GenAndInst(AndInst *inst);
    void GenOrInst(OrInst *inst);
    void GenXorInst(XorInst *inst);
    void GenFPToUIInst(FPToUIInst *inst);
    void GenFPToSIInst(FPToSIInst *inst);
    void GenUIToFPInst(UIToFPInst *inst);
    void GenSIToFPInst(SIToFPInst *inst);
    void GenICmpInst(ICmpInst *inst);
    void GenFCmpInst(FCmpInst *inst);
    void GenPhiInst(PhiInst *inst);
    void GenCallInst(CallInst *inst);

    void Push();
    void Pop();

private:
    struct GeneratorState {
        BasicBlock *cur_block  = nullptr;
        int64_t     cur_pstack = 0; // 栈的深度
        Allocator  *allocator  = nullptr;
        FILE       *asmFile    = nullptr;
    };

    GeneratorState generator_;
};
} // namespace slime::backend