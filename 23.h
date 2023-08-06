#pragma once

#include <set>

namespace slime::experimental::ir {

class BasicBlock;
class Value;

//! NOTE: CFGNode derives the BasicBlock
class CFGNode {
public:
    inline bool isOrphan() const;
    inline bool isIncomplete() const;
    inline bool isTerminal() const;
    inline bool isLinear() const;
    inline bool isBranched() const;

    inline Value*      control() const;
    inline BasicBlock* branch() const;
    inline BasicBlock* branchElse() const;

    void reset();
    void reset(BasicBlock* branch);
    void reset(Value* control, BasicBlock* branch, BasicBlock* branchElse);

    bool tryMarkAsTerminal(Value* hint = nullptr);
    void syncFlowWithInstUnsafe();

    inline size_t                       totalInBlocks() const;
    inline const std::set<BasicBlock*>& inBlocks() const;

    inline bool maybeFrom(BasicBlock* block);

protected:
    static BasicBlock* terminal();

    void checkAndSolveOutdated();

    void addIncoming(BasicBlock* inBlock);
    void unlinkFrom(BasicBlock* inBlock);

private:
    Value*                control_    = nullptr;
    BasicBlock*           branch_     = nullptr;
    BasicBlock*           branchElse_ = nullptr;
    std::set<BasicBlock*> inBlocks_;
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline bool CFGNode::isOrphan() const {
    return branch_ == nullptr && inBlocks_.empty();
}

inline bool CFGNode::isIncomplete() const {
    return branch_ == nullptr;
}

inline bool CFGNode::isTerminal() const {
    return branch_ == terminal();
}

inline bool CFGNode::isLinear() const {
    return branch_ != nullptr && branchElse_ == nullptr;
}

inline bool CFGNode::isBranched() const {
    return branchElse_ != nullptr;
}

inline Value* CFGNode::control() const {
    return control_;
}

inline BasicBlock* CFGNode::branch() const {
    return branch_;
}

inline BasicBlock* CFGNode::branchElse() const {
    return branchElse_;
}

inline const std::set<BasicBlock*>& CFGNode::inBlocks() const {
    return inBlocks_;
}

inline size_t CFGNode::totalInBlocks() const {
    return inBlocks_.size();
}

inline bool CFGNode::maybeFrom(BasicBlock* block) {
    return inBlocks_.count(block) == 1;
}

} // namespace slime::experimental::ir