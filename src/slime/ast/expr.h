#pragma once

#include "../utils/list.h"
#include "../utils/cast.def"
#include "type.h"
#include "stmt.h"
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

    Expr *decay() {
        return static_cast<Expr *>(this);
    }

    ConstantExpr *tryEvaluate() const;

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

    static ConstantExpr *createF32(float data) {
        return new ConstantExpr(data);
    }

    static ConstantExpr *createI32(int32_t data) {
        return new ConstantExpr(data);
    }

    inline bool operator==(int32_t value) const {
        return type == ConstantType::i32 && value == i32;
    }

    inline bool operator==(float value) const {
        return type == ConstantType::f32 && value == i32;
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

    static inline BinaryExpr *create(BinaryOperator op, Expr *lhs, Expr *rhs);

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
        : Expr(ExprID::Subscript, UnresolvedType::get())
        , lhs{nullptr}
        , rhs{nullptr} {}

    SubscriptExpr(Expr *lhs, Expr *rhs)
        : Expr(ExprID::Subscript, Type::getElementType(lhs->valueType))
        , lhs{lhs}
        , rhs{rhs} {}

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

inline BinaryExpr *BinaryExpr::create(BinaryOperator op, Expr *lhs, Expr *rhs) {
    return new BinaryExpr(
        resolveType(op, lhs->valueType, rhs->valueType), op, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createAssign(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Assign, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createAdd(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Add, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createSub(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Sub, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createMul(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Mul, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createDiv(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Div, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createMod(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Mod, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createAnd(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::And, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createOr(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Or, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createXor(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Xor, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLAnd(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::LAnd, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLOr(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::LOr, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLT(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::LT, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createLE(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::LE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createGT(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::GT, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createGE(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::GE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createEQ(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::EQ, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createNE(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::NE, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createShl(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Shl, lhs, rhs);
}

inline BinaryExpr *BinaryExpr::createShr(Expr *lhs, Expr *rhs) {
    return BinaryExpr::create(BinaryOperator::Shr, lhs, rhs);
}

}; // namespace slime::ast
