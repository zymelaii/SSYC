#pragma once

#include "enum_trait.h"

namespace slime::utils {

namespace detail {
template <typename Self, typename E, auto FnEnum>
class TryIntoTrait;
}; // namespace detail

template <
    typename Self,
    auto FnEnum,
    typename Fn = decltype(FnEnum),
    typename R  = std::invoke_result_t<Fn, Self>,
    typename    = std::enable_if_t<std::is_scoped_enum_v<R>>,
    typename    = std::enable_if_t<std::is_same_v<R (Self::*)() const, Fn>>>
using TryIntoTraitWrapper = detail::TryIntoTrait<Self, R, FnEnum>;

namespace detail {

template <typename Self, typename E, auto FnEnum>
class TryIntoTrait : public Self {
private:
    using wrapper_type = TryIntoTraitWrapper<Self, FnEnum>;
    using inner_type   = Self;

public:
    template <typename... Args>
    TryIntoTrait(Args&&... args)
        : Self(std::forward<Args>(args)...) {}

public:
    template <typename G>
    inline auto as() {
        return static_cast<std::add_pointer_t<G>>(self());
    }

    template <typename G>
    inline auto as() const {
        return static_cast<std::add_pointer_t<std::add_const_t<G>>>(self());
    }

    template <typename G>
    inline auto into() const {
        return static_cast<std::add_pointer_t<G>>(self());
    }

    template <typename G>
    inline auto tryInto() const {
        return is<G>() ? into<G>() : nullptr;
    }

    template <typename G>
    inline bool is() const {
        using wrapper_type = TryIntoTraitWrapper<Self, FnEnum>;
        using this_type    = std::add_pointer_t<wrapper_type>;
        using other_type   = std::add_pointer_t<G>;
        return (this->*FnEnum)() == declare_t<G>;
    }

    template <>
    inline constexpr bool is<wrapper_type>() const {
        return true;
    }

    template <>
    inline constexpr bool is<inner_type>() const {
        return true;
    }

    template <typename G>
    static inline constexpr E declare_t = static_cast<E>(-1);

private:
    inline constexpr auto self() const {
        return const_cast<std::add_pointer_t<wrapper_type>>(this);
    }
};

} // namespace detail

} // namespace slime::utils
