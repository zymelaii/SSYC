#pragma once

#include "../utils/cast.def"
#include "../utils/traits.h"

#include <tuple>
#include <vector>
#include <type_traits>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

namespace slime::ir {

class Type;
class ArrayType;
class PointerType;
class FunctionType;

enum class TypeKind : uint8_t {
    //! primitive types begin
    FirstPrimitive,
    Token = FirstPrimitive,
    Label,
    Void,

    //! element types
    FirstBuiltin,
    Integer = FirstBuiltin, //<! i32 only
    Float,                  //<! f32 only
    LastBuiltin   = Float,
    LastPrimitive = LastBuiltin,

    //! compound types begin
    FirstCompound,
    Array = FirstCompound,
    Pointer,
    Function,
    LastCompound = Pointer,
};

class Type {
public:
    inline bool isToken() const;
    inline bool isLabel() const;
    inline bool isVoid() const;
    inline bool isInteger() const;
    inline bool isFloat() const;
    inline bool isArray() const;
    inline bool isPointer() const;
    inline bool isFunction() const;

    inline bool isPrimitiveType() const;
    inline bool isBuiltinType() const;
    inline bool isCompoundType() const;
    inline bool isElementType() const;

    //! have same type kind
    inline bool equals(const Type *other) const;
    inline bool equals(const Type &other) const;

    static inline Type *getTokenType();
    static inline Type *getLabelType();
    static inline Type *getVoidType();
    static inline Type *getIntegerType();
    static inline Type *getFloatType();

    static inline ArrayType   *createArrayType(Type *elementType, size_t size);
    static inline PointerType *createPointerType(Type *elementType);
    template <typename... Args>
    static inline FunctionType *createFunctionType(Args &&...args);

    RegisterCastDecl(kind, Function, Type, TypeKind);
    RegisterCastDecl(kind, Array, Type, TypeKind);
    RegisterCastDecl(kind, Pointer, Type, TypeKind);

protected:
    Type(TypeKind kind)
        : kind_{kind} {}

private:
    TypeKind kind_;
};

class SequentialType : public Type {
public:
    //! element type in SequentialType only indicates the inner type
    inline Type *elementType() const;

protected:
    SequentialType(TypeKind kind, Type *elementType)
        : Type(kind)
        , elementType_{elementType} {
        assert(elementType != nullptr);
    }

private:
    Type *elementType_;
};

class ArrayType
    : public SequentialType
    , public utils::BuildTrait<ArrayType> {
public:
    inline size_t size() const;

    ArrayType(Type *elementType, size_t size)
        : SequentialType(TypeKind::Array, elementType)
        , size_{size} {
        assert(size > 0);
    }

private:
    size_t size_;
};

class PointerType
    : public SequentialType
    , public utils::BuildTrait<PointerType> {
public:
    PointerType(Type *elementType)
        : SequentialType(TypeKind::Pointer, elementType) {}
};

class FunctionType
    : public Type
    , public utils::BuildTrait<FunctionType> {
public:
    template <typename... Args>
    static FunctionType *create(Type *returnType, Args &&...args);

    inline Type  *returnType() const;
    inline Type  *paramTypeAt(int index) const;
    inline size_t totalParams() const;

    FunctionType(Type *returnType)
        : Type(TypeKind::Function)
        , returnType_{returnType} {
        assert(returnType != nullptr);
    }

    template <typename... Args>
    FunctionType(Type *returnType, Args &&...args)
        : FunctionType(returnType) {
        if constexpr (sizeof...(Args) > 0) {
            using first_type = utils::nth_type<0, Args...>;
            //! case1: create(returnType, paramType1, paramType2, ...)
            constexpr bool case1 =
                std::is_pointer_v<first_type>
                && std::is_base_of_v<Type, std::remove_pointer_t<first_type>>;
            //! case2: create(returnType, iterableContainer)
            constexpr bool case2 = utils::is_iterable_as<first_type, Type *>;
            static_assert(
                case1 || case2,
                "unexpected arguments for FunctionType::create(...)");
            if constexpr (case1) {
                std::initializer_list<Type *> paramTypes{
                    std::forward<Args>(args)...};
                paramsTypes_.assign(paramTypes.begin(), paramTypes.end());
            } else if constexpr (case2) {
                auto &list = utils::firstValueOfTArguments(args...);
                paramsTypes_.assign(list.begin(), list.end());
            }
        }
    }

private:
    Type               *returnType_;
    std::vector<Type *> paramsTypes_;
};

inline bool Type::isToken() const {
    return kind_ == TypeKind::Token;
}

inline bool Type::isLabel() const {
    return kind_ == TypeKind::Label;
}

inline bool Type::isVoid() const {
    return kind_ == TypeKind::Void;
}

inline bool Type::isInteger() const {
    return kind_ == TypeKind::Integer;
}

inline bool Type::isFloat() const {
    return kind_ == TypeKind::Float;
}

inline bool Type::isArray() const {
    return kind_ == TypeKind::Array;
}

inline bool Type::isPointer() const {
    return kind_ == TypeKind::Pointer;
}

inline bool Type::isFunction() const {
    return kind_ == TypeKind::Function;
}

inline bool Type::isPrimitiveType() const {
    const auto v     = static_cast<uint8_t>(kind_);
    const auto first = static_cast<uint8_t>(TypeKind::FirstPrimitive);
    const auto last  = static_cast<uint8_t>(TypeKind::LastPrimitive);
    return v >= first && v <= last;
}

inline bool Type::isBuiltinType() const {
    const auto v     = static_cast<uint8_t>(kind_);
    const auto first = static_cast<uint8_t>(TypeKind::FirstBuiltin);
    const auto last  = static_cast<uint8_t>(TypeKind::LastBuiltin);
    return v >= first && v <= last;
}

inline bool Type::isCompoundType() const {
    const auto v     = static_cast<uint8_t>(kind_);
    const auto first = static_cast<uint8_t>(TypeKind::FirstCompound);
    const auto last  = static_cast<uint8_t>(TypeKind::LastCompound);
    return v >= first && v <= last;
}

inline bool Type::isElementType() const {
    return isBuiltinType() || kind_ == TypeKind::Pointer;
}

inline bool Type::equals(const Type *other) const {
    return kind_ == other->kind_;
}

inline bool Type::equals(const Type &other) const {
    return kind_ == other.kind_;
}

RegisterCastImpl(kind_, Function, Type, TypeKind);
RegisterCastImpl(kind_, Array, Type, TypeKind);
RegisterCastImpl(kind_, Pointer, Type, TypeKind);

inline Type *Type::getTokenType() {
    static Type singleton(TypeKind::Token);
    return &singleton;
}

inline Type *Type::getLabelType() {
    static Type singleton(TypeKind::Label);
    return &singleton;
}

inline Type *Type::getVoidType() {
    static Type singleton(TypeKind::Void);
    return &singleton;
}

inline Type *Type::getIntegerType() {
    static Type singleton(TypeKind::Integer);
    return &singleton;
}

inline Type *Type::getFloatType() {
    static Type singleton(TypeKind::Float);
    return &singleton;
}

inline ArrayType *Type::createArrayType(Type *elementType, size_t size) {
    return ArrayType::create(elementType, size);
}

inline PointerType *Type::createPointerType(Type *elementType) {
    return PointerType::create(elementType);
}

template <typename... Args>
inline FunctionType *Type::createFunctionType(Args &&...args) {
    return FunctionType::create(std::forward<Args>(args)...);
}

inline Type *SequentialType::elementType() const {
    return elementType_;
}

inline size_t ArrayType::size() const {
    return size_;
}

inline Type *FunctionType::returnType() const {
    return returnType_;
}

inline Type *FunctionType::paramTypeAt(int index) const {
    const int n = totalParams();
    if (index < 0) { index = n + index; }
    return (index < 0 || index >= n) ? nullptr : paramsTypes_[index];
}

inline size_t FunctionType::totalParams() const {
    return paramsTypes_.size();
}

} // namespace slime::ir
