#include "cfg.h"
#include "value.h"
#include "instruction.h"

#include <assert.h>

namespace slime::ir {

namespace detail {
static const CFGNode TERMINAL;
} // namespace detail

void CFGNode::reset() {
    auto self = static_cast<BasicBlock*>(this);
    if (!isIncomplete()) {
        if (isBranched()) { branchElse_->unlinkFrom(self); }
        branch_->unlinkFrom(self);
        //! strip branch instruction
        assert(self->instructions().size() > 0);
        auto br = self->instructions().tail()->value();
        assert(br->id() == InstructionID::Br || br->id() == InstructionID::Ret);
        if (br->id() == InstructionID::Br) {
            auto inst = br->asBr();
            inst->op<0>().reset();
            inst->op<1>().reset();
            inst->op<2>().reset();
        } else {
            br->asRet()->operand().reset();
        }
        br->removeFromBlock();
    }
    control_    = nullptr;
    branch_     = nullptr;
    branchElse_ = nullptr;
}

void CFGNode::reset(BasicBlock* branch) {
    reset();
    auto self = static_cast<BasicBlock*>(this);
    if (branch != terminal()) {
        Instruction::createBr(branch)->insertToTail(self);
        branch_ = branch;
        branch_->addIncoming(self);
    } else if (!isTerminal()) {
        auto type = self->parent()->type()->asFunctionType()->returnType();
        assert(type->isVoid());
        Instruction::createRet()->insertToTail(self);
        branch_ = terminal();
    }
}

void CFGNode::reset(
    Value* control, BasicBlock* branch, BasicBlock* branchElse) {
    assert(control->type()->isBoolean());
    reset();
    auto self = static_cast<BasicBlock*>(this);
    Instruction::createBr(control, branch, branchElse)->insertToTail(self);
    control_    = control;
    branch_     = branch;
    branchElse_ = branchElse;
    branch_->addIncoming(self);
    branchElse_->addIncoming(self);
}

bool CFGNode::tryMarkAsTerminal(Value* hint) {
    auto self = static_cast<BasicBlock*>(this);
    if (isIncomplete()) {
        auto type = self->parent()->type()->asFunctionType()->returnType();
        Instruction* ret = nullptr;
        if (type->isVoid()) {
            ret = Instruction::createRet();
        } else if (hint != nullptr && hint->type()->equals(type)) {
            ret = Instruction::createRet(hint);
        }
        if (!ret) { return false; }
        reset();
        ret->insertToTail(self);
        auto ptr = const_cast<CFGNode*>(&detail::TERMINAL);
        branch_  = static_cast<BasicBlock*>(ptr);
        assert(isTerminal());
    }
    return false;
}

BasicBlock* CFGNode::terminal() {
    auto ptr = const_cast<CFGNode*>(&detail::TERMINAL);
    return static_cast<BasicBlock*>(ptr);
}

void CFGNode::addIncoming(BasicBlock* inBlock) {
    if (this != terminal()) {
        if (maybeFrom(inBlock)) { return; }
        inBlocks_.insert(inBlock);
    }
}

void CFGNode::unlinkFrom(BasicBlock* inBlock) {
    //! WANRING: never call it at 'this' context
    if (this != terminal()) {
        assert(maybeFrom(inBlock));
        inBlocks_.erase(inBlock);
    }
}

} // namespace slime::ir
