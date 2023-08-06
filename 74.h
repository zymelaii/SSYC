#pragma once

#include "85.h"

#include <functional>

namespace slime::pass {

class FunctionInliningPass : public UniversalIRPass {
public:
    void run(ir::Module *module) override;
    void runOnFunction(ir::Function *target) override;

protected:
    bool tryExecuteFunctionInlining(ir::BasicBlock *block, ir::CallInst *iter);

    ir::BasicBlock *insertNewBlockAfter(ir::Instruction *inst);

    inline bool isRecursiveFunction(ir::Function *fn) {
        return depsMap[fn].count(fn) > 0;
    }

    void testAndSetRecursionFlag(ir::Function *function);

    ir::Instruction *cloneInstruction(
        ir::Instruction                        *instruction,
        std::function<ir::Value *(ir::Value *)> mappingStrategy);

private:
    //! function dependency map
    std::map<ir::Function *, std::set<ir::Function *>> depsMap;
};

} // namespace slime::pass