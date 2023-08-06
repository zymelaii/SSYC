#pragma once

#include "87.h"

#include "47.h"
#include <set>
#include <map>
#include <vector>

namespace slime::pass {

class MemoryToRegisterPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

protected:
    using BlockSetMap = std::map<ir::BasicBlock *, std::set<ir::BasicBlock *>>;
    using BlockMap    = std::map<ir::BasicBlock *, ir::BasicBlock *>;
    void computeDomFrontier(
        BlockMap &idom, BlockSetMap &domfr, ir::Function *target);
};

} // namespace slime::pass