#pragma once

#include <string_view>

namespace slime::ir {

class Value {
public:
    Value(const Value&)            = delete;
    Value& operator=(const Value&) = delete;

protected:
    Value();
};

} // namespace slime::ir
