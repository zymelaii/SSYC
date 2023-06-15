#pragma once

#include "../utils/list.h"

#include <vector>
#include <type_traits>
#include <tuple>

namespace slime::ast {

struct Expr;
struct Type;

using ExprList = slime::utils::ListTrait<Expr*>;
using TypeList = slime::utils::ListTrait<Type*>;

enum class BuiltinTypeID {
    Int,
    Float,
    Void,
};

struct Type {};

struct BuiltinType : public Type {
    BuiltinType(BuiltinTypeID type)
        : type{type} {}

    static BuiltinType* create(BuiltinTypeID type) {
        return new BuiltinType(type);
    }

    static BuiltinType* getIntType() {
        return create(BuiltinTypeID::Int);
    }

    static BuiltinType* getFloatType() {
        return create(BuiltinTypeID::Float);
    }

    static BuiltinType* getVoidType() {
        return create(BuiltinTypeID::Void);
    }

    BuiltinTypeID type;
};

struct ArrayType
    : public Type
    , public ExprList {
    ArrayType(Type* type)
        : type{type} {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = decltype(std::get<0>(std::tuple<Args...>())),
        typename       = std::enable_if_t<std::is_convertible_v<First, Expr*>>>
    ArrayType(Type* type, Args... args)
        : ArrayType(type, std::initializer_list<Expr*>{args...}) {}

    ArrayType(Type* type, const std::initializer_list<Expr*>& list)
        : type(type) {
        for (auto& e : list) { insertToTail(e); }
    }

    ArrayType(Type* type, const std::vector<Expr*>& list)
        : type(type) {
        for (auto& e : list) { insertToTail(e); }
    }

    template <typename... Args>
    static ArrayType* create(Type* type, Args... args) {
        return new ArrayType(type, args...);
    }

    Type* type;
};

struct IncompleteArrayType : public ArrayType {};

struct FunctionProtoType
    : public Type
    , public TypeList {
    FunctionProtoType(Type* type)
        : returnType{type} {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = decltype(std::get<0>(std::tuple<Args...>())),
        typename       = std::enable_if_t<std::is_convertible_v<First, Type*>>>
    FunctionProtoType(Type* type, Args... args)
        : FunctionProtoType(type, std::initializer_list<Type*>{args...}) {}

    FunctionProtoType(Type* type, const std::initializer_list<Type*>& paramList)
        : returnType(type) {
        for (auto& e : paramList) { insertToTail(e); }
    }

    FunctionProtoType(Type* type, const std::vector<Type*>& paramList)
        : returnType(type) {
        for (auto& e : paramList) { insertToTail(e); }
    }

    Type* returnType;
};

} // namespace slime::ast
