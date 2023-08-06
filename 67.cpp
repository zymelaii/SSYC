#if 0
#include "68.h"

#include <stack>
#include <set>
#include <vector>

namespace slime::pass {

using namespace ir;

void ConstantPropagation::run(Module* target) {
    collectReadOnlyGlobalVariables(target);
    for (auto obj : target->globalObjects()) {
        if (obj->isFunction()) {
            auto fn = obj->asFunction();
            if (fn->basicBlocks().size() > 0) { runOnFunction(fn); }
        }
    }
}

void ConstantPropagation::runOnFunction(Function* target) {
    std::stack<BasicBlock*>  blocks;
    std::stack<Instruction*> stack;
    std::set<BasicBlock*>    visited;

    blocks.push(target->front());
    while (!blocks.empty()) {
        auto block = blocks.top();
        blocks.pop();

        for (auto inst : block->instructions()) { stack.push(inst); }

        while (!stack.empty()) {
            auto inst = stack.top();
            stack.pop();
            if (!inst->parent()) { continue; }

            bool   folded = false;
            Value* value  = nullptr;

            switch (inst->id()) {
                case InstructionID::Load: {
                } break;
                case InstructionID::Br: {
                } break;
                case InstructionID::Add:
                case InstructionID::Sub:
                case InstructionID::Mul:
                case InstructionID::UDiv:
                case InstructionID::SDiv:
                case InstructionID::URem:
                case InstructionID::SRem:
                case InstructionID::Shl:
                case InstructionID::LShr:
                case InstructionID::AShr:
                case InstructionID::And:
                case InstructionID::Or:
                case InstructionID::Xor: {
                } break;
                case InstructionID::FNeg:
                case InstructionID::FAdd:
                case InstructionID::FSub:
                case InstructionID::FMul:
                case InstructionID::FDiv:
                case InstructionID::FRem: {
                    tryFoldFloatInst(inst);
                } break;
                case InstructionID::FPToUI:
                case InstructionID::FPToSI:
                case InstructionID::UIToFP:
                case InstructionID::SIToFP:
                case InstructionID::ZExt: {
                } break;
                case InstructionID::ICmp:
                case InstructionID::FCmp: {
                } break;
                default: {
                } break;
            }
        }

        visited.insert(block);
        if (block->isBranched()) {
            if (!visited.count(block->branch())) {
                blocks.push(block->branch());
            }
            if (!visited.count(block->branch())) {
                blocks.push(block->branchElse());
            }
        } else if (block->isLinear() && !block->isTerminal()) {
            if (!visited.count(block->branch())) {
                blocks.push(block->branch());
            }
        }
    }
}

void ConstantPropagation::collectReadOnlyGlobalVariables(Module* target) {
    for (auto obj : target->globalObjects()) {
        if (obj->isFunction()) { continue; }
        auto var = obj->asGlobalVariable();
        if (var->isConst()) {
            readOnlyAddrSet_.insert(var);
            continue;
        }
        bool               maybeWrite = false;
        std::stack<Value*> stack;
        std::set<Value*>   source;
        for (auto use : var->uses()) { stack.push(use->owner()); }
        source.insert(var);
        //! FIXME: side-effect of values across procedure is unevaluatable
        while (!stack.empty()) {
            auto value = stack.top();
            stack.pop();
            if (!value->isInstruction()) { continue; }
            auto inst = value->asInstruction();
            if (inst->id() == InstructionID::Store) {
                auto store = inst->asStore();
                if (source.count(store->lhs())) {
                    maybeWrite = true;
                    break;
                } else if (source.count(store->rhs())) {
                    auto addr = store->lhs();
                    if (!addr->isFunction() && addr->isGlobal()) {
                        maybeWrite = true;
                        break;
                    }
                    for (auto use : addr->uses()) {
                        auto user = use->owner();
                        if (user->isInstruction()
                            && user->asInstruction()->id()
                                   == InstructionID::Load) {
                            auto load = user->asInstruction()->asLoad();
                            for (auto use : load->uses()) {
                                stack.push(use->owner());
                            }
                            source.insert(load);
                        }
                    }
                }
            } else if (inst->id() == InstructionID::GetElementPtr) {
                auto gep = inst->asGetElementPtr();
                if (source.count(gep->op()[0])) {
                    for (auto use : gep->uses()) { stack.push(use->owner()); }
                    source.insert(value);
                }
            } else if (inst->id() == InstructionID::Ret) {
                auto ret = inst->asRet();
                assert(!ret->type()->isVoid());
                assert(source.count(ret->operand()));
                maybeWrite = true;
                break;
            }
        }
        if (!maybeWrite) {
            for (auto value : source) {
                if (!value->isFunction() && value->isGlobal()) {
                    readOnlyAddrSet_.insert(value);
                } else if (
                    value->isInstruction()
                    && value->asInstruction()->id()
                           == InstructionID::GetElementPtr) {
                    readOnlyAddrSet_.insert(value);
                }
            }
        }
    }
}

Value* ConstantPropagation::tryFoldLoadInst(LoadInst* inst) {
    if (!readOnlyAddrSet_.count(inst->operand())) { return nullptr; }
    auto addr = inst->operand();
    if (!addr->isFunction() && addr->isGlobal()) {
        return const_cast<ConstantData*>(addr->asGlobalVariable()->data());
    }

    std::vector<int> indices;
    auto             gep = addr->asInstruction()->asGetElementPtr();

    while (true) {

    }

}

Value* ConstantPropagation::tryFoldIntInst(Instruction* inst) {}

Value* ConstantPropagation::tryFoldFloatInst(Instruction* inst) {}

Value* ConstantPropagation::tryFoldCmpInst(Instruction* inst) {}

Value* ConstantPropagation::tryFoldCastInst(Instruction* inst) {}

Value* ConstantPropagation::tryFoldBranchInst(BrInst* inst) {}

} // namespace slime::pass
#endif