#pragma once

#include "87.h"

#include "49.h"

namespace slime::pass {

class ControlFlowSimplificationPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function* target) override;
};

} // namespace slime::pass
