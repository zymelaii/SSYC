#pragma once

#include "../utils/list.h"
#include "type.h"
#include "decl.h"

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

enum class StmtID {
    Null,
    Decl,
    Expr,
    Compound,
    If,
    Do,
    While,
    Break,
    Continue,
    Return,
};

struct Stmt {
    Stmt(StmtID stmtId)
        : stmtId{stmtId} {}

    NullStmt* asNullStmt() {
        assert(stmtId == StmtID::Null);
        return reinterpret_cast<NullStmt*>(this);
    }

    DeclStmt* asDeclStmt() {
        assert(stmtId == StmtID::Decl);
        return reinterpret_cast<DeclStmt*>(this);
    }

    ExprStmt* asExprStmt() {
        assert(stmtId == StmtID::Expr);
        return reinterpret_cast<ExprStmt*>(this);
    }

    CompoundStmt* asCompoundStmt() {
        assert(stmtId == StmtID::Compound);
        return reinterpret_cast<CompoundStmt*>(this);
    }

    IfStmt* asIfStmt() {
        assert(stmtId == StmtID::If);
        return reinterpret_cast<IfStmt*>(this);
    }

    DoStmt* asDoStmt() {
        assert(stmtId == StmtID::Do);
        return reinterpret_cast<DoStmt*>(this);
    }

    WhileStmt* asWhileStmt() {
        assert(stmtId == StmtID::While);
        return reinterpret_cast<WhileStmt*>(this);
    }

    BreakStmt* asBreakStmt() {
        assert(stmtId == StmtID::Break);
        return reinterpret_cast<BreakStmt*>(this);
    }

    ContinueStmt* asContinueStmt() {
        assert(stmtId == StmtID::Continue);
        return reinterpret_cast<ContinueStmt*>(this);
    }

    ReturnStmt* asReturnStmt() {
        assert(stmtId == StmtID::Return);
        return reinterpret_cast<ReturnStmt*>(this);
    }

    NullStmt* tryIntoNullStmt() {
        return stmtId == StmtID::Null ? asNullStmt() : nullptr;
    }

    DeclStmt* tryIntoDeclStmt() {
        return stmtId == StmtID::Decl ? asDeclStmt() : nullptr;
    }

    ExprStmt* tryIntoExprStmt() {
        return stmtId == StmtID::Expr ? asExprStmt() : nullptr;
    }

    CompoundStmt* tryIntoCompoundStmt() {
        return stmtId == StmtID::Compound ? asCompoundStmt() : nullptr;
    }

    IfStmt* tryIntoIfStmt() {
        return stmtId == StmtID::If ? asIfStmt() : nullptr;
    }

    DoStmt* tryIntoDoStmt() {
        return stmtId == StmtID::Do ? asDoStmt() : nullptr;
    }

    WhileStmt* tryIntoWhileStmt() {
        return stmtId == StmtID::While ? asWhileStmt() : nullptr;
    }

    BreakStmt* tryIntoBreakStmt() {
        return stmtId == StmtID::Break ? asBreakStmt() : nullptr;
    }

    ContinueStmt* tryIntoContinueStmt() {
        return stmtId == StmtID::Continue ? asContinueStmt() : nullptr;
    }

    ReturnStmt* tryIntoReturnStmt() {
        return stmtId == StmtID::Return ? asReturnStmt() : nullptr;
    }

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
    , public TopLevelVarDecl
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
