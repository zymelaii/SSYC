#include "regalloc.h"

#include <slime/ir/value.h>
#include <slime/ir/instruction.h>

namespace slime::backend {
const char *regTable[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "sp",
    "lr",
    "pc"};

void Allocator::initVarInterval(Function *func) {
    for (auto block : func->basicBlocks()) {
        for (auto inst : block->instructions()) {
            total_inst += inst->totalOperands();
            
            inst->operands()[0]->isConstant()
                | inst->operands()[0]->isImmediate();
            inst->operands()[0].value();
            varLiveIntervals->insert({});
        }
        // LiveInterval *interval = LiveInterval::create();
    }
}

void Allocator::computeInterval(Function *func) {
    auto bit  = func->basicBlocks().rbegin();
    auto bend = func->basicBlocks().rend();
    while (bit != bend) {
        //! TODO: 获取LIVE IN、LIVE OUT
    }
}

} // namespace slime::backend