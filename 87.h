#pragma once

#include <type_traits>

#if (__cplusplus >= 201703L && __cplusplus < 202002L)
namespace std {

template <typename E>
static constexpr inline auto is_scoped_enum_v = std::bool_constant<
    std::is_enum_v<E>
    && !std::is_convertible_v<E, std::underlying_type_t<E>>>::value;

template <typename E>
constexpr inline auto to_underlying(E e)
    -> std::enable_if_t<is_scoped_enum_v<E>, std::underlying_type_t<E>> {
    using underlying_type = std::underlying_type_t<E>;
    return static_cast<underlying_type>(e);
}

}; // namespace std
#endif
