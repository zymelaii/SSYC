#pragma once

#include "ast_decl.h"

#include <string_view>

namespace ssyc::ast {

constexpr std::string_view translate(const AstNodeType auto &e) {
    using T = std::remove_cvref_t<std::decay_t<decltype(e)>>;
    if constexpr (T::id() == Type::TypeDecl) {
        return "type-decl";
    } else if constexpr (T::id() == Type::InitializeList) {
        return "initialize-list";
    } else if constexpr (T::id() == Type::Program) {
        return "program";
    } else if constexpr (T::id() == Type::FuncDef) {
        return "func-def";
    } else if constexpr (T::id() == Type::Block) {
        return "block";
    } else if constexpr (T::id() == Type::DeclStatement) {
        return "general-statement";
    } else if constexpr (T::id() == Type::DeclStatement) {
        return "decl-statement";
    } else if constexpr (T::id() == Type::NestedStatement) {
        return "nested-statement";
    } else if constexpr (T::id() == Type::ExprStatement) {
        return "expr-statement";
    } else if constexpr (T::id() == Type::IfElseStatement) {
        return "if-else-statement";
    } else if constexpr (T::id() == Type::WhileStatement) {
        return "while-statement";
    } else if constexpr (T::id() == Type::BreakStatement) {
        return "break-statement";
    } else if constexpr (T::id() == Type::ContinueStatement) {
        return "continue-statement";
    } else if constexpr (T::id() == Type::ReturnStatement) {
        return "return-statement";
    } else if constexpr (T::id() == Type::Expr) {
        return "general-expr";
    } else if constexpr (T::id() == Type::UnaryExpr) {
        return "unary-expr";
    } else if constexpr (T::id() == Type::BinaryExpr) {
        return "binary-expr";
    } else if constexpr (T::id() == Type::FnCallExpr) {
        return "func-call";
    } else if constexpr (T::id() == Type::ConstExprExpr) {
        return "constexpr";
    } else if constexpr (T::id() == Type::OrphanExpr) {
        return "orphan-expr";
    } else if constexpr (std::is_same_v<T, ProgramUnit>) {
        return "general-unit";
    } else {
        std::unreachable();
    }
}

constexpr std::string_view translate(const AstNodeType auto *e) {
    return translate(*e);
}

} // namespace ssyc::ast
