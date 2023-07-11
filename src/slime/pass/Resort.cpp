#include "Resort.h"

#include <slime/ir/value.h>
#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void ResortPass::runOnFunction(Function *target) {
    Instruction *lastAlloc = nullptr;
    for (auto block : target->basicBlocks()) {
        auto &instructions = block->instructions();
        auto  it           = instructions.begin();
        while (it != instructions.end()) {
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
    }
}

} // namespace slime::pass
