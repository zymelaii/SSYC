#include "84.h"

#include "47.h"

namespace slime::pass {

using namespace ir;

void SCCPPass::runOnFunction(Function *target) {
    // if (target->size() == 0) { return; }

    // cfgWorkList_.push_back({nullptr, target->front()});

    // for (auto block : target->basicBlocks()) {
    //     for (auto inst : block->instructions()) {
    //         valueStatus_[inst->unwrap()] = Status::TOP;
    //     }
    // }

    // int i = 0, j = 0;
    // while (i < cfgWorkList_.size() && j < ssaWorkList_.size()) {
    //     while (i < cfgWorkList_.size()) {
    //         auto &e = cfgWorkList_[i++];
    //         if (flags_.count(e) != 0) { continue; }
    //         flags_.insert(e);
    //         for (auto inst : e.to->instructions()) { runOnInstruction(inst);
    //         }
    //     }
    //     while (j < ssaWorkList_.size()) {
    //         auto inst = ssaWorkList_[j++];
    //         for (auto block : inst->parent()->inBlocks()) {
    //             if (flags_.count({block, inst->parent()}) != 0) {
    //                 runOnInstruction(inst);
    //                 break;
    //             }
    //         }
    //     }
    // }
}

void SCCPPass::runOnInstruction(Instruction *inst) {}

} // namespace slime::pass