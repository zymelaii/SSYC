#include "cfg.h"
#include "value.h"

#include <assert.h>

namespace slime::ir {

void CFGNode::resetBranch() {
    //! NOTE: CFGNode derives the BasicBlock
    auto self = static_cast<BasicBlock*>(this);
    if (control_ != nullptr) {
        control_ = nullptr;
        jmpIf_->unlinkFrom(self);
        jmpElse_->unlinkFrom(self);
    } else if (jmpIf_ != nullptr) {
        jmpIf_->unlinkFrom(self);
    }
    control_ = nullptr;
    jmpIf_   = nullptr;
    jmpElse_ = nullptr;
}

void CFGNode::resetBranch(BasicBlock* block) {
    assert(block != nullptr);
    //! NOTE: CFGNode derives the BasicBlock
    auto self = static_cast<BasicBlock*>(this);
    resetBranch();
    jmpIf_ = block;
    block->addIncoming(self);
}

void CFGNode::resetBranch(
    Value* control, BasicBlock* branchIf, BasicBlock* branchElse) {
    assert(control != nullptr);
    assert(branchIf != nullptr);
    assert(branchElse != nullptr);
    //! NOTE: CFGNode derives the BasicBlock
    auto self = static_cast<BasicBlock*>(this);
    resetBranch();
    control_ = control;
    jmpIf_   = branchIf;
    jmpElse_ = branchElse;
    branchIf->addIncoming(self);
    branchElse->addIncoming(self);
}

void CFGNode::addIncoming(BasicBlock* inBlock) {
    if (maybeFrom(inBlock)) { return; }
    inBlocks_.insert(inBlock);
}

void CFGNode::unlinkFrom(BasicBlock* inBlock) {
    //! WANRING: never call it at 'this' context
    assert(maybeFrom(inBlock));
    inBlocks_.erase(inBlock);
}

} // namespace slime::ir
