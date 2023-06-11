#pragma once

#include "value.h"

#include <string_view>

namespace slime::ir {

class Function;

class Argument final : public Value {
public:
    Argument(
        Type            *type,
        std::string_view name  = "",
        Function        *fn    = nullptr,
        int              index = 0)
        : Value(type)
        , parent_{fn}
        , index_{index} {}

    Function *parent() {
        return parent_;
    }

    int index() const {
        return index_;
    }

private:
    Function *parent_;
    int       index_;
};

} // namespace slime::ir
