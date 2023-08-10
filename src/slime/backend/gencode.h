#include <slime/ir/user.h>
#include <slime/ir/type.h>
#include <slime/ast/decl.h>
#include <slime/ir/instruction.def>
#include <slime/ir/value.h>
#include <slime/ir/module.h>
#include <slime/utils/list.h>
#include <slime/ir/instruction.h>
#include <slime/experimental/utils/LinkedList.h>

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

using namespace ir;

struct Variable;
struct Stack;

struct InstCode {
    InstCode(Instruction *inst)
        : inst{inst} {};

    std::string  code;
    Instruction *inst;
};

using InstCodeList = slime::LinkedList<InstCode *>;

struct BlockCode {
    BlockCode()
        : instcodes{nullptr} {};

    BlockCode(InstCodeList *instcodes)
        : instcodes{instcodes} {};

    std::string   code;
    InstCodeList *instcodes;
};

class Allocator;
enum class ARMGeneralRegs;
enum class ARMFloatRegs;

using RegList         = utils::ListTrait<ARMGeneralRegs>;
using FpRegList       = utils::ListTrait<ARMFloatRegs>;
using BlockCodeList   = slime::LinkedList<BlockCode *>;
using UsedGlobalVars  = std::map<Variable *, std::string>;
using FloatConstants  = LinkedList<std::pair<int, float>>;
using FpConstVersoins = std::map<int, int>;

class Generator {
    friend class InstrOp;

    Generator(){};
    size_t relInstrCursor = 0; //<! helps to insert constant data
    size_t nextLabelId    = 0;
    size_t nextImmId      = 0;
    std::map<int32_t, size_t> recentUsedImmediates;

public:
    static Generator *generate();
    std::string       genCode(Module *module);
    std::string       genGlobalArrayInitData(
              ConstantArray *globarr, uint32_t baseSize);
    std::string genFloatConstants();
    std::string genGlobalDef(GlobalObject *obj);
    std::string genFloatConstans();
    std::string genUsedGlobVars();
    std::string genAssembly(Function *func);
    std::string genInFuncDataSection();
    std::string requireImmediate(int32_t imm);
    std::string genImmediatePool();

    Variable          *findVariable(Value *val);
    static const char *reg2str(ARMGeneralRegs reg);
    static const char *reg2str(ARMFloatRegs reg);
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

    std::string instrln(const char *instr, const char *fmt, ...) {
        ++relInstrCursor;
        const auto opcodeWidth     = 5;
        const auto longOpcodeWidth = 14;
        const auto indent          = 4;
        const auto space           = 2;
        char       args[64]{};
        va_list    ap;
        va_start(ap, fmt);
        vsprintf(args, fmt, ap);
        va_end(ap);
        auto code = sprintln(
            "%*s%-*s%*s%s",
            indent,
            "",
            strlen(instr) <= opcodeWidth ? opcodeWidth : longOpcodeWidth,
            instr,
            space,
            "",
            args);
        if (relInstrCursor % 512 == 0) {
            auto section = genInFuncDataSection();
            if (!section.empty()) {
                code += instrln("b", ".LBDATA.%d", nextLabelId);
                code += section;
                code += sprintln(".LBDATA.%d:", nextLabelId);
                ++nextLabelId;
            }
        }
        return code;
    }

protected:
    void attachIRInformation(InstCode *instcode);

public:
    std::string cgMov(
        ARMGeneralRegs         rd,
        ARMGeneralRegs         rs,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgMov(
        ARMGeneralRegs         rd,
        int32_t                imm,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgMov(
        ARMGeneralRegs         rd,
        ARMFloatRegs           rs,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset);
    std::string cgLdr(
        ARMGeneralRegs dst, ARMGeneralRegs src, ARMGeneralRegs offset);
    std::string cgLdr(ARMGeneralRegs dst, int32_t imm);
    std::string cgLdr(ARMGeneralRegs dst, Variable *var); // only for globalvar
    std::string cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset);
    std::string cgStr(
        ARMGeneralRegs src, ARMGeneralRegs dst, ARMGeneralRegs offset);
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
    std::string cgPush(FpRegList &reglist);
    std::string cgPop(RegList &reglist);
    std::string cgPop(FpRegList &reglist);
    std::string cgB(
        Value                 *brTarget,
        ComparePredicationType cond = ComparePredicationType::TRUE);
    std::string cgBl(Function *callee);
    std::string cgBl(const char *libfuncname);
    std::string cgBx(ARMGeneralRegs rd);

    std::string cgVmov(
        ARMFloatRegs           rd,
        float                  imm,
        ComparePredicationType cond = ComparePredicationType::TRUE);

    std::string cgVmov(ARMFloatRegs rd, ARMFloatRegs rs);
    std::string cgVmov(ARMFloatRegs rd, ARMGeneralRegs rs);
    std::string cgVmov(ARMGeneralRegs rd, ARMFloatRegs rs);
    std::string cgVldr(ARMFloatRegs dst, ARMGeneralRegs src, int32_t offset);
    std::string cgVldr(
        ARMFloatRegs dst, ARMGeneralRegs src, ARMGeneralRegs offset);
    std::string cgVldr(ARMGeneralRegs dst, Variable *var);
    std::string cgVstr(ARMFloatRegs src, ARMGeneralRegs dst, int32_t offset);
    std::string cgVstr(
        ARMFloatRegs src, ARMGeneralRegs dst, ARMGeneralRegs offset);
    std::string cgVcvt(
        ARMFloatRegs fltreg,
        ARMFloatRegs intreg,
        bool         direction,
        bool         sextflag);
    std::string cgVcvt(ARMGeneralRegs intreg, float fltval, bool sextflag);
    std::string cgVcvt(ARMFloatRegs intreg, int intval, bool sextflag);
    std::string cgVadd(ARMFloatRegs rd, ARMFloatRegs op1, ARMFloatRegs op2);
    std::string cgVsub(ARMFloatRegs rd, ARMFloatRegs op1, ARMFloatRegs op2);
    std::string cgVcmp(ARMFloatRegs op1, ARMFloatRegs op2);
    std::string cgVneg(ARMFloatRegs rd, ARMFloatRegs rs);
    std::string cgVmul(ARMFloatRegs rd, ARMFloatRegs op1, ARMFloatRegs op2);
    std::string cgVdiv(ARMFloatRegs rd, ARMFloatRegs op1, ARMFloatRegs op2);

protected:
    std::string            saveCallerReg();
    std::string            restoreCallerReg();
    std::string            loadFloatConstant(ARMFloatRegs rd, float imm);
    std::string            indirectLoadFpConstant(ARMFloatRegs rd, int index);
    std::string            genIndirectFpConstantAddrs();
    std::string            unpackInstCodeList(InstCodeList &instCodeList);
    std::string            unpackBlockCodeList(BlockCodeList &blockCodeList);
    void                   checkStackChanges(BlockCodeList &blockCodeList);
    void                   addUsedGlobalVar(Variable *var);
    void                   refreshUsedGlobalVar();
    BasicBlock            *getNextBlock();
    int                    getBlockNum(int blockid);
    ComparePredicationType reversePredict(ComparePredicationType predict);

    InstCodeList *genInstList(InstructionList *instlist);
    InstCode     *genInst(Instruction *inst);
    int           genAllocaInst(AllocaInst *inst);
    InstCode     *genLoadInst(LoadInst *inst);
    InstCode     *genStoreInst(StoreInst *inst);
    InstCode     *genRetInst(RetInst *inst);
    InstCode     *genBrInst(BrInst *inst);
    InstCode     *genGetElemPtrInst(GetElementPtrInst *inst);
    InstCode     *genAddInst(AddInst *inst);
    InstCode     *genSubInst(SubInst *inst);
    InstCode     *genMulInst(MulInst *inst);
    InstCode     *genUDivInst(UDivInst *inst);
    InstCode     *genSDivInst(SDivInst *inst);
    InstCode     *genURemInst(URemInst *inst);
    InstCode     *genSRemInst(SRemInst *inst);
    InstCode     *genFNegInst(FNegInst *inst);
    InstCode     *genFAddInst(FAddInst *inst);
    InstCode     *genFSubInst(FSubInst *inst);
    InstCode     *genFMulInst(FMulInst *inst);
    InstCode     *genFDivInst(FDivInst *inst);
    InstCode     *genFRemInst(FRemInst *inst);
    InstCode     *genShlInst(ShlInst *inst);
    InstCode     *genLShrInst(LShrInst *inst);
    InstCode     *genAShrInst(AShrInst *inst);
    InstCode     *genAndInst(AndInst *inst);
    InstCode     *genOrInst(OrInst *inst);
    InstCode     *genXorInst(XorInst *inst);
    InstCode     *genFPToUIInst(FPToUIInst *inst);
    InstCode     *genFPToSIInst(FPToSIInst *inst);
    InstCode     *genUIToFPInst(UIToFPInst *inst);
    InstCode     *genSIToFPInst(SIToFPInst *inst);
    InstCode     *genICmpInst(ICmpInst *inst);
    InstCode     *genFCmpInst(FCmpInst *inst);
    InstCode     *genZExtInst(ZExtInst *inst);
    InstCode     *genCallInst(CallInst *inst);

private:
    std::set<std::string> libfunc = {
        //! built-in
        "memset",
        "__aeabi_idiv",
        "__aeabi_uidiv",
        //! stdlib - debug & profile
        "starttime",
        "stoptime",
        "__slime_starttime",
        "__slime_stoptime",
        "_sysy_starttime",
        "_sysy_stoptime",
        //! stdlib - io
        "getint",
        "getch",
        "getfloat",
        "putint",
        "putch",
        "putarray",
        "putfarray",
        "getfarray",
        "getarray",
        "putfloat",
        "putf",
    };

    struct GeneratorState {
        bool             longfunc       = false;
        BasicBlock      *cur_block      = nullptr;
        Function        *cur_func       = nullptr;
        size_t           cur_funcnum    = 0;
        Allocator       *allocator      = nullptr;
        Stack           *stack          = nullptr;
        UsedGlobalVars  *usedGlobalVars = nullptr;
        FloatConstants  *floatConstants = nullptr;
        FpConstVersoins *fpVersions     = nullptr;
    };

    // std::ostream    os;
    GeneratorState generator_;
};
} // namespace slime::backend
