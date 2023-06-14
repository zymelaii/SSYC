#pragma once

#include <stddef.h>
#include <assert.h>
#include <type_traits>
#include <utility>

namespace slime::utils {

template <typename T>
class ListNode;

template <typename T>
class AbstractListTrait {
public:
    using node_type  = ListNode<T>;
    using value_type = typename node_type::value_type;

    friend node_type;

    size_t size() const {
        return size_;
    }

    node_type *head() {
        auto node = headGuard()->next_;
        return node == tailGuard() ? nullptr : node;
    }

    node_type *tail() {
        auto node = tailGuard()->next_;
        return node == headGuard() ? nullptr : node;
    }

protected:
    virtual node_type *headGuard() = 0;
    virtual node_type *tailGuard() = 0;

private:
    size_t size_;
};

template <typename T>
class ListNode {
public:
    using value_type = T;

    ListNode()
        : isPtr_{std::is_trivial_v<value_type>}
        , prev_{nullptr}
        , next_{nullptr}
        , parent_{nullptr} {
        if constexpr (std::is_trivial_v<value_type>) {
            valuePtr_ = new value_type;
        } else {
            new (&valueObj_) value_type;
        }
    }

    ListNode(value_type &value)
        : isPtr_{false}
        , valueObj_(std::move(value))
        , prev_{nullptr}
        , next_{nullptr}
        , parent_{nullptr} {}

    ~ListNode() {
        if (isPtr_) {
            delete valuePtr_;
            valuePtr_ = nullptr;
        }
    }

    ListNode *prev() {
        if (parent_ == nullptr) {
            return prev_;
        } else {
            return prev_ == nullptr ? parent_->headGuard() : prev_;
        }
    }

    ListNode *next() {
        if (parent_ == nullptr) {
            return next_;
        } else {
            return next_ == nullptr ? parent_->tailGuard() : next_;
        }
    }

    value_type &value() {
        if (isPtr_) {
            assert(valuePtr_ != nullptr);
            return *valuePtr_;
        } else {
            return valueObj_;
        }
    }

    void removeFromList() {
        assert(parent_ != nullptr);
        prev()->next_ = next_;
        next()->prev_ = prev_;
        --parent_->size_;
        prev_   = nullptr;
        next_   = nullptr;
        parent_ = nullptr;
    }

    void insertBefore(ListNode *node) {
        assert(node->parent_ != nullptr);
        prev_        = node->prev();
        next_        = node;
        prev_->next_ = this;
        next_->prev_ = this;
        if (parent_ != node->parent_) {
            removeFromList();
            parent_ = node->parent_;
        }
        ++parent_->size_;
    }

    void insertAfter(ListNode *node) {
        assert(node->parent_ != nullptr);
        prev_        = node;
        next_        = node->next();
        prev_->next_ = this;
        next_->prev_ = this;
        if (parent_ != node->parent_) {
            removeFromList();
            parent_ = node->parent_;
        }
        ++parent_->size_;
    }

    void insertToHead(AbstractListTrait<T> &list) {
        if (parent_ != nullptr) { removeFromList(); }
        auto node          = list.headGuard();
        node->next_->prev_ = this;
        node->next_        = this;
        if (parent_ != &list) {
            removeFromList();
            parent_ = &list;
        }
        ++list.size_;
    }

    void insertToTail(AbstractListTrait<T> &list) {
        if (parent_ != nullptr) { removeFromList(); }
        auto node          = list.tailGuard();
        node->prev_->next_ = this;
        node->prev_        = this;
        if (parent_ != &list) {
            removeFromList();
            parent_ = &list;
        }
        ++list.size_;
    }

    void moveToPrev() {
        assert(parent_ != nullptr);
        if (prev_ != nullptr) {
            prev_->next_ = next_;
            next_        = prev_;
            prev_->prev_ = this;
        }
    }

    void moveToNext() {
        assert(parent_ != nullptr);
        if (next_ != nullptr) {
            next_->prev_ = prev_;
            prev_        = next_;
            next_->next_ = this;
        }
    }

private:
    const bool isPtr_;

    union {
        value_type *valuePtr_;
        value_type  valueObj_;
    };

    ListNode             *prev_;
    ListNode             *next_;
    AbstractListTrait<T> *parent_;
};

template <typename T>
class ListTrait : public AbstractListTrait<T> {
public:
    using base_type  = AbstractListTrait<T>;
    using node_type  = typename base_type::node_type;
    using value_type = typename node_type::value_type;

    ListTrait() {
        memset(guard_, 0, sizeof(node_type) * 2);
        guard_[0].parent_ = this;
        guard_[1].parent_ = this;
        guard_[0].next_   = &guard_[1];
        guard_[1].prev_   = &guard_[0];
    }

private:
    node_type *headGuard() override {
        return &guard_[0];
    }

    node_type *tailGuard() override {
        return &guard_[1];
    }

private:
    node_type guard_[2];
};

} // namespace slime::utils