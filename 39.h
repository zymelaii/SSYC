#pragma once

#include "21.h"
#include <type_traits>
#include <variant>
#include <functional>
#include <assert.h>

namespace slime {

template <typename T>
class LinkedList;

namespace detail {

template <typename T>
class LinkedListNode {
public:
    using value_type  = T;
    using node_type   = LinkedListNode<value_type>;
    using parent_type = LinkedList<value_type>;
    static_assert(std::is_same_v<T, value_type>);

public:
    template <typename... Args>
    LinkedListNode(Args&&... args);
    ~LinkedListNode();

    inline parent_type*&      parent();
    inline const parent_type* parent() const;

    inline node_type*&      prev();
    inline const node_type* prev() const;

    inline node_type*&      next();
    inline const node_type* next() const;

    inline value_type&       value();
    inline const value_type& value() const;

    inline value_type&       operator*();
    inline const value_type& operator*() const;

    inline value_type*       operator->();
    inline const value_type* operator->() const;

    void removeFromList();
    void insertToHead(parent_type* list);
    void insertToTail(parent_type* list);
    void insertBefore(node_type* node);
    void insertAfter(node_type* node);
    void moveBefore(node_type* node);
    void moveAfter(node_type* node);
    void insertOrMoveBefore(node_type* node);
    void insertOrMoveAfter(node_type* node);
    void moveToPrev();
    void moveToNext();
    void moveToHead();
    void moveToTail();

private:
    std::variant<std::monostate, value_type, value_type*> value_;

    parent_type* parent_;
    node_type*   prev_;
    node_type*   next_;
};

} // namespace detail

template <typename T>
class LinkedList {
public:
    using value_type = std::decay_t<T>;
    using node_type  = detail::LinkedListNode<value_type>;
    static_assert(std::is_same_v<T, value_type>);

protected:
    template <typename G>
    class iterator_wrapper {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::decay_t<G>;
        using pointer           = value_type*;
        using reference         = value_type&;
        static_assert(std::is_convertible_v<T, G>);

    private:
        using node_type = LinkedList::node_type;

    public:
        inline iterator_wrapper(const LinkedList* parent, node_type* first);
        inline iterator_wrapper(const iterator_wrapper& other);
        inline iterator_wrapper& operator=(const iterator_wrapper& other);
        inline iterator_wrapper& operator++();
        inline iterator_wrapper  operator++(int);
        inline pointer           operator->() const;
        inline reference         operator*() const;
        inline bool operator==(const iterator_wrapper& other) const;
        inline bool operator!=(const iterator_wrapper& other) const;

    private:
        const LinkedList*  parent_;
        mutable node_type* ptr_;
    };

    class reverse_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = LinkedList::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;

    private:
        using node_type = LinkedList::node_type;

    public:
        inline reverse_iterator(const LinkedList* parent, node_type* first);
        inline reverse_iterator(const reverse_iterator& other);
        inline reverse_iterator& operator=(const reverse_iterator& other);
        inline reverse_iterator& operator++();
        inline reverse_iterator  operator++(int);
        inline pointer           operator->() const;
        inline reference         operator*() const;
        inline bool operator==(const reverse_iterator& other) const;
        inline bool operator!=(const reverse_iterator& other) const;

    private:
        const LinkedList*  parent_;
        mutable node_type* ptr_;
    };

    class node_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = LinkedList::node_type;
        using pointer           = value_type*;
        using reference         = value_type&;

    private:
        using node_type = LinkedList::node_type;

    public:
        inline node_iterator(const LinkedList* parent, node_type* first);
        inline node_iterator(const node_iterator& other);
        inline node_iterator& operator=(const node_iterator& other);
        inline node_iterator& operator++();
        inline node_iterator  operator++(int);
        inline pointer        operator->() const;
        inline reference      operator*() const;
        inline bool           operator==(const node_iterator& other) const;
        inline bool           operator!=(const node_iterator& other) const;

    private:
        const LinkedList*  parent_;
        mutable node_type* ptr_;
    };

public:
    using iterator         = iterator_wrapper<value_type>;
    using const_iterator   = iterator_wrapper<const value_type>;
    using reverse_iterator = LinkedList::reverse_iterator;
    using node_iterator    = LinkedList::node_iterator;

    iterator         begin();
    iterator         end();
    const_iterator   cbegin() const;
    const_iterator   cend() const;
    const_iterator   begin() const;
    const_iterator   end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;
    node_iterator    node_begin() const;
    node_iterator    node_end() const;

public:
    LinkedList();
    ~LinkedList();

    inline node_type*&      head();
    inline const node_type* head() const;

    inline node_type*&      tail();
    inline const node_type* tail() const;

    inline size_t size() const;

    node_type* insertToHead(node_type* node);
    node_type* insertToTail(node_type* node);
    node_type* insertBefore(node_type* loc, node_type* node);
    node_type* insertAfter(node_type* loc, node_type* node);

    [[nodiscard]] node_type* clear(); //<! return node chain

    [[nodiscard]] node_type*        remove(node_type* node);
    [[nodiscard]] inline node_type* remove(node_iterator it);

    void swap(node_type* lhs, node_type* rhs);
    void swap(node_iterator lhs, node_iterator rhs);

    template <typename... Args>
    [[nodiscard]] static inline node_type* create(Args&&... args);

    template <typename... Args>
    inline node_type* insertToHead(Args&&... args);
    template <typename... Args>
    inline node_type* insertToTail(Args&&... args);
    template <typename... Args>
    inline node_type* insertBefore(node_type* loc, Args&&... args);
    template <typename... Args>
    inline node_type* insertAfter(node_type* loc, Args&&... args);

    node_iterator find(const value_type& target) const;

    template <typename Predicate> //<! Predicate := bool(const node_type&)
    node_iterator find_if(Predicate pred) const;

private:
    node_type* head_;
    node_type* tail_;
    size_t     size_;
};

} // namespace slime

namespace slime {

namespace detail {

template <typename T>
template <typename... Args>
inline LinkedListNode<T>::LinkedListNode(Args&&... args)
    : parent_{nullptr}
    , prev_{nullptr}
    , next_{nullptr} {
    if constexpr (std::is_trivial_v<value_type>) {
        static_assert(sizeof...(Args) <= 1);
        if constexpr (sizeof...(Args) == 0) {
            value_ = value_type{};
        } else {
            value_ = firstValueOfTArguments(std::forward<Args>(args)...);
        }
    } else {
        value_ = new value_type(std::forward<Args>(args)...);
    }
}

template <typename T>
inline LinkedListNode<T>::~LinkedListNode() {
    if (!std::is_trivial_v<value_type>) {
        auto& ptr = std::get<value_type*>(value_);
        delete ptr;
        ptr = nullptr;
    }
    value_ = std::monostate{};
}

template <typename T>
inline auto LinkedListNode<T>::parent() -> parent_type*& {
    return parent_;
}

template <typename T>
inline auto LinkedListNode<T>::parent() const -> const parent_type* {
    return parent_;
}

template <typename T>
inline auto LinkedListNode<T>::prev() -> node_type*& {
    return prev_;
}

template <typename T>
inline auto LinkedListNode<T>::prev() const -> const node_type* {
    return prev_;
}

template <typename T>
inline auto LinkedListNode<T>::next() -> node_type*& {
    return next_;
}

template <typename T>
inline auto LinkedListNode<T>::next() const -> const node_type* {
    return next_;
}

template <typename T>
inline auto LinkedListNode<T>::value() -> value_type& {
    if constexpr (std::is_trivial_v<value_type>) {
        return std::get<value_type>(value_);
    } else {
        return *std::get<value_type*>(value_);
    }
}

template <typename T>
inline auto LinkedListNode<T>::value() const -> const value_type& {
    return const_cast<LinkedListNode<T>*>(this)->value();
}

template <typename T>
inline auto LinkedListNode<T>::operator*() -> value_type& {
    return value();
}

template <typename T>
inline auto LinkedListNode<T>::operator*() const -> const value_type& {
    return value();
}

template <typename T>
inline auto LinkedListNode<T>::operator->() -> value_type* {
    return std::addressof(value());
}

template <typename T>
inline auto LinkedListNode<T>::operator->() const -> const value_type* {
    return std::addressof(value());
}

template <typename T>
inline void LinkedListNode<T>::removeFromList() {
    assert(parent_ != nullptr);
    auto _ = parent_->remove(this);
}

template <typename T>
inline void LinkedListNode<T>::insertToHead(parent_type* list) {
    assert(list != nullptr);
    if (parent_ == list) {
        moveToHead();
        return;
    } else if (parent_ != nullptr) {
        removeFromList();
    }
    list->insertToHead(this);
}

template <typename T>
inline void LinkedListNode<T>::insertToTail(parent_type* list) {
    assert(list != nullptr);
    if (parent_ == list) {
        moveToTail();
        return;
    } else if (parent_ != nullptr) {
        removeFromList();
    }
    list->insertToTail(this);
}

template <typename T>
inline void LinkedListNode<T>::insertBefore(node_type* node) {
    assert(node != nullptr);
    assert(node->parent_ != nullptr);
    if (node == this) { return; }
    if (parent_ == node->parent_) {
        moveBefore(node);
        return;
    } else if (parent_ != nullptr) {
        removeFromList();
    }
    node->parent_->insertBefore(node, this);
}

template <typename T>
inline void LinkedListNode<T>::insertAfter(node_type* node) {
    assert(node != nullptr);
    assert(node->parent_ != nullptr);
    if (node == this) { return; }
    if (parent_ == node->parent_) {
        moveAfter(node);
        return;
    } else if (parent_ != nullptr) {
        removeFromList();
    }
    node->parent_->insertAfter(node, this);
}

template <typename T>
inline void LinkedListNode<T>::moveBefore(node_type* node) {
    assert(node != nullptr);
    assert(parent_ != nullptr);
    assert(parent_ == node->parent_);
    if (this == node || node->prev_ == this) { return; }
    if (node->next_ == this) {
        moveToPrev();
        return;
    }
    if (parent_->head() == this) {
        parent_->head() = next_;
        next_->prev_    = nullptr;
    } else if (parent_->tail() == this) {
        parent_->tail() = prev_;
        prev_->next_    = nullptr;
    }
    if (next_ != nullptr) { next_->prev_ = prev_; }
    if (prev_ != nullptr) { prev_->next_ = next_; }
    prev_       = node->prev_;
    next_       = node;
    node->prev_ = this;
    if (prev_ != nullptr) {
        prev_->next_ = this;
    } else {
        parent_->head() = this;
    }
}

template <typename T>
inline void LinkedListNode<T>::moveAfter(node_type* node) {
    assert(node != nullptr);
    assert(parent_ != nullptr);
    assert(parent_ == node->parent_);
    if (this == node || node->next_ == this) { return; }
    if (node->prev_ == this) {
        moveToNext();
        return;
    }
    if (parent_->head() == this) {
        parent_->head() = next_;
        next_->prev_    = nullptr;
    } else if (parent_->tail() == this) {
        parent_->tail() = prev_;
        prev_->next_    = nullptr;
    }
    if (next_ != nullptr) { next_->prev_ = prev_; }
    if (prev_ != nullptr) { prev_->next_ = next_; }
    prev_       = node;
    next_       = node->next_;
    node->next_ = this;
    if (next_ != nullptr) {
        next_->prev_ = this;
    } else {
        parent_->tail() = this;
    }
}

template <typename T>
inline void LinkedListNode<T>::insertOrMoveBefore(node_type* node) {
    assert(node != nullptr);
    assert(node->parent_ != nullptr);
    if (parent_ != node->parent_) {
        insertBefore(node);
    } else {
        moveBefore(node);
    }
}

template <typename T>
inline void LinkedListNode<T>::insertOrMoveAfter(node_type* node) {
    assert(node != nullptr);
    assert(node->parent_ != nullptr);
    if (parent_ != node->parent_) {
        insertAfter(node);
    } else {
        moveAfter(node);
    }
}

template <typename T>
inline void LinkedListNode<T>::moveToPrev() {
    assert(parent_ != nullptr);
    if (parent_->head() == this) { return; }
    auto prev    = prev_->prev_;
    prev_->prev_ = this;
    prev_->next_ = next_;
    next_        = prev_;
    prev_        = prev;
    if (prev_ != nullptr) {
        prev_->next_ = this;
    } else {
        parent_->head() = this;
    }
    if (next_->next_ != nullptr) {
        next_->next_->prev_ = next_;
    } else {
        parent_->tail() = next_;
    }
}

template <typename T>
inline void LinkedListNode<T>::moveToNext() {
    assert(parent_ != nullptr);
    if (parent_->tail() == this) { return; }
    auto next    = next_->next_;
    next_->prev_ = prev_;
    next_->next_ = this;
    prev_        = next_;
    next_        = next;
    if (prev_->prev_ != nullptr) {
        prev_->prev_->next_ = prev_;
    } else {
        parent_->head() = prev_;
    }
    if (next_ != nullptr) {
        next_->prev_ = this;
    } else {
        parent_->tail() = this;
    }
}

template <typename T>
inline void LinkedListNode<T>::moveToHead() {
    assert(parent_ != nullptr);
    if (parent_->head() == this) { return; }
    prev_->next_ = next_;
    if (next_ != nullptr) {
        next_->prev_ = prev_;
    } else {
        parent_->tail() = prev_;
    }
    prev_           = nullptr;
    next_           = parent_->head();
    parent_->head() = this;
}

template <typename T>
inline void LinkedListNode<T>::moveToTail() {
    assert(parent_ != nullptr);
    if (parent_->tail() == this) { return; }
    if (prev_ != nullptr) {
        prev_->next_ = next_;
    } else {
        parent_->head() = prev_;
    }
    next_->prev_    = prev_;
    prev_           = parent_->tail();
    next_           = nullptr;
    parent_->tail() = this;
}

} // namespace detail

template <typename T>
template <typename G>
inline LinkedList<T>::iterator_wrapper<G>::iterator_wrapper(
    const LinkedList* parent, node_type* first)
    : parent_{parent}
    , ptr_{first} {
    assert(parent != nullptr);
    assert(first == nullptr || first->parent() == parent);
}

template <typename T>
template <typename G>
inline LinkedList<T>::iterator_wrapper<G>::iterator_wrapper(
    const iterator_wrapper& other)
    : parent_{other.parent_}
    , ptr_{other.ptr_} {}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator=(
    const iterator_wrapper& other) -> iterator_wrapper& {
    parent_ = other.parent_;
    ptr_    = other.ptr_;
    return *this;
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator++()
    -> iterator_wrapper& {
    assert(ptr_ != nullptr);
    ptr_ = ptr_->next();
    return *this;
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator++(int)
    -> iterator_wrapper {
    auto it = *this;
    ++*this;
    return it;
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator->() const -> pointer {
    assert(ptr_ != nullptr);
    return ptr_->operator->();
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator*() const -> reference {
    assert(ptr_ != nullptr);
    return ptr_->operator*();
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator==(
    const iterator_wrapper& other) const -> bool {
    return parent_ == other.parent_ && ptr_ == other.ptr_;
}

template <typename T>
template <typename G>
inline auto LinkedList<T>::iterator_wrapper<G>::operator!=(
    const iterator_wrapper& other) const -> bool {
    return parent_ != other.parent_ || ptr_ != other.ptr_;
}

template <typename T>
inline LinkedList<T>::reverse_iterator::reverse_iterator(
    const LinkedList* parent, node_type* first)
    : parent_{parent}
    , ptr_{first} {
    assert(parent != nullptr);
    assert(first == nullptr || first->parent() == parent);
}

template <typename T>
inline LinkedList<T>::reverse_iterator::reverse_iterator(
    const reverse_iterator& other)
    : parent_{other.parent_}
    , ptr_{other.ptr_} {}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator=(
    const reverse_iterator& other) -> reverse_iterator& {
    parent_ = other.parent_;
    ptr_    = other.ptr_;
    return *this;
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator++() -> reverse_iterator& {
    assert(ptr_ != nullptr);
    ptr_ = ptr_->prev();
    return *this;
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator++(int)
    -> reverse_iterator {
    auto it = *this;
    ++*this;
    return it;
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator->() const -> pointer {
    assert(ptr_ != nullptr);
    return ptr_->operator->();
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator*() const -> reference {
    assert(ptr_ != nullptr);
    return ptr_->operator*();
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator==(
    const reverse_iterator& other) const -> bool {
    return parent_ == other.parent_ && ptr_ == other.ptr_;
}

template <typename T>
inline auto LinkedList<T>::reverse_iterator::operator!=(
    const reverse_iterator& other) const -> bool {
    return parent_ != other.parent_ || ptr_ != other.ptr_;
}

template <typename T>
inline LinkedList<T>::node_iterator::node_iterator(
    const LinkedList* parent, node_type* first)
    : parent_{parent}
    , ptr_{first} {
    assert(parent != nullptr);
    assert(first == nullptr || first->parent() == parent);
}

template <typename T>
inline LinkedList<T>::node_iterator::node_iterator(const node_iterator& other)
    : parent_{other.parent_}
    , ptr_{other.ptr_} {}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator=(const node_iterator& other)
    -> node_iterator& {
    parent_ = other.parent_;
    ptr_    = other.ptr_;
    return *this;
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator++() -> node_iterator& {
    assert(ptr_ != nullptr);
    ptr_ = ptr_->next();
    return *this;
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator++(int) -> node_iterator {
    auto it = *this;
    ++*this;
    return it;
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator->() const -> pointer {
    assert(ptr_ != nullptr);
    return std::addressof(*ptr_);
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator*() const -> reference {
    assert(ptr_ != nullptr);
    return *ptr_;
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator==(
    const node_iterator& other) const -> bool {
    return parent_ == other.parent_ && ptr_ == other.ptr_;
}

template <typename T>
inline auto LinkedList<T>::node_iterator::operator!=(
    const node_iterator& other) const -> bool {
    return parent_ != other.parent_ || ptr_ != other.ptr_;
}

template <typename T>
auto LinkedList<T>::begin() -> iterator {
    return iterator(this, head());
}

template <typename T>
auto LinkedList<T>::end() -> iterator {
    return iterator(this, nullptr);
}

template <typename T>
auto LinkedList<T>::cbegin() const -> const_iterator {
    return const_iterator(this, const_cast<node_type*>(head()));
}

template <typename T>
auto LinkedList<T>::cend() const -> const_iterator {
    return const_iterator(this, nullptr);
}

template <typename T>
auto LinkedList<T>::begin() const -> const_iterator {
    return cbegin();
}

template <typename T>
auto LinkedList<T>::end() const -> const_iterator {
    return cend();
}

template <typename T>
auto LinkedList<T>::rbegin() const -> reverse_iterator {
    return reverse_iterator(this, const_cast<node_type*>(tail()));
}

template <typename T>
auto LinkedList<T>::rend() const -> reverse_iterator {
    return reverse_iterator(this, nullptr);
}

template <typename T>
auto LinkedList<T>::node_begin() const -> node_iterator {
    return node_iterator(this, const_cast<node_type*>(head()));
}

template <typename T>
auto LinkedList<T>::node_end() const -> node_iterator {
    return node_iterator(this, nullptr);
}

template <typename T>
LinkedList<T>::LinkedList()
    : head_{nullptr}
    , tail_{nullptr}
    , size_{0} {}

template <typename T>
LinkedList<T>::~LinkedList() {
    //! FIXME: decide how to release resources
}

template <typename T>
inline auto LinkedList<T>::head() -> node_type*& {
    return head_;
}

template <typename T>
inline auto LinkedList<T>::head() const -> const node_type* {
    return head_;
}

template <typename T>
inline auto LinkedList<T>::tail() -> node_type*& {
    return tail_;
}

template <typename T>
inline auto LinkedList<T>::tail() const -> const node_type* {
    return tail_;
}

template <typename T>
inline size_t LinkedList<T>::size() const {
    return size_;
}

template <typename T>
auto LinkedList<T>::insertToHead(node_type* node) -> node_type* {
    assert(node != nullptr);
    assert(node->parent() == nullptr);
    assert(node->prev() == nullptr);
    if (size_ != 0) {
        node->next()  = head_;
        head_->prev() = node;
        head_         = node;
    } else {
        head_ = tail_ = node;
    }
    node->parent() = this;
    ++size_;
    return node;
}

template <typename T>
auto LinkedList<T>::insertToTail(node_type* node) -> node_type* {
    assert(node != nullptr);
    assert(node->parent() == nullptr);
    assert(node->next() == nullptr);
    if (size_ != 0) {
        node->prev()  = tail_;
        tail_->next() = node;
        tail_         = node;
    } else {
        head_ = tail_ = node;
    }
    node->parent() = this;
    ++size_;
    return node;
}

template <typename T>
auto LinkedList<T>::insertBefore(node_type* loc, node_type* node)
    -> node_type* {
    assert(loc != nullptr);
    assert(loc->parent() == this);
    assert(node != nullptr);
    assert(node->parent() == nullptr);
    node->prev() = loc->prev();
    node->next() = loc;
    loc->prev()  = node;
    if (node->prev() != nullptr) {
        node->prev()->next() = node;
    } else {
        head_ = node;
    }
    node->parent() = this;
    ++size_;
    return node;
}

template <typename T>
auto LinkedList<T>::insertAfter(node_type* loc, node_type* node) -> node_type* {
    assert(loc != nullptr);
    assert(loc->parent() == this);
    assert(node != nullptr);
    assert(node->parent() == nullptr);
    node->prev() = loc;
    node->next() = loc->next();
    loc->next()  = node;
    if (node->next() != nullptr) {
        node->next()->prev() = node;
    } else {
        tail_ = node;
    }
    node->parent() = this;
    ++size_;
    return node;
}

template <typename T>
auto LinkedList<T>::clear() -> node_type* {
    if (size_ > 0) {
        auto first = head_;
        auto node  = first;
        do {
            node->parent() = nullptr;
            node           = node->next();
        } while (node != nullptr);
        head_ = tail_ = nullptr;
        size_         = 0;
        return first;
    }
    return nullptr;
}

template <typename T>
auto LinkedList<T>::remove(node_type* node) -> node_type* {
    assert(node != nullptr);
    assert(node->parent() == this);
    assert(size_ > 0);
    auto& next = node->next();
    auto& prev = node->prev();
    if (prev != nullptr) {
        prev->next() = next;
    } else {
        head_ = next;
    }
    if (next != nullptr) {
        next->prev() = prev;
    } else {
        tail_ = prev;
    }
    prev           = nullptr;
    next           = nullptr;
    node->parent() = nullptr;
    --size_;
    return node;
}

template <typename T>
inline auto LinkedList<T>::remove(node_iterator it) -> node_type* {
    if (it == node_end()) { return nullptr; }
    return remove(std::addressof(*it));
}

template <typename T>
void LinkedList<T>::swap(node_type* lhs, node_type* rhs) {
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    assert(lhs->parent() == this);
    assert(rhs->parent() == this);
    if (lhs == rhs) { return; }
    if (lhs->next() == rhs) {
        lhs->moveToNext();
        return;
    } else if (rhs->next() == lhs) {
        rhs->moveToNext();
        return;
    }
    auto ll = lhs->prev();
    auto lr = lhs->next();
    auto rl = rhs->prev();
    auto rr = rhs->next();
    if (ll != nullptr) {
        rhs->moveAfter(ll);
    } else {
        rhs->moveBefore(lr);
    }
    if (rl != nullptr) {
        lhs->moveAfter(rl);
    } else {
        lhs->moveBefore(rr);
    }
}

template <typename T>
void LinkedList<T>::swap(node_iterator lhs, node_iterator rhs) {
    const auto end = node_end();
    if (lhs != end && rhs != end) {
        swap(std::addressof(*lhs), std::addressof(*rhs));
    } else if (lhs != end) {
        lhs->moveToTail();
    } else if (rhs != end) {
        rhs->moveToTail();
    }
}

template <typename T>
template <typename... Args>
inline auto LinkedList<T>::create(Args&&... args) -> node_type* {
    //! FIXME: decide allocate method
    return new node_type(std::forward<Args>(args)...);
}

template <typename T>
template <typename... Args>
inline auto LinkedList<T>::insertToHead(Args&&... args) -> node_type* {
    return insertToHead(create(std::forward<Args>(args)...));
}

template <typename T>
template <typename... Args>
inline auto LinkedList<T>::insertToTail(Args&&... args) -> node_type* {
    return insertToTail(create(std::forward<Args>(args)...));
}

template <typename T>
template <typename... Args>
inline auto LinkedList<T>::insertBefore(node_type* loc, Args&&... args)
    -> node_type* {
    return insertBefore(loc, create(std::forward<Args>(args)...));
}

template <typename T>
template <typename... Args>
inline auto LinkedList<T>::insertAfter(node_type* loc, Args&&... args)
    -> node_type* {
    return insertAfter(loc, create(std::forward<Args>(args)...));
}

template <typename T>
auto LinkedList<T>::find(const value_type& target) const -> node_iterator {
    auto it  = node_begin();
    auto end = node_end();
    while (it != end) {
        if (it->value() == target) { break; }
        ++it;
    }
    return it;
}

template <typename T>
template <typename Predicate>
auto LinkedList<T>::find_if(Predicate pred) const -> node_iterator {
    auto it  = node_begin();
    auto end = node_end();
    while (it != end) {
        if (pred(*it)) { break; }
        ++it;
    }
    return it;
}

} // namespace slime