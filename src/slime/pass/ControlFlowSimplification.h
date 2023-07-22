#pragma once

#include "pass.h"

#include <slime/ir/instruction.h>
#include <tuple>
#include <stddef.h>
#include <stdint.h>

namespace slime::pass {

class ControlFlowSimplificationPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function* target) override;
};

} // namespace slime::pass
