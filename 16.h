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
    None
};

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
        , is_funcparam(0)
        , is_global(val->isGlobal())
        , stackpos(0)
        , reg(ARMGeneralRegs::None)
        , livIntvl(new LiveInterval()) {}

    Value         *val;
    ARMGeneralRegs reg;
    bool           is_spilled;
    bool           is_alloca;
    bool           is_global;
    bool           is_funcparam;
    size_t         stackpos; // only valid when is_spiiled or is_alloca is true
    LiveInterval  *livIntvl;

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
        auto it  = onStackVars->node_begin();
        auto end = onStackVars->node_end();
        auto tmp = it;
        while (it != end) {
            tmp = it;
            it++;
        }
        auto stackEnd = tmp->value();
        //! FIXME: unexpected assert failure
        // assert(var == stackEnd->var && size == stackEnd->size);
        stackEnd->size = 0;
        stackSize      -= size;
    }

    // return true if the space is newly allocated
    bool spillVar(Variable *var, uint32_t size) {
        assert(var->reg == ARMGeneralRegs::None);
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

    bool releaseOnStackVar(Variable *var) {
        auto it  = onStackVars->node_begin();
        auto end = onStackVars->node_end();
        auto tmp = it;
        while (it != end) {
            auto tmpvar = *it->value();
            tmp         = it;
            it++;
            if (tmpvar.var == var) {
                tmpvar.var = nullptr;
                if (it == end) {
                    stackSize -= tmpvar.size;
                    tmp->removeFromList();
                    return true;
                }
                return false;
            }
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
    size_t max_funcargs;
    //! TODO: 针对有函数调用时的寄存器分配进行优化
    bool                     has_funccall;
    bool                     regAllocatedMap[12];
    bool                     strImmFlag;
    std::set<ARMGeneralRegs> usedRegs;

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
    Variable             *getMinIntervalRegVar(std::set<Variable *>);
    Variable             *createVariable(Value *val);

    ARMGeneralRegs allocateRegister(
        bool                  force     = false,
        std::set<Variable *> *whitelist = nullptr,
        Generator            *gen       = nullptr,
        InstCode             *instcode  = nullptr);
    void releaseRegister(Variable *var);
    void releaseRegister(ARMGeneralRegs reg);
    void freeAllRegister();

    void initAllocator();

    static Allocator *create() {
        return new Allocator();
    }

    static bool isVariable(Value *val);
};
} // namespace slime::backend
