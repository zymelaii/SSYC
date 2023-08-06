#include "68.h"

#include <stack>
#include <assert.h>

namespace slime::pass {

using namespace ir;

void ControlFlowSimplificationPass::runOnFunction(ir::Function* target) {
    auto& blocks    = target->basicBlocks();
    auto  blockIter = blocks.begin();
    while (blockIter != blocks.end()) {
        auto thisBlockIter = blockIter;
        auto block         = *blockIter++;

        //! 1. fold constant branch
        if (block->isBranched()) {
            if (block->branch() == block->branchElse()) {
                block->reset(block->branch());
            } else {
                if (!block->control()->isImmediate()) { continue; }
                auto imm = static_cast<ConstantInt*>(block->control())->value;
                if (imm) {
                    block->reset(block->branch());
                } else {
                    block->reset(block->branchElse());
                }
            }
        }

        //! 2. remove unreachable incoming blocks
        std::vector<BasicBlock*> inblocks;
        for (auto inblock : block->inBlocks()) {
            if (inblock->totalInBlocks() == 0) { inblocks.push_back(inblock); }
        }
        for (auto inblock : inblocks) {
            if (inblock != target->front()) {
                auto ok = inblock->remove();
                assert(ok);
            }
        }
        auto it   = thisBlockIter;
        blockIter = ++it;

        //! 3. remove unreachable block
        bool isEntry = block == target->front();
        if (!isEntry && block->totalInBlocks() == 0) {
            auto ok = block->remove();
            assert(ok);
            continue;
        }
        if (!block->isLinear() || block->isTerminal()) { continue; }
        auto nextBlock = block->branch();
        if (nextBlock == block) {
            if (!isEntry && block->totalInBlocks() == 1) {
                block->reset();
                auto ok = block->remove();
                assert(ok);
            }
            continue;
        }

        //! 4 check simplifiable
        bool simplifiable = true;
        for (auto use : block->uses()) {
            if (use->owner()->asInstruction()->id() != InstructionID::Br) {
                simplifiable = false;
                break;
            }
        }
        if (!simplifiable) { continue; }

        //! 5. remove useless branch
        if (block->size() == 1) {
            std::vector<Use*> uses(block->uses().begin(), block->uses().end());
            for (auto use : uses) {
                use->reset(nextBlock);
                use->owner()
                    ->asInstruction()
                    ->parent()
                    ->syncFlowWithInstUnsafe();
            }
            auto ok = block->remove();
            assert(ok);
            continue;
        }

        //! 6. combine linear branch
        if (nextBlock->totalInBlocks() == 1) {
            block->reset();
            std::vector<Instruction*> instrs(
                nextBlock->begin(), nextBlock->end());
            for (auto inst : instrs) { inst->insertToTail(block); }
            block->syncFlowWithInstUnsafe();
            auto ok = nextBlock->remove();
            assert(ok);
            blockIter = thisBlockIter;
        }
    }
}

} // namespace slime::pass
