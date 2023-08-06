#include "47.h"

#include <array>

namespace slime::ir {

std::string_view getPredicateName(ComparePredicationType predicate) {
    constexpr size_t N = static_cast<size_t>(ComparePredicationType::TRUE) + 1;
    static constexpr auto LOOKUP = std::array<std::string_view, N>{
        "eq",  "ne",  "ugt",   "uge", "ult", "ule",  "sgt", "sge",
        "slt", "sle", "false", "oeq", "ogt", "oge",  "olt", "ole",
        "one", "ord", "ueq",   "une", "uno", "true",
    };
    return LOOKUP[static_cast<int>(predicate)];
}

std::string_view getInstructionName(InstructionID id) {
    static constexpr auto INST_LOOKUP = std::array<
        std::string_view,
        static_cast<size_t>(InstructionID::LAST_INST) + 1>{
        "alloca", "load", "store", "ret",  "br",     "getelementptr", "add",
        "sub",    "mul",  "udiv",  "sdiv", "urem",   "srem",          "fneg",
        "fadd",   "fsub", "fmul",  "fdiv", "frem",   "shl",           "lshr",
        "ashr",   "and",  "or",    "xor",  "fptoui", "fptosi",        "uitofp",
        "sitofp", "zext", "icmp",  "fcmp", "phi",    "call",
    };
    return INST_LOOKUP[static_cast<int>(id)];
}

bool Instruction::insertToHead(BasicBlock* block) {
    //! FIXME: instruction is not always movable
    assert(block != nullptr);
    if (parent_ != nullptr) { removeFromBlock(false); }
    parent_ = block;
    self_   = parent_->insertToHead(this);
    return true;
}

bool Instruction::insertToTail(BasicBlock* block) {
    //! FIXME: instruction is not always movable
    assert(block != nullptr);
    if (parent_ != nullptr) { removeFromBlock(false); }
    parent_ = block;
    self_   = parent_->insertToTail(this);
    return true;
}

bool Instruction::insertBefore(Instruction* inst) {
    //! FIXME: instruction is not always movable
    assert(inst != nullptr);
    if (parent_ != nullptr) { removeFromBlock(false); }
    if (!inst->parent_) { return false; }
    parent_ = inst->parent_;
    self_   = parent_->insertToTail(this);
    self_->insertBefore(inst->self_);
    return true;
}

bool Instruction::insertAfter(Instruction* inst) {
    //! FIXME: instruction is not always movable
    assert(inst != nullptr);
    if (parent_ != nullptr) { removeFromBlock(false); }
    if (!inst->parent_) { return false; }
    parent_ = inst->parent_;
    self_   = parent_->insertToTail(this);
    self_->insertAfter(inst->self_);
    return true;
}

bool Instruction::moveToPrev() {
    //! FIXME: instruction is not always movable
    if (!parent_ || !self_) { return false; }
    if (self_ == parent_->head()) { return false; }
    self_->moveToPrev();
    return true;
}

bool Instruction::moveToNext() {
    //! FIXME: instruction is not always movable
    if (!parent_ || !self_) { return false; }
    if (self_ == parent_->tail()) { return false; }
    self_->moveToNext();
    return true;
}

bool Instruction::removeFromBlock(bool reset) {
    if (reset && unwrap()->uses().size() > 0) { return false; }
    if (!parent_ || !self_) { return false; }
    if (reset) {
        for (int i = 0; i < totalOperands(); ++i) { useAt(i).reset(); }
    }
    self_->removeFromList();
    self_   = nullptr;
    parent_ = nullptr;
    return true;
}

bool PhiInst::addIncomingValue(Value* value, BasicBlock* block) {
    if (!value->type()->equals(type())) { return false; }
    const int n = totalUse();
    for (int i = 1; i < n; i += 2) {
        if (op()[i] == block) { return false; }
    }
    resize(n + 2);
    op()[n]     = value;
    op()[n + 1] = block;
    return true;
}

bool PhiInst::removeValueFrom(BasicBlock* block) {
    const int n = totalUse();
    int       i = 1;
    while (i < n) {
        if (op()[i] == block) { break; }
        i += 2;
    }
    if (i < n) {
        op()[i - 1].reset();
        op()[i].reset();
        op()[i - 1] = op()[n - 2];
        op()[i]     = op()[n - 1];
    }
    return true;
}

} // namespace slime::ir