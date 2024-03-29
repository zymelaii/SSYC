#pragma once

#include <slime/experimental/Utility.h>
#include <slime/utils/list.h>
#include <slime/utils/cast.def>
#include <vector>
#include <assert.h>

namespace slime::ast {

struct Expr;
struct Type;
struct BuiltinType;
struct ArrayType;
struct IncompleteArrayType;
struct FunctionProtoType;

using ExprList = utils::ListTrait<Expr*>;
using TypeList = utils::ListTrait<Type*>;

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
    Char,
    Float,
    Void,
};

struct Type {
    Type(TypeID typeId)
        : typeId{typeId} {}

    static Type* getElementType(Type* type);

    Type* decay() {
        return static_cast<Type*>(this);
    }

    RegisterCastWithoutSuffixDecl(typeId, Builtin, Type, TypeID);
    RegisterCastWithoutSuffixDecl(typeId, Array, Type, TypeID);
    RegisterCastWithoutSuffixDecl(typeId, IncompleteArray, Type, TypeID);
    RegisterCastWithoutSuffixDecl(typeId, FunctionProto, Type, TypeID);

    Type* extendIntoArrayType(Expr* length);

    bool isArrayLike() const {
        return typeId == TypeID::Array || typeId == TypeID::IncompleteArray;
    }

    bool equals(const Type* other) const;

    TypeID typeId;
};

struct NoneType : Type {
    NoneType()
        : Type(TypeID::None) {}

    static NoneType* get() {
        static NoneType singleton;
        return &singleton;
    }
};

struct UnresolvedType : Type {
    UnresolvedType()
        : Type(TypeID::Unresolved) {}

    static UnresolvedType* get() {
        static UnresolvedType singleton;
        return &singleton;
    }
};

struct BuiltinType : public Type {
    BuiltinType(BuiltinTypeID type)
        : Type(TypeID::Builtin)
        , type{type} {}

    static BuiltinType* get(BuiltinTypeID type) {
        switch (type) {
            case BuiltinTypeID::Int: {
                return getIntType();
            } break;
            case BuiltinTypeID::Char: {
                return getCharType();
            } break;
            case BuiltinTypeID::Float: {
                return getFloatType();
            } break;
            case BuiltinTypeID::Void: {
                return getVoidType();
            } break;
        }
    }

    static BuiltinType* getIntType() {
        static BuiltinType singleton(BuiltinTypeID::Int);
        return &singleton;
    }

    static BuiltinType* getCharType() {
        static BuiltinType singleton(BuiltinTypeID::Char);
        return &singleton;
    }

    static BuiltinType* getFloatType() {
        static BuiltinType singleton(BuiltinTypeID::Float);
        return &singleton;
    }

    static BuiltinType* getVoidType() {
        static BuiltinType singleton(BuiltinTypeID::Void);
        return &singleton;
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

    bool equals(const Type* other) const {
        if (this == other) { return true; }
        if (auto builtin = const_cast<Type*>(other)->tryIntoBuiltin()) {
            return type == builtin->type;
        }
        return false;
    }

    BuiltinTypeID type;
};

struct ArrayType
    : public Type
    , public ExprList {
    ArrayType(TypeID typeId, Type* type)
        : Type(typeId)
        , type{type} {}

    ArrayType(Type* type)
        : ArrayType(TypeID::Array, type) {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = nth_type<0, Args...>,
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

    size_t totalDimension() const {
        return typeId == TypeID::IncompleteArray ? size() + 1 : size();
    }

    bool equals(const Type* other) const;

    Type* type;
};

struct IncompleteArrayType : public ArrayType {
    IncompleteArrayType(Type* type)
        : ArrayType(TypeID::IncompleteArray, type) {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = nth_type<0, Args...>,
        typename       = std::enable_if_t<std::is_convertible_v<First, Expr*>>>
    IncompleteArrayType(Type* type, Args... args)
        : ArrayType(type, std::initializer_list<Expr*>{args...}) {
        typeId = TypeID::IncompleteArray;
    }

    template <typename... Args>
    static IncompleteArrayType* create(Type* type, Args... args) {
        return new IncompleteArrayType(type, args...);
    }

    bool equals(const Type* other) const;
};

struct FunctionProtoType
    : public Type
    , public TypeList {
    FunctionProtoType(Type* type)
        : Type(TypeID::FunctionProto)
        , returnType{type} {}

    FunctionProtoType(Type* type, TypeList& paramList)
        : Type(TypeID::FunctionProto)
        , TypeList(std::move(paramList))
        , returnType{type} {}

    template <
        typename... Args,
        typename Guard = std::enable_if_t<(sizeof...(Args) > 0)>,
        typename First = nth_type<0, Args...>,
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

    template <typename... Args>
    static FunctionProtoType* create(Type* returnType, Args... args) {
        auto e = new FunctionProtoType(returnType, args...);
        return e;
    }

    bool equals(const Type* other) const;

    Type* returnType;
};

RegisterCastWithoutSuffixImpl(typeId, Builtin, Type, TypeID);
RegisterCastWithoutSuffixImpl(typeId, Array, Type, TypeID);
RegisterCastWithoutSuffixImpl(typeId, IncompleteArray, Type, TypeID);
RegisterCastWithoutSuffixImpl(typeId, FunctionProto, Type, TypeID);

} // namespace slime::ast
