#include "12.h"

#include "21.h"
#include "94.h"
#include <vector>

namespace slime::ast {

using visitor::ASTExprSimplifier;

Type* Type::getElementType(Type* type) {
    switch (type->typeId) {
        case TypeID::None:
        case TypeID::Unresolved:
        case TypeID::Builtin:
        case TypeID::FunctionProto: {
            return NoneType::get();
        } break;
        case TypeID::Array: {
            auto t = type->asArray();
            if (t->size() == 1) {
                return t->type;
            } else {
                std::vector<Expr*> lengthList(t->size() - 1);
                //! FIXME: why here is head()->next()?
                auto node = t->head()->next();
                for (auto& e : lengthList) {
                    e    = node->value();
                    node = node->next();
                }
                return ArrayType::create(t->type, lengthList);
            }
        } break;
        case TypeID::IncompleteArray: {
            auto t = type->asIncompleteArray();
            if (t->size() == 0) {
                return t->type;
            } else {
                std::vector<Expr*> lengthList(t->size());
                auto               node = t->head();
                for (auto& e : lengthList) {
                    e    = node->value();
                    node = node->next();
                }
                return ArrayType::create(t->type, lengthList);
            }
        } break;
        default: {
            unreachable();
        } break;
    }
}

Type* Type::extendIntoArrayType(Expr* length) {
    switch (typeId) {
        case TypeID::None:
        case TypeID::Unresolved: {
            return NoneType::get();
        } break;
        case TypeID::Builtin:
        case TypeID::FunctionProto:
        case TypeID::IncompleteArray: {
            return ArrayType::create(this, length);
        } break;
        case TypeID::Array: {
            asArray()->insertToTail(length);
            return this;
        } break;
        default: {
            unreachable();
        } break;
    }
}

bool Type::equals(const Type* other) const {
    if (this == other) { return true; }
    auto self = const_cast<Type*>(this);
    switch (typeId) {
        case TypeID::None:
        case TypeID::Unresolved: {
            return typeId == other->typeId;
        } break;
        case TypeID::Builtin: {
            return self->asBuiltin()->equals(other);
        } break;
        case TypeID::Array: {
            return self->asArray()->equals(other);
        } break;
        case TypeID::IncompleteArray: {
            return self->asIncompleteArray()->equals(other);
        } break;
        case TypeID::FunctionProto: {
            return self->asFunctionProto()->equals(other);
        } break;
        default: {
            unreachable();
        } break;
    }
}

bool ArrayType::equals(const Type* other) const {
    if (this == other) { return true; }
    if (auto self = const_cast<ArrayType*>(this)->tryIntoIncompleteArray()) {
        return self->equals(other);
    }
    if (auto array = const_cast<Type*>(other)->tryIntoArray()) {
        if (size() != array->size()) { return false; }
        auto it = array->begin();
        for (auto lhs : *this) {
            auto rhs = *it++;
            if (lhs == rhs) { continue; }
            auto lval = ASTExprSimplifier::tryEvaluateCompileTimeExpr(lhs);
            auto rval = ASTExprSimplifier::tryEvaluateCompileTimeExpr(rhs);
            if (!lval || !rval) { return false; }
            assert(lval->tryIntoConstant() && rval->tryIntoConstant());
            assert(
                lval->asConstant()->type == ConstantType::i32
                && rval->asConstant()->type == ConstantType::i32);
            if (lval->asConstant()->i32 != rval->asConstant()->i32) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool IncompleteArrayType::equals(const Type* other) const {
    if (this == other) { return true; }
    if (auto array = const_cast<Type*>(other)->tryIntoIncompleteArray()) {
        if (size() != array->size()) { return false; }
        auto it = array->begin();
        for (auto lhs : *this) {
            auto rhs = *it++;
            if (lhs == rhs) { continue; }
            auto lval = ASTExprSimplifier::tryEvaluateCompileTimeExpr(lhs);
            auto rval = ASTExprSimplifier::tryEvaluateCompileTimeExpr(rhs);
            if (!lval || !rval) { return false; }
            assert(lval->tryIntoConstant() && rval->tryIntoConstant());
            assert(
                lval->asConstant()->type == ConstantType::i32
                && rval->asConstant()->type == ConstantType::i32);
            if (lval->asConstant()->i32 != rval->asConstant()->i32) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool FunctionProtoType::equals(const Type* other) const {
    if (this == other) { return true; }
    if (auto fn = const_cast<Type*>(other)->tryIntoFunctionProto()) {
        if (size() != fn->size()) { return true; }
        if (!returnType->equals(fn->returnType)) { return false; }
        auto it = fn->begin();
        for (auto param : *this) {
            if (!param->equals(*it++)) { return false; }
        }
        return true;
    }
    return false;
}

} // namespace slime::ast
