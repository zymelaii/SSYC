#include "PhiElimination.h"

#include <slime/ir/value.h>
#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void PhiEliminationPass::runOnFunction(Function *target) {
    for (auto block : target->basicBlocks()) {
        auto &instructions = block->instructions();
        auto  it           = instructions.begin();
        while (it != instructions.end()) {
            auto inst = *it++;
            if (inst->id() != InstructionID::Phi) { continue; }
            auto phi = inst->asPhi();
            assert(phi->totalUse() > 0);
            auto address = Instruction::createAlloca(phi->type());
            address->insertToHead(target->front());
            for (int i = 0; i < phi->totalUse(); i += 2) {
                auto user   = phi->op()[i];
                auto source = phi->op()[i + 1];
                Instruction::createStore(address->unwrap(), user)
                    ->insertAfter(user->asInstruction());
            }
            auto load = Instruction::createLoad(address->unwrap());
            load->insertAfter(phi);
            auto value = load->unwrap();
            for (auto use : value->uses()) {}
            std::vector<Use *> v(phi->uses().begin(), phi->uses().end());
            for (auto use : v) { use->reset(value); }
            phi->removeFromBlock();
        }
    }
}

} // namespace slime::pass
