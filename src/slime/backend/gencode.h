#include <cassert>
#include <slime/ir/type.h>
#include "slime/ast/decl.h"
#include "slime/ir/value.h"
#include <slime/ir/module.h>
#include <slime/utils/list.h>
#include <slime/ir/instruction.h>
#include <bits/types/FILE.h>

#include <cstdint>
#include <cstddef>
#include <stdarg.h>
#include <string_view>

namespace slime::backend {

struct Variable;
struct Stack;
class Allocator;
enum class ARMGeneralRegs;

using namespace ir;
using RegList        = utils::ListTrait<ARMGeneralRegs>;
using UsedGlobalVars = std::map<Variable *, std::string_view>;

class Generator {
    Generator(){};

public:
    static Generator *generate();
    void              genGlobalDef(GlobalObject *obj);
    void              genUsedGlobVars();
    void              genCode(FILE *fp, Module *module);
    void              genAssembly(Function *func);

    Variable          *findVariable(Value *val);
    static const char *reg2str(ARMGeneralRegs reg);

    void println(const char *fmt, ...) {
        assert(generator_.asmFile != nullptr);
        FILE   *output_file = generator_.asmFile;
        va_list ap;
        va_start(ap, fmt);
        vfprintf(output_file, fmt, ap);
        va_end(ap);
        fprintf(output_file, "\n");
    }

    void cgMov(ARMGeneralRegs rd, ARMGeneralRegs rs);
    void cgMov(ARMGeneralRegs rd, int32_t imm);
    void cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset);
    void cgLdr(ARMGeneralRegs dst, Variable *var); // only for globalvar
    void cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset);
    void cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    void cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    void cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    void cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    void cgPush(RegList &reglist);
    void cgPop(RegList &reglist);
    void cgBl(Function *callee);
    void cgBx(ARMGeneralRegs rd);

protected:
    void saveCallerReg();
    void restoreCallerReg();
    void addUsedGlobalVar(Variable *var);

    void genInstList(InstructionList *instlist);
    void genInst(Instruction *inst);
    int  genAllocaInst(AllocaInst *inst);
    void genLoadInst(LoadInst *inst);
    void genStoreInst(StoreInst *inst);
    void genRetInst(RetInst *inst);
    void genBrInst(BrInst *inst);
    void genGetElemPtrInst(GetElementPtrInst *inst);
    void genAddInst(AddInst *inst);
    void genSubInst(SubInst *inst);
    void genMulInst(MulInst *inst);
    void genUDivInst(UDivInst *inst);
    void genSDivInst(SDivInst *inst);
    void genURemInst(URemInst *inst);
    void genSRemInst(SRemInst *inst);
    void genFNegInst(FNegInst *inst);
    void genFAddInst(FAddInst *inst);
    void genFSubInst(FSubInst *inst);
    void genFMulInst(FMulInst *inst);
    void genFDivInst(FDivInst *inst);
    void genFRemInst(FRemInst *inst);
    void genShlInst(ShlInst *inst);
    void genLShrInst(LShrInst *inst);
    void genAShrInst(AShrInst *inst);
    void genAndInst(AndInst *inst);
    void genOrInst(OrInst *inst);
    void genXorInst(XorInst *inst);
    void genFPToUIInst(FPToUIInst *inst);
    void genFPToSIInst(FPToSIInst *inst);
    void genUIToFPInst(UIToFPInst *inst);
    void genSIToFPInst(SIToFPInst *inst);
    void genICmpInst(ICmpInst *inst);
    void genFCmpInst(FCmpInst *inst);
    void genPhiInst(PhiInst *inst);
    void genCallInst(CallInst *inst);

private:
    struct GeneratorState {
        BasicBlock     *cur_block      = nullptr;
        Function       *cur_func       = nullptr;
        Allocator      *allocator      = nullptr;
        Stack          *stack          = nullptr;
        FILE           *asmFile        = nullptr;
        UsedGlobalVars *usedGlobalVars = nullptr;
    };

    GeneratorState generator_;
};
} // namespace slime::backend