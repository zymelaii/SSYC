#pragma once

#include "87.h"

#include "47.h"
#include <map>

namespace slime::pass {

class ConstantPropagation : public UniversalIRPass {
public:
    void run(ir::Module* target) override;
    void runOnFunction(ir::Function* target) override;

protected:
    void collectReadOnlyGlobalVariables(ir::Module* target);

    ir::Value* tryFoldLoadInst(ir::LoadInst *inst);
    ir::Value* tryFoldIntInst(ir::Instruction* inst);
    ir::Value* tryFoldFloatInst(ir::Instruction* inst);
    ir::Value* tryFoldCmpInst(ir::Instruction* inst);
    ir::Value* tryFoldCastInst(ir::Instruction* inst);
    bool       tryFoldBranchInst(ir::BrInst* inst);

private:
    std::set<ir::Value*> readOnlyAddrSet_;
};

} // namespace slime::pass