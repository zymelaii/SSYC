#pragma once

#include "ast.h"
#include "parser_context.h"

namespace ssyc::ast {

template <AstNodeType T>
bool validate(T *unit, ParserContext *context) {
    return true;
}

} // namespace ssyc::ast
