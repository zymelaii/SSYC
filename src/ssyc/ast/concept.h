#pragma once

#include "common.h"

#include <type_traits>
#include <concepts>

namespace ssyc::ast {

inline namespace details {

template <typename T>
concept ast_node = std::derived_from<T, AbstractAstNode>;

} // namespace details

} // namespace ssyc::ast
