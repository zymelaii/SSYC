#pragma once

#include "TypeFlag.h"

#include <slime/experimental/Utility.h>
#include <slime/experimental/utils/TryIntoTrait.h>

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <initializer_list>
#include <memory>
#include <string_view>

namespace slime::experimental::ir {

class VoidType;  //<! void
class IntType;   //<! integer
class FPType;    //<! IEEE 754 floating point
class PtrType;   //<! pointer
class ArrayType; //<! array
class FnType;    //<! function proto

class BoolType; //<! i1
class I8Type;   //<! i8
class U8Type;   //<! u8
class I16Type;  //<! i16
class U16Type;  //<! u16
class I32Type;  //<! i32
class U32Type;  //<! u32
class I64Type;  //<! i64
class U64Type;  //<! u64
class UPtrType; //<! uintptr

class FP32Type;  //<! 32-bit floating point
class FP64Type;  //<! 64-bit floating point
class FP128Type; //<! 128-bit floating point

namespace detail {

template <typename T>
struct datatype_of;

template <>
struct datatype_of<BoolType> {
    using type = bool;
};

template <>
struct datatype_of<I8Type> {
    using type = int8_t;
};

template <>
struct datatype_of<U8Type> {
    using type = uint8_t;
};

template <>
struct datatype_of<I16Type> {
    using type = int16_t;
};

template <>
struct datatype_of<U16Type> {
    using type = uint16_t;
};

template <>
struct datatype_of<I32Type> {
    using type = int32_t;
};

template <>
struct datatype_of<U32Type> {
    using type = uint32_t;
};

template <>
struct datatype_of<I64Type> {
    using type = int64_t;
};

template <>
struct datatype_of<U64Type> {
    using type = uint64_t;
};

template <>
struct datatype_of<UPtrType> {
    //! NOTE: use the longest integer type to store address
    using type = uint64_t;
};

template <>
struct datatype_of<FP32Type> {
    using type = float;
};

template <>
struct datatype_of<FP64Type> {
    using type = double;
};

template <>
struct datatype_of<FP128Type> {
    using type = long double;
};

} // namespace detail

template <typename T>
using datatype_of = typename detail::datatype_of<T>::type;

class TypeImpl {
protected:
    inline TypeImpl(TypeKind kind, TypeFlag property);
    ~TypeImpl();

    template <typename T>
    void generateAndResetTypeSignature();

public:
    inline TypeKind         kind() const;
    inline std::string_view signature() const;

    static inline VoidType*  getVoidTy();
    static inline BoolType*  getBoolTy();
    static inline I8Type*    getI8Ty();
    static inline U8Type*    getU8Ty();
    static inline I16Type*   getI16Ty();
    static inline U16Type*   getU16Ty();
    static inline I32Type*   getI32Ty();
    static inline U32Type*   getU32Ty();
    static inline I64Type*   getI64Ty();
    static inline U64Type*   getU64Ty();
    static inline UPtrType*  getUPtrTy();
    static inline FP32Type*  getFP32Ty();
    static inline FP64Type*  getFP64Ty();
    static inline FP128Type* getFP128Ty();

    static IntType* getIntTy(size_t bitWidth, bool isSigned);
    template <size_t BitWidth, bool IsSigned>
    static inline IntType* getIntTy();

    static FPType* getFPTy(size_t bitWidth);
    template <size_t BitWidth>
    static inline FPType* getFPTy();

    static PtrType*   getPtrTy(TypeImpl* dataType);
    static ArrayType* getArrayTy(TypeImpl* dataType, size_t length);

    template <typename... Args>
    static inline FnType* getFnTy(Args&&... args);

protected:
    inline TypeFlag flag() const;
    inline TypeFlag property() const;

private:
    TypeFlag    flag_;
    const char* signature_;
};

using TypeBase = EnumBasedTryIntoTraitWrapper<TypeImpl, &TypeImpl::kind>;

class Type : public TypeBase {
public:
    template <typename... Args>
    Type(Args&&... args)
        : TypeBase(std::forward<Args>(args)...) {}

    static Type* from(const char* sign);
};

class VoidType final : public Type {
public:
    inline VoidType();
};

class IntType : public Type {
protected:
    inline IntType(size_t bitWidth, bool isSigned);

public:
    inline bool   isSigned() const;
    inline size_t bitWidth() const;
    inline size_t byteWidth() const;
};

class FPType : public Type {
protected:
    inline FPType(size_t bitWidth);

public:
    inline size_t bitWidth() const;
    inline size_t digits() const;
    inline size_t expBits() const;
    inline size_t expBias() const;
    inline int    expMin() const;
    inline int    expMax() const;
};

class PtrType final : public Type {
public:
    inline PtrType(const Type* dataType);

    inline const Type* dataType() const;

private:
    const Type* const dataType_;
};

class ArrayType final : public Type {
public:
    inline ArrayType(const Type* dataType, size_t size);

    inline const Type* dataType() const;
    inline size_t      size() const;

private:
    const Type* const dataType_;
};

class FnType : public Type {
public:
    template <
        typename T,
        typename = std::enable_if_t<is_iterable_as<T, const Type*>>>
    inline FnType(bool variadic, const Type* rtype, const T& iterable);

    template <
        typename... Args,
        typename T = std::decay_t<nth_type<0, Args...>>,
        typename   = std::enable_if_t<
            std::is_same_v<T, std::false_type>
            || std::is_convertible_v<T, const Type*>>>
    inline FnType(bool variadic, const Type* rtype, Args... params);

    inline const Type* rtype() const;
    inline const Type* paramTypeAt(size_t index) const;
    inline size_t      totalParams() const;
    inline bool        isVariadic() const;

private:
    using param_type   = const Type*;
    using deleter_type = std::default_delete<param_type[]>;

    const Type* const                           rtype_;
    std::unique_ptr<param_type[], deleter_type> paramTypes_;
};

class BoolType final : public IntType {
public:
    inline BoolType();
};

class I8Type final : public IntType {
public:
    inline I8Type();
};

class U8Type final : public IntType {
public:
    inline U8Type();
};

class I16Type final : public IntType {
public:
    inline I16Type();
};

class U16Type final : public IntType {
public:
    inline U16Type();
};

class I32Type final : public IntType {
public:
    inline I32Type();
};

class U32Type final : public IntType {
public:
    inline U32Type();
};

class I64Type final : public IntType {
public:
    inline I64Type();
};

class U64Type final : public IntType {
public:
    inline U64Type();
};

//! bit width of UPtrType is decided by target arch
class UPtrType final : public IntType {
public:
    inline UPtrType();
};

class FP32Type final : public FPType {
public:
    inline FP32Type();
};

class FP64Type final : public FPType {
public:
    inline FP64Type();
};

class FP128Type final : public FPType {
public:
    inline FP128Type();
};

template <>
void TypeImpl::generateAndResetTypeSignature<VoidType>();

template <>
void TypeImpl::generateAndResetTypeSignature<IntType>();

template <>
void TypeImpl::generateAndResetTypeSignature<FPType>();

template <>
void TypeImpl::generateAndResetTypeSignature<PtrType>();

template <>
void TypeImpl::generateAndResetTypeSignature<ArrayType>();

template <>
void TypeImpl::generateAndResetTypeSignature<FnType>();

template <>
void TypeImpl::generateAndResetTypeSignature<BoolType>();

template <>
void TypeImpl::generateAndResetTypeSignature<I8Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<U8Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<I16Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<U16Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<I32Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<U32Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<I64Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<U64Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<UPtrType>();

template <>
void TypeImpl::generateAndResetTypeSignature<FP32Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<FP64Type>();

template <>
void TypeImpl::generateAndResetTypeSignature<FP128Type>();

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline TypeImpl::TypeImpl(TypeKind kind, TypeFlag property)
    : flag_{0}
    , signature_{nullptr} {
    const auto f1 = TypeFlagMask::makePropertyField(property);
    const auto f2 = static_cast<TypeFlag>(kind);
    flag_         = f1 | f2;
}

inline TypeKind TypeImpl::kind() const {
    return TypeFlagMask::getKindField(flag());
}

inline std::string_view TypeImpl::signature() const {
    assert(signature_ != nullptr);
    return signature_;
}

inline VoidType* TypeImpl::getVoidTy() {
    static VoidType SINGLETON;
    return &SINGLETON;
}

inline BoolType* TypeImpl::getBoolTy() {
    static BoolType SINGLETON;
    return &SINGLETON;
}

inline I8Type* TypeImpl::getI8Ty() {
    static I8Type SINGLETON;
    return &SINGLETON;
}

inline U8Type* TypeImpl::getU8Ty() {
    static U8Type SINGLETON;
    return &SINGLETON;
}

inline I16Type* TypeImpl::getI16Ty() {
    static I16Type SINGLETON;
    return &SINGLETON;
}

inline U16Type* TypeImpl::getU16Ty() {
    static U16Type SINGLETON;
    return &SINGLETON;
}

inline I32Type* TypeImpl::getI32Ty() {
    static I32Type SINGLETON;
    return &SINGLETON;
}

inline U32Type* TypeImpl::getU32Ty() {
    static U32Type SINGLETON;
    return &SINGLETON;
}

inline I64Type* TypeImpl::getI64Ty() {
    static I64Type SINGLETON;
    return &SINGLETON;
}

inline U64Type* TypeImpl::getU64Ty() {
    static U64Type SINGLETON;
    return &SINGLETON;
}

inline UPtrType* TypeImpl::getUPtrTy() {
    static UPtrType SINGLETON;
    return &SINGLETON;
}

inline FP32Type* TypeImpl::getFP32Ty() {
    static FP32Type SINGLETON;
    return &SINGLETON;
}

inline FP64Type* TypeImpl::getFP64Ty() {
    static FP64Type SINGLETON;
    return &SINGLETON;
}

inline FP128Type* TypeImpl::getFP128Ty() {
    static FP128Type SINGLETON;
    return &SINGLETON;
}

template <size_t BitWidth, bool IsSigned>
inline IntType* TypeImpl::getIntTy() {
    if constexpr (BitWidth == 0) {
        return getUPtrTy();
    } else if constexpr (BitWidth == 1) {
        return getBoolTy();
    } else if constexpr (BitWidth == 8) {
        if constexpr (IsSigned) {
            return getI8Ty();
        } else {
            return getU8Ty();
        }
    } else if constexpr (BitWidth == 16) {
        if constexpr (IsSigned) {
            return getI16Ty();
        } else {
            return getU16Ty();
        }
    } else if constexpr (BitWidth == 32) {
        if constexpr (IsSigned) {
            return getI32Ty();
        } else {
            return getU32Ty();
        }
    } else if constexpr (BitWidth == 64) {
        if constexpr (IsSigned) {
            return getI64Ty();
        } else {
            return getU64Ty();
        }
    } else {
        unreachable();
    }
}

template <size_t BitWidth>
inline FPType* TypeImpl::getFPTy() {
    if constexpr (BitWidth == 32) {
        return getFP32Ty();
    } else if constexpr (BitWidth == 64) {
        return getFP64Ty();
    } else if constexpr (BitWidth == 128) {
        return getFP128Ty();
    } else {
        unreachable();
    }
}

template <typename... Args>
inline FnType* TypeImpl::getFnTy(Args&&... args) {
    return new FnType(std::forward<Args>(args)...);
}

inline TypeFlag TypeImpl::flag() const {
    return flag_;
}

inline TypeFlag TypeImpl::property() const {
    return TypeFlagMask::getPropertyField(flag());
}

inline VoidType::VoidType()
    : Type(TypeKind::Void, 0) {
    generateAndResetTypeSignature<VoidType>();
}

inline IntType::IntType(size_t bitWidth, bool isSigned)
    : Type(TypeKind::Integer, isSigned | (bitWidth << 1)) {
    generateAndResetTypeSignature<IntType>();
}

inline bool IntType::isSigned() const {
    return property() & 1;
}

inline size_t IntType::bitWidth() const {
    return property() >> 1;
}

inline size_t IntType::byteWidth() const {
    return (bitWidth() + 7) / 8;
}

inline FPType::FPType(size_t bitWidth)
    : Type(TypeKind::Float, bitWidth) {
    generateAndResetTypeSignature<FPType>();
}

inline size_t FPType::bitWidth() const {
    return property();
}

inline size_t FPType::digits() const {
    if (bitWidth() == 32) {
        return 24;
    } else if (bitWidth() == 64) {
        return 53;
    } else if (bitWidth() == 128) {
        return 113;
    } else {
        unreachable();
    }
}

inline size_t FPType::expBits() const {
    return bitWidth() - digits();
}

inline size_t FPType::expBias() const {
    return (1 << (expBits() - 1)) - 1;
}

inline int FPType::expMin() const {
    return -static_cast<int>(expBias()) + 1;
}

inline int FPType::expMax() const {
    return static_cast<int>(expBias());
}

inline PtrType::PtrType(const Type* dataType)
    : Type(TypeKind::Pointer, 0)
    , dataType_{dataType} {
    generateAndResetTypeSignature<PtrType>();
}

inline const Type* PtrType::dataType() const {
    return dataType_;
}

inline ArrayType::ArrayType(const Type* dataType, size_t size)
    : Type(TypeKind::Array, size)
    , dataType_{dataType} {
    assert(size >= 0 && size <= TypeFlagMask::Property);
    generateAndResetTypeSignature<ArrayType>();
}

inline const Type* ArrayType::dataType() const {
    return dataType_;
}

inline size_t ArrayType::size() const {
    return property();
}

template <typename... Args, typename, typename>
inline FnType::FnType(bool variadic, const Type* rtype, Args... params)
    : FnType(variadic, rtype, std::initializer_list<const Type*>{params...}) {}

template <typename T, typename>
FnType::FnType(bool variadic, const Type* rtype, const T& iterable)
    : Type(TypeKind::Function, (iterable.size() << 1) | variadic)
    , rtype_{rtype}
    , paramTypes_{nullptr} {
    assert((iterable.size() << 1) <= TypeFlagMask::Property);
    if (const auto n = totalParams()) {
        paramTypes_.reset(new param_type[n]);
        int index = 0;
        for (auto type : iterable) { paramTypes_[index++] = type; }
    }
    generateAndResetTypeSignature<FnType>();
}

inline const Type* FnType::rtype() const {
    return rtype_;
}

inline const Type* FnType::paramTypeAt(size_t index) const {
    assert(paramTypes_.get() != nullptr);
    assert(index >= 0 && index < totalParams());
    return paramTypes_[index];
}

inline size_t FnType::totalParams() const {
    return property() >> 1;
}

inline bool FnType::isVariadic() const {
    return property() & 1;
}

inline BoolType::BoolType()
    : IntType(1, false) {}

inline I8Type::I8Type()
    : IntType(8, true) {}

inline U8Type::U8Type()
    : IntType(8, false) {}

inline I16Type::I16Type()
    : IntType(16, true) {}

inline U16Type::U16Type()
    : IntType(16, false) {}

inline I32Type::I32Type()
    : IntType(32, true) {}

inline U32Type::U32Type()
    : IntType(32, false) {}

inline I64Type::I64Type()
    : IntType(64, true) {}

inline U64Type::U64Type()
    : IntType(64, false) {}

inline UPtrType::UPtrType()
    : IntType(0, false) {}

inline FP32Type::FP32Type()
    : FPType(32) {}

inline FP64Type::FP64Type()
    : FPType(64) {}

inline FP128Type::FP128Type()
    : FPType(128) {}

} // namespace slime::experimental::ir

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::VoidType, slime::experimental::ir::TypeKind::Void);

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::IntType,
    slime::experimental::ir::TypeKind::Integer);

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::FPType, slime::experimental::ir::TypeKind::Float);

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::PtrType,
    slime::experimental::ir::TypeKind::Pointer);

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::ArrayType,
    slime::experimental::ir::TypeKind::Array);

emit(auto) slime::experimental::ir::TypeBase::declareTryIntoItem(
    slime::experimental::ir::FnType,
    slime::experimental::ir::TypeKind::Function);

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::BoolType>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 1 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::I8Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 8 && e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::U8Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 8 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::I16Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 16 && e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::U16Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 16 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::I32Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 32 && e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::U32Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 32 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::I64Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 64 && e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::U64Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 64 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::UPtrType>()
        const {
    const auto e = tryInto<slime::experimental::ir::IntType>();
    return e && e->bitWidth() == 0 && !e->isSigned();
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::FP32Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::FPType>();
    return e && e->bitWidth() == 32;
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::FP64Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::FPType>();
    return e && e->bitWidth() == 64;
}

emit(bool)
    slime::experimental::ir::TypeBase::is<slime::experimental::ir::FP128Type>()
        const {
    const auto e = tryInto<slime::experimental::ir::FPType>();
    return e && e->bitWidth() == 128;
}
