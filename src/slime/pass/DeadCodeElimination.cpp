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

        if (!block->isLinear()) {
            if (block->control()->isImmediate()) {
                bool control =
                    static_cast<ConstantInt *>(block->control())->value;
                auto brInst = static_cast<BrInst *>(block->tail()->value());
                brInst->op<1>().reset();
                brInst->op<2>().reset();
                if (control) {
                    brInst->op<0>() = block->jmpIf();
                    block->resetBranch(block->jmpIf());
                } else {
                    brInst->op<0>() = block->jmpElse();
                    block->resetBranch(block->jmpElse());
                }
            }
        }

        while (block->isLinear() && block->hasBranchOut()) {
            auto br = block->jmpIf();
            if (br->isLinear() && block->hasBranchOut() && br->size() == 1
                && br->tail()->value()->id() == InstructionID::Br) {
                auto brInst     = static_cast<BrInst *>(block->tail()->value());
                brInst->op<0>() = br->jmpIf();
                block->resetBranch(br->jmpIf());
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
            assert(target->proto()->returnType()->isVoid());
            Instruction::createRet()->insertToTail(block);
        }
    }
}

} // namespace slime::pass
