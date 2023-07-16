#include "CSE.h"

#include <vector>

namespace slime::pass {

using namespace ir;

inline intptr_t ptr2int(void* ptr) {
    return reinterpret_cast<intptr_t>(ptr);
}

void CSEPass::runOnFunction(ir::Function* target) {
    for (auto block : target->basicBlocks()) {
        auto instIter = block->instructions().begin();
        while (instIter != block->instructions().end()) {
            auto inst           = *instIter++;
            auto [succeed, key] = encode(inst);
            if (succeed) {
                if (numberingTable_.count(key)) {
                    auto value = numberingTable_.at(key);
                    assert(value != nullptr);
                    auto&             uses = inst->unwrap()->uses();
                    std::vector<Use*> useList(uses.begin(), uses.end());
                    for (auto use : useList) { use->reset(value); }
                    auto ok = inst->removeFromBlock();
                    assert(ok);
                } else {
                    numberingTable_.insert_or_assign(key, inst->unwrap());
                }
                continue;
            }
            if (inst->id() == InstructionID::Store) {
                auto                  address = inst->asStore()->lhs();
                std::vector<key_type> dirtyValues;
                for (auto& [k, v] : numberingTable_) {
                    assert(v->isInstruction());
                    auto inst = v->asInstruction();
                    for (int i = 0; i < inst->totalOperands(); ++i) {
                        auto& use = inst->useAt(i);
                        if (use == nullptr) {
                            //! FIXME: nullptr check due to unreasonable design
                            //! of totalOperands
                            break;
                        }
                        if (use->isInstruction()) {
                            auto inst = use->asInstruction();
                            if (inst->id() == InstructionID::Load
                                && inst->asLoad()->operand() == address) {
                                dirtyValues.push_back(k);
                            }
                        }
                    }
                }
                for (auto key : dirtyValues) { numberingTable_.erase(key); }
            }
        }
    }
}

std::pair<bool, CSEPass::key_type> CSEPass::encode(ir::Instruction* inst) {
    //! only encode binary or unary instruction;
    key_type ret{};
    bool     ok = false;
    if (auto user = inst->tryIntoUser<1>();
        user && inst->id() != ir::InstructionID::Load
        && inst->id() != ir::InstructionID::Ret) {
        ret = key_type{inst->id(), ptr2int(user->operand()), 0, 0};
        ok  = true;
    } else if (auto user = inst->tryIntoUser<2>();
               user && inst->id() != ir::InstructionID::Store) {
        assert(ptr2int(user->rhs()) != 0);
        auto lhs = ptr2int(user->lhs());
        auto rhs = ptr2int(user->rhs());
        switch (inst->id()) {
            case ir::InstructionID::Sub:
            case ir::InstructionID::UDiv:
            case ir::InstructionID::SDiv:
            case ir::InstructionID::URem:
            case ir::InstructionID::SRem:
            case ir::InstructionID::FSub:
            case ir::InstructionID::FDiv:
            case ir::InstructionID::FRem:
            case ir::InstructionID::Shl:
            case ir::InstructionID::LShr:
            case ir::InstructionID::AShr: {
            } break;
            default: {
                //! let swappable inst with lhs <= rhs
                if (lhs > rhs) { std::swap(lhs, rhs); }
            } break;
        }
        ret = key_type{inst->id(), lhs, rhs, 0};
        ok  = true;
    } else if (inst->id() == InstructionID::GetElementPtr) {
        auto gep     = inst->asGetElementPtr();
        auto address = gep->op<0>();
        auto index1  = gep->op<1>();
        auto index2  = gep->op<2>();
        if ((address->isInstruction()
             && address->asInstruction()->id() == InstructionID::Alloca)
            || address->isGlobal()) {
            if (index1->isImmediate() && index2->isImmediate()) {
                ret = key_type{
                    inst->id(),
                    ptr2int(address),
                    ptr2int(index1),
                    ptr2int(index2)};
                ok = true;
            }
        }
    }
    return std::pair{ok, ret};
}
} // namespace slime::pass
