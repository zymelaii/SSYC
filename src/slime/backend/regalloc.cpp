#include "regalloc.h"
#include "gencode.h"
#include "slime/ir/instruction.def"

#include <cstdint>
#include <slime/ir/value.h>
#include <slime/ir/instruction.h>

namespace slime::backend {

bool Allocator::isVariable(Value *val) {
    return !(val->isConstant() || val->isImmediate() || val->isLabel());
}

void Allocator::initAllocator() {
    cur_inst   = 0;
    total_inst = 0;
    blockVarTable->clear();
    stack->clear();
    memset(regAllocatedMap, false, 12);

    // init liveVars
    auto it  = liveVars->node_begin();
    auto end = liveVars->node_end();
    while (it != end) {
        auto tmp = *it++;
        tmp.removeFromList();
    }
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

    // int cnt = 0;
    // for (auto e : func->basicBlocks()) {
    //     auto e2 = blockVarTable->find(e)->second;
    //     for (auto e3 : *e2) {
    //         printf(
    //             "%d: begin:%lu end:%lu\n",
    //             cnt++,
    //             e3.second->livIntvl->start,
    //             e3.second->livIntvl->end);
    //     }
    // }
    assert(cur_inst == 0);
}

Variable *Allocator::getMinIntervalRegVar() {
    uint32_t  min = UINT32_MAX;
    Variable *retVar;
    for (auto e : *liveVars) {
        if (e->livIntvl->end < min && e->reg != ARMGeneralRegs::None) {
            min    = e->livIntvl->end;
            retVar = e;
        }
    }
    return retVar;
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

void Allocator::releaseRegister(Variable *var) {
    assert(
        regAllocatedMap[static_cast<int>(var->reg)]
        && "It must be a bug here!");
    assert(static_cast<int>(var->reg) < 12);
    regAllocatedMap[static_cast<int>(var->reg)] = false;
    var->reg                                    = ARMGeneralRegs::None;
}

void Allocator::releaseRegister(ARMGeneralRegs reg) {
    assert(regAllocatedMap[static_cast<int>(reg)] && "It must be a bug here!");
    assert(static_cast<int>(reg) < 12);
    regAllocatedMap[static_cast<int>(reg)] = false;
}

//! TODO: 添加spill逻辑
void Allocator::updateAllocation(
    Generator *gen, BasicBlock *block, size_t instnum) {
    auto valVarTable = blockVarTable->find(block)->second;
    for (auto e : *valVarTable) {
        auto   var   = e.second;
        size_t start = var->livIntvl->start;
        size_t end   = var->livIntvl->end;
        if (instnum == start) {
            liveVars->insertToTail(var);
            if (var->is_global) continue;
            ARMGeneralRegs allocReg = allocateRegister();
            if (allocReg != ARMGeneralRegs::None)
                var->reg = allocReg;
            else {
                Variable *minlntvar = getMinIntervalRegVar();
                assert(!minlntvar->is_spilled);
                if (minlntvar->livIntvl->end < end) {
                    var->is_spilled = true;
                    if (stack->spillVar(var, 4))
                        gen->cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, 4);
                } else {
                    minlntvar->is_spilled = true;
                    if (stack->spillVar(minlntvar, 4))
                        gen->cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, 4);
                    gen->cgStr(
                        minlntvar->reg,
                        ARMGeneralRegs::SP,
                        stack->stackSize - stack->lookupOnStackVar(minlntvar));
                    var->reg       = minlntvar->reg;
                    minlntvar->reg = ARMGeneralRegs::None;
                }
            }
        } else if (instnum == end + 1) {
            if (!var->is_global) {
                if (!var->is_spilled && !var->is_alloca)
                    releaseRegister(var);
                else if (!var->is_global) {
                    var->is_spilled = false;
                    stack->releaseOnStackVar(var);
                }
            }
            auto it  = liveVars->node_begin();
            auto end = liveVars->node_end();
            while (it != end) {
                auto v = it->value();
                if (v == var) {
                    it->removeFromList();
                    break;
                }
                ++it;
            }
        }
    }
}

} // namespace slime::backend