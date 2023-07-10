#pragma once

#include <set>
#include <assert.h>

namespace slime::ir {

class BasicBlock;
class Value;

class CFGNode {
public:
    void resetBranch();
    void resetBranch(BasicBlock* block);
    void resetBranch(
        Value* control, BasicBlock* branchIf, BasicBlock* branchElse);

    inline Value* control() {
        return control_;
    }

    inline BasicBlock* jmpIf() {
        return jmpIf_;
    }

    inline BasicBlock* jmpElse() {
        return jmpElse_;
    }

    inline bool isLinear() const {
        return jmpIf_ == nullptr || jmpElse_ == nullptr;
    }

    inline bool hasBranchOut() const {
        return jmpIf_ != nullptr;
    }

    inline size_t totalInBlocks() const {
        return inBlocks_.size();
    }

    inline bool maybeFrom(BasicBlock* block) {
        return inBlocks_.count(block) == 1;
    }

protected:
    void addIncoming(BasicBlock* inBlock);
    void unlinkFrom(BasicBlock* inBlock);

private:
    Value*                control_ = nullptr;
    BasicBlock*           jmpIf_   = nullptr;
    BasicBlock*           jmpElse_ = nullptr;
    std::set<BasicBlock*> inBlocks_;
};

} // namespace slime::ir
