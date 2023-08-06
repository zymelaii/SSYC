#include "36.h"

namespace slime::experimental::ir {

void BasicBlock::moveToHead() {
    //! FIXME: decide wether motion has side effect
    self_->moveToHead();
}

void BasicBlock::moveToTail() {
    //! FIXME: decide wether motion has side effect
    self_->moveToTail();
}

bool BasicBlock::insertOrMoveAfter(BasicBlock *block) {
    //! FIXME: decide wether motion has side effect
    self_->insertOrMoveAfter(block->self_);
    return true;
}

bool BasicBlock::insertOrMoveBefore(BasicBlock *block) {
    //! FIXME: decide wether motion has side effect
    self_->insertOrMoveBefore(block->self_);
    return true;
}

bool BasicBlock::remove() {
    if (totalInBlocks() > 0) { return false; }
    self_->removeFromList();
    return true;
}

} // namespace slime::experimental::ir