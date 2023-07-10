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
        void(std::declval<T&>().begin() != std::declval<T&>().end()),
        //! check operator++
        void(++std::declval<decltype(std::declval<T&>().end())&>()),
        //! check operator* and value type
        check<decltype(*std::declval<T&>().begin())>(0, 0));

    template <typename G>
    static constexpr std::false_type check(...);

    using type = decltype(check<T>(0));
};

template <typename T, typename R>
using is_iterable_as_t = typename is_iterable_as<T, R>::type;

template <typename T, bool Unique>
struct BuildTrait;

template <typename T>
struct BuildTrait<T, false> {
    using this_type  = BuildTrait<T, false>;
    using inner_type = T;

    template <typename... Args>
    static inline inner_type* create(Args&&... args) {
        return new inner_type(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline inner_type* construct(void* ptr, Args&&... args) {
        return new (ptr) inner_type(std::forward<Args>(args)...);
    }
};

template <typename T>
struct BuildTrait<T, true> {
    using this_type  = BuildTrait<T, false>;
    using inner_type = T;

    static inline inner_type* get() {
        static inner_type singleton;
        return &singleton;
    }
};

} // namespace detail

template <typename T, typename G>
static inline constexpr bool is_iterable_as =
    detail::is_iterable_as_t<T, G>::value;

template <size_t N, typename... Args>
using nth_type = decltype(std::get<N>(std::declval<std::tuple<Args...>>()));

template <typename T, typename... Args>
decltype(auto) firstValueOfTArguments(T&& first, const Args&... args) {
    return std::forward<T>(first);
}

template <typename T>
using BuildTrait = detail::BuildTrait<T, false>;

template <typename T>
using UniqueBuildTrait = detail::BuildTrait<T, true>;

} // namespace slime::utils
