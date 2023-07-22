#pragma once

#include "pass.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

class PeekholePass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

protected:
    void foldConstant(ir::Instruction *inst);
    void foldUnaryConstant(ir::User<1> *inst);
    void foldBinaryConstant(ir::User<2> *inst);
    void binUserPeekholeOptimize(ir::User<2> *inst);
};

} // namespace slime::pass
