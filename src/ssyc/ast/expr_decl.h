#pragma once

#include "common.h" // IWYU pragma: export

namespace ssyc::ast {

//! 初始化列表
struct InitListExpr;

//! 函数调用
struct CallExpr;

//! 括号表达式
struct ParenExpr;

//! 一元表达式
struct UnaryOperatorExpr;

//! 二元表达式
struct BinaryOperatorExpr;

//! 符号引用
struct DeclRef;

//! 常量
struct Literal;

//! 整形常量
struct IntegerLiteral;

//! 浮点常量
struct FloatingLiteral;

//! 字符串常量
struct StringLiteral;

} // namespace ssyc::ast
