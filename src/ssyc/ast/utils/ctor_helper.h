#pragma once

#include "../typedef.h"

namespace ssyc::ast::utils::ctor {

namespace Type {

namespace details {

template <typename T, size_t Dim>
struct ArrayAssocOnce {
    static inline auto emit() {
        auto e         = new ConstantArrayType;
        auto dim       = new IntegerLiteral;
        dim->value     = Dim;
        dim->type      = IntegerLiteral::IntegerType::u64;
        dim->literal   = *new std::string{std::to_string(Dim)};
        e->elementType = T::emit();
        e->length      = dim;
        return e;
    }
};

} // namespace details

template <typename T>
struct Qualify {
    static inline auto emit() {
        auto e  = new QualifiedType;
        e->type = T::emit();
        return e;
    }
};

struct Void {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Void;
        return e;
    }
};

struct Int {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Int;
        return e;
    }
};

struct Float {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Float;
        return e;
    }
};

template <typename T, size_t Dim, size_t... Left>
struct Array {
    static inline auto emit() {
        if constexpr (sizeof...(Left) == 0) {
            return details::ArrayAssocOnce<T, Dim>::emit();
        } else {
            return Array<details::ArrayAssocOnce<T, Dim>, Left...>::emit();
        }
    }
};

template <typename R, typename... Params>
struct Function {
    static inline auto emit() {
        auto e        = new FunctionProtoType;
        e->retvalType = R::emit();
        e->params.clear();
        if constexpr (sizeof...(Params) > 0) {
            std::array types{static_cast<ast::Type *>(Params::emit())...};
            for (auto &type : types) {
                auto param        = new ParamVarDecl;
                param->paramType  = type;
                param->defaultVal = nullptr;
                e->params.push_back(param);
            }
        }
        return e;
    }
};

}; // namespace Type

} // namespace ssyc::ast::utils::ctor
