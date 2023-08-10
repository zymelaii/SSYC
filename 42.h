#pragma once

#include <exception>

namespace slime {

[[noreturn]] inline void unreachable() noexcept {
    std::terminate();
}

} // namespace slime
