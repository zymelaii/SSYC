#pragma once

#include "34.h"

#include <string_view>
#include <array>
#include <vector>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

namespace slime::experimental::ir {

class UserBase : public Value {
    friend ValueBase;

protected:
    inline UserBase(Type *type, uint32_t tag, int totalUseIndicator);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;
    inline Use   &useAt(int index) const;

    inline void *delegator() const;
    inline void  setDelegator(void *ptr);

protected:
    inline int totalUseIndicator() const;

    template <typename Derived>
    static bool usedForwardFn(UserBase *self, const Use *use);
    template <typename Derived>
    static size_t totalUseForwardFn(UserBase *self);
    template <typename Derived>
    static Use *usesForwardFn(UserBase *self);

private:
    const int totalUseIndicator_;

private:
    template <typename Fn>
    struct FnTypeConverter;

    template <typename R, typename... Args>
    struct FnTypeConverter<R (UserBase::*)(Args...) const> {
        using type = R (*)(UserBase *, Args...);
    };

    template <auto Fn>
    using invoke_type = typename FnTypeConverter<decltype(Fn)>::type;

    template <auto Fn, size_t N = 5>
    using FnPtrTable = std::array<invoke_type<Fn>, N>;

    static FnPtrTable<&UserBase::used>     used_FNPTR_TABLE;
    static FnPtrTable<&UserBase::totalUse> totalUse_FNPTR_TABLE;
    static FnPtrTable<&UserBase::uses>     uses_FNPTR_TABLE;

    void *delegator_;
};

//! alias for forward declaration
class User : public UserBase {
protected:
    inline User(Type *type, uint32_t tag, int totalUseIndicator);

public:
    inline User *withDelegator(void *ptr);
};

template <int N>
class SpecificUser;

template <>
class SpecificUser<-1> : public User {
protected:
    inline SpecificUser(Type *type, uint32_t tag);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;

    template <size_t I>
    inline Use &op() const;

private:
    std::vector<Use> uses_;
};

template <>
class SpecificUser<0> : public User {
protected:
    inline SpecificUser(Type *type, uint32_t tag);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;
};

template <>
class SpecificUser<1> : public User {
protected:
    inline SpecificUser(Type *type, uint32_t tag);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;

    inline Use &operand() const;

private:
    Use operand_;
};

template <>
class SpecificUser<2> : public User {
protected:
    inline SpecificUser(Type *type, uint32_t tag);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;

    inline Use &lhs() const;
    inline Use &rhs() const;

private:
    Use uses_[2];
};

template <>
class SpecificUser<3> : public User {
protected:
    inline SpecificUser(Type *type, uint32_t tag);

public:
    inline bool   used(const Use *use) const;
    inline size_t totalUse() const;
    inline Use   *uses() const;

    template <size_t I>
    inline Use &op() const;

private:
    Use uses_[3];
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline UserBase::UserBase(Type *type, uint32_t tag, int totalUseIndicator)
    : Value(type, tag)
    , totalUseIndicator_{std::min(totalUseIndicator, 3) + 1} {
    assert(totalUseIndicator_ >= 0 && totalUseIndicator_ <= 4);
}

inline bool UserBase::used(const Use *use) const {
    const auto invoke = used_FNPTR_TABLE[totalUseIndicator_];
    const auto self   = const_cast<UserBase *>(this);
    return invoke(self, use);
}

inline size_t UserBase::totalUse() const {
    const auto invoke = totalUse_FNPTR_TABLE[totalUseIndicator_];
    const auto self   = const_cast<UserBase *>(this);
    return invoke(self);
}

inline Use *UserBase::uses() const {
    const auto invoke = uses_FNPTR_TABLE[totalUseIndicator_];
    const auto self   = const_cast<UserBase *>(this);
    return invoke(self);
}

inline Use &UserBase::useAt(int index) const {
    assert(index >= 0 && index < totalUse());
    assert(uses() != nullptr);
    return uses()[index];
}

inline void *UserBase::delegator() const {
    return delegator_;
}

inline void UserBase::setDelegator(void *ptr) {
    delegator_ = ptr;
}

inline int UserBase::totalUseIndicator() const {
    return totalUseIndicator_;
}

template <typename Derived>
inline bool UserBase::usedForwardFn(UserBase *self, const Use *use) {
    return static_cast<Derived *>(self)->used(use);
}

template <typename Derived>
inline size_t UserBase::totalUseForwardFn(UserBase *self) {
    return static_cast<Derived *>(self)->totalUse();
}

template <typename Derived>
inline Use *UserBase::usesForwardFn(UserBase *self) {
    return static_cast<Derived *>(self)->uses();
}

inline User::User(Type *type, uint32_t tag, int totalUseIndicator)
    : UserBase(type, tag, totalUseIndicator) {}

inline User *User::withDelegator(void *ptr) {
    setDelegator(ptr);
    return this;
}

inline SpecificUser<-1>::SpecificUser(Type *type, uint32_t tag)
    : User(type, tag, -1) {
    for (auto &use : uses_) { use.attachTo(this); }
}

inline bool SpecificUser<-1>::used(const Use *use) const {
    const auto offset = reinterpret_cast<intptr_t>(use)
                      - reinterpret_cast<intptr_t>(uses_.data());
    return offset >= 0 && offset < sizeof(Use) * totalUse()
        && offset % sizeof(Use) == 0;
}

inline size_t SpecificUser<-1>::totalUse() const {
    return uses_.size();
}

inline Use *SpecificUser<-1>::uses() const {
    return const_cast<Use *>(uses_.data());
}

template <size_t I>
inline Use &SpecificUser<-1>::op() const {
    assert(I >= 0 && I < totalUse());
    return const_cast<Use &>(uses_[I]);
}

inline SpecificUser<0>::SpecificUser(Type *type, uint32_t tag)
    : User(type, tag, 0) {}

inline bool SpecificUser<0>::used(const Use *use) const {
    return false;
}

inline size_t SpecificUser<0>::totalUse() const {
    return 0;
}

inline Use *SpecificUser<0>::uses() const {
    return nullptr;
}

inline SpecificUser<1>::SpecificUser(Type *type, uint32_t tag)
    : User(type, tag, 1) {
    operand_.attachTo(this);
}

inline bool SpecificUser<1>::used(const Use *use) const {
    return std::addressof(operand_) == use;
}

inline size_t SpecificUser<1>::totalUse() const {
    return 1;
}

inline Use *SpecificUser<1>::uses() const {
    return const_cast<Use *>(&operand_);
}

inline Use &SpecificUser<1>::operand() const {
    return const_cast<Use &>(operand_);
}

inline SpecificUser<2>::SpecificUser(Type *type, uint32_t tag)
    : User(type, tag, 2) {
    uses_[0].attachTo(this);
    uses_[1].attachTo(this);
}

inline bool SpecificUser<2>::used(const Use *use) const {
    return std::addressof(uses_[0]) == use || std::addressof(uses_[1]) == use;
}

inline size_t SpecificUser<2>::totalUse() const {
    return 2;
}

inline Use *SpecificUser<2>::uses() const {
    return const_cast<Use *>(uses_);
}

inline Use &SpecificUser<2>::lhs() const {
    return const_cast<Use &>(uses_[0]);
}

inline Use &SpecificUser<2>::rhs() const {
    return const_cast<Use &>(uses_[1]);
}

inline SpecificUser<3>::SpecificUser(Type *type, uint32_t tag)
    : User(type, tag, 3) {
    uses_[0].attachTo(this);
    uses_[1].attachTo(this);
    uses_[2].attachTo(this);
}

inline bool SpecificUser<3>::used(const Use *use) const {
    return std::addressof(uses_[0]) == use || std::addressof(uses_[1]) == use
        || std::addressof(uses_[2]) == use;
}

inline size_t SpecificUser<3>::totalUse() const {
    return 3;
}

inline Use *SpecificUser<3>::uses() const {
    return const_cast<Use *>(uses_);
}

template <size_t I>
inline Use &SpecificUser<3>::op() const {
    static_assert(I >= 0 && I < 3);
    return const_cast<Use &>(uses_[I]);
}

} // namespace slime::experimental::ir

emit(bool)
    slime::experimental::ir::ValueBase::is<slime::experimental::ir::User>()
        const {
    return isUser();
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::SpecificUser<-1>>() const {
    auto user = tryInto<slime::experimental::ir::User>();
    return user && user->totalUseIndicator() == -1;
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::SpecificUser<0>>() const {
    auto user = tryInto<slime::experimental::ir::User>();
    return user && user->totalUseIndicator() == 0;
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::SpecificUser<1>>() const {
    auto user = tryInto<slime::experimental::ir::User>();
    return user && user->totalUseIndicator() == 1;
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::SpecificUser<2>>() const {
    auto user = tryInto<slime::experimental::ir::User>();
    return user && user->totalUseIndicator() == 2;
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::SpecificUser<3>>() const {
    auto user = tryInto<slime::experimental::ir::User>();
    return user && user->totalUseIndicator() == 3;
}
