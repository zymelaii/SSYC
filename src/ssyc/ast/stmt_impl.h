#pragma once

#include "stmt_decl.h"

#include <vector>
#include <variant>
#include <optional>

namespace ssyc::ast {

struct NullStmt : public Stmt {};

struct CompoundStmt : public Stmt {
    std::vector<std::variant<Stmt *, Expr *>> stmtList;
};

struct DeclStmt : public Stmt {
    //! NOTE: 至少含有一个变量定义
    std::vector<VarDecl *> varDeclList;
};

struct IfElseStmt : public Stmt {
    Expr *cond;

    std::variant<Expr *, Stmt *> onTrue;

    //! NOTE: 为 std::monostate 表示不存在 else 分支
    std::variant<Expr *, Stmt *, std::monostate> onFalse;
};

struct WhileStmt : public Stmt {
    Expr *cond;

    std::variant<Expr *, Stmt *> body;
};

struct DoWhileStmt : public Stmt {
    Expr *cond;

    std::variant<Expr *, Stmt *> body;
};

struct ForStmt : public Stmt {
    std::variant<Expr *, DeclStmt *, std::monostate> init;
    std::variant<Expr *, std::monostate>             cond;
    std::variant<Expr *, std::monostate>             loop;

    std::variant<Expr *, Stmt *> body;
};

struct ContinueStmt : public Stmt {};

struct BreakStmt : public Stmt {};

struct ReturnStmt : public Stmt {
    std::optional<Expr *> retval;
};

} // namespace ssyc::ast
