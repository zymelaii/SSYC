#pragma once

#include <tuple>
#include <type_traits>
#include <iterator>
#include <assert.h>

namespace slime::utils {

namespace detail {

template <typename T, typename R>
struct is_iterable_as {
    template <typename V>
    static constexpr auto check(int, int) -> std::is_same<std::decay_t<V>, R>;

    template <typename G>
    static constexpr auto check(int) -> decltype(
        //! check begin, end, operator!=
        void(std::begin(std::declval<T&>()) != std::end(std::declval<T&>())),
        //! check operator++
        void(++std::declval<decltype(std::begin(std::declval<T&>()))&>()),
        //! check operator* and value type
        check<decltype(*std::begin(std::declval<T&>()))>(0, 0));

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

template <size_t N, typename... Args>
using nth_type = decltype(std::get<N>(std::tuple<Args...>()));

} // namespace slime::utils
