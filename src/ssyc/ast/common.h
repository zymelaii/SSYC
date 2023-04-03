#pragma once

#include <stdint.h>

namespace ssyc::ast {

//! AST 语法节点基本类型
enum class NodeBaseType : uint8_t {
    Type,
    Decl,
    Expr,
    Stmt,
    Unknown,
};

} // namespace ssyc::ast
