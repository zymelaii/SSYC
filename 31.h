#pragma once

#include <type_traits>
#include <tuple>

namespace slime {

template <size_t N, typename... Args>
using nth_type = decltype(std::get<((sizeof...(Args) > 0) ? N + 1 : 0)>(
    std::declval<std::tuple<std::false_type, Args...>>()));

template <typename T, typename... Args>
decltype(auto) firstValueOfTArguments(T&& first, const Args&... args) {
    return std::forward<T>(first);
}

} // namespace slime