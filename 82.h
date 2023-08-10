#pragma once

#include "85.h"

#include "53.h"
#include <vector>
#include <set>
#include <map>

namespace slime::pass {

class SCCPPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

protected:
    void runOnInstruction(ir::Instruction *inst);

private:
    struct CFGFlow {
        ir::BasicBlock *from;
        ir::BasicBlock *to;
    };

    enum Status {
        BOT   = 0b00,
        CONST = 0b01,
        TOP   = 0b10,
    };

    std::vector<ir::Instruction *> ssaWorkList_;
    std::vector<CFGFlow>           cfgWorkList_;
    std::map<ir::Value *, Status>  valueStatus_;
    std::set<CFGFlow>              flags_;
};

} // namespace slime::pass
