#pragma once

#include "type.h"
#include "use.h"

#include <string_view>

namespace slime::ir {

class Function;

class Value {
public:
    Value(const Value &)            = delete;
    Value &operator=(const Value &) = delete;

    Value(Type *type)
        : type_{type} {}

private:
    Type *type_;
};

class Block final : public Value {};

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

class User : public Value {
public:
    User(const User &)            = delete;
    User &operator=(const User &) = delete;

    User(Type *type, Use *operands, size_t totalOps)
        : Value(type)
        , operands_{operands}
        , totalOps_(totalOps) {}

protected:
    Use &operandAt(int index) {
        index = index >= 0 ? index : totalOps_ + index;
        assert(index >= 0 && index < totalOps_);
        return operands_[index];
    }

private:
    Use   *operands_;
    size_t totalOps_;
};

} // namespace slime::ir
