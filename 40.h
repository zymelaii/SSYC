#pragma once

#include <type_traits>

namespace slime {

namespace detail {

template <typename T, typename R>
struct is_iterable_as {
    template <typename V>
    static constexpr auto check(int, int)
        -> std::is_convertible<std::decay_t<V>, R>;

    template <typename G>
    static constexpr auto check(int) -> decltype(
        //! check begin, end, operator!=
        void(std::declval<G&>().begin() != std::declval<G&>().end()),
        //! check operator++
        void(++std::declval<decltype(std::declval<G&>().end())&>()),
        //! check operator* and value type
        check<decltype(*std::declval<G&>().begin())>(0, 0));

    template <typename G>
    static constexpr std::false_type check(...);

    using type = decltype(check<T>(0));
};

template <typename T, typename R>
using is_iterable_as_t = typename is_iterable_as<T, R>::type;

} // namespace detail

template <typename T, typename G>
static inline constexpr bool is_iterable_as =
    detail::is_iterable_as_t<T, G>::value;

} // namespace slime
