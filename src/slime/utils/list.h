#pragma once

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <type_traits>
#include <iterator>
#include <utility>

namespace slime::utils {

template <typename T>
class ListNode;

template <typename T>
class ListTrait;

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
        auto node = tailGuard()->prev_;
        return node == headGuard() ? nullptr : node;
    }

    node_type *head() const {
        return const_cast<AbstractListTrait<T> *>(this)->head();
    }

    node_type *tail() const {
        return const_cast<AbstractListTrait<T> *>(this)->tail();
    }

protected:
    node_type *headGuard() const {
        return const_cast<AbstractListTrait<T> *>(this)->headGuard();
    }

    node_type *tailGuard() const {
        return const_cast<AbstractListTrait<T> *>(this)->tailGuard();
    }

    virtual node_type *headGuard() = 0;

    virtual node_type *tailGuard() = 0;

private:
    size_t size_ = 0;
};

template <typename T>
class ListNode {
public:
    using value_type = T;

    friend AbstractListTrait<value_type>;
    friend ListTrait<value_type>;

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
        assert(node != nullptr);
        assert(node->parent_ != nullptr);
        if (parent_ != nullptr) { removeFromList(); }
        parent_      = node->parent_;
        prev_        = node->prev();
        next_        = node;
        next_->prev_ = this;
        prev_->next_ = this;
        ++parent_->size_;
    }

    void insertAfter(ListNode *node) {
        assert(node != nullptr);
        assert(node->parent_ != nullptr);
        if (parent_ != nullptr) { removeFromList(); }
        parent_      = node->parent_;
        prev_        = node;
        next_        = node->next();
        prev_->next_ = this;
        next_->prev_ = this;
        ++parent_->size_;
    }

    void insertToHead(AbstractListTrait<T> &list) {
        if (parent_ != nullptr) { removeFromList(); }
        auto node          = list.headGuard();
        prev_              = node;
        next_              = node->next_;
        node->next_->prev_ = this;
        node->next_        = this;
        parent_            = &list;
        ++list.size_;
    }

    void insertToTail(AbstractListTrait<T> &list) {
        if (parent_ != nullptr) { removeFromList(); }
        auto node          = list.tailGuard();
        prev_              = node->prev_;
        next_              = node;
        node->prev_->next_ = this;
        node->prev_        = this;
        parent_            = &list;
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

    template <typename... Args>
    ListNode *emplaceAfter(Args &&...args) {
        value_type e(std::forward<Args>(args)...);
        auto       node = new ListNode(e);
        insertAfter(node);
        return node;
    }

    template <typename... Args>
    ListNode *emplaceBefore(Args &&...args) {
        value_type e(std::forward<Args>(args)...);
        auto       node = new ListNode(e);
        insertBefore(node);
        return node;
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
}; // namespace slime::utils

template <typename T>
class ListTrait : public AbstractListTrait<T> {
public:
    using base_type  = AbstractListTrait<T>;
    using node_type  = typename base_type::node_type;
    using value_type = typename node_type::value_type;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = value_type;
        using pointer           = value_type *;
        using reference         = value_type &;

        iterator(node_type *ptr)
            : ptr_{ptr} {
            assert(ptr_ != nullptr && ptr_->parent_ != nullptr);
            parent_ = static_cast<ListTrait *>(ptr_->parent_);
        }

        iterator(const iterator &other)
            : iterator(other.ptr_) {}

        iterator &operator++() {
            if (ptr_ != parent_->tailGuard()) { ptr_ = ptr_->next(); }
            return *this;
        }

        iterator operator++(int) {
            auto it = *this;
            ++*this;
            return it;
        }

        reference operator*() {
            assert(ptr_ != parent_->tailGuard());
            return ptr_->value();
        }

        pointer operator->() {
            return ptr_->value();
        }

        bool operator==(const iterator &other) const {
            return parent_ == other.parent_ && ptr_ == other.ptr_;
        }

        bool operator!=(const iterator &other) const {
            return parent_ != other.parent_ || ptr_ != other.ptr_;
        }

    private:
        ListTrait *parent_;
        node_type *ptr_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = ListTrait::value_type;
        using pointer           = value_type *;
        using reference         = value_type &;

        const_iterator(node_type *ptr)
            : ptr_{ptr} {
            assert(ptr_ != nullptr && ptr_->parent_ != nullptr);
            parent_ = static_cast<ListTrait *>(ptr_->parent_);
        }

        const_iterator(const const_iterator &other)
            : const_iterator(other.ptr_) {}

        const_iterator &operator++() {
            if (ptr_ != parent_->tailGuard()) { ptr_ = ptr_->next(); }
            return *this;
        }

        const_iterator operator++(int) {
            auto it = *this;
            ++*this;
            return it;
        }

        reference operator*() {
            assert(ptr_ != parent_->tailGuard());
            return ptr_->value();
        }

        pointer operator->() {
            return &ptr_->value();
        }

        bool operator==(const const_iterator &other) const {
            return parent_ == other.parent_ && ptr_ == other.ptr_;
        }

        bool operator!=(const const_iterator &other) const {
            return parent_ != other.parent_ || ptr_ != other.ptr_;
        }

    private:
        ListTrait *parent_;
        node_type *ptr_;
    };

    class reverse_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = ListTrait::value_type;
        using pointer           = value_type *;
        using reference         = value_type &;

        reverse_iterator(node_type *ptr)
            : ptr_{ptr} {
            assert(ptr_ != nullptr && ptr_->parent_ != nullptr);
            parent_ = static_cast<ListTrait *>(ptr_->parent_);
        }

        reverse_iterator(const reverse_iterator &other)
            : reverse_iterator(other.ptr_) {}

        reverse_iterator &operator++() {
            if (ptr_ != parent_->headGuard()) { ptr_ = ptr_->prev(); }
            return *this;
        }

        reverse_iterator operator++(int) {
            auto it = *this;
            ++*this;
            return it;
        }

        reference operator*() {
            assert(ptr_ != parent_->headGuard());
            return ptr_->value();
        }

        pointer operator->() {
            return ptr_->value();
        }

        bool operator==(const reverse_iterator &other) const {
            return parent_ == other.parent_ && ptr_ == other.ptr_;
        }

        bool operator!=(const reverse_iterator &other) const {
            return parent_ != other.parent_ || ptr_ != other.ptr_;
        }

    private:
        ListTrait *parent_;
        node_type *ptr_;
    };

    class node_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = node_type;
        using pointer           = value_type *;
        using reference         = value_type &;

        node_iterator(node_type *ptr)
            : ptr_{ptr} {
            assert(ptr_ != nullptr && ptr_->parent_ != nullptr);
            parent_ = static_cast<ListTrait *>(ptr_->parent_);
        }

        node_iterator(const node_iterator &other)
            : node_iterator(other.ptr_) {}

        node_iterator &operator++() {
            if (ptr_ != parent_->tailGuard()) { ptr_ = ptr_->next(); }
            return *this;
        }

        node_iterator operator++(int) {
            auto it = *this;
            ++*this;
            return it;
        }

        reference operator*() {
            assert(ptr_ != parent_->tailGuard());
            return *ptr_;
        }

        pointer operator->() {
            return ptr_;
        }

        bool operator==(const node_iterator &other) const {
            return parent_ == other.parent_ && ptr_ == other.ptr_;
        }

        bool operator!=(const node_iterator &other) const {
            return parent_ != other.parent_ || ptr_ != other.ptr_;
        }

    private:
        ListTrait *parent_;
        node_type *ptr_;
    };

    iterator begin() {
        return iterator(headGuard()->next_);
    }

    iterator end() {
        return iterator(tailGuard());
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    const_iterator cbegin() const {
        return const_iterator(AbstractListTrait<T>::headGuard()->next_);
    }

    const_iterator cend() const {
        return const_iterator(AbstractListTrait<T>::tailGuard());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(tailGuard()->prev_);
    }

    reverse_iterator rend() {
        return reverse_iterator(headGuard());
    }

    node_iterator node_begin() {
        return node_iterator(headGuard()->next_);
    }

    node_iterator node_end() {
        return node_iterator(tailGuard());
    }

    ListTrait() {
        memset(guard_, 0, sizeof(node_type) * 2);
        guard_[0].parent_ = this;
        guard_[1].parent_ = this;
        guard_[0].next_   = &guard_[1];
        guard_[1].prev_   = &guard_[0];
    }

    ListTrait(const ListTrait &list)
        : ListTrait() {
        for (auto &e : list) { insertToTail(e); }
    }

    ListTrait(ListTrait &&list)
        : ListTrait() {
        for (auto &e : list) { insertToTail(std::move(e)); }
    }

    ListTrait(const std::initializer_list<value_type> &list)
        : ListTrait() {
        for (const auto &e : list) { insertToTail(e); }
    }

    template <typename... Args>
    static ListTrait *from(Args &&...args) {
        return new ListTrait(
            std::initializer_list<value_type>{std::forward<Args>(args)...});
    }

    template <typename... Args>
    node_type *insertToHead(Args &&...args) {
        value_type e(std::forward<Args>(args)...);
        auto       node = new node_type(e);
        node->insertToHead(*this);
        return node;
    }

    template <typename... Args>
    node_type *insertToTail(Args &&...args) {
        value_type e(std::forward<Args>(args)...);
        auto       node = new node_type(e);
        node->insertToTail(*this);
        return node;
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
