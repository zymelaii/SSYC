#include "28.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sstream>
#include <vector>

namespace slime::experimental::ir {

TypeImpl::~TypeImpl() {
    free(const_cast<char*>(signature_));
    signature_ = nullptr;
}

template <>
void TypeImpl::generateAndResetTypeSignature<VoidType>() {
    assert(signature_ == nullptr);
    signature_ = strdup("void");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<IntType>() {
    assert(signature_ == nullptr);
    const auto e = static_cast<Type*>(this)->as<IntType>();
    switch (e->bitWidth()) {
        case 0: {
            generateAndResetTypeSignature<UPtrType>();
        } break;
        case 1: {
            generateAndResetTypeSignature<BoolType>();
        } break;
        case 8: {
            if (e->isSigned()) {
                generateAndResetTypeSignature<I8Type>();
            } else {
                generateAndResetTypeSignature<U8Type>();
            };
        } break;
        case 16: {
            if (e->isSigned()) {
                generateAndResetTypeSignature<I16Type>();
            } else {
                generateAndResetTypeSignature<U16Type>();
            };
        } break;
        case 32: {
            if (e->isSigned()) {
                generateAndResetTypeSignature<I32Type>();
            } else {
                generateAndResetTypeSignature<U32Type>();
            };
        } break;
        case 64: {
            if (e->isSigned()) {
                generateAndResetTypeSignature<I64Type>();
            } else {
                generateAndResetTypeSignature<U64Type>();
            };
        } break;
        default: {
            unreachable();
        } break;
    }
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<FPType>() {
    assert(signature_ == nullptr);
    const auto e = static_cast<Type*>(this)->as<FPType>();
    switch (e->bitWidth()) {
        case 32: {
            generateAndResetTypeSignature<FP32Type>();
        } break;
        case 64: {
            generateAndResetTypeSignature<FP64Type>();
        } break;
        case 128: {
            generateAndResetTypeSignature<FP128Type>();
        } break;
        default: {
            unreachable();
        } break;
    }
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<PtrType>() {
    //! sign(PtrType) := 'P' sign(dataType)
    assert(signature_ == nullptr);
    const auto   e            = static_cast<Type*>(this)->as<PtrType>();
    const auto   dataTypeSign = e->dataType()->signature();
    const size_t n            = dataTypeSign.size();
    auto         signature    = (char*)malloc(n + 2);
    sprintf(signature, "P%s", dataTypeSign.data());
    signature_ = signature;
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<ArrayType>() {
    //! sign(ArrayType) := 'A' { size '_' } sign(elementType)
    assert(signature_ == nullptr);
    auto              e = static_cast<Type*>(this)->as<ArrayType>();
    std::stringstream ss;
    ss << "A" << e->size() << "_";
    if (auto arrayType = e->dataType()->tryInto<ArrayType>()) {
        const auto dataTypeSign = arrayType->signature();
        ss << dataTypeSign.substr(1);
    } else {
        const auto elemTypeSign = e->dataType()->signature();
        ss << elemTypeSign;
    }
    signature_ = strdup(ss.str().c_str());
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<FnType>() {
    //! sign(FnType) := 'F' [ 'V' ] sign(rtype) [ '_' { sign(param) } ] 'T'
    assert(signature_ == nullptr);
    auto              e = static_cast<Type*>(this)->as<FnType>();
    std::stringstream ss;
    ss << "F";
    if (e->isVariadic()) { ss << "V"; }
    ss << e->rtype()->signature();
    if (const auto n = e->totalParams()) {
        ss << "_";
        for (int i = 0; i < n; ++i) { ss << e->paramTypeAt(i)->signature(); }
    }
    ss << "T";
    signature_ = strdup(ss.str().c_str());
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<BoolType>() {
    assert(signature_ == nullptr);
    signature_ = strdup("bool");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<I8Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("i8");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<U8Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("u8");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<I16Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("i16");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<U16Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("u16");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<I32Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("i32");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<U32Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("u32");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<I64Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("i64");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<U64Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("u64");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<UPtrType>() {
    assert(signature_ == nullptr);
    signature_ = strdup("addr");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<FP32Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("f32");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<FP64Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("f64");
    assert(signature_ != nullptr);
}

template <>
void TypeImpl::generateAndResetTypeSignature<FP128Type>() {
    assert(signature_ == nullptr);
    signature_ = strdup("f128");
    assert(signature_ != nullptr);
}

IntType* TypeImpl::getIntTy(size_t bitWidth, bool isSigned) {
    switch (bitWidth) {
        case 0: {
            return getUPtrTy();
        } break;
        case 1: {
            return getBoolTy();
        } break;
        case 8: {
            return isSigned ? getI8Ty()->as<IntType>() : getU8Ty();
        } break;
        case 16: {
            return isSigned ? getI16Ty()->as<IntType>() : getU16Ty();
        } break;
        case 32: {
            return isSigned ? getI32Ty()->as<IntType>() : getU32Ty();
        } break;
        case 64: {
            return isSigned ? getI64Ty()->as<IntType>() : getU64Ty();
        } break;
        default: {
            unreachable();
        } break;
    }
}

FPType* TypeImpl::getFPTy(size_t bitWidth) {
    switch (bitWidth) {
        case 32: {
            return getFP32Ty();
        } break;
        case 64: {
            return getFP64Ty();
        } break;
        case 128: {
            return getFP128Ty();
        } break;
        default: {
            unreachable();
        } break;
    }
}

PtrType* TypeImpl::getPtrTy(TypeImpl* dataType) {
    return new PtrType(static_cast<Type*>(dataType));
}

ArrayType* TypeImpl::getArrayTy(TypeImpl* dataType, size_t length) {
    return new ArrayType(static_cast<Type*>(dataType), length);
}

Type* Type::from(const char* sign) {
    const char* p = sign;
    switch (*p) {
        case 'v': { //<! void
            return Type::getVoidTy();
        } break;
        case 'a': { //<! addr
            return Type::getUPtrTy();
        } break;
        case 'b': { //<! bool
            return Type::getBoolTy();
        } break;
        case 'i': { //<! i<bitWidth>
            int bitWidth = 0;
            sscanf(p, "i%d", &bitWidth);
            switch (bitWidth) {
                case 8: {
                    return Type::getI8Ty();
                } break;
                case 16: {
                    return Type::getI16Ty();
                } break;
                case 32: {
                    return Type::getI32Ty();
                } break;
                case 64: {
                    return Type::getI64Ty();
                } break;
                default: {
                    unreachable();
                } break;
            }
        } break;
        case 'u': { //<! u<bitWidth>
            int bitWidth = 0;
            sscanf(p, "u%d", &bitWidth);
            switch (bitWidth) {
                case 8: {
                    return Type::getU8Ty();
                } break;
                case 16: {
                    return Type::getU16Ty();
                } break;
                case 32: {
                    return Type::getU32Ty();
                } break;
                case 64: {
                    return Type::getU64Ty();
                } break;
                default: {
                    unreachable();
                } break;
            }
        } break;
        case 'f': { //<! f<bitWidth>
            int bitWidth = 0;
            sscanf(p, "f%d", &bitWidth);
            switch (bitWidth) {
                case 32: {
                    return Type::getFP32Ty();
                } break;
                case 64: {
                    return Type::getFP64Ty();
                } break;
                case 128: {
                    return Type::getFP128Ty();
                } break;
                default: {
                    unreachable();
                } break;
            }
        } break;
        case 'P': { //<! PtrType
            return Type::getPtrTy(from(++p));
        } break;
        case 'A': { //<! ArrayType
            const char* q = p + 1;
            assert(std::isdigit(*q));
            while (!std::isalpha(*++q)) {}
            auto type = from(q--);
            assert(*q == '_');
            do {
                while (std::isdigit(*--q)) {}
                assert(*q == 'A' || *q == '_');
                int size = 0;
                sscanf(q + 1, "%d_", &size);
                type = Type::getArrayTy(type, size);
            } while (q != p);
            return type;
        } break;
        case 'F': { //<! FnType
            bool isVariadic = *++p == 'V';
            if (isVariadic) { ++p; }
            auto rtype = from(p);
            p          += rtype->signature().size();
            assert(*p == '_' || *p == 'T');
            std::vector<Type*> paramTypes{};
            if (*p == '_') {
                ++p;
                do {
                    auto paramType = from(p);
                    paramTypes.push_back(paramType);
                    p += paramType->signature().size();
                } while (*p != 'T');
            }
            assert(p[0] == 'T');
            return Type::getFnTy(isVariadic, rtype, paramTypes);
        } break;
        default: {
            unreachable();
        }
    }
}

} // namespace slime::experimental::ir