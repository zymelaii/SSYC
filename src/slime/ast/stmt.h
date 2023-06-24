#pragma once

#include "../utils/list.h"
#include "type.h"
#include "decl.h"
#include "statements.def"
#include "cast.def"

namespace slime::ast {

struct Expr;
struct Stmt;
struct VarDecl;
struct NullStmt;
struct DeclStmt;
struct ExprStmt;
struct CompoundStmt;
struct IfStmt;
struct DoStmt;
struct WhileStmt;
struct BreakStmt;
struct ContinueStmt;
struct ReturnStmt;

using StmtList    = slime::utils::ListTrait<Stmt*>;
using VarDeclList = slime::utils::ListTrait<VarDecl*>;

struct Stmt {
    Stmt(StmtID stmtId)
        : stmtId{stmtId} {}

    RegisterCast(stmtId, Null, Stmt, StmtID);
    RegisterCast(stmtId, Decl, Stmt, StmtID);
    RegisterCast(stmtId, Expr, Stmt, StmtID);
    RegisterCast(stmtId, Compound, Stmt, StmtID);
    RegisterCast(stmtId, If, Stmt, StmtID);
    RegisterCast(stmtId, Do, Stmt, StmtID);
    RegisterCast(stmtId, While, Stmt, StmtID);
    RegisterCast(stmtId, Break, Stmt, StmtID);
    RegisterCast(stmtId, Continue, Stmt, StmtID);
    RegisterCast(stmtId, Return, Stmt, StmtID);

    Type* implicitValueType();

    StmtID stmtId;
};

//! sometimes useful
struct NullStmt : public Stmt {
    NullStmt()
        : Stmt(StmtID::Null) {}

    static NullStmt* get() {
        static NullStmt singleton;
        return &singleton;
    }
};

struct DeclStmt
    : public Stmt
    , public VarDeclList {
    DeclStmt()
        : Stmt(StmtID::Decl) {}
};

//! adapt type derives Expr
struct ExprStmt : public Stmt {
    ExprStmt()
        : Stmt(StmtID::Expr) {}

    static ExprStmt* from(Expr* expr) {
        return reinterpret_cast<ExprStmt*>(expr);
    }

    Expr* unwrap() {
        return reinterpret_cast<Expr*>(this);
    }
};

//! '{' { Stmt } '}'
struct CompoundStmt
    : public Stmt
    , public StmtList {
    CompoundStmt()
        : Stmt(StmtID::Compound) {}
};

//! if-else
struct IfStmt : public Stmt {
    IfStmt()
        : Stmt(StmtID::If)
        , condition{nullptr} {}

    bool haveElseBranch() const {
        return !(branchElse == nullptr || branchElse->stmtId == StmtID::Null);
    }

    Stmt* condition; //<! usually ExprStmt
    Stmt* branchIf;
    Stmt* branchElse;
};

struct LoopStmt : public Stmt {
    LoopStmt(StmtID stmtId)
        : Stmt(stmtId) {}

    bool isEmptyLoop() const {
        return loopBody == nullptr || loopBody->stmtId == StmtID::Null;
    }

    Stmt* loopBody; //<! usually CompoundStmt
};

//! do-while
struct DoStmt : public LoopStmt {
    DoStmt()
        : LoopStmt(StmtID::Do) {}

    Stmt* condition; //<! usually ExprStmt
};

//! while
struct WhileStmt : public LoopStmt {
    WhileStmt()
        : LoopStmt(StmtID::While) {}

    Stmt* condition; //<! usually ExprStmt
};

struct BreakStmt : public Stmt {
    BreakStmt()
        : Stmt(StmtID::Break)
        , parent{nullptr} {}

    Stmt* parent; //<! always LoopStmt
};

struct ContinueStmt : public Stmt {
    ContinueStmt()
        : Stmt(StmtID::Continue)
        , parent{nullptr} {}

    Stmt* parent; //<! always LoopStmt
};

struct ReturnStmt : public Stmt {
    ReturnStmt()
        : Stmt(StmtID::Return) {}

    Type* typeOfReturnValue() {
        return returnValue->implicitValueType();
    }

    Stmt* returnValue; //<! always ExprStmt or NullStmt
};

} // namespace slime::ast
