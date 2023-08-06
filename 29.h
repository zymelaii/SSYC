#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

namespace slime::experimental::ir {

using TypeFlag = uint32_t;

enum class TypeKind : uint8_t;

struct TypeFlagMask {
    static constexpr size_t   MetaWidth = 3;
    static constexpr TypeFlag Meta      = (1 << MetaWidth) - 1;
    static_assert(MetaWidth > 0 && MetaWidth < sizeof(TypeFlag) * 8);

    static constexpr size_t   IdWidth = 3;
    static constexpr TypeFlag Id      = ((1 << IdWidth) - 1) << MetaWidth;
    static_assert(IdWidth > 0 && (IdWidth + MetaWidth) < sizeof(TypeFlag) * 8);

    static constexpr size_t   KindWidth = 8;
    static constexpr TypeFlag Kind      = (1 << KindWidth) - 1;
    static_assert(MetaWidth + IdWidth <= KindWidth);

    static constexpr size_t   PropertyWidth = sizeof(TypeFlag) * 8 - KindWidth;
    static constexpr TypeFlag Property      = (1 << PropertyWidth) - 1;
    static_assert(PropertyWidth > 0 && PropertyWidth < sizeof(TypeFlag) * 8);

    static constexpr TypeFlag Primitive  = 0b001;
    static constexpr TypeFlag Compound   = 0b010;
    static constexpr TypeFlag Sequential = 0b100;
    static_assert(Primitive <= Meta);
    static_assert(Sequential <= Meta);

    template <TypeFlag ID>
    static constexpr TypeFlag makeIdField() {
        static_assert(ID >= 0 && ID <= Id);
        return ID << MetaWidth;
    }

    static inline TypeKind getKindField(TypeFlag flag);

    template <TypeFlag P>
    static constexpr TypeFlag makePropertyField() {
        static_assert(P >= 0 && P <= Property);
        return P << KindWidth;
    }

    static inline TypeFlag makePropertyField(TypeFlag property);
    static inline TypeFlag getPropertyField(TypeFlag flag);
};

enum class TypeKind : uint8_t {
    Void    = TypeFlagMask::makeIdField<0>() | TypeFlagMask::Primitive,
    Integer = TypeFlagMask::makeIdField<1>() | TypeFlagMask::Primitive,
    Float   = TypeFlagMask::makeIdField<2>() | TypeFlagMask::Primitive,
    Pointer = TypeFlagMask::makeIdField<3>() | TypeFlagMask::Compound
            | TypeFlagMask::Sequential,
    Array = TypeFlagMask::makeIdField<4>() | TypeFlagMask::Compound
          | TypeFlagMask::Sequential,
    Function = TypeFlagMask::makeIdField<5>() | TypeFlagMask::Compound,
};

inline TypeKind TypeFlagMask::getKindField(TypeFlag flag) {
    return static_cast<TypeKind>(flag & Kind);
}

inline TypeFlag TypeFlagMask::makePropertyField(TypeFlag property) {
    assert(property >= 0 && property <= Property);
    return property << KindWidth;
}

inline TypeFlag TypeFlagMask::getPropertyField(TypeFlag flag) {
    return (flag >> KindWidth) & Property;
}

} // namespace slime::experimental::ir