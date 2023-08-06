#include "84.h"

#include "53.h"
#include "51.h"
#include "47.h"

namespace slime::pass {

using namespace ir;

void ValueNumberingPass::runOnFunction(Function *target) {
    nextId_ = 0;
    doneSet_.clear();

    for (int i = 0; i < target->totalParams(); ++i) {
        auto param = const_cast<Parameter *>(&target->params()[i]);
        param->setIdUnsafe(nextId_++, 0);
    }

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
}

} // namespace slime::pass