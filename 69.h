#pragma once

#include "39.h"
#include "41.h"
#include "0.h"
#include <type_traits>

namespace slime::pass {

class UniversalIRPass {
public:
    virtual void run(ir::Module *target) {
        for (auto fn : target->globalObjects()) {
            if (fn->isFunction()) { runOnFunction(fn->asFunction()); }
        }
    }

    virtual void runOnFunction(ir::Function *target) {}
};

} // namespace slime::pass