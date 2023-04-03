#pragma once

#include "context.h"
#include "ssa.h"

#include <memory>

namespace ssyc::ir {

class Builder {
public:
    inline Builder(Context *context)
        : context_{context} {}

    inline Context &context() const {
        return *context_;
    }

private:
    Context *const context_;
};

} // namespace ssyc::ir
