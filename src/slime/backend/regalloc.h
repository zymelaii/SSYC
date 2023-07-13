#include "slime/ir/user.h"
#include "slime/ir/value.h"
#include "slime/utils/list.h"
#include <algorithm>
#include <cstdint>
#include <set>
#include <slime/ir/module.h>
#include <slime/ir/type.h>

#include <map>

namespace slime::backend {

struct Variable;
struct LiveInterval;
struct Stack;
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
        , is_global(val->isGlobal())
        , is_used_funcparam(0)
        , reg(ARMGeneralRegs::None)
        , livIntvl(new LiveInterval()) {}

    Value         *val;
    ARMGeneralRegs reg;
    bool           is_spilled;
    bool           is_alloca;
    bool           is_global;
    bool           is_used_funcparam;
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

    // return true if the space is newly allocated
    bool spillVar(Variable *var, uint32_t size) {
        assert(var->reg == ARMGeneralRegs::None);
        auto it  = onStackVars->node_begin();
        auto end = onStackVars->node_end();
        while (it != end) {
            auto stackvar = it->value();
            // merge fragments
            if (stackvar->var == nullptr) {
                auto tmp = it++;
                auto it2 = tmp;
                it       = tmp;
                while (it2 != end) {
                    auto tmpvar = it->value();
                    if (tmpvar->var != nullptr)
                        break;
                    else {
                        auto tmp = *it2++;
                        tmp.removeFromList();
                        stackvar->size += tmpvar->size;
                    }
                }
                if (stackvar->size == size) {
                    stackvar->var = var;
                    return false;
                } else if (stackvar->size > size) {
                    it->emplaceAfter(
                        new OnStackVar(nullptr, stackvar->size - size));
                    stackvar->var  = var;
                    stackvar->size = size;
                    return false;
                }
            }
            ++it;
        }
        // not found enough space in fragment
        onStackVars->insertToTail(new OnStackVar(var, size));
        stackSize += size;
        return true;
    }

    void releaseOnStackVar(Variable *var) {
        for (auto e : *onStackVars) {
            if (var == e->var) {
                e->var = nullptr;
                return;
            }
        }
        assert(0 && "it must be an error");
    }

    uint32_t lookupOnStackVar(Variable *var) {
        int offset = 0;
        for (auto e : *onStackVars) {
            offset += e->size;
            if (e->var == var) return offset;
        }
        assert(0 && "it must be an error");
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
        , blockVarTable(new BlockVarTable)
        , stack(new Stack) {
        memset(regAllocatedMap, false, 12);
    }

public:
    uint64_t       cur_inst;
    uint64_t       total_inst;
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

    void computeInterval(Function *func);
    void initVarInterval(Function *func);
    void updateAllocation(Generator *gen, BasicBlock *block, uint64_t instnum);
    Variable *getMinIntervalRegVar();
    void      getUsedRegs(BasicBlockList &blocklist);

    ARMGeneralRegs allocateRegister();
    void           releaseRegister(Variable *var);
    void           releaseRegister(ARMGeneralRegs reg);
    void           freeAllRegister();

    void initAllocator();

    static Allocator *create() {
        return new Allocator();
    }

    static bool isVariable(Value *val);
};
} // namespace slime::backend