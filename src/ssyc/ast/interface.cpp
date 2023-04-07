#include "interface.h"
#include "type/base.h"

#include <assert.h>

namespace ssyc::ast {

TreeWalkState::TreeWalkState(const AbstractAstNode *root)
    : walkCursor_{nullptr}
    , pushCursor_{nullptr}
    , pushNumber_{0}
    , currentDepth_{0}
    , freeList_{}
    , walkProc_{} {
    push(root);
    const auto n = complete();
    assert(n == 1);
    walkProc_.push({n, 0});
}

TreeWalkState::~TreeWalkState() {
    while (walkCursor_ != nullptr) {
        auto p = walkCursor_->next;
        delete walkCursor_;
        walkCursor_ = p;
    }

    while (!freeList_.empty()) {
        auto p = freeList_.top();
        delete p;
        freeList_.pop();
    }
}

void TreeWalkState::push(const AbstractAstNode *node) {
    if (node == nullptr) { return; }
    if (pushCursor_ == nullptr) { pushCursor_ = walkCursor_; }

    auto e  = acquireNewNode();
    e->node = node;

    if (pushCursor_ == nullptr) {
        e->prev     = nullptr;
        e->next     = nullptr;
        walkCursor_ = e;
        pushCursor_ = walkCursor_;
    } else {
        e->prev           = pushCursor_;
        e->next           = pushCursor_->next;
        pushCursor_->next = e;
        pushCursor_       = e;
        if (e->next != nullptr) { e->next->prev = e; }
    }

    ++pushNumber_;
}

const AbstractAstNode *TreeWalkState::next() {
    if (walkCursor_ == nullptr) {
        assert(walkProc_.empty());
        return nullptr;
    }

    const auto node = walkCursor_->node;
    walkProc_.top().number--;
    currentDepth_ = walkProc_.top().depth;
    if (walkProc_.top().number == 0) { walkProc_.pop(); }

    pushCursor_ = walkCursor_;
    node->flattenInsert(*this);
    const auto n = complete();

    if (n > 0) { walkProc_.push({n, currentDepth_ + 1}); }

    freeList_.push(walkCursor_);
    walkCursor_ = walkCursor_->next;
    if (walkCursor_ != nullptr) { walkCursor_->prev = nullptr; }

    return node;
}

const int TreeWalkState::depth() const {
    return currentDepth_;
}

int TreeWalkState::complete() const {
    auto num    = pushNumber_;
    pushNumber_ = 0;
    return num;
}

TreeWalkState::node_type *TreeWalkState::acquireNewNode() {
    if (freeList_.empty()) {
        return new node_type{};
    } else {
        auto e = freeList_.top();
        freeList_.pop();
        return e;
    }
}

} // namespace ssyc::ast
