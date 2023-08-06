#pragma once

#include <slime/utils/cast.def>
#include <slime/utils/traits.h>
#include <slime/experimental/Utility.h>
#include <vector>
#include <type_traits>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

namespace slime::ir {

class Type;
class IntegerType;
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
    Integer =
        FirstBuiltin, //<! i32, i8, i1 only
                      //<! NOTE: i8 is only used for string literal currently
    Float,            //<! f32 only
    LastBuiltin   = Float,
    LastPrimitive = LastBuiltin,

    //! compound types begin
    FirstCompound,
    Array = FirstCompound,
    Pointer,
    Function,
    LastCompound = Pointer,
};

enum class IntegerKind : uint8_t {
    i1,
    i8,
    i32,
};

class Type {
public:
    inline TypeKind kind() const;

    inline bool isToken() const;
    inline bool isLabel() const;
    inline bool isVoid() const;
    inline bool isInteger() const;
    inline bool isBoolean() const;
    inline bool isFloat() const;
    inline bool isArray() const;
    inline bool isPointer() const;
    inline bool isFunction() const;

    inline bool isPrimitiveType() const;
    inline bool isBuiltinType() const;
    inline bool isCompoundType() const;
    inline bool isElementType() const;

    //! have same type kind
    //! FIXME: equals do not check equality deeply
    inline bool equals(const Type *other) const;
    inline bool equals(const Type &other) const;

    static inline Type *getTokenType();
    static inline Type *getLabelType();
    static inline Type *getVoidType();
    static inline Type *getIntegerType(IntegerKind kind = IntegerKind::i32);
    static inline Type *getBooleanType();
    static inline Type *getFloatType();

    static inline ArrayType   *createArrayType(Type *elementType, size_t size);
    static inline PointerType *createPointerType(Type *elementType);
    template <typename... Args>
    static inline FunctionType *createFunctionType(Args &&...args);

    static inline Type *tryGetElementTypeOf(Type *type);
    inline Type        *tryGetElementType();

    RegisterCastDecl(kind, Integer, Type, TypeKind);
    RegisterCastDecl(kind, Function, Type, TypeKind);
    RegisterCastDecl(kind, Array, Type, TypeKind);
    RegisterCastDecl(kind, Pointer, Type, TypeKind);

protected:
    Type(TypeKind kind)
        : kind_{kind} {}

private:
    TypeKind kind_;
};

class IntegerType final : public Type {
public:
    IntegerType(IntegerKind kind)
        : Type(TypeKind::Integer)
        , kind_{kind} {}

    static inline IntegerType *get(IntegerKind kind = IntegerKind::i32);

    inline IntegerKind kind() const;

    inline bool isI1() const;
    inline bool isI8() const;
    inline bool isI32() const;

private:
    const IntegerKind kind_;
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
            using first_type =
                std::remove_reference_t<utils::nth_type<0, Args...>>;
            if constexpr (
                std::is_pointer_v<first_type>
                && std::is_base_of_v<Type, std::remove_pointer_t<first_type>>) {
                //! case1: create(returnType, paramType1, paramType2, ...)
                std::initializer_list<Type *> paramTypes{
                    std::forward<Args>(args)...};
                paramsTypes_.assign(paramTypes.begin(), paramTypes.end());
            } else if constexpr (utils::is_iterable_as<first_type, Type *>) {
                //! case2: create(returnType, iterableContainer)
                auto &list = utils::firstValueOfTArguments(args...);
                paramsTypes_.assign(list.begin(), list.end());
            } else {
                abort();
            }
        }
    }

private:
    Type               *returnType_;
    std::vector<Type *> paramsTypes_;
};

inline TypeKind Type::kind() const {
    return kind_;
}

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

inline bool Type::isBoolean() const {
    return isInteger() && asIntegerType()->isI1();
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

RegisterCastImpl(kind_, Integer, Type, TypeKind);
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

inline Type *Type::getIntegerType(IntegerKind kind) {
    return IntegerType::get(kind);
}

inline Type *Type::getBooleanType() {
    return getIntegerType(IntegerKind::i1);
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

inline Type *Type::tryGetElementTypeOf(Type *type) {
    if (type->isArray() || type->isPointer()) {
        return static_cast<SequentialType *>(type)->elementType();
    }
    return nullptr;
}

inline Type *Type::tryGetElementType() {
    return Type::tryGetElementTypeOf(this);
}

inline IntegerType *IntegerType::get(IntegerKind kind) {
    static IntegerType i1Singleton(IntegerKind::i1);
    static IntegerType i8Singleton(IntegerKind::i8);
    static IntegerType i32Singleton(IntegerKind::i32);
    switch (kind) {
        case IntegerKind::i1: {
            return &i1Singleton;
        } break;
        case IntegerKind::i8: {
            return &i8Singleton;
        } break;
        case IntegerKind::i32: {
            return &i32Singleton;
        } break;
        default: {
            unreachable();
        } break;
    }
}

inline IntegerKind IntegerType::kind() const {
    return kind_;
}

inline bool IntegerType::isI1() const {
    return kind_ == IntegerKind::i1;
}

inline bool IntegerType::isI8() const {
    return kind_ == IntegerKind::i8;
}

inline bool IntegerType::isI32() const {
    return kind_ == IntegerKind::i32;
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
