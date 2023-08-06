#pragma once

#include "49.h"
#include "51.h"
#include "0.h"
#include <type_traits>

namespace slime::pass {

class UniversalIRPass {
public:
    virtual void run(ir::Module *target) {
        for (auto obj : target->globalObjects()) {
            if (obj->isFunction()) {
                auto fn = obj->asFunction();
                if (fn->basicBlocks().size() > 0) { runOnFunction(fn); }
            }
        }
    }

    virtual void runOnFunction(ir::Function *target) {}
};

} // namespace slime::pass
