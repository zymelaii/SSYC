#pragma once

#include "../type_declare.h"
#include "base.h"

#include <vector>
#include <variant>
#include <optional>

namespace ssyc::ast {

struct NullStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE
};

struct CompoundStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    std::vector<std::variant<Stmt *, Expr *>> stmtList;
};

struct DeclStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    //! NOTE: 至少含有一个变量定义
    std::vector<VarDecl *> varDeclList;
};

struct IfElseStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    Expr *cond;

    std::variant<Expr *, Stmt *> onTrue;

    //! NOTE: 为 std::monostate 表示不存在 else 分支
    std::variant<Expr *, Stmt *, std::monostate> onFalse;
};

struct WhileStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    Expr *cond;

    std::variant<Expr *, Stmt *> body;
};

struct DoWhileStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    Expr *cond;

    std::variant<Expr *, Stmt *> body;
};

struct ForStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    std::variant<Expr *, DeclStmt *, std::monostate> init;
    std::variant<Expr *, std::monostate>             cond;
    std::variant<Expr *, std::monostate>             loop;

    std::variant<Expr *, Stmt *> body;
};

struct ContinueStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE
};

struct BreakStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE
};

struct ReturnStmt : public Stmt {
    SSYC_IMPL_AST_INTERFACE

    std::optional<Expr *> retval;
};

} // namespace ssyc::ast
