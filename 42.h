#pragma once

#include "39.h"

/*!
 * \brief magic macros and types for universal try-into inplements
 *
 *  1. declare your own base type
 *
 *  enum ID { A, B, C, D };
 *  class TypeBase {
 *  public:
 *      TypeBase(ID id) : id_{id} {}
 *      auto id() const { return id_; }
 *
 *  private:
 *      ID id_;
 *  };
 *
 *  using Type = TryIntoTraitWrapper<TypeBase, &TypeBase::id>;
 *
 *  2. declare your derived types
 *
 *  class DA : public Type {
 *  public:
 *      DA() : Type(ID::A) {}
 *  };
 *
 *  class DB : public Type {
 *  public:
 *      DB() : Type(ID::B) {}
 *  };
 *
 *  class DC : public Type {
 *  public:
 *      DC(ID id = ID::C) : Type(id) {}
 *  };
 *
 * class DD : public DC {
 *  public:
 *      DD() : DC(id) {}
 *  };
 *
 *  3. specify enum type for derived type
 *
 *  emit(auto) Type::declareTryIntoItem(DA, ID::A);
 *  emit(auto) Type::declareTryIntoItem(DB, ID::B);
 *  emit(auto) Type::declareTryIntoItem(DD, ID::D);
 *
 *  4. provide specilized is<G> check
 *
 *  emit(bool) Type::is<DC>() const {
 *      return wrapper()->id() == ID::C || is<DC>();
 *  }
 *
 *  5. enjoy yourself
 *
 *  int main() {
 *      Type* t = new DD;
 *      if (auto e = t->tryInto<DC>()) {
 *          //! TODO: ...
 *      }
 *      return 0;
 *  }
 */

namespace slime {

namespace detail {

template <typename Self, typename E, auto FnEnum>
class TryIntoTrait;

enum class EmptyEnum;

}; // namespace detail

template <
    typename Self,
    auto FnEnum,
    typename Fn = decltype(FnEnum),
    typename R  = std::invoke_result_t<Fn, Self>,
    typename    = std::enable_if_t<is_scoped_enum<R>>,
    typename    = std::enable_if_t<std::is_same_v<R (Self::*)() const, Fn>>>
using EnumBasedTryIntoTraitWrapper = detail::TryIntoTrait<Self, R, FnEnum>;

template <typename Self>
using UniversalTryIntoTraitWrapper =
    detail::TryIntoTrait<Self, detail::EmptyEnum, nullptr>;

namespace detail {
template <typename Self, typename E, auto FnEnum>
class TryIntoTrait : public Self {
private:
    using wrapper_type = TryIntoTrait<Self, E, FnEnum>;
    using inner_type   = Self;

public:
    template <typename... Args>
    TryIntoTrait(Args&&... args)
        : Self(std::forward<Args>(args)...) {}

public:
    template <typename G>
    inline auto as() {
        using forward_type = std::add_pointer_t<G>;
        return static_cast<forward_type>(self());
    }

    template <typename G>
    inline auto as() const {
        using const_type   = std::add_const_t<G>;
        using forward_type = std::add_pointer_t<const_type>;
        return static_cast<forward_type>(self()->template as<G>());
    }

    template <typename G>
    inline auto into() const {
        return self()->template as<G>();
    }

    template <typename G>
    inline auto tryInto() const {
        return is<G>() ? into<G>() : nullptr;
    }

    template <typename G>
    inline bool is() const {
        using this_type  = std::add_pointer_t<wrapper_type>;
        using other_type = std::add_pointer_t<G>;
        if constexpr (std::is_same_v<G, wrapper_type>) {
            return true;
        } else if constexpr (std::is_same_v<G, inner_type>) {
            return true;
        } else if constexpr (std::is_same_v<E, detail::EmptyEnum>) {
            return false;
        } else {
            return (this->*FnEnum)() == declare_t<G>;
        }
    }

    //! FIXME: -1 can be a valid of scoped enum E
    template <typename G>
    static inline constexpr E declare_t = static_cast<E>(-1);

private:
    inline constexpr auto self() const {
        using forward_type = std::add_pointer_t<wrapper_type>;
        return const_cast<forward_type>(this);
    }
};

} // namespace detail

#ifndef TRY_INTO_MAGIC_MACROS
#define TRY_INTO_MAGIC_MACROS

#if defined(emit) || defined(declareTryIntoItem)
#error macro `emit` or `declareTryIntoItem` is reserved for try-into trait
#endif

#define emit(type) \
 template <>       \
 template <>       \
 inline /*constexpr*/ type

#define DECLARE_TRY_INTO_ITEM_M4(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define DECLARE_TRY_INTO_ITEM_M3(...) \
 DECLARE_TRY_INTO_ITEM_M4(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define DECLARE_TRY_INTO_ITEM_M2(n, a, b, ...) \
 TRY_INTO_DECLARE_DISPATCH_##n(a, b, ##__VA_ARGS__)
#define DECLARE_TRY_INTO_ITEM_M1(...) DECLARE_TRY_INTO_ITEM_M2(__VA_ARGS__)

#define declareTryIntoItem(a, b, ...) \
 DECLARE_TRY_INTO_ITEM_M1(            \
     DECLARE_TRY_INTO_ITEM_M3(b, ##__VA_ARGS__), a, b, ##__VA_ARGS__)

#define TRY_INTO_DECLARE_DISPATCH_1(a, b)          declare_t<a> = b
#define TRY_INTO_DECLARE_DISPATCH_2(a, b, c)       declare_t<a, b> = c
#define TRY_INTO_DECLARE_DISPATCH_3(a, b, c, d)    declare_t<a, b, c> = d
#define TRY_INTO_DECLARE_DISPATCH_4(a, b, c, d, e) declare_t<a, b, c, d> = e
#define TRY_INTO_DECLARE_DISPATCH_5(a, b, c, d, e, f) \
 declare_t<a, b, c, d, e> = f
#define TRY_INTO_DECLARE_DISPATCH_6(a, b, c, d, e, f, g) \
 declare_t<a, b, c, d, e, f> = g
#define TRY_INTO_DECLARE_DISPATCH_7(a, b, c, d, e, f, g, h) \
 declare_t<a, b, c, d, e, f, g> = h
#define TRY_INTO_DECLARE_DISPATCH_8(a, b, c, d, e, f, g, h, i) \
 declare_t<a, b, c, d, e, f, g, h> = i
#define TRY_INTO_DECLARE_DISPATCH_9(a, b, c, d, e, f, g, h, i, j) \
 declare_t<a, b, c, d, e, f, g, h, i> = j

#endif

} // namespace slime
