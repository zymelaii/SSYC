#pragma once

#include "12.h"
#include "10.h"
#include "6.def"
#include "4.def"

#include "72.h"
#include "70.def"
#include "73.h"
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

    Expr *decay() {
        return static_cast<Expr *>(this);
    }

    ExprStmt *intoStmt() {
        return ExprStmt::from(this);
    }

    ConstantExpr *tryEvaluate() const;
    Expr         *intoSimplified();

    RegisterCastWithoutSuffixDecl(exprId, DeclRef, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Constant, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Unary, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Binary, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Comma, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Paren, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Stmt, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Call, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, Subscript, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, InitList, Expr, ExprID);
    RegisterCastWithoutSuffixDecl(exprId, NoInit, Expr, ExprID);

    bool isNoEffectExpr();

    ExprID exprId;
    Type  *valueType;
};

//! variable or function
struct DeclRefExpr
    : public Expr
    , utils::BuildTrait<DeclRefExpr> {
    DeclRefExpr(NamedDecl *source = nullptr)
        : Expr(ExprID::DeclRef, UnresolvedType::get()) {
        setSource(source);
    }

    void setSource(NamedDecl *source) {
        this->source = source;
        if (source != nullptr) { valueType = this->source->type(); }
    }

    NamedDecl *source;
};

struct ConstantExpr
    : public Expr
    , public utils::BuildTrait<ConstantExpr> {
    template <typename T>
    ConstantExpr(T data)
        : Expr(ExprID::Constant, UnresolvedType::get()) {
        setData(data);
    }

    template <typename T>
    void setData(T data) {
        if constexpr (std::is_integral_v<T>) {
            type      = ConstantType::i32;
            i32       = data;
            valueType = BuiltinType::getIntType();
        } else if constexpr (std::is_floating_point_v<T>) {
            type      = ConstantType::f32;
            f32       = data;
            valueType = BuiltinType::getFloatType();
        } else {
            assert(false && "unsupport constant type");
        }
    }

    static inline ConstantExpr *createF32(float data);
    static inline ConstantExpr *createI32(int32_t data);

    bool operator==(int32_t value) const {
        return type == ConstantType::i32 && value == i32;
    }

    bool operator==(float value) const {
        return type == ConstantType::f32 && value == i32;
    }

    union {
        int32_t i32;
        float   f32;
    };

    ConstantType type;
};

//! BinaryExpr -> UnaryOperator Expr
struct UnaryExpr
    : public Expr
    , public utils::BuildTrait<UnaryExpr> {
    UnaryExpr(Type *type, UnaryOperator op, Expr *operand)
        : Expr(ExprID::Unary, type)
        , op{op}
        , operand{operand} {}

    UnaryExpr(UnaryOperator op, Expr *operand)
        : UnaryExpr(resolveType(op, operand->valueType), op, operand) {}

    static Type *resolveType(UnaryOperator op, Type *type);

    static inline UnaryExpr *createNot(Expr *operand);
    static inline UnaryExpr *createInv(Expr *operand);

    UnaryOperator op;
    Expr         *operand;
};

//! BinaryExpr -> Expr BinaryOperator Expr
struct BinaryExpr
    : public Expr
    , utils::BuildTrait<BinaryExpr> {
    BinaryExpr(Type *type, BinaryOperator op, Expr *lhs, Expr *rhs)
        : Expr(ExprID::Binary, type)
        , op{op}
        , lhs{lhs}
        , rhs{rhs} {}

    BinaryExpr(BinaryOperator op, Expr *lhs, Expr *rhs)
        : Expr(ExprID::Binary, resolveType(op, lhs->valueType, rhs->valueType))
        , op{op}
        , lhs{lhs}
        , rhs{rhs} {}

    static Type *resolveType(BinaryOperator op, Type *lhsType, Type *rhsType);

    static inline BinaryExpr *createAssign(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createAdd(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createSub(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createMul(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createDiv(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createMod(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createAnd(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createOr(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createXor(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createLAnd(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createLOr(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createLT(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createLE(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createGT(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createGE(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createEQ(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createNE(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createShl(Expr *lhs, Expr *rhs);
    static inline BinaryExpr *createShr(Expr *lhs, Expr *rhs);

    BinaryOperator op;
    Expr          *lhs;
    Expr          *rhs;
};

//! CommaExpr -> Expr { ',' Expr }
struct CommaExpr
    : public Expr
    , public ExprList
    , public utils::BuildTrait<CommaExpr> {
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
struct ParenExpr
    : public Expr
    , public utils::BuildTrait<ParenExpr> {
    ParenExpr(Expr *inner)
        : Expr(ExprID::Paren, inner->valueType)
        , inner{inner} {}

    Expr *inner;
};

//! StmtExpr -> '(' CompoundStmt ')'
//! TODO: complete after Stmt done
struct StmtExpr : public Expr {
    StmtExpr(CompoundStmt *stmt) = delete;
    StmtExpr(Stmt *stmt)         = delete;

    Stmt *inner;
};

//! CallExpr -> Expr '(' { Expr } ')'
struct CallExpr
    : public Expr
    , public utils::BuildTrait<CallExpr> {
    CallExpr(Expr *callable)
        : Expr(ExprID::Call, callable->valueType->asFunctionProto()->returnType)
        , callable{callable} {}

    CallExpr(Expr *callable, ExprList &argList)
        : Expr(ExprID::Call, callable->valueType->asFunctionProto()->returnType)
        , callable{callable}
        , argList(std::move(argList)) {}

    Expr    *callable;
    ExprList argList;
};

//! SubscriptExpr -> Expr '[' Expr ']'
struct SubscriptExpr
    : public Expr
    , public utils::BuildTrait<SubscriptExpr> {
    SubscriptExpr()
        : Expr(ExprID::Subscript, UnresolvedType::get())
        , lhs{nullptr}
        , rhs{nullptr} {}

    SubscriptExpr(Expr *lhs, Expr *rhs)
        : Expr(ExprID::Subscript, Type::getElementType(lhs->valueType))
        , lhs{lhs}
        , rhs{rhs} {}

    Expr *lhs;
    Expr *rhs;
};

//! InitListExpr -> '{' [ Expr { ',' Expr } ] '}'
struct InitListExpr
    : public Expr
    , public ExprList
    , public utils::BuildTrait<InitListExpr> {
    InitListExpr()
        : Expr(ExprID::InitList, UnresolvedType::get()) {}

    InitListExpr(ExprList &list)
        : Expr(ExprID::InitList, UnresolvedType::get())
        , ExprList(std::move(list)) {}
};

//! sugar expr to mark up var decl without init
struct NoInitExpr
    : public Expr
    , public utils::UniqueBuildTrait<NoInitExpr> {
    NoInitExpr()
        : Expr(ExprID::NoInit, UnresolvedType::get()) {}
};

RegisterCastWithoutSuffixImpl(exprId, DeclRef, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Constant, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Unary, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Binary, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Comma, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Paren, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Stmt, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Call, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, Subscript, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, InitList, Expr, ExprID);
RegisterCastWithoutSuffixImpl(exprId, NoInit, Expr, ExprID);

inline ConstantExpr *ConstantExpr::createF32(float data) {
    return create(data);
}

inline ConstantExpr *ConstantExpr::createI32(int32_t data) {
    return create(data);
}

inline UnaryExpr *UnaryExpr::createNot(Expr *operand) {
    return create(UnaryOperator::Not, operand);
}

inline UnaryExpr *UnaryExpr::createInv(Expr *operand) {
    return create(UnaryOperator::Inv, operand);
}

inline BinaryExpr *BinaryExpr::createAssign(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Assign, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createAdd(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Add, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createSub(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Sub, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createMul(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Mul, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createDiv(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Div, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createMod(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Mod, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createAnd(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::And, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createOr(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Or, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createXor(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Xor, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLAnd(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::LAnd, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLOr(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::LOr, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLT(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::LT, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLE(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::LE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createGT(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::GT, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createGE(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::GE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createEQ(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::EQ, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createNE(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::NE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createShl(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Shl, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createShr(Expr *lhs, Expr *rhs) {
    return create(BinaryOperator::Shr, lhs, rhs);
}

}; // namespace slime::ast