#pragma once

#include "../utils/list.h"
#include "type.h"

namespace slime::ast {

struct Expr;
struct Stmt;
struct CompoundStmt;

using ExprList = slime::utils::ListTrait<Expr *>;

enum class UnaryOperator {
    Pos,   //<! `+` positive
    Neg,   //<! `-` negative
    Not,   //<! `!` logical not
    Inv,   //<! `~` bitwise inverse
    Paren, //<! `()` (derived)
};

enum class BinaryOperator {
    Assign,    //<! `=` assign
    Add,       //<! `+` add
    Sub,       //<! `-` sub
    Mul,       //<! `*` mul
    Div,       //<! `/` div
    Mod,       //<! `%` remainder
    And,       //<! `&` bitwise and
    Or,        //<! `|` bitwise or
    Xor,       //<! `^` xor
    LAnd,      //<! `&&` logical and
    LOr,       //<! `||` logical or
    LT,        //<! `<` less than
    LE,        //<! `<=` less than or equal
    GT,        //<! `>` greater than
    GE,        //<! `>=` greater than or equal
    EQ,        //<! `==` equal
    NE,        //<! `!=` not equal
    Shl,       //<! `<<` bitwise shift left
    Shr,       //<! `>>` bitwise shift right
    Comma,     //<! `,` (derived)
    Subscript, //<! `[]` (derived)
};

struct Expr {
    Expr(Type *valueType)
        : valueType{valueType} {}

    Type *valueType;
};

//! BinaryExpr -> UnaryOperator Expr
struct UnaryExpr : public Expr {
    UnaryExpr(UnaryOperator op, Expr *operand)
        : Expr(resolveType(op, operand->valueType))
        , op{op}
        , operand{operand} {}

    static Type *resolveType(UnaryOperator op, Type *type);

    UnaryOperator op;
    Expr         *operand;
};

//! BinaryExpr -> Expr BinaryOperator Expr
struct BinaryExpr : public Expr {
    BinaryExpr(Type *type, BinaryOperator op, Expr *lhs, Expr *rhs)
        : Expr(type)
        , op{op}
        , lhs{lhs}
        , rhs{rhs} {}

    static Type *resolveType(BinaryOperator op, Type *lhsType, Type *rhsType);

    BinaryOperator op;
    Expr          *lhs;
    Expr          *rhs;
};

//! CommaExpr -> Expr { ',' Expr }
struct CommaExpr
    : public Expr
    , public ExprList {
    CommaExpr()
        : Expr(NoneType::get()) {}

    CommaExpr(ExprList &list)
        : Expr(NoneType::get())
        , ExprList(std::move(list)) {
        if (tail() != nullptr) { valueType = tail()->value()->valueType; }
    }

    void append(Expr *e) {
        insertToTail(e);
        valueType = e->valueType;
    }
};

//! ParenExpr -> '(' Expr ')'
struct ParenExpr : public Expr {
    ParenExpr(Expr *inner)
        : Expr(inner->valueType)
        , inner{inner} {}

    static ParenExpr *create(Expr *inner) {
        return new ParenExpr(inner);
    }

    Expr *inner;
};

//! StmtExpr -> '(' CompoundStmt ')'
//! TODO: complete after Stmt done
struct StmtExpr : public Expr {
    StmtExpr(CompoundStmt *stmt);
    StmtExpr(Stmt *stmt);

    Stmt *inner;
};

//! CallExpr -> Expr '(' { Expr } ')'
struct CallExpr : public Expr {
    CallExpr(Expr *callable)
        : Expr(callable->valueType->asFunctionProto()->returnType)
        , callable{callable} {}

    CallExpr(Expr *callable, ExprList &argList)
        : Expr(callable->valueType->asFunctionProto()->returnType)
        , callable{callable}
        , argList(std::move(argList)) {}

    static CallExpr *create(Expr *callable) {
        return new CallExpr(callable);
    }

    static CallExpr *create(Expr *callable, ExprList &argList) {
        return new CallExpr(callable, argList);
    }

    Expr    *callable;
    ExprList argList;
};

//! SubscriptExpr -> Expr '[' Expr ']'
struct SubscriptExpr : public Expr {
    SubscriptExpr()
        : Expr(UnresolvedType::get()) {}

    SubscriptExpr(Expr *lhs, Expr *rhs)
        : Expr(Type::getElementType(lhs->valueType)) {}

    static SubscriptExpr *create(Expr *lhs, Expr *rhs) {
        return new SubscriptExpr(lhs, rhs);
    }

    Expr *lhs;
    Expr *rhs;
};

//! InitListExpr -> '{' [ Expr { ',' Expr } ] '}'
struct InitListExpr
    : public Expr
    , public ExprList {
    InitListExpr()
        : Expr(UnresolvedType::get()) {}

    InitListExpr(ExprList &list)
        : Expr(UnresolvedType::get())
        , ExprList(std::move(list)) {}

    static InitListExpr *create() {
        return new InitListExpr;
    }

    static InitListExpr *create(ExprList &list) {
        return new InitListExpr(list);
    }
};

//! sugar expr to mark up var decl without init
struct NoInitExpr : public Expr {
    NoInitExpr()
        : Expr(UnresolvedType::get()) {}

    static NoInitExpr *create() {
        return new NoInitExpr;
    }
};

}; // namespace slime::ast
