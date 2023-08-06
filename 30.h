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
    inline Use();

    inline void attachTo(User* user);

public:
    inline Value* value() const;
    inline User*  user() const;

    void        reset(const Value* value = nullptr);
    inline void transfer(Use& dest);

    inline Use& operator=(const Value* value);
    inline Use& operator=(Use& other);

    inline Value* operator->() const;

private:
    User*  user_;
    Value* value_;
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline Use::Use()
    : user_{nullptr}
    , value_{nullptr} {}

inline void Use::attachTo(User* user) {
    user_ = user;
}

inline Value* Use::value() const {
    return value_;
}

inline User* Use::user() const {
    return user_;
}

inline void Use::transfer(Use& dest) {
    dest.reset(value());
    reset(nullptr);
}

inline Use& Use::operator=(const Value* value) {
    reset(value);
    return *this;
}

inline Use& Use::operator=(Use& other) {
    transfer(other);
    return *this;
}

inline Value* Use::operator->() const {
    assert(value() != nullptr);
    return value();
}

} // namespace slime::experimental::ir