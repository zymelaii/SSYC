#include "slime/ir/user.h"
#include "slime/ir/value.h"
#include <cstdint>
#include <slime/ir/module.h>
#include <slime/ir/type.h>

#include <map>

namespace slime::backend {

struct LiveInterval;
struct Variable;

using namespace ir;

enum class ARMGeneralRegs {
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
    R12,
    None
};

using ValVarTable   = std::map<Value *, Variable *>;
using BlockVarTable = std::map<BasicBlock *, ValVarTable *>;
using LiveVarible   = utils::ListTrait<LiveInterval *>;

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
        , is_global(val->isGlobal())
        , reg(ARMGeneralRegs::None)
        , stackpos(0)
        , livIntvl(new LiveInterval()) {}

    Value         *val;
    ARMGeneralRegs reg;
    bool           is_spilled;
    bool           is_global;
    LiveInterval  *livIntvl;
    int64_t stackpos; // variable's location on stack. Only valid when symbol is
                      // spilled

    static Variable *create(Value *val) {
        return new Variable(val);
    }
};

class Allocator {
    Allocator()
        : cur_inst(0)
        , total_inst(0)
        , liveVars(new LiveVarible())
        , blockVarTable(new BlockVarTable) {
        memset(regAllocatedMap, false, 12);
    }

public:
    uint64_t       cur_inst;
    uint64_t       total_inst;
    BlockVarTable *blockVarTable;
    LiveVarible   *liveVars;
    bool           regAllocatedMap[12];

    const char *reg2str(ARMGeneralRegs reg);
    void        computeInterval(Function *func);
    void        initVarInterval(Function *func);
    void        updateAllocation(BasicBlock *block, uint64_t instnum);

    ARMGeneralRegs allocateRegister();
    void           releaseRegister(ARMGeneralRegs reg);

    static Allocator *create() {
        return new Allocator();
    }

protected:
    bool isVariable(Value *val);
};
} // namespace slime::backend