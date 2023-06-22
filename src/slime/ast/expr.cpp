#include "expr.h"

namespace slime::ast {

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
        }
        case BinaryOperator::And:
        case BinaryOperator::Or:
        case BinaryOperator::Xor: {
            return BuiltinType::getIntType();
        }
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
        case BinaryOperator::Comma: {
            return rhsType;
        } break;
        case BinaryOperator::Subscript: {
            return Type::getElementType(lhsType);
        } break;
    }
}

} // namespace slime::ast
