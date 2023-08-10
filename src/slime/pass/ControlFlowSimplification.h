#pragma once

#include "pass.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

class ControlFlowSimplificationPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function* target) override;
};

} // namespace slime::pass
