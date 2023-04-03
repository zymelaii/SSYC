#pragma once

#include <type_traits>
#include <concepts>
#include <string_view>
#include <stdint.h>

namespace ssyc::ast {

struct AbstractAstNode {
    //! 语法节点所在文件的 ID
    int fileId;

    //! 语法节点起始位置
    int row;
    int col;

    //! 语法节点原文
    std::string_view source;
};

//! 类型
struct Type : public AbstractAstNode {};

//! 声明
struct Decl : public AbstractAstNode {};

//! 表达式
struct Expr : public AbstractAstNode {};

//! 语句
struct Stmt : public AbstractAstNode {};

inline namespace details {

template <typename T>
concept ast_node =
    std::derived_from<T, AbstractAstNode>
    && std::is_same_v<std::remove_cvref_t<std::decay_t<T>>, AbstractAstNode>;

}; // namespace details

}; // namespace ssyc::ast

#include "type_decl.h"
#include "decl_decl.h"
#include "expr_decl.h"
#include "stmt_decl.h"
