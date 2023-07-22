#include "41.h"
#include "37.h"

namespace slime::ir {

Instruction *Value::asInstruction() {
    return reinterpret_cast<Instruction *>(patch());
}

GlobalObject *Value::asGlobalObject() {
    return static_cast<GlobalObject *>(this);
}

ConstantData *Value::asConstantData() {
    return static_cast<ConstantData *>(this);
}

GlobalVariable *Value::asGlobalVariable() {
    return static_cast<GlobalVariable *>(this);
}

Function *Value::asFunction() {
    return static_cast<Function *>(this);
}

Instruction *Value::tryIntoInstruction() {
    return isInstruction() ? static_cast<Instruction *>(patch()) : nullptr;
}

GlobalObject *Value::tryIntoGlobalObject() {
    return isGlobal() ? static_cast<GlobalObject *>(this) : nullptr;
}

ConstantData *Value::tryIntoConstantData() {
    return isReadOnly() && !isFunction() ? static_cast<ConstantData *>(this)
                                         : nullptr;
}

GlobalVariable *Value::tryIntoGlobalVariable() {
    return isGlobal() && !isFunction() ? static_cast<GlobalVariable *>(this)
                                       : nullptr;
}

Function *Value::tryIntoFunction() {
    return isFunction() ? static_cast<Function *>(this) : nullptr;
}

void BasicBlock::insertOrMoveToHead() {
    if (!isInserted()) {
        node_ = parent_->insertToHead(this);
    } else {
        node_->insertToHead(*parent_);
    }
}

void BasicBlock::insertOrMoveToTail() {
    if (!isInserted()) {
        node_ = parent_->insertToTail(this);
    } else {
        node_->insertToTail(*parent_);
    }
}

bool BasicBlock::insertOrMoveBefore(BasicBlock *block) {
    if (block->parent_ != parent_ || !block->isInserted()) { return false; }
    if (!isInserted()) { node_ = parent_->insertToHead(this); }
    node_->insertBefore(block->node_);
    return true;
}

bool BasicBlock::insertOrMoveAfter(BasicBlock *block) {
    if (block->parent_ != parent_ || !block->isInserted()) { return false; }
    if (!isInserted()) { node_ = parent_->insertToTail(this); }
    node_->insertAfter(block->node_);
    return true;
}

bool BasicBlock::remove() {
    if (totalInBlocks() > 0) { return false; }
    reset();
    node_->removeFromList();
    return true;
}

} // namespace slime::ir