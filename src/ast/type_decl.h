#pragma once

#include "common.h" // IWYU pragma: export

namespace ssyc::ast {

//! 内置类型
struct BuiltinType;

//! 数组类型
struct ArrayType;

//! 常量数组类型
struct ConstantArrayType;

//! 边长数组类型
struct VariableArrayType;

//! 函数原型
struct FunctionProtoType;

} // namespace ssyc::ast
