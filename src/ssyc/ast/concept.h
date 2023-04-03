#pragma once

#include "common.h"

#include <type_traits>
#include <concepts>

namespace ssyc::ast {

inline namespace details {

template <typename T>
concept ast_node =
    std::derived_from<T, AbstractAstNode>
    && std::is_same_v<std::remove_cvref_t<std::decay_t<T>>, AbstractAstNode>;

} // namespace details

} // namespace ssyc::ast
