#pragma once

#include "69.h"

#include "37.h"
#include <tuple>
#include <stddef.h>
#include <stdint.h>

namespace slime::pass {

//! Common Subexpression Elimiantion
class CSEPass : public UniversalIRPass {
private:
    using key_type =
        std::tuple<ir::InstructionID, intptr_t, intptr_t, intptr_t>;

public:
    void runOnFunction(ir::Function* target) override;

protected:
    std::pair<bool, key_type> encode(ir::Instruction* inst);

private:
    std::map<key_type, ir::Value*>   numberingTable_;
    std::set<ir::GetElementPtrInst*> constantPtrInst_;
};

} // namespace slime::pass