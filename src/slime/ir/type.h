#pragma once

#include <stdint.h>
#include <assert.h>
#include <vector>

namespace slime::ir {

namespace detail {
static constexpr uint8_t DerivedTypeFlag = 0x80;
} // namespace detail

enum class TypeID : uint8_t {
    //! primitive types
    Float = 0,
    Label,
    Token,
    Void,
    //! derived types
    Integer = detail::DerivedTypeFlag,
    Pointer,
    Array,
    Function,
};

class Type {
public:
    Type(TypeID id)
        : id_{id} {}

    TypeID id() const {
        return id_;
    }

    bool isElementType() const {
        return id_ != TypeID::Label && id_ != TypeID::Token
            && id_ != TypeID::Void;
    }

    bool isDerivedType() const {
        return static_cast<uint8_t>(id_) & detail::DerivedTypeFlag;
    }

    bool isPrimitiveType() const {
        return !(static_cast<uint8_t>(id_) & detail::DerivedTypeFlag);
    }

private:
    TypeID id_;
};

class IntegerType final : public Type {
public:
    IntegerType()
        : Type(TypeID::Integer) {}
};

class PointerType final : public Type {
public:
    PointerType(const PointerType&)            = delete;
    PointerType& operator=(const PointerType&) = delete;

    PointerType(Type* element)
        : Type(TypeID::Pointer)
        , elemType_{element} {}

    Type* elementType() {
        return elemType_;
    }

private:
    Type* elemType_;
};

class ArrayType final : public Type {
public:
    ArrayType(const ArrayType&)            = delete;
    ArrayType& operator=(const ArrayType&) = delete;

    ArrayType(Type* element, size_t n)
        : Type(TypeID::Array)
        , elemType_{element}
        , totalElem_{n} {}

    Type* elementType() {
        return elemType_;
    }

    size_t totalElement() {
        return totalElem_;
    }

private:
    Type*  elemType_;
    size_t totalElem_;
};

class FunctionType final : public Type {
public:
    FunctionType(const FunctionType&)            = delete;
    FunctionType& operator=(const FunctionType&) = delete;

    FunctionType(Type* result, std::vector<Type*>& params)
        : Type(TypeID::Function)
        , resultType_{result}
        , paramTypes_(std::move(params)) {}

    Type* resultType() {
        return resultType_;
    }

    Type* paramTypeAt(int i) {
        assert(i >= 0 && i < paramTypes_.size());
        return paramTypes_[i];
    }

    size_t totalParams() const {
        return paramTypes_.size();
    }

private:
    Type*              resultType_;
    std::vector<Type*> paramTypes_;
};

} // namespace slime::ir
