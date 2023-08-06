#include "80.h"

#include "53.h"
#include "47.h"

namespace slime::pass {

using namespace ir;

void ResortPass::runOnFunction(Function *target) {
    Instruction *lastAlloc    = nullptr;
    auto         blockIter    = target->basicBlocks().begin();
    bool         isFirstBlock = true;
    while (blockIter != target->basicBlocks().end()) {
        auto block = *blockIter++;
        auto it    = block->instructions().begin();
        while (it != block->instructions().end()) {
            auto inst = *it++;
            if (inst->id() != InstructionID::Alloca) { continue; }
            if (lastAlloc == nullptr) {
                inst->insertToHead(target->front());
                lastAlloc = inst;
                continue;
            }
            inst->insertAfter(lastAlloc);
            lastAlloc = inst;
        }
        if (block->isOrphan() && !isFirstBlock) {
            block->remove();
        } else if (block->isIncomplete()) {
            block->tryMarkAsTerminal();
        }
        isFirstBlock = false;
    }
}

} // namespace slime::pass