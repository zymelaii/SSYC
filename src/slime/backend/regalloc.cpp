#include "regalloc.h"
#include "slime/ir/instruction.def"

#include <cstdint>
#include <slime/ir/value.h>
#include <slime/ir/instruction.h>

namespace slime::backend {

bool Allocator::isVariable(Value *val) {
    return !(val->isConstant() || val->isImmediate() || val->isLabel());
}

void Allocator::initVarInterval(Function *func) {
    for (auto block : func->basicBlocks()) {
        auto valVarTable = new ValVarTable;
        blockVarTable->insert({block, valVarTable});
        total_inst += block->instructions().size();
        for (auto inst : block->instructions()) {
            for (int i = 0; i < inst->totalOperands(); i++) {
                if (isVariable(inst->useAt(i))) {
                    Value *val = inst->useAt(i).value();
                    valVarTable->insert({val, Variable::create(val)});
                }
            }
            if (!inst->unwrap()->type()->isVoid())
                //! NOTE:
                //! map.insert在插入键值对时如果已经存在将插入的键值时应该是会插入失败的
                valVarTable->insert(
                    {inst->unwrap(), Variable::create(inst->unwrap())});
        }
    }
}

void Allocator::computeInterval(Function *func) {
    initVarInterval(func);
    auto bit  = func->basicBlocks().rbegin();
    auto bend = func->basicBlocks().rend();
    cur_inst  = total_inst;
    while (bit != bend) {
        //! TODO: 获取LIVE IN、LIVE OUT
        auto it          = (*bit)->instructions().rbegin();
        auto end         = (*bit)->instructions().rend();
        auto valVarTable = blockVarTable->find(*(bit))->second;
        while (it != end) {
            Instruction *inst = *it;
            //! update live-in
            for (int i = 0; i < inst->totalOperands(); i++) {
                if (isVariable(inst->useAt(i))) {
                    auto interval =
                        valVarTable->find(inst->useAt(i))->second->livIntvl;
                    interval->end = std::max(interval->end, cur_inst);
                    if (inst->useAt(i)->isGlobal()
                        && interval->start == UINT64_MAX) {
                        interval->start = interval->end;
                    }
                }
            }
            //! update live-out
            if (!inst->unwrap()->type()->isVoid()) {
                auto interval =
                    valVarTable->find(inst->unwrap())->second->livIntvl;
                assert(
                    interval->start == UINT64_MAX && "SSA only assign ONCE!");
                interval->start = cur_inst;
                // assume the retval of function may not be used
                if (inst->id() == InstructionID::Call && interval->end == 0) {
                    interval->end = cur_inst;
                }
            }
            --cur_inst;
            ++it;
        }
        ++bit;
    }

    int cnt = 0;
    for (auto e : func->basicBlocks()) {
        auto e2 = blockVarTable->find(e)->second;
        for (auto e3 : *e2) {
            printf(
                "%d: begin:%lu end:%lu\n",
                cnt++,
                e3.second->livIntvl->start,
                e3.second->livIntvl->end);
        }
    }
    assert(cur_inst == 0);
}

ARMGeneralRegs Allocator::allocateRegister() {
    for (int i = 0; i < 12; i++) {
        if (!regAllocatedMap[i]) {
            regAllocatedMap[i] = true;
            return static_cast<ARMGeneralRegs>(i);
        }
    }
    return ARMGeneralRegs::None;
}

void Allocator::releaseRegister(ARMGeneralRegs reg) {
    assert(regAllocatedMap[static_cast<int>(reg)] && "It must be a bug here!");
    assert(static_cast<int>(reg) < 12);
    regAllocatedMap[static_cast<int>(reg)] = false;
}

//! TODO: 添加spill逻辑
void Allocator::updateAllocation(BasicBlock *block, uint64_t instnum) {
    auto valVarTable = blockVarTable->find(block)->second;
    for (auto e : *valVarTable) {
        auto     var   = e.second;
        uint64_t start = var->livIntvl->start;
        uint64_t end   = var->livIntvl->end;
        if (instnum == start) {
            liveVars->insertToTail(var);
            ARMGeneralRegs allocReg = allocateRegister();
            if (allocReg != ARMGeneralRegs::None)
                var->reg = allocReg;
            else {
                //! TODO: spill
                assert(0);
            }
        } else if (instnum == end + 1) {
            if (!var->is_spilled) releaseRegister(var->reg);
            auto node = liveVars->head();
            while (node != liveVars->tail()) {
                auto v = node->value();
                if (v == var) {
                    node->removeFromList();
                    break;
                }
                node = node->next();
            }
        }
    }
}

} // namespace slime::backend