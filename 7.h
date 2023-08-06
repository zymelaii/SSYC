#pragma once

#include <string_view>
#include <string>
#include <stddef.h>

namespace slime::ast {

struct Scope {
    Scope()
        : scope()
        , depth{0} {}

    bool isGlobal() const {
        return depth == 0;
    }

    std::string toString() {
        return std::string(scope) + "#" + std::to_string(depth);
    }

    std::string_view scope;
    size_t           depth;
};

} // namespace slime::ast
