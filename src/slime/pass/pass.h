#pragma once

#include <slime/ir/module.h>
#include <slime/ir/user.h>
#include <slime/ast/ast.h>
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
