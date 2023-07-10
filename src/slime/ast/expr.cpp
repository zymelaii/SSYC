#include "expr.h"

#include <slime/visitor/ASTExprSimplifier.h>

namespace slime::ast {

ConstantExpr *Expr::tryEvaluate() const {
    auto result = visitor::ASTExprSimplifier::tryEvaluateCompileTimeExpr(
        const_cast<Expr *>(this));
    return result ? result->tryIntoConstant() : nullptr;
}

Expr *Expr::intoSimplified() {
    return visitor::ASTExprSimplifier::trySimplify(this);
}

bool Expr::isNoEffectExpr() {
    switch (exprId) {
        case ExprID::DeclRef:
        case ExprID::Constant: {
            return true;
        } break;
        case ExprID::Unary: {
            return asUnary()->operand->isNoEffectExpr();
        } break;
        case ExprID::Binary: {
            auto e = asBinary();
            return e->op != BinaryOperator::Assign && e->lhs->isNoEffectExpr()
                && e->rhs->isNoEffectExpr();
        } break;
        case ExprID::Comma: {
            for (auto value : *asComma()) {
                if (!value->isNoEffectExpr()) { return false; }
            }
            return true;
        } break;
        case ExprID::Paren: {
            return asParen()->inner->isNoEffectExpr();
        } break;
        case ExprID::Stmt: {
            //! FIXME: unsupport StmtExpr now
            return false;
        } break;
        case ExprID::Call: {
            return false;
        } break;
        case ExprID::Subscript: {
            return false;
        } break;
        case ExprID::InitList: {
            for (auto value : *asInitList()) {
                if (!value->isNoEffectExpr()) { return false; }
            }
            return true;
        } break;
        case ExprID::NoInit: {
            return false;
        } break;
    }
}

Type *UnaryExpr::resolveType(UnaryOperator op, Type *type) {
    switch (op) {
        case UnaryOperator::Pos:
        case UnaryOperator::Neg: {
            if (type->typeId == TypeID::Builtin
                && static_cast<BuiltinType *>(type)->type
                       != BuiltinTypeID::Void) {
                return type;
            } else {
                return NoneType::get();
            }
        } break;
        case UnaryOperator::Not:
        case UnaryOperator::Inv: {
            return BuiltinType::getIntType();
        } break;
        case UnaryOperator::Paren: {
            return type;
        } break;
    }
}

Type *BinaryExpr::resolveType(BinaryOperator op, Type *lhsType, Type *rhsType) {
    //! FIXME: handle illegal binary expr
    switch (op) {
        case BinaryOperator::Assign: {
            return lhsType;
        } break;
        case BinaryOperator::Add:
        case BinaryOperator::Sub:
        case BinaryOperator::Mul:
        case BinaryOperator::Div:
        case BinaryOperator::Mod: {
            if (lhsType->typeId != TypeID::Builtin
                || rhsType->typeId != TypeID::Builtin) {
                return NoneType::get();
            }
            auto lhs = static_cast<BuiltinType *>(lhsType);
            auto rhs = static_cast<BuiltinType *>(rhsType);
            if (lhs->isVoid() || rhs->isVoid()) { return NoneType::get(); }
            if (lhs->isFloat() || rhs->isFloat()) {
                return BuiltinType::getFloatType();
            }
            return BuiltinType::getIntType();
        } break;
        case BinaryOperator::And:
        case BinaryOperator::Or:
        case BinaryOperator::Xor: {
            return BuiltinType::getIntType();
        } break;
        case BinaryOperator::LAnd:
        case BinaryOperator::LOr:
        case BinaryOperator::LT:
        case BinaryOperator::LE:
        case BinaryOperator::GT:
        case BinaryOperator::GE:
        case BinaryOperator::EQ:
        case BinaryOperator::NE: {
            return BuiltinType::getIntType();
        } break;
        case BinaryOperator::Shl:
        case BinaryOperator::Shr: {
            return BuiltinType::getIntType();
        } break;
        case BinaryOperator::Unreachable: {
            assert(false && "unreachable");
            return NoneType::get();
        } break;
    }
}

} // namespace slime::ast
