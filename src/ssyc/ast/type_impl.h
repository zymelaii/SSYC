#pragma once

#include "type_decl.h"

#include <vector>

namespace ssyc::ast {

struct BuiltinType : public Type {
    //! TODO: to be completed
};

struct ArrayType : public Type {
    //! NOTE: elementType 不能为 void
    Type *elementType;

    //! NOTE: length 必须为非负整型
    Expr *length;
};

struct ConstantArrayType : public ArrayType {
    //! TODO: to be completed
    //! NOTE: ArrayType::length 必须可编译期求值
};

struct VariableArrayType {
    //! TODO: to be completed
};

struct FunctionProtoType : public Type {
    Type                       *retvalType;
    std::vector<ParamVarDecl *> params;
};

} // namespace ssyc::ast
