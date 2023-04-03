#pragma once

#include "common.h" // IWYU pragma: export

namespace ssyc::ast {

//! 修饰符限定类型
struct QualifiedType;

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
