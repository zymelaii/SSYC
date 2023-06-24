#pragma once

#include "../utils/list.h"
#include "type.h"
#include "stmt.h"
#include "cast.def"
#include "operators.def"
#include "expr.def"

#include <type_traits>

namespace slime::ast {

struct Expr;
struct Stmt;
struct ExprStmt;
struct CompoundStmt;
struct NamedDecl;
struct DeclRefExpr;
struct ConstantExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CommaExpr;
struct ParenExpr;
struct StmtExpr;
struct CallExpr;
struct SubscriptExpr;
struct InitListExpr;
struct NoInitExpr;

using ExprList = slime::utils::ListTrait<Expr *>;

enum class ConstantType {
    i32,
    f32,
};

struct Expr : public ExprStmt {
    Expr(ExprID exprId, Type *valueType)
        : exprId{exprId}
        , valueType{valueType} {}

    RegisterCastWithoutSuffix(exprId, DeclRef, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Constant, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Unary, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Binary, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Comma, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Paren, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Stmt, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Call, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, Subscript, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, InitList, Expr, ExprID);
    RegisterCastWithoutSuffix(exprId, NoInit, Expr, ExprID);

    bool isNoEffectExpr();

    ExprID exprId;
    Type  *valueType;
};

//! variable or function
struct DeclRefExpr : public Expr {
    DeclRefExpr()
        : Expr(ExprID::DeclRef, UnresolvedType::get()) {}

    void setSource(NamedDecl *source) {
        this->source = source;
        valueType    = this->source->type();
    }

    NamedDecl *source;
};

struct ConstantExpr : public Expr {
    template <typename T>
    ConstantExpr(T data)
        : Expr(ExprID::Constant, UnresolvedType::get()) {
        setData(data);
    }

    template <typename T>
    void setData(T data) {
        if constexpr (std::is_integral_v<T>) {
            type = ConstantType::i32;
            i32  = data;
        } else if constexpr (std::is_floating_point_v<T>) {
            type = ConstantType::f32;
            f32  = data;
        } else {
            assert(false && "unsupport constant type");
        }
    }

    static ConstantExpr *createF32(float data) {
        return new ConstantExpr(data);
    }

    static ConstantExpr *createI32(int32_t data) {
        return new ConstantExpr(data);
    }

    union {
        int32_t i32;
        float   f32;
    };

    ConstantType type;
};

//! BinaryExpr -> UnaryOperator Expr
struct UnaryExpr : public Expr {
    UnaryExpr(UnaryOperator op, Expr *operand)
        : Expr(ExprID::Unary, resolveType(op, operand->valueType))
        , op{op}
        , operand{operand} {}

    static Type *resolveType(UnaryOperator op, Type *type);

    UnaryOperator op;
    Expr         *operand;
};

//! BinaryExpr -> Expr BinaryOperator Expr
struct BinaryExpr : public Expr {
    BinaryExpr(Type *type, BinaryOperator op, Expr *lhs, Expr *rhs)
        : Expr(ExprID::Binary, type)
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
        : Expr(ExprID::Comma, NoneType::get()) {}

    CommaExpr(ExprList &list)
        : Expr(ExprID::Comma, NoneType::get())
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
        : Expr(ExprID::Paren, inner->valueType)
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
        : Expr(ExprID::Call, callable->valueType->asFunctionProto()->returnType)
        , callable{callable} {}

    CallExpr(Expr *callable, ExprList &argList)
        : Expr(ExprID::Call, callable->valueType->asFunctionProto()->returnType)
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
        : Expr(ExprID::Subscript, UnresolvedType::get()) {}

    SubscriptExpr(Expr *lhs, Expr *rhs)
        : Expr(ExprID::Subscript, Type::getElementType(lhs->valueType)) {}

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
        : Expr(ExprID::InitList, UnresolvedType::get()) {}

    InitListExpr(ExprList &list)
        : Expr(ExprID::InitList, UnresolvedType::get())
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
        : Expr(ExprID::NoInit, UnresolvedType::get()) {}

    static NoInitExpr *get() {
        static NoInitExpr singleton;
        return &singleton;
    }
};

}; // namespace slime::ast
