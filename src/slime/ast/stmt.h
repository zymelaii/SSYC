#pragma once

#include "../utils/list.h"
#include "../utils/cast.def"
#include "../utils/traits.h"
#include "type.h"
#include "decl.h"
#include "stmt.def"

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
struct ForStmt;
struct BreakStmt;
struct ContinueStmt;
struct ReturnStmt;

using StmtList    = slime::utils::ListTrait<Stmt*>;
using VarDeclList = slime::utils::ListTrait<VarDecl*>;

struct Stmt {
    Stmt(StmtID stmtId)
        : stmtId{stmtId} {}

    Stmt* decay() {
        return static_cast<Stmt*>(this);
    }

    const Stmt* decay() const {
        return static_cast<const Stmt*>(this);
    }

    RegisterCastDecl(stmtId, Null, Stmt, StmtID);
    RegisterCastDecl(stmtId, Decl, Stmt, StmtID);
    RegisterCastDecl(stmtId, Expr, Stmt, StmtID);
    RegisterCastDecl(stmtId, Compound, Stmt, StmtID);
    RegisterCastDecl(stmtId, If, Stmt, StmtID);
    RegisterCastDecl(stmtId, Do, Stmt, StmtID);
    RegisterCastDecl(stmtId, While, Stmt, StmtID);
    RegisterCastDecl(stmtId, For, Stmt, StmtID);
    RegisterCastDecl(stmtId, Break, Stmt, StmtID);
    RegisterCastDecl(stmtId, Continue, Stmt, StmtID);
    RegisterCastDecl(stmtId, Return, Stmt, StmtID);

    Type* implicitValueType();

    StmtID stmtId;
};

//! sometimes useful
struct NullStmt
    : public Stmt
    , public utils::UniqueBuildTrait<NullStmt> {
    NullStmt()
        : Stmt(StmtID::Null) {}
};

struct DeclStmt
    : public Stmt
    , public VarDeclList
    , public utils::BuildTrait<DeclStmt> {
    DeclStmt()
        : Stmt(StmtID::Decl) {}
};

//! adapt type derives Expr
struct ExprStmt : public Stmt {
    ExprStmt()
        : Stmt(StmtID::Expr) {}

    static ExprStmt* from(Expr* expr) {
        assert(expr != nullptr);
        return reinterpret_cast<ExprStmt*>(expr);
    }

    Expr* unwrap() {
        return reinterpret_cast<Expr*>(this);
    }
};

//! '{' { Stmt } '}'
struct CompoundStmt
    : public Stmt
    , public StmtList
    , public utils::BuildTrait<CompoundStmt> {
    CompoundStmt()
        : Stmt(StmtID::Compound) {}
};

//! if-else
struct IfStmt
    : public Stmt
    , public utils::BuildTrait<IfStmt> {
    IfStmt(
        Expr* condition  = nullptr,
        Stmt* branchIf   = NullStmt::get(),
        Stmt* branchElse = NullStmt::get())
        : Stmt(StmtID::If)
        , condition{condition != nullptr ? ExprStmt::from(condition) : nullptr}
        , branchIf{branchIf}
        , branchElse{branchElse} {}

    bool hasBranchElse() const {
        return !(branchElse == nullptr || branchElse->stmtId == StmtID::Null);
    }

    Stmt* condition; //<! usually ExprStmt
    Stmt* branchIf;
    Stmt* branchElse;
};

struct LoopStmt : public Stmt {
    LoopStmt(StmtID stmtId, Stmt* loopBody = NullStmt::get())
        : Stmt(stmtId)
        , loopBody{loopBody} {}

    bool isEmptyLoop() const {
        return loopBody == nullptr || loopBody->stmtId == StmtID::Null;
    }

    Stmt* loopBody; //<! usually CompoundStmt
};

//! do-while
struct DoStmt
    : public LoopStmt
    , public utils::BuildTrait<DoStmt> {
    DoStmt(Expr* condition = nullptr, Stmt* loopBody = NullStmt::get())
        : LoopStmt(StmtID::Do, loopBody)
        , condition{condition ? ExprStmt::from(condition) : nullptr} {}

    Stmt* condition; //<! usually ExprStmt
};

//! while
struct WhileStmt
    : public LoopStmt
    , public utils::BuildTrait<WhileStmt> {
    WhileStmt(Expr* condition = nullptr, Stmt* loopBody = NullStmt::get())
        : LoopStmt(StmtID::While, loopBody)
        , condition{condition ? ExprStmt::from(condition) : nullptr} {}

    Stmt* condition; //<! usually ExprStmt
};

struct ForStmt
    : public LoopStmt
    , public utils::BuildTrait<ForStmt> {
    ForStmt(
        Stmt* init      = NullStmt::get(),
        Stmt* condition = NullStmt::get(),
        Stmt* increment = NullStmt::get(),
        Stmt* body      = NullStmt::get())
        : LoopStmt(StmtID::For, body)
        , init{init}
        , condition{condition}
        , increment{increment} {}

    //! NOTE: if condition is a NullStmt, probably this will decay into a plain
    //! endless loop.

    Stmt* init;      //<! always NullStmt or DeclStmt or ExprStmt
    Stmt* condition; //<! always NullStmt or ExprStmt
    Stmt* increment; //<! always NullStmt or ExprStmt
};

struct BreakStmt
    : public Stmt
    , public utils::BuildTrait<BreakStmt> {
    BreakStmt(Stmt* parent = nullptr)
        : Stmt(StmtID::Break)
        , parent{parent} {}

    Stmt* parent; //<! always LoopStmt
};

struct ContinueStmt
    : public Stmt
    , public utils::BuildTrait<ContinueStmt> {
    ContinueStmt(Stmt* parent = nullptr)
        : Stmt(StmtID::Continue)
        , parent{parent} {}

    Stmt* parent; //<! always LoopStmt
};

struct ReturnStmt
    : public Stmt
    , public utils::BuildTrait<ReturnStmt> {
    ReturnStmt(Stmt* returnValue)
        : Stmt(StmtID::Return)
        , returnValue{returnValue} {}

    ReturnStmt()
        : ReturnStmt(NullStmt::get()) {}

    ReturnStmt(Expr* value)
        : ReturnStmt(ExprStmt::from(value)) {}

    Type* typeOfReturnValue() {
        return returnValue->implicitValueType();
    }

    Stmt* returnValue; //<! always ExprStmt or NullStmt
};

RegisterCastImpl(stmtId, Null, Stmt, StmtID);
RegisterCastImpl(stmtId, Decl, Stmt, StmtID);
RegisterCastImpl(stmtId, Expr, Stmt, StmtID);
RegisterCastImpl(stmtId, Compound, Stmt, StmtID);
RegisterCastImpl(stmtId, If, Stmt, StmtID);
RegisterCastImpl(stmtId, Do, Stmt, StmtID);
RegisterCastImpl(stmtId, While, Stmt, StmtID);
RegisterCastImpl(stmtId, For, Stmt, StmtID);
RegisterCastImpl(stmtId, Break, Stmt, StmtID);
RegisterCastImpl(stmtId, Continue, Stmt, StmtID);
RegisterCastImpl(stmtId, Return, Stmt, StmtID);

} // namespace slime::ast
