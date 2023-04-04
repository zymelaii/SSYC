#pragma once

#include "../type_declare.h"
#include "base.h"

#include <vector>

namespace ssyc::ast {

struct QualifiedType : public Type {
    SSYC_IMPL_AST_INTERFACE

    Type *type;
};

struct BuiltinType : public Type {
    SSYC_IMPL_AST_INTERFACE

    enum class Type {
        Void,
        Int,
        Float,
    };

    Type typeId;
};

struct ArrayType : public Type {
    SSYC_IMPL_AST_INTERFACE

    //! NOTE: elementType 不能为 void
    Type *elementType;

    //! NOTE: length 必须为非负整型
    Expr *length;
};

struct ConstantArrayType : public ArrayType {
    SSYC_IMPL_AST_INTERFACE

    //! TODO: to be completed
    //! NOTE: ArrayType::length 必须可编译期求值
};

struct VariableArrayType : public ArrayType {
    SSYC_IMPL_AST_INTERFACE

    //! TODO: to be completed
};

struct FunctionProtoType : public Type {
    SSYC_IMPL_AST_INTERFACE

    Type                       *retvalType;
    std::vector<ParamVarDecl *> params;
};

} // namespace ssyc::ast
