#include "interface.h"
#include "type/base.h"

#include <assert.h>

namespace ssyc::ast {

TreeWalkState::TreeWalkState(const AbstractAstNode *root)
    : walkCursor_{nullptr}
    , pushCursor_{nullptr}
    , pushNumber_{0}
    , freeList_{}
    , walkProc_{} {
    push(root);
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
        e->next->prev     = e;
        pushCursor_->next = e;
        pushCursor_       = e;
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
    const auto depth = walkProc_.top().depth;
    if (walkProc_.top().number == 0) { walkProc_.pop(); }

    pushCursor_ = walkCursor_;
    node->flattenInsert(*this);
    const auto n = complete();

    if (n > 0) { walkProc_.push({n, depth + 1}); }

    freeList_.push(walkCursor_);
    walkCursor_       = walkCursor_->next;
    walkCursor_->prev = nullptr;

    return node;
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
