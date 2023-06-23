#pragma once

#include <stddef.h>
#include <string>
#include <string_view>

struct Scope {
    Scope()
        : scope()
        , depth{0} {}

    bool isGlobal() const {
        return scope.empty();
    }

    std::string toString() const {
        return std::string(scope) + "#" + std::to_string(depth);
    }

    std::string_view scope;
    size_t           depth;
};
