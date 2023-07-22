#pragma once

#include "Value.h"
#include "CFG.h"

#include <slime/experimental/utils/LinkedList.h>
#include <stddef.h>

namespace slime::experimental::ir {

class Function;
class BasicBlock;
class Instruction;

using InstructionList = LinkedList<Instruction *>;
using BasicBlockList  = LinkedList<BasicBlock *>;

class BasicBlock final
    : public Value
    , public CFGNode {
public:
    inline BasicBlock(Function *parent);

    inline Function *parent() const;

    inline bool isInserted() const;

    inline InstructionList       &instructions();
    inline const InstructionList &instructions() const;

    void insertOrMoveToHead();
    void insertOrMoveToTail();

    bool insertOrMoveAfter(BasicBlock *block);
    bool insertOrMoveBefore(BasicBlock *block);

    bool remove();

private:
    Function *const            parent_;
    BasicBlockList::node_type *self_;
    InstructionList            instList_;
};

class Parameter final : public Value {
    friend class Function;

protected:
    inline Parameter();
    inline Parameter(Function *parent, size_t index);
    inline void attachTo(Function *parent, size_t index);

public:
    inline Function *parent() const;
    inline size_t    index() const;

private:
    Function *parent_;
    size_t    index_;
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline BasicBlock::BasicBlock(Function *parent)
    : Value(Type::getVoidTy(), static_cast<uint32_t>(ValueTag::Label))
    , parent_{parent}
    , self_{nullptr}
    , instList_() {}

inline Function *BasicBlock::parent() const {
    return parent_;
}

inline bool BasicBlock::isInserted() const {
    return self_ != nullptr;
}

inline InstructionList &BasicBlock::instructions() {
    return instList_;
}

inline const InstructionList &BasicBlock::instructions() const {
    return instList_;
}

inline Parameter::Parameter()
    : Value(Type::getVoidTy(), static_cast<uint32_t>(ValueTag::Parameter))
    , parent_{nullptr}
    , index_{0} {}

inline Parameter::Parameter(Function *parent, size_t index)
    : Parameter() {
    attachTo(parent, index);
}

inline void Parameter::attachTo(Function *parent, size_t index) {
    parent_ = parent;
    index_  = index;
}

inline Function *Parameter::parent() const {
    return parent_;
}

inline size_t Parameter::index() const {
    return index_;
}

} // namespace slime::experimental::ir
