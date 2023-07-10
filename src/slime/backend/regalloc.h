#include "slime/ir/user.h"
#include "slime/ir/value.h"
#include <cstdint>
#include <slime/ir/module.h>
#include <slime/ir/type.h>

#include <map>

namespace slime::backend {

struct LiveInterval;

using namespace ir;

using LiveIntervalTable = std::map<Value *, LiveInterval *>;
using LiveVarible       = utils::ListTrait<LiveInterval *>;

struct Symbol {
    int      num; // all local variables are organised with '%' + "<number>"
    bool     is_spilled;
    uint64_t stackpos; // only valid when symbol is spilled
};

struct LiveInterval {
    LiveInterval(Symbol sym)
        : sym{sym}
        , start{UINT64_MAX}
        , end{UINT64_MAX} {}

    Symbol   sym;
    uint64_t start; // start of Live Interval
    uint64_t end;   // end of Live Interval

    static LiveInterval *create(Symbol sym) {
        return new LiveInterval(sym);
    }
};

class Allocator {
    Allocator()
        : cur_inst(0)
        , total_inst(0)
        , liveVars(new LiveVarible())
        , varLiveIntervals(new LiveIntervalTable) {}

public:
    uint64_t           cur_inst;
    uint64_t           total_inst;
    LiveIntervalTable *varLiveIntervals;
    LiveVarible       *liveVars;
    void               computeInterval(Function *func);
    void               initVarInterval(Function *func);
    void               updateAllocation(Instruction *inst);

    static Allocator *create() {
        return new Allocator();
    }
};
} // namespace slime::backend