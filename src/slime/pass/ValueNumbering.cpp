#include "ValueNumbering.h"

#include <slime/ir/value.h>
#include <slime/ir/user.h>
#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void ValueNumberingPass::run(Module *target) {
    for (auto fn : target->globalObjects()) {
        if (fn->isFunction()) { runOnFunction(fn->asFunction()); }
    }
}

void ValueNumberingPass::runOnFunction(Function *target) {
    for (auto block : target->basicBlocks()) {
        if (block->name().empty()) {
            block->setIdUnsafe(nextId_++);
            doneSet_.insert(block);
        }
        for (auto inst : block->instructions()) {
            auto value = inst->unwrap();
            if (value->type()->isVoid()) { continue; }
            if (doneSet_.count(value) == 0) {
                value->setIdUnsafe(nextId_++);
                doneSet_.insert(value);
            }
        }
    }
    if (target->size() > 1) {
        auto       it        = target->begin();
        const auto end       = target->end();
        auto       prevBlock = *it++;
        while (it != end) {
            auto thisBlock = *it++;
            if (prevBlock->id() > thisBlock->id()) {
                auto id = prevBlock->id();
                prevBlock->setIdUnsafe(thisBlock->id());
                thisBlock->setIdUnsafe(id);
            }
        }
    }
    doneSet_.clear();
}

} // namespace slime::pass
