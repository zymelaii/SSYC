#include "34.h"
#include "43.h"
#include "37.h"

#include <assert.h>

namespace slime::ir {

namespace detail {
static const CFGNode TERMINAL;
} // namespace detail

void CFGNode::reset() {
    checkAndSolveOutdated();
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
        return true;
    }
    return false;
}

void CFGNode::syncFlowWithInstUnsafe() {
    auto self = static_cast<BasicBlock*>(this);
    if (self->size() == 0) { return; }
    auto inst = self->instructions().tail()->value();
    if (inst->id() == InstructionID::Br) {
        auto br = inst->asBr();
        if (branch_ != nullptr) { branch_->unlinkFrom(self); }
        if (branchElse_ != nullptr) { branchElse_->unlinkFrom(self); }
        if (br->op<0>()->isLabel()) {
            control_    = nullptr;
            branch_     = static_cast<BasicBlock*>(br->op<0>().value());
            branchElse_ = nullptr;
            branch_->addIncoming(self);
        } else {
            control_    = br->op<0>();
            branch_     = static_cast<BasicBlock*>(br->op<1>().value());
            branchElse_ = static_cast<BasicBlock*>(br->op<2>().value());
            branch_->addIncoming(self);
            branchElse_->addIncoming(self);
        }
    } else if (inst->id() == InstructionID::Ret) {
        auto ret = inst->asRet();
        if (branch_ != nullptr) { branch_->unlinkFrom(self); }
        if (branchElse_ != nullptr) { branchElse_->unlinkFrom(self); }
        control_    = nullptr;
        branch_     = terminal();
        branchElse_ = nullptr;
    }
}

BasicBlock* CFGNode::terminal() {
    auto ptr = const_cast<CFGNode*>(&detail::TERMINAL);
    return static_cast<BasicBlock*>(ptr);
}

void CFGNode::checkAndSolveOutdated() {
    if (isIncomplete()) { return; }
    auto self        = static_cast<BasicBlock*>(this);
    bool shouldReset = self->size() == 0;
    if (!shouldReset) {
        auto brInst = self->instructions().tail()->value();
        if (brInst->id() == InstructionID::Br) {
            auto br     = brInst->asBr();
            shouldReset = !(br->op<0>()->isLabel() ? isLinear() : isBranched());
        } else if (brInst->id() == InstructionID::Ret) {
            shouldReset = !isTerminal();
        } else {
            shouldReset = true;
        }
    }
    if (shouldReset) {
        control_ = nullptr;
        if (branchElse_ != nullptr) { branchElse_->unlinkFrom(self); }
        if (isTerminal()) {
            branch_ = nullptr;
        } else {
            branch_->unlinkFrom(self);
        }
        control_    = nullptr;
        branch_     = nullptr;
        branchElse_ = nullptr;
    }
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