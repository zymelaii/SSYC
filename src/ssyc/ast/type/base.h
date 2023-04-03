#pragma once

#include "../type_declare.h"

namespace ssyc::ast {

struct AbstractAstNode : public IBrief {
    SSYC_IMPL_AST_INTERFACE

    //! 语法节点所在文件的 ID
    int fileId;

    //! 语法节点起始位置
    int row;
    int col;

    //! 语法节点原文
    std::string_view source;
};

//! 程序模块
struct Program : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE

    std::vector<std::variant<VarDecl*, FunctionDecl*>> unitList;
};

//! 类型
struct Type : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE
};

//! 声明
struct Decl : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE
};

//! 表达式
struct Expr : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE
};

//! 语句
struct Stmt : public AbstractAstNode {
    SSYC_IMPL_AST_INTERFACE
};

} // namespace ssyc::ast
