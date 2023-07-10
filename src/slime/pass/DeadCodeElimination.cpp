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
        if (block->totalInBlocks() == 0 && !isFirst) {
            block->remove();
            isFirst = false;
            continue;
        }
        isFirst = false;

        if (block->isBranched() && block->control()->isImmediate()) {
            auto v = static_cast<ConstantInt *>(block->control())->value;
            if (v) {
                block->reset(block->branch());
            } else {
                block->reset(block->branchElse());
            }
        }

        while (block->isLinear() && !block->isTerminal()) {
            auto branch = block->branch();
            if (branch->isLinear() && branch->size() == 1) {
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

        if (block->size() == 0 && block->totalInBlocks() > 0) {
            block->tryMarkAsTerminal();
        }
    }
}

} // namespace slime::pass