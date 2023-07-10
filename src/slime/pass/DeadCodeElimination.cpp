#include "DeadCodeElimination.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void DeadCodeEliminationPass::runOnFunction(ir::Function *target) {
    for (auto block : target->basicBlocks()) {
        auto instructions = block->instructions();
        auto it           = instructions.begin();
        while (it != instructions.end()) {
            auto inst  = *it;
            auto value = inst->unwrap();
            if (!value->type()->isVoid() && value->uses().size() == 0) {
                inst->removeFromBlock();
                delete inst;
            }
            ++it;
        }
    }
}

} // namespace slime::pass
