#pragma once

namespace slime::ir {

class Value;

class Use {
public:
    void set(Value *value);

    Value     *operator=(Value *rhs);
    const Use &operator=(const Use &rhs);

private:
    Value *value_;
};

} // namespace slime::ir
