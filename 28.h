#pragma once

#include "38.h"

#include <set>

namespace slime::experimental::ir {

struct LoopInfo {
    BasicBlock* entry;
    size_t      depth;
};

class Module final {
private:
    std::set<Type*> typePool_;
};

} // namespace slime::experimental::ir
