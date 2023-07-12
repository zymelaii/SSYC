#pragma once

#include "pass.h"

#include <slime/ir/instruction.h>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>

namespace slime::pass {

class Mem2RegPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

    ~Mem2RegPass();

protected:
    void solveDominanceFrontier(ir::Function *target);

private:
    using IndexMap = std::unordered_map<ir::BasicBlock *, int>;
    using DomSet   = std::set<ir::BasicBlock *>;

    IndexMap              id_;
    std::vector<DomSet *> domSetList_;
    std::vector<DomSet *> dfSetList_;
};

} // namespace slime::pass
