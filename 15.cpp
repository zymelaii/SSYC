#include "16.h"
#include "14.h"

#include "36.def"
#include "43.h"
#include "37.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

namespace slime::backend {

bool Allocator::isVariable(Value *val) {
    return !(val->isConstant() || val->isImmediate() || val->isLabel());
}

void Allocator::initAllocator() {
    cur_inst   = 0;
    total_inst = 0;
    blockVarTable->clear();
    stack->clear();
    usedRegs.clear();
    strImmFlag   = true;
    max_funcargs = 0;
    freeAllRegister();
    // init liveVars
    auto it  = liveVars->node_begin();
    auto end = liveVars->node_end();
    while (it != end) {
        auto tmp = *it++;
        tmp.removeFromList();
    }
}

Variable *Allocator::createVariable(Value *val) {
    auto it = funcValVarTable->find(val);
    if (it != funcValVarTable->end())
        return it->second;
    else {
        auto var = Variable::create(val);
        funcValVarTable->insert({val, var});
        return var;
    }
}

void Allocator::initVarInterval(Function *func) {
    ValVarTable funcparams;
    for (int i = 0; i < func->totalParams(); i++) {
        Variable *var =
            Variable::create(const_cast<Parameter *>(func->paramAt(i)));
        if (i < 4) {
            var->reg           = static_cast<ARMGeneralRegs>(i);
            regAllocatedMap[i] = true;
            funcparams.insert({const_cast<Parameter *>(func->paramAt(i)), var});
        } else {
            var->is_spilled = true;
            stack->pushVar(var, 4);
        }
    }
    for (auto block : func->basicBlocks()) {
        auto valVarTable = new ValVarTable;
        blockVarTable->insert({block, valVarTable});
        total_inst += block->instructions().size();
        for (auto inst : block->instructions()) {
            if (inst->id() == InstructionID::Call
                || inst->id() == InstructionID::SDiv
                || inst->id() == InstructionID::SRem) {
                has_funccall = true;
                if (inst->id() == InstructionID::Call)
                    max_funcargs = std::max(
                        max_funcargs,
                        inst->asCall()->callee()->asFunction()->totalParams());
                else
                    max_funcargs = std::max(max_funcargs, (size_t)4);
            } else if (!strImmFlag && inst->id() == InstructionID::Store) {
                if (inst->asStore()->useAt(0)->isImmediate()) strImmFlag = true;
            }
            for (int i = 0; i < inst->totalOperands(); i++) {
                if (inst->useAt(i) && isVariable(inst->useAt(i))) {
                    Value *val = inst->useAt(i).value();
                    auto   it  = funcparams.find(val);
                    if (it != funcparams.end()) {
                        it->second->livIntvl->start = 1;
                        funcValVarTable->insert({val, it->second});
                        liveVars->insertToTail(it->second);
                        funcparams.erase(it);
                    }
                    valVarTable->insert({val, createVariable(val)});
                }
            }

            if (!inst->unwrap()->type()->isVoid())
                //! NOTE:
                //! map.insert在插入键值对时如果已经存在将插入的键值时应该是会插入失败的
                valVarTable->insert(
                    {inst->unwrap(), createVariable(inst->unwrap())});
        }
    }

    // release register occupied by unused params
    for (auto it : funcparams) {
        if (it.second->reg != ARMGeneralRegs::None) {
            Allocator::releaseRegister(it.second);
        }
    }
}

void Allocator::checkLiveInterval() {
    auto it  = liveVars->node_begin();
    auto end = liveVars->node_end();
    while (it != end) {
        auto var      = it->value();
        auto interval = var->livIntvl;
        if (interval->end <= cur_inst) {
            auto tmp = it++;
            if (var->reg != ARMGeneralRegs::None)
                releaseRegister(var);
            else if (var->is_spilled || var->is_alloca) {
                var->is_spilled = false;
                stack->releaseOnStackVar(var);
            }
            tmp->removeFromList();
        } else
            it++;
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
                Value *val = inst->useAt(i);
                if (inst->useAt(i) && isVariable(inst->useAt(i))) {
                    auto var      = valVarTable->find(inst->useAt(i))->second;
                    auto interval = var->livIntvl;
                    interval->end = std::max(interval->end, cur_inst);
                    if (inst->useAt(i)->isGlobal()) {
                        interval->start = std::min(interval->start, cur_inst);
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
                if (interval->end == 0) { interval->end = cur_inst; }
            }
            --cur_inst;
            ++it;
        }
        ++bit;
    }

    // for (auto e : func->basicBlocks()) {
    //     auto e2 = blockVarTable->find(e)->second;
    //     for (auto e3 : *e2) {
    //         printf(
    //             "%%%d: begin:%lu end:%lu\n",
    //             e3.first->id(),
    //             e3.second->livIntvl->start,
    //             e3.second->livIntvl->end);
    //     }
    // }
    assert(cur_inst == 0);
}

Variable *Allocator::getMinIntervalRegVar(std::set<Variable *> whitelist) {
    uint32_t  min = UINT32_MAX;
    Variable *retVar;
    for (auto e : *liveVars) {
        if (whitelist.find(e) != whitelist.end()) continue;
        if (e->livIntvl->end < min && e->reg != ARMGeneralRegs::None) {
            min    = e->livIntvl->end;
            retVar = e;
        }
    }
    return retVar;
}

std::set<Variable *> *Allocator::getInstOperands(Instruction *inst) {
    static std::set<Variable *> operands;
    if (!operands.empty()) operands.clear();
    auto valVarTable = blockVarTable->find(inst->parent())->second;
    for (int i = 0; i < inst->totalOperands(); i++) {
        if (!inst->useAt(i)) continue;
        if (isVariable(inst->useAt(i))) {
            Variable *var = valVarTable->find(inst->useAt(i))->second;
            operands.insert(var);
        }
    }
    if (!inst->unwrap()->type()->isVoid() && isVariable(inst->unwrap())) {
        Variable *var = valVarTable->find(inst->unwrap())->second;
        operands.insert(var);
    }
    return &operands;
}

//! TODO: 把统计使用过的寄存器从生成指令前预测改为实际生成完指令后统计
//! TODO: 目前采用println的方法难以向已经打印的指令处追加新内容
void Allocator::getUsedRegs(BasicBlockList &blocklist) {
    int max_regs = 0;
    for (auto block : blocklist) {
        int                 count = 0; // 最多几个变量同时被分配
        auto                valVarTable = blockVarTable->find(block)->second;
        std::vector<size_t> startVec, endVec;
        for (auto it : *valVarTable) {
            size_t start = it.second->livIntvl->start;
            size_t end   = it.second->livIntvl->end;
            if (!it.second->is_alloca) {
                startVec.insert(startVec.begin(), start);
                endVec.insert(endVec.begin(), end);
            }
        }
        std::sort(startVec.begin(), startVec.end());
        std::sort(endVec.begin(), endVec.end());
        size_t tmp = 0;
        for (int i = 0, j = 0; i < startVec.size() && j < startVec.size();) {
            if (startVec[i] < endVec[j]) {
                count++, i++;
                max_regs = std::max(max_regs, count);
            } else if (startVec[i] == endVec[j])
                i++, j++;
            else
                count--, j++;
        }
    }
    if (strImmFlag) max_regs += 1;
    //! FIXME: 带参数的函数会把参数也当做使用过的寄存器保存起来
    //! FIXME: 有些情况下会push多余的寄存器，如10_var_defn_func.sy
    int regBegin = max_funcargs == 0 ? 1 : max_funcargs;
    for (int i = regBegin; i < std::min(max_funcargs + max_regs, (size_t)12);
         i++)
        usedRegs.insert(static_cast<ARMGeneralRegs>(i));
}

ARMGeneralRegs Allocator::allocateRegister(
    bool force, std::set<Variable *> *whitelist, Generator *gen) {
    if (!has_funccall) {
        for (int i = 0; i < 12; i++) {
            if (!regAllocatedMap[i]) {
                regAllocatedMap[i] = true;
                return static_cast<ARMGeneralRegs>(i);
            }
        }
    } else {
        for (int i = max_funcargs; i < 12; i++) {
            if (!regAllocatedMap[i]) {
                regAllocatedMap[i] = true;
                return static_cast<ARMGeneralRegs>(i);
            }
        }
        for (int i = max_funcargs - 1; i >= 0; i--) {
            if (!regAllocatedMap[i]) {
                regAllocatedMap[i] = true;
                return static_cast<ARMGeneralRegs>(i);
            }
        }
    }
    if (!force) return ARMGeneralRegs::None;
    assert(gen && "Generator couldn't be null in this case");
    if (!whitelist) {
        std::set<Variable *> emptylistHolder;
        whitelist = &emptylistHolder;
    }
    Variable *minlntvar = getMinIntervalRegVar(*whitelist);
    assert(!minlntvar->is_spilled);
    minlntvar->is_spilled = true;
    auto ret              = minlntvar->reg;
    minlntvar->reg        = ARMGeneralRegs::None;
    gen->println("# Spill %d to stack", minlntvar->val->id());
    if (stack->spillVar(minlntvar, 4))
        gen->cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, 4);
    gen->cgStr(ret, ARMGeneralRegs::SP, stack->stackSize - minlntvar->stackpos);
    return ret;
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

void Allocator::freeAllRegister() {
    memset(regAllocatedMap, false, 12);
}

void Allocator::updateAllocation(
    Generator *gen, BasicBlock *block, Instruction *inst) {
    std::set<Variable *> *operands    = getInstOperands(inst);
    auto                  valVarTable = blockVarTable->find(block)->second;

    for (auto e : *valVarTable) {
        auto   var   = e.second;
        size_t start = var->livIntvl->start;
        size_t end   = var->livIntvl->end;

        // if (inst->id() == InstructionID::GetElementPtr
        //     && inst->unwrap() == var->val) {
        //     auto arrbase = valVarTable->find(inst->useAt(0).value())->second;
        //     if (arrbase->is_global) {
        //         assert(0 && "global array unsupported yet");
        //     } else if(arrbase->is_alloca){
        //         size_t arrsize = 1;
        //         size_t offset = 0;
        //         auto   e       = inst->unwrap()->type()->tryGetElementType();
        //         while (e != nullptr && e->isArray()) {
        //             arrsize *= e->asArrayType()->size();
        //             e     = e->tryGetElementType();
        //         }

        //         var->is_alloca = true;
        //         var->stackpos  = arrbase->stackpos;
        //     }
        //     continue;
        // }

        if (cur_inst == start) {
            liveVars->insertToTail(var);
            if (inst->id() == InstructionID::ICmp
                && inst->unwrap() == var->val) {
                Instruction *nextinst = gen->getNextInst(inst);
                if (nextinst->id() == InstructionID::Br && end == cur_inst + 1)
                    continue;
            }

            ARMGeneralRegs allocReg = allocateRegister();
            if (allocReg != ARMGeneralRegs::None)
                var->reg = allocReg;
            else {
                //! NOTE: spill
                Variable *minlntvar = getMinIntervalRegVar(*operands);
                assert(!minlntvar->is_spilled);

                var->reg       = minlntvar->reg;
                minlntvar->reg = ARMGeneralRegs::None;
                if (!minlntvar->is_global) {
                    minlntvar->is_spilled = true;
                    if (stack->spillVar(minlntvar, 4))
                        gen->cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, 4);
                    gen->println("# Spill %d to stack", minlntvar->val->id());
                    gen->cgStr(
                        var->reg,
                        ARMGeneralRegs::SP,
                        stack->stackSize - minlntvar->stackpos);
                }
            }
            if (var->is_global) {}
        }
    }

    // check if current instruction holds spiiled variable
    for (auto var : *operands) {
        if (var->is_spilled) {
            stack->releaseOnStackVar(var);
            var->reg        = allocateRegister(true, operands, gen);
            var->is_spilled = false;
        } else if (
            var->is_global && var->reg == ARMGeneralRegs::None
            && inst->id() != InstructionID::Load) {
            var->reg        = allocateRegister(true, operands, gen);
        }
    }
}

} // namespace slime::backend