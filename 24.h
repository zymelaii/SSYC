#pragma once

#include "26.h"

#include <string_view>
#include <array>
#include <vector>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

namespace slime::experimental::ir {

class UserBase : public Value {
protected:
    UserBase(Type *type, uint32_t tag, int totalUseIndicator)
        : Value(type, tag)
        , totalUseIndicator_{std::min(totalUseIndicator, 3) + 1} {
        assert(totalUseIndicator_ >= 0 && totalUseIndicator_ <= 4);
    }

public:
    bool used(const Use *use) const {
        const auto invoke = used_FNPTR_TABLE[totalUseIndicator_];
        const auto self   = const_cast<UserBase *>(this);
        return invoke(self, use);
    }

    size_t totalUse() const {
        const auto invoke = totalUse_FNPTR_TABLE[totalUseIndicator_];
        const auto self   = const_cast<UserBase *>(this);
        return invoke(self);
    }

    Use *uses() const {
        const auto invoke = uses_FNPTR_TABLE[totalUseIndicator_];
        const auto self   = const_cast<UserBase *>(this);
        return invoke(self);
    }

    Use &useAt(int index) const {
        assert(index >= 0 && index < totalUse());
        assert(uses() != nullptr);
        return uses()[index];
    }

protected:
    template <typename Derived>
    static bool usedForwardFn(UserBase *self, const Use *use) {
        return static_cast<Derived *>(self)->used(use);
    }

    template <typename Derived>
    static size_t totalUseForwardFn(UserBase *self) {
        return static_cast<Derived *>(self)->totalUse();
    }

    template <typename Derived>
    static Use *usesForwardFn(UserBase *self) {
        return static_cast<Derived *>(self)->uses();
    }

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
};

//! alias for forward declaration
class User : public UserBase {
protected:
    User(Type *type, uint32_t tag, int totalUseIndicator)
        : UserBase(type, tag, totalUseIndicator) {}
};

template <int N>
class SpecificUser;

template <>
class SpecificUser<-1> : public User {
protected:
    SpecificUser(Type *type, uint32_t tag)
        : User(type, tag, -1) {
        for (auto &use : uses_) { use.attachTo(this); }
    }

public:
    bool used(const Use *use) const {
        const auto offset = reinterpret_cast<intptr_t>(use)
                          - reinterpret_cast<intptr_t>(uses_.data());
        return offset >= 0 && offset < sizeof(Use) * totalUse()
            && offset % sizeof(Use) == 0;
    }

    size_t totalUse() const {
        return uses_.size();
    }

    Use *uses() const {
        return const_cast<Use *>(uses_.data());
    }

    template <size_t I>
    Use &op() const {
        assert(I >= 0 && I < totalUse());
        return const_cast<Use &>(uses_[I]);
    }

private:
    std::vector<Use> uses_;
};

template <>
class SpecificUser<0> : public User {
protected:
    SpecificUser(Type *type, uint32_t tag)
        : User(type, tag, 0) {}

public:
    bool used(const Use *use) const {
        return false;
    }

    size_t totalUse() const {
        return 0;
    }

    Use *uses() const {
        return nullptr;
    }
};

template <>
class SpecificUser<1> : public User {
protected:
    SpecificUser(Type *type, uint32_t tag)
        : User(type, tag, 1) {
        operand_.attachTo(this);
    }

public:
    bool used(const Use *use) const {
        return std::addressof(operand_) == use;
    }

    size_t totalUse() const {
        return 1;
    }

    Use *uses() const {
        return const_cast<Use *>(&operand_);
    }

    Use &operand() const {
        return const_cast<Use &>(operand_);
    }

private:
    Use operand_;
};

template <>
class SpecificUser<2> : public User {
protected:
    SpecificUser(Type *type, uint32_t tag)
        : User(type, tag, 2) {
        uses_[0].attachTo(this);
        uses_[1].attachTo(this);
    }

public:
    bool used(const Use *use) const {
        return std::addressof(uses_[0]) == use
            || std::addressof(uses_[1]) == use;
    }

    size_t totalUse() const {
        return 2;
    }

    Use *uses() const {
        return const_cast<Use *>(uses_);
    }

    Use &lhs() const {
        return const_cast<Use &>(uses_[0]);
    }

    Use &rhs() const {
        return const_cast<Use &>(uses_[1]);
    }

private:
    Use uses_[2];
};

template <>
class SpecificUser<3> : public User {
protected:
    SpecificUser(Type *type, uint32_t tag)
        : User(type, tag, 3) {
        uses_[0].attachTo(this);
        uses_[1].attachTo(this);
        uses_[2].attachTo(this);
    }

public:
    bool used(const Use *use) const {
        return std::addressof(uses_[0]) == use
            || std::addressof(uses_[1]) == use
            || std::addressof(uses_[2]) == use;
    }

    size_t totalUse() const {
        return 3;
    }

    Use *uses() const {
        return const_cast<Use *>(uses_);
    }

    template <size_t I>
    Use &op() const {
        static_assert(I >= 0 && I < 3);
        return const_cast<Use &>(uses_[I]);
    }

private:
    Use uses_[3];
};

} // namespace slime::experimental::ir