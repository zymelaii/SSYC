#pragma once

#include "pass.h"

#include <slime/ir/instruction.h>
#include <stack>
#include <map>
#include <set>
#include <vector>

namespace slime::pass {

class Mem2RegPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

protected:
    void searchAndRename(ir::BasicBlock *block);
    void solveDominanceFrontier(ir::Function *target);

private:
    using IndexMap = std::unordered_map<ir::BasicBlock *, int>;
    using DomSet   = std::set<ir::BasicBlock *>;

    std::map<ir::BasicBlock *, DomSet>             domfrSetList_;
    std::map<ir::Value *, std::stack<ir::Value *>> valueStacks_;
    std::map<ir::PhiInst *, ir::AllocaInst *>      phiParent_;
    std::set<ir::BasicBlock *>                     blockVisited_;
};

} // namespace slime::pass
