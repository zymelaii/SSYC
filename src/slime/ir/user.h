#pragma once

#include "value.h"
#include "use.h"

namespace slime::ir {

class User : public Value {
public:
    User(const User&)            = delete;
    User& operator=(const User&) = delete;

    User(Type* type, Use* operands, size_t totalOps)
        : Value(type)
        , operands_{operands}
        , totalOps_(totalOps) {}

protected:
    Use& operandAt(int index) {
        index = index >= 0 ? index : totalOps_ + index;
        assert(index >= 0 && index < totalOps_);
        return operands_[index];
    }

private:
    Use*   operands_;
    size_t totalOps_;
};

} // namespace slime::ir
