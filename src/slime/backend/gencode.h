#include <slime/ir/type.h>
#include <slime/ast/decl.h>
#include <slime/ir/instruction.def>
#include <slime/ir/value.h>
#include <slime/ir/module.h>
#include <slime/utils/list.h>
#include <slime/ir/instruction.h>

#include <set>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string_view>
#include <vector>
#include <ostream>

namespace slime::backend {

struct Variable;
struct Stack;
class Allocator;
enum class ARMGeneralRegs;

using namespace ir;
using RegList        = utils::ListTrait<ARMGeneralRegs>;
using UsedGlobalVars = std::map<Variable *, std::string>;

class Generator {
    Generator(){};

public:
    static Generator *generate();
    std::string       genCode(Module *module);
    std::string       genGlobalDef(GlobalObject *obj);
    std::string       genUsedGlobVars();
    std::string       genAssembly(Function *func);

    Variable          *findVariable(Value *val);
    static const char *reg2str(ARMGeneralRegs reg);
    Instruction       *getNextInst(Instruction *inst);
    int                sizeOfType(ir::Type *type);
    bool               isImmediateValid(uint32_t imm);

    static std::string sprintln(const char *fmt, ...) {
        static char strbuf[64];
        va_list     ap;
        va_start(ap, fmt);
        vsprintf(strbuf, fmt, ap);
        va_end(ap);
        sprintf(strbuf + strlen(strbuf), "\n");
        return std::string(strbuf);
    }

    std::string cgMov(
        ARMGeneralRegs         rd,
        ARMGeneralRegs         rs,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgMov(
        ARMGeneralRegs         rd,
        int32_t                imm,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset);
    std::string cgLdr(ARMGeneralRegs dst, int32_t imm);
    std::string cgLdr(ARMGeneralRegs dst, Variable *var); // only for globalvar
    std::string cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset);
    std::string cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    std::string cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    std::string cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    std::string cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    std::string cgMul(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    std::string cgMul(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    std::string cgAnd(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    std::string cgAnd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    // void cgLsl(ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2);
    std::string cgLsl(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    std::string cgAsr(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2);
    std::string cgCmp(ARMGeneralRegs op1, ARMGeneralRegs op2);
    std::string cgCmp(ARMGeneralRegs op1, int32_t op2);
    // void cgTst(ARMGeneralRegs op1, ARMGeneralRegs op2);
    std::string cgTst(ARMGeneralRegs op1, int32_t op2);
    std::string cgPush(RegList &reglist);
    std::string cgPop(RegList &reglist);
    std::string cgB(
        Value                 *brTarget,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgBl(Function *callee);
    std::string cgBl(const char *libfuncname);
    std::string cgBx(ARMGeneralRegs rd);

protected:
    std::string            saveCallerReg();
    std::string            restoreCallerReg();
    void                   addUsedGlobalVar(Variable *var);
    BasicBlock            *getNextBlock();
    int                    getBlockNum(int blockid);
    ComparePredicationType reversePredict(ComparePredicationType predict);

    std::string genInstList(InstructionList *instlist);
    std::string genInst(Instruction *inst);
    int         genAllocaInst(AllocaInst *inst);
    std::string genLoadInst(LoadInst *inst);
    std::string genStoreInst(StoreInst *inst);
    std::string genRetInst(RetInst *inst);
    std::string genBrInst(BrInst *inst);
    std::string genGetElemPtrInst(GetElementPtrInst *inst);
    std::string genAddInst(AddInst *inst);
    std::string genSubInst(SubInst *inst);
    std::string genMulInst(MulInst *inst);
    std::string genUDivInst(UDivInst *inst);
    std::string genSDivInst(SDivInst *inst);
    std::string genURemInst(URemInst *inst);
    std::string genSRemInst(SRemInst *inst);
    std::string genFNegInst(FNegInst *inst);
    std::string genFAddInst(FAddInst *inst);
    std::string genFSubInst(FSubInst *inst);
    std::string genFMulInst(FMulInst *inst);
    std::string genFDivInst(FDivInst *inst);
    std::string genFRemInst(FRemInst *inst);
    std::string genShlInst(ShlInst *inst);
    std::string genLShrInst(LShrInst *inst);
    std::string genAShrInst(AShrInst *inst);
    std::string genAndInst(AndInst *inst);
    std::string genOrInst(OrInst *inst);
    std::string genXorInst(XorInst *inst);
    std::string genFPToUIInst(FPToUIInst *inst);
    std::string genFPToSIInst(FPToSIInst *inst);
    std::string genUIToFPInst(UIToFPInst *inst);
    std::string genSIToFPInst(SIToFPInst *inst);
    std::string genICmpInst(ICmpInst *inst);
    std::string genFCmpInst(FCmpInst *inst);
    std::string genZExtInst(ZExtInst *inst);
    std::string genCallInst(CallInst *inst);

private:
    std::set<std::string> libfunc = {
        "memset",
        "putint",
        "getint",
        "putarray",
        "getarray",
        "putch",
        "getch",
        "__aeabi_idiv",
        "__aeabi_uidiv"};

    struct GeneratorState {
        BasicBlock     *cur_block      = nullptr;
        Function       *cur_func       = nullptr;
        size_t          cur_funcnum    = 0;
        Allocator      *allocator      = nullptr;
        Stack          *stack          = nullptr;
        UsedGlobalVars *usedGlobalVars = nullptr;
    };

    // std::ostream    os;
    GeneratorState generator_;
};
} // namespace slime::backend
