#pragma once

#include "../utils/list.h"

#include <vector>
#include <assert.h>
#include <type_traits>
#include <tuple>

namespace slime::ast {

struct Expr;
struct Type;
struct BuiltinType;
struct ArrayType;
struct IncompleteArrayType;
struct FunctionProtoType;

using ExprList = slime::utils::ListTrait<Expr*>;
using TypeList = slime::utils::ListTrait<Type*>;

enum class TypeID {
    None,
    Unresolved,
    Builtin,
    Array,
    IncompleteArray,
    FunctionProto,
};

enum class BuiltinTypeID {
    Int,
    Float,
    Void,
};

struct Type {
    Type(TypeID typeId)
        : typeId{typeId} {}

    static Type* getElementType(Type* type);

    BuiltinType* asBuiltin() {
        assert(typeId == TypeID::Builtin);
        return reinterpret_cast<BuiltinType*>(this);
    }

    ArrayType* asArray() {
        assert(typeId == TypeID::Array);
        return reinterpret_cast<ArrayType*>(this);
    }

    IncompleteArrayType* asIncompleteArray() {
        assert(typeId == TypeID::IncompleteArray);
        return reinterpret_cast<IncompleteArrayType*>(this);
    }

    FunctionProtoType* asFunctionProto() {
        assert(typeId == TypeID::FunctionProto);
        return reinterpret_cast<FunctionProtoType*>(this);
    }

    TypeID typeId;
};

struct NoneType : Type {
    NoneType()
        : Type(TypeID::None) {}

    static NoneType* create() {
        return new NoneType;
    }
};

struct UnresolvedType : Type {
    UnresolvedType()
        : Type(TypeID::Unresolved) {}

    static UnresolvedType* create() {
        return new UnresolvedType;
    }
};

struct BuiltinType : public Type {
    BuiltinType(BuiltinTypeID type)
        : Type(TypeID::Builtin)
        , type{type} {}

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

    bool isInt() const {
        return type == BuiltinTypeID::Int;
    }

    bool isFloat() const {
        return type == BuiltinTypeID::Float;
    }

    bool isVoid() const {
        return type == BuiltinTypeID::Void;
    }

    BuiltinTypeID type;
};

struct ArrayType
    : public Type
    , public ExprList {
    ArrayType(Type* type)
        : Type(TypeID::Array)
        , type{type} {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = decltype(std::get<0>(std::tuple<Args...>())),
        typename       = std::enable_if_t<std::is_convertible_v<First, Expr*>>>
    ArrayType(Type* type, Args... args)
        : ArrayType(type, std::initializer_list<Expr*>{args...}) {}

    ArrayType(Type* type, const std::initializer_list<Expr*>& list)
        : ArrayType(type) {
        for (auto& e : list) { insertToTail(e); }
    }

    ArrayType(Type* type, const std::vector<Expr*>& list)
        : ArrayType(type) {
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
        : Type(TypeID::FunctionProto)
        , returnType{type} {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = decltype(std::get<0>(std::tuple<Args...>())),
        typename       = std::enable_if_t<std::is_convertible_v<First, Type*>>>
    FunctionProtoType(Type* type, Args... args)
        : FunctionProtoType(type, std::initializer_list<Type*>{args...}) {}

    FunctionProtoType(Type* type, const std::initializer_list<Type*>& paramList)
        : FunctionProtoType(type) {
        for (auto& e : paramList) { insertToTail(e); }
    }

    FunctionProtoType(Type* type, const std::vector<Type*>& paramList)
        : FunctionProtoType(type) {
        for (auto& e : paramList) { insertToTail(e); }
    }

    Type* returnType;
};

} // namespace slime::ast
