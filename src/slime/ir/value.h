#pragma once

#include "type.h"

#include <string_view>

namespace slime::ir {

class Value {
public:
    Value(const Value&)            = delete;
    Value& operator=(const Value&) = delete;

    Value(Type* type)
        : type_{type} {}

private:
    Type* type_;
};

} // namespace slime::ir
