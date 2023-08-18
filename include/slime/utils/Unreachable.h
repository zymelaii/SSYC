#pragma once

#include <exception>

inline namespace slime_utils {

[[noreturn]] inline void unreachable() noexcept {
    std::terminate();
}

} // namespace slime_utils
