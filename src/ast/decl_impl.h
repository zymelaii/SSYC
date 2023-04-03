#pragma once

#include "decl_decl.h"

#include <string_view>

namespace ssyc::ast {

struct VarDecl : public Decl {
    std::string_view ident;

    Type *varType;

    //! NOTE: 为空时表示变量未初始化
    //! NOTE: 变量类型被 const 修饰是必须初始化
    Expr *initVal;
};

struct ParamVarDecl : public Decl {
    //! NOTE: 为空时表示实参匿名
    std::string_view ident;

    Type *paramType;

    //! NOTE: 非空表示含默认值
    //! WARNING: 默认参数是 C++ 的特性
    Expr *defaultVal;
};

struct FunctionDecl : public Decl {
    std::string_view   ident;
    FunctionProtoType *protoType;

    //! NOTE: body 为空表示函数声明但未定义
    CompoundStmt *body;
};

}; // namespace ssyc::ast
