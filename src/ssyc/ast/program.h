#pragma once

#include "common.h"

#include <vector>
#include <variant>

namespace ssyc::ast {

struct Program : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE

    std::vector<std::variant<VarDecl*, FunctionDecl*>> unitList;
};

} // namespace ssyc::ast
