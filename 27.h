#pragma once

#include <type_traits>

namespace slime {

template <typename E>
static constexpr inline auto is_scoped_enum =
#if __cplusplus >= 202002L
    std::is_scis_scoped_enum_v<E>
#else
    std::bool_constant<
        std::is_enum_v<E>
        && !std::is_convertible_v<E, std::underlying_type_t<E>>>::value
#endif
    ;

template <typename E>
constexpr inline auto to_underlying(E e)
    -> std::enable_if_t<is_scoped_enum<E>, std::underlying_type_t<E>> {
    using underlying_type = std::underlying_type_t<E>;
    return static_cast<underlying_type>(e);
}

} // namespace slime