#include "cfg.h"
#include "value.h"

#include <assert.h>

namespace slime::ir {

void CFGNode::resetBranch(BasicBlock* block) {
    assert(block != nullptr);
    //! NOTE: CFGNode derives the BasicBlock
    auto self = static_cast<BasicBlock*>(this);

    if (jmpIf_ != nullptr) {
        if (control_ != nullptr) {
            control_ = nullptr;
            jmpElse_->unlinkFrom(self);
            jmpElse_ = nullptr;
        } else if (jmpIf_ != block) {
            jmpIf_->unlinkFrom(self);
        }
    }
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
    if (jmpIf_ != nullptr) {
        if (!control_) {
            jmpIf_->unlinkFrom(self);
            jmpIf_ = nullptr;
        } else {
            jmpIf_->unlinkFrom(self);
            jmpElse_->unlinkFrom(self);
        }
    }
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