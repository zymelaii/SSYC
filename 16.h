#include "46.h"
#include "47.h"
#include "51.h"
#include "53.h"
#include "88.h"
#include "49.h"
#include "50.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <map>
#include <type_traits>

namespace slime::backend {

struct Variable;
struct LiveInterval;
struct Stack;
struct InstCode;
class Generator;
struct GeneratorState;

using namespace ir;

enum class ARMGeneralRegs {
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    IP,
    SP,
    LR,
    PC,
    None = 77
};

enum class ARMFloatRegs {
    S0,
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
    S8,
    S9,
    S10,
    S11,
    S12,
    S13,
    S14,
    S15,
    S16,
    S17,
    S18,
    S19,
    S20,
    S21,
    S22,
    S23,
    S24,
    S25,
    S26,
    S27,
    S28,
    S29,
    S30,
    S31,
    None = 77 // Magic num
};

struct ARMRegister {
    ARMRegister()
        : holder(nullptr)
        , gpr{ARMGeneralRegs::None} {}

    ARMRegister(Variable *var)
        : holder(var)
        , gpr{ARMGeneralRegs::None} {};

    ARMRegister &operator=(const ARMFloatRegs reg) {
        this->fpr = reg;
        return *this;
    }

    ARMRegister &operator=(const ARMGeneralRegs reg) {
        this->gpr = reg;
        return *this;
    }

    Variable *holder;

    union {
        ARMGeneralRegs gpr;
        ARMFloatRegs   fpr;
    };
};

bool operator!=(const ARMRegister &a, const ARMFloatRegs &b);
bool operator!=(const ARMRegister &a, const ARMGeneralRegs &b);
bool operator==(const ARMRegister &a, const ARMFloatRegs &b);
bool operator==(const ARMRegister &a, const ARMGeneralRegs &b);

using ValVarTable   = std::map<Value *, Variable *>;
using BlockVarTable = std::map<BasicBlock *, ValVarTable *>;
using LiveVarible   = utils::ListTrait<Variable *>;

struct LiveInterval {
    LiveInterval()
        : start{UINT64_MAX}
        , end{0} {}

    uint64_t start; // start of Live Interval
    uint64_t end;   // end of Live Interval

    static LiveInterval *create() {
        return new LiveInterval();
    }
};

struct Variable {
    Variable(Value *val)
        : val(val)
        , is_spilled(0)
        , is_alloca(0)
        , is_general(!val->type()->isFloat())
        , is_funcparam(0)
        , is_global(val->isGlobal())
        , reg(this)
        , stackpos(0)
        , livIntvl(new LiveInterval()) {}

    Value      *val;
    ARMRegister reg;

    bool          is_spilled;
    bool          is_alloca;
    bool          is_global;
    bool          is_funcparam;
    bool          is_general; // true if allocated with a general register
    size_t        stackpos;   // only valid when is_spiiled or is_alloca is true
    LiveInterval *livIntvl;

    static Variable *create(Value *val) {
        return new Variable(val);
    }
};

struct OnStackVar {
    OnStackVar(Variable *var, uint32_t size)
        : var(var)
        , size(size){};
    Variable *var; // set nullptr when variable is no longer in stack(return to
                   // register or just exipire)
    uint32_t size;
};

struct Stack {
    Stack() {
        onStackVars = new utils::ListTrait<OnStackVar *>;
        stackSize   = 0;
    };

    utils::ListTrait<OnStackVar *> *onStackVars;
    uint32_t                        stackSize;

    void pushVar(Variable *var, uint32_t size) {
        onStackVars->insertToTail(new OnStackVar(var, size));
        stackSize += size;
    }

    void popVar(Variable *var, uint32_t size) {
        assert(var != nullptr);
        assert(size > 0);
        assert(onStackVars->tail() != nullptr);
        auto stackEnd = onStackVars->tail()->value();
        assert(var == stackEnd->var);
        assert(size == stackEnd->size);
        onStackVars->tail()->removeFromList();
        assert(stackSize >= size);
        stackSize -= size;
    }

    // return true if the space is newly allocated
    bool spillVar(Variable *var, uint32_t size) {
        assert(var->reg.gpr == ARMGeneralRegs::None);
        auto   it      = onStackVars->node_begin();
        auto   end     = onStackVars->node_end();
        size_t sizecnt = 0;
        while (it != end) {
            auto stackvar = it->value();
            sizecnt       += stackvar->size;
            // merge fragments
            if (stackvar->var == nullptr) {
                auto tmp = it++;
                auto it2 = it;
                it       = tmp;
                while (it2 != end) {
                    auto tmpvar = it2->value();
                    if (tmpvar->var != nullptr)
                        break;
                    else {
                        stackvar->size += tmpvar->size;
                        sizecnt        += tmpvar->size;
                        auto tmp       = *it2;
                        it2++;
                        tmp.removeFromList();
                    }
                }
                if (stackvar->size == size) {
                    stackvar->var = var;
                    var->stackpos = sizecnt;
                    assert(var->stackpos == lookupOnStackVar(var));
                    return false;
                } else if (stackvar->size > size) {
                    assert(it->value() == stackvar);
                    it->emplaceAfterThis(
                        new OnStackVar(nullptr, stackvar->size - size));
                    sizecnt        = sizecnt - (stackvar->size - size);
                    stackvar->var  = var;
                    stackvar->size = size;
                    var->stackpos  = sizecnt;
                    assert(var->stackpos == lookupOnStackVar(var));
                    return false;
                }
            }
            ++it;
        }
        // not found enough space in fragment
        onStackVars->insertToTail(new OnStackVar(var, size));
        stackSize     += size;
        var->stackpos = stackSize;
        assert(var->stackpos == lookupOnStackVar(var));
        return true;
    }

    // returen size of released spaces
    uint32_t releaseOnStackVar(Variable *var) {
        auto it  = onStackVars->node_begin();
        auto end = onStackVars->node_end();
        auto pre = it;
        while (it != end) {
            auto stackvar = it->value();
            if (stackvar->var == var) {
                stackvar->var = nullptr;
                auto tmp      = it;
                it++;
                if (it == end) {
                    auto     prevar = pre->value();
                    uint32_t fragments =
                        prevar->var || pre == tmp ? 0 : prevar->size;
                    stackSize -= stackvar->size + fragments;
                    tmp->removeFromList();
                    if (fragments != 0) pre->removeFromList();
                    return stackvar->size + fragments;
                }
                return 0;
            } else if (stackvar->var == nullptr) {
                auto tmp = it++;
                auto it2 = it;
                it       = tmp;
                while (it2 != end) {
                    auto tmpvar = it2->value();
                    if (tmpvar->var != nullptr)
                        break;
                    else {
                        stackvar->size += tmpvar->size;
                        auto tmp       = *it2;
                        it2++;
                        tmp.removeFromList();
                    }
                }
            }
            pre = it;
            it++;
        }
        assert(0 && "it must be an error");
        return false;
    }

    uint32_t lookupOnStackVar(Variable *var) {
        int offset = 0;
        for (auto e : *onStackVars) {
            offset += e->size;
            if (e->var == var) return offset;
        }
        assert(0 && "it must be an error");
        return UINT32_MAX;
    }

    void clear() {
        auto it = onStackVars->node_begin();
        while (it != onStackVars->node_end()) {
            auto tmp = *it++;
            tmp.removeFromList();
        }
        stackSize = 0;
    }
};

class Allocator {
    Allocator()
        : cur_inst(0)
        , total_inst(0)
        , liveVars(new LiveVarible())
        , has_funccall(false)
        , strImmFlag(false)
        , funcValVarTable(new ValVarTable)
        , blockVarTable(new BlockVarTable)
        , stack(new Stack) {
        memset(regAllocatedMap, false, 12);
    }

public:
    uint64_t       cur_inst;
    uint64_t       total_inst;
    ValVarTable   *funcValVarTable;
    BlockVarTable *blockVarTable;
    LiveVarible   *liveVars;
    Stack         *stack;
    // 当前函数将会调用的函数中参数个数的最大值
    size_t maxIntegerArgs, maxFloatArgs;
    //! TODO: 针对有函数调用时的寄存器分配进行优化
    bool                     has_funccall;
    bool                     regAllocatedMap[12];
    bool                     floatRegAllocatedMap[32];
    bool                     strImmFlag;
    std::set<ARMGeneralRegs> usedGeneralRegs;
    std::set<ARMFloatRegs>   usedFloatRegs;

    void initVarInterval(Function *func);
    void computeInterval(Function *func);
    void checkLiveInterval(std::string *instcode);
    void updateAllocation(
        Generator   *gen,
        InstCode    *instcode,
        BasicBlock  *block,
        Instruction *inst);
    std::set<Variable *> *getInstOperands(Instruction *inst);
    Variable             *getVarOfAllocatedReg(ARMGeneralRegs reg);
    Variable             *getVarOfAllocatedReg(ARMFloatRegs reg);
    Variable             *getMinIntervalRegVar(std::set<Variable *>, bool is_general);
    Variable             *createVariable(Value *val);

    ARMGeneralRegs allocateGeneralRegister(
        bool                  force     = false,
        std::set<Variable *> *whitelist = nullptr,
        Generator            *gen       = nullptr,
        InstCode             *instcode  = nullptr);
    ARMFloatRegs allocateFloatRegister(
        bool                  force     = false,
        std::set<Variable *> *whitelist = nullptr,
        Generator            *gen       = nullptr,
        InstCode             *instcode  = nullptr);
    void releaseRegister(Variable *var);
    void releaseRegister(ARMGeneralRegs reg);
    void releaseRegister(ARMFloatRegs reg);
    void freeAllRegister();

    void initAllocator();

    static Allocator *create() {
        return new Allocator();
    }

    static bool isVariable(Value *val);
};
} // namespace slime::backend
