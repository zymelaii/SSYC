#include "type.h"

#include <vector>

namespace slime::ast {

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
                auto               node = t->head()->next();
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
    }
}

} // namespace slime::ast
