#pragma once

#include "decl_decl.h"

#include <string_view>

namespace ssyc::ast {

struct VarDecl : public Decl {

};

struct ParamVarDecl : public Decl {

};

struct FunctionDecl : public Decl {
    std::string_view   ident;
    FunctionProtoType *protoType;

    //! NOTE: body 为空表示函数声明但未定义
    CompoundStmt *body;
};

}; // namespace ssyc::ast
