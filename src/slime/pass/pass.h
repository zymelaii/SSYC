#pragma once

#include <slime/ir/module.h>
#include <slime/ast/ast.h>
#include <type_traits>

namespace slime::pass {

class UniversalIRPass {
public:
    virtual void run(ir::Module *target) = 0;

    virtual void runOnFunction(ir::Function *target) {}
};

} // namespace slime::pass
