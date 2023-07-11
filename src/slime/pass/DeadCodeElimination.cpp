#include "DeadCodeElimination.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void DeadCodeEliminationPass::runOnFunction(ir::Function *target) {
    auto &blocks  = target->basicBlocks();
    auto  itBlock = blocks.begin();
    bool  isFirst = true;
    while (itBlock != blocks.end()) {
        auto block = *itBlock++;

        if (block->totalInBlocks() == 0) {
            if (block != target->front()) {
                block->remove();
                continue;
            }
        }

        if (block->isBranched() && block->control()->isImmediate()) {
            auto v = static_cast<ConstantInt *>(block->control())->value;
            if (v) {
                block->reset(block->branch());
            } else {
                block->reset(block->branchElse());
            }
        }

        while (block->isLinear()) {
            auto branch = block->branch();
            if (branch->isLinear() && branch->size() == 1
                && !branch->isTerminal()) {
                block->reset(branch->branch());
                continue;
            }
            break;
        }

        auto instructions = block->instructions();
        auto it           = instructions.begin();
        while (it != instructions.end()) {
            auto inst  = *it++;
            auto value = inst->unwrap();
            if (!value->type()->isVoid() && inst->id() != InstructionID::Call
                && value->uses().size() == 0) {
                inst->removeFromBlock();
                delete inst;
            }
        }

        if (block->isIncomplete()) { block->tryMarkAsTerminal(); }
    }
}

} // namespace slime::pass
