#pragma once

#include <assert.h>

namespace slime::experimental::ir {

class Value;
class User;

class Use final {
    friend class User;
    template <int N>
    friend class SpecificUser;

protected:
    Use() = default;

    void attachTo(User* user) {
        user_ = user;
    }

public:
    Value* value() const {
        return value_;
    }

    User* user() const {
        return user_;
    }

    void reset(const Value* value = nullptr);

    void transfer(Use& dest) {
        dest.reset(value());
        reset(nullptr);
    }

    Use& operator=(const Value* value) {
        reset(value);
        return *this;
    }

    Use& operator=(Use& other) {
        transfer(other);
        return *this;
    }

    Value* operator->() const {
        assert(value() != nullptr);
        return value();
    }

private:
    User*  user_  = nullptr;
    Value* value_ = nullptr;
};

} // namespace slime::experimental::ir