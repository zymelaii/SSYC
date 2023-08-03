#include "gencode.h"
#include "regalloc.h"

#include <slime/ir/instruction.def>
#include <slime/ir/module.h>
#include <slime/ir/value.h>
#include <slime/ir/user.h>
#include <slime/ir/instruction.h>
#include <slime/utils/list.h>
#include <set>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <type_traits>

namespace slime::backend {

Generator *Generator::generate() {
    return new Generator();
}

std::string Generator::genCode(Module *module) {
    // generator_.os;
    generator_.allocator         = Allocator::create();
    generator_.stack             = generator_.allocator->stack;
    generator_.usedGlobalVars    = new UsedGlobalVars;
    GlobalObjectList *global_var = new GlobalObjectList;
    std::string       modulecode;
    modulecode += sprintln("   .arch armv7a");
    modulecode += sprintln("   .text");
    for (auto e : *module) {
        if (e->type()->isFunction()) {
            generator_.allocator->initAllocator();
            generator_.cur_func = static_cast<Function *>(e);
            if (libfunc.find(e->name().data()) != libfunc.end()) continue;
            modulecode += genAssembly(static_cast<Function *>(e));
            generator_.cur_funcnum++;
            modulecode += sprintln("   .pool\n");
        } else {
            if (e->tryIntoGlobalVariable() != nullptr)
                global_var->insertToTail(e);
        }
    }
    // generator constant pool
    modulecode += genUsedGlobVars();
    for (auto e : *global_var) { modulecode += genGlobalDef(e); }
    return modulecode;
}

const char *Generator::reg2str(ARMGeneralRegs reg) {
    switch (reg) {
        case ARMGeneralRegs::R0:
            return "r0";
        case ARMGeneralRegs::R1:
            return "r1";
        case ARMGeneralRegs::R2:
            return "r2";
        case ARMGeneralRegs::R3:
            return "r3";
        case ARMGeneralRegs::R4:
            return "r4";
        case ARMGeneralRegs::R5:
            return "r5";
        case ARMGeneralRegs::R6:
            return "r6";
        case ARMGeneralRegs::R7:
            return "r7";
        case ARMGeneralRegs::R8:
            return "r8";
        case ARMGeneralRegs::R9:
            return "r9";
        case ARMGeneralRegs::R10:
            return "r10";
        case ARMGeneralRegs::R11:
            return "r11";
        case ARMGeneralRegs::IP:
            return "ip";
        case ARMGeneralRegs::SP:
            return "sp";
        case ARMGeneralRegs::LR:
            return "lr";
        case ARMGeneralRegs::None:
        default:
            fprintf(stderr, "Invalid Register!");
            exit(-1);
    }
}

std::string Generator::genGlobalDef(GlobalObject *obj) {
    std::string globdefs;
    if (obj->tryIntoFunction() != nullptr) {
        auto func  = obj->asFunction();
        globdefs  += sprintln("   .globl %s", func->name().data());
        globdefs  += sprintln("   .p2align 2");
        globdefs  += sprintln("   .type %s, %%function", func->name().data());
        globdefs  += sprintln("%s:", func->name().data());
    } else if (obj->tryIntoGlobalVariable()) {
        auto globvar  = obj->asGlobalVariable();
        auto initData = globvar->type()->tryGetElementType();
        globdefs += sprintln("   .type %s, %%object", globvar->name().data());
        if (initData->isArray()) {
            uint32_t size = 1;
            auto     e    = initData;
            while (e != nullptr && e->isArray()) {
                size *= e->asArrayType()->size();
                e     = e->tryGetElementType();
            }
            globdefs += sprintln(
                "   .comm %s, %d, %d", globvar->name().data(), size * 4, 4);
        } else {
            globdefs += sprintln("   .data");
            globdefs += sprintln("   .globl %s", globvar->name().data());
            globdefs += sprintln("   .p2align 2");
            globdefs += sprintln("%s:", globvar->name().data());
            //! TODO: float global var
            assert(!initData->isFloat());
            if (initData->isInteger()) {
                globdefs += sprintln(
                    "   .long   %d",
                    static_cast<const ConstantInt *>(globvar->data())->value);
                globdefs +=
                    sprintln("   .size %s, 4\n", globvar->name().data());
            }
        }
    }
    return globdefs;
}

std::string Generator::genUsedGlobVars() {
    std::string globvars;
    for (auto e : *generator_.usedGlobalVars) {
        globvars += sprintln("%s:", e.second.data());
        globvars += sprintln("   .long %s", e.first->val->name().data());
    }
    globvars += sprintln("");
    return globvars;
}

std::string Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    std::string funccode;
    std::string blockcodes;
    funccode += genGlobalDef(func);
    for (auto block : func->basicBlocks()) {
        blockcodes += sprintln(
            ".F%dBB.%d:",
            generator_.cur_funcnum,
            getBlockNum(block->id())); // label
        generator_.cur_block  = block;
        blockcodes           += genInstList(&block->instructions());
    }
    funccode += saveCallerReg();
    funccode += blockcodes;
    funccode += restoreCallerReg();
    return funccode;
}

Variable *Generator::findVariable(Value *val) {
    auto blockVarTable = generator_.allocator->blockVarTable;
    auto valVarTable   = blockVarTable->find(generator_.cur_block)->second;
    auto it            = valVarTable->find(val);
    if (it == valVarTable->end()) { assert(0 && "unexpected error"); }
    return it->second;
}

void Generator::addUsedGlobalVar(Variable *var) {
    assert(var->is_global);
    static uint32_t nrGlobVar = 0;
    static char     str[10];
    if (generator_.usedGlobalVars->find(var)
        != generator_.usedGlobalVars->end())
        return;
    sprintf(str, "GlobAddr%d", nrGlobVar++);
    generator_.usedGlobalVars->insert({var, str});
}

std::string Generator::saveCallerReg() {
    RegList regList;
    auto    usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    return cgPush(regList);
}

std::string Generator::restoreCallerReg() {
    RegList regList;
    auto    usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    return cgPop(regList);
}

// 将IR中基本块的id转成从0开始的顺序编号
int Generator::getBlockNum(int blockid) {
    BasicBlockList blocklist = generator_.cur_func->basicBlocks();
    int            num       = 0;
    for (auto block : blocklist) {
        if (block->id() == blockid) { return num; }
        num++;
    }
    assert(0 && "Can't reach here");
    return -1;
}

// 获得下一个基本块，如果没有则返回空指针
BasicBlock *Generator::getNextBlock() {
    BasicBlockList blocklist = generator_.cur_func->basicBlocks();
    auto           it        = blocklist.node_begin();
    auto           end       = blocklist.node_end();
    while (it != end) {
        auto block = it->value();
        if (block == generator_.cur_block) {
            ++it;
            if (it != end)
                return it->value();
            else
                return nullptr;
        }
        ++it;
    }
    assert(0 && "Can't reach here");
    return nullptr;
}

Instruction *Generator::getNextInst(Instruction *inst) {
    auto instlist = generator_.cur_block->instructions();
    auto it       = instlist.node_begin();
    auto end      = instlist.node_end();
    while (it != end) {
        if (it->value() == inst) {
            it++;
            if (it != end)
                return it->value();
            else
                return nullptr;
        }
        ++it;
    }
    assert(0 && "Can't reach here");
    return nullptr;
}

std::string Generator::genInstList(InstructionList *instlist) {
    int         allocaSize = 0;
    bool        flag       = false;
    std::string blockcode;
    for (auto inst : *instlist) {
        generator_.allocator->cur_inst++;
        if (inst->id() == InstructionID::Alloca) {
            allocaSize += genAllocaInst(inst->asAlloca());
            flag        = true;
            continue;
        } else if (
            (flag || generator_.allocator->cur_inst == 1)
            && inst->id() != InstructionID::Alloca) {
            flag = false;
            if (inst->parent()
                == generator_.cur_func->basicBlocks().head()->value())
                blockcode +=
                    cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, allocaSize);
        }
        blockcode += genInst(inst);
    }
    return blockcode;
}

std::string Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    assert(instID != InstructionID::Alloca);
    std::string instcode;
    generator_.allocator->updateAllocation(
        this, &instcode, generator_.cur_block, inst);

    switch (inst->id()) {
        case InstructionID::Alloca:
            instcode += genAllocaInst(inst->asAlloca());
            break;
        case InstructionID::Load:
            instcode += genLoadInst(inst->asLoad());
            break;
        case InstructionID::Store:
            instcode += genStoreInst(inst->asStore());
            break;
        case InstructionID::Ret:
            instcode += genRetInst(inst->asRet());
            break;
        case InstructionID::Br:
            instcode += genBrInst(inst->asBr());
            break;
        case InstructionID::GetElementPtr:
            instcode += genGetElemPtrInst(inst->asGetElementPtr());
            break;
        case InstructionID::Add:
            instcode += genAddInst(inst->asAdd());
            break;
        case InstructionID::Sub:
            instcode += genSubInst(inst->asSub());
            break;
        case InstructionID::Mul:
            instcode += genMulInst(inst->asMul());
            break;
        case InstructionID::UDiv:
            instcode += genUDivInst(inst->asUDiv());
            break;
        case InstructionID::SDiv:
            instcode += genSDivInst(inst->asSDiv());
            break;
        case InstructionID::URem:
            instcode += genURemInst(inst->asURem());
            break;
        case InstructionID::SRem:
            instcode += genSRemInst(inst->asSRem());
            break;
        case InstructionID::FNeg:
            instcode += genFNegInst(inst->asFNeg());
            break;
        case InstructionID::FAdd:
            instcode += genFAddInst(inst->asFAdd());
            break;
        case InstructionID::FSub:
            instcode += genFSubInst(inst->asFSub());
            break;
        case InstructionID::FMul:
            instcode += genFMulInst(inst->asFMul());
            break;
        case InstructionID::FDiv:
            instcode += genFDivInst(inst->asFDiv());
            break;
        case InstructionID::FRem:
            instcode += genFRemInst(inst->asFRem());
            break;
        case InstructionID::Shl:
            instcode += genShlInst(inst->asShl());
            break;
        case InstructionID::LShr:
            instcode += genLShrInst(inst->asLShr());
            break;
        case InstructionID::AShr:
            instcode += genAShrInst(inst->asAShr());
            break;
        case InstructionID::And:
            instcode += genAndInst(inst->asAnd());
            break;
        case InstructionID::Or:
            instcode += genOrInst(inst->asOr());
            break;
        case InstructionID::Xor:
            instcode += genXorInst(inst->asXor());
            break;
        case InstructionID::FPToUI:
            instcode += genFPToUIInst(inst->asFPToUI());
            break;
        case InstructionID::FPToSI:
            instcode += genFPToSIInst(inst->asFPToSI());
            break;
        case InstructionID::UIToFP:
            instcode += genUIToFPInst(inst->asUIToFP());
            break;
        case InstructionID::SIToFP:
            instcode += genSIToFPInst(inst->asSIToFP());
            break;
        case InstructionID::ICmp:
            instcode += genICmpInst(inst->asICmp());
            break;
        case InstructionID::FCmp:
            instcode += genFCmpInst(inst->asFCmp());
            break;
        case InstructionID::ZExt:
            instcode += genZExtInst(inst->asZExt());
            break;
        case InstructionID::Call:
            instcode += genCallInst(inst->asCall());
            break;
        default:
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            exit(-1);
    }
    generator_.allocator->checkLiveInterval();
    return instcode;
}

int Generator::genAllocaInst(AllocaInst *inst) {
    Instruction *pinst = inst;
    auto         e     = inst->unwrap()->type()->tryGetElementType();
    int          size  = 1;
    while (e != nullptr && e->isArray()) {
        size *= e->asArrayType()->size();
        e     = e->tryGetElementType();
    }
    auto var       = findVariable(inst->unwrap());
    var->is_alloca = true;
    generator_.stack->spillVar(var, size * 4);
    return size * 4;
}

std::string Generator::genLoadInst(LoadInst *inst) {
    auto           targetVar = findVariable(inst->unwrap());
    auto           source    = inst->useAt(0);
    int            offset;
    std::string    loadcode;
    ARMGeneralRegs sourceReg;
    loadcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (source->isLabel()) {
        //! TODO: label
        assert(0);
    } else {
        assert(source->type()->isPointer());
        auto sourceVar = findVariable(source);

        if (sourceVar->is_alloca) {
            sourceReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize - sourceVar->stackpos;
        } else if (sourceVar->is_global) {
            addUsedGlobalVar(sourceVar);
            offset = 0;
            // assert(sourceVar->reg != ARMGeneralRegs::None);
            loadcode  += cgLdr(targetVar->reg, sourceVar);
            sourceReg  = targetVar->reg;
            // if (sourceVar->reg == ARMGeneralRegs::None) {
            //     cgLdr(targetVar->reg, sourceVar);
            //     sourceReg = targetVar->reg;
            // } else
            //     sourceReg = sourceVar->reg;
        } else {
            sourceReg = sourceVar->reg;
            offset    = 0;
        }
        loadcode += cgLdr(targetVar->reg, sourceReg, offset);
    }
    return loadcode;
}

std::string Generator::genStoreInst(StoreInst *inst) {
    auto           target = inst->useAt(0);
    auto           source = inst->useAt(1);
    ARMGeneralRegs sourceReg, targetReg;
    int            offset;
    std::string    storecode;

    ARMGeneralRegs tmpreg = ARMGeneralRegs::None;
    if (source.value()->isConstant()) {
        if (source.value()->type()->isInteger()) {
            std::set<Variable *> whitelist;
            sourceReg = generator_.allocator->allocateRegister(
                true, &whitelist, this, &storecode);
            tmpreg      = sourceReg;
            int32_t imm = static_cast<ConstantInt *>(source.value())->value;
            if (sourceReg == ARMGeneralRegs::None) {
                //! TODO: spill something
                assert(0 && "TODO!");
            }
            storecode += cgMov(sourceReg, imm);
        } else if (source.value()->type()->isFloat()) {
            //! TODO: float
            assert(0);
        }
    } else {
        auto sourceVar = findVariable(source);
        if (sourceVar->is_spilled) {
            //! TODO: handle spilled variable
            assert(0);
        }
        sourceReg = sourceVar->reg;
    }
    auto targetVar = findVariable(target);
    if (targetVar->is_global) {
        assert(targetVar->reg != ARMGeneralRegs::None);
        if (targetVar->reg != ARMGeneralRegs::None) {
            targetReg = targetVar->reg;
            addUsedGlobalVar(targetVar);
            storecode += cgLdr(targetReg, targetVar);
            storecode += cgStr(sourceReg, targetReg, 0);
        }
    } else {
        if (targetVar->is_alloca) {
            targetReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize - targetVar->stackpos;
        } else {
            targetReg = targetVar->reg;
            offset    = 0;
        }
        storecode += cgStr(sourceReg, targetReg, offset);
    }
    if (source.value()->isConstant())
        generator_.allocator->releaseRegister(sourceReg);
    return storecode;
}

std::string Generator::genRetInst(RetInst *inst) {
    std::string retcode;
    if (inst->totalOperands() != 0) {
        auto operand = inst->useAt(0).value();
        if (operand->isImmediate()) {
            assert(operand->type()->isInteger());
            retcode += cgMov(
                ARMGeneralRegs::R0, static_cast<ConstantInt *>(operand)->value);
        } else {
            assert(Allocator::isVariable(operand));
            auto var = findVariable(operand);
            if (var->reg != ARMGeneralRegs::R0)
                retcode += cgMov(ARMGeneralRegs::R0, var->reg);
        }
    }
    if (generator_.stack->stackSize > 0)
        retcode += cgAdd(
            ARMGeneralRegs::SP,
            ARMGeneralRegs::SP,
            generator_.stack->stackSize);
    retcode += cgBx(ARMGeneralRegs::LR);
    return retcode;
}

std::string Generator::genBrInst(BrInst *inst) {
    std::string brcode;
    BasicBlock *nextblock = getNextBlock();
    if (generator_.cur_block->isLinear()) {
        Value *target = inst->useAt(0);
        if (target->id() != nextblock->id()) brcode += cgB(inst->useAt(0));
    } else {
        auto control = inst->parent()->control()->asInstruction();
        ComparePredicationType predict;
        if (control->id() == InstructionID::ICmp) {
            predict = control->asICmp()->predicate();
        } else if (control->id() == InstructionID::Load) {
            predict = ComparePredicationType::EQ;
        } else {
            assert(0 && "FCMP");
        }

        Variable *cond    = findVariable(inst->useAt(0));
        Value    *target1 = inst->useAt(1), *target2 = inst->useAt(2);
        if (cond->reg != ARMGeneralRegs::None) {
            brcode += cgTst(cond->reg, 1);
            brcode += cgB(target2, ComparePredicationType::EQ);
            brcode += cgB(target1);
            return brcode;
        }
        // if (nextblock->id() == target1->id()) {
        //     cgB(target2, ComparePredicationType::EQ);
        // } else if (nextblock->id() == target2->id()) {
        //     cgB(target1, ComparePredicationType::NE);
        // } else {
        brcode += cgB(target1, predict);
        brcode += cgB(target2);
        // }
    }
    return brcode;
}

int Generator::sizeOfType(ir::Type *type) {
    switch (type->kind()) {
        case TypeKind::Integer:
        case TypeKind::Float:
        case TypeKind::Pointer: {
            //! FIXME: depends on 32-bit arch
            return 4;
        } break;
        case TypeKind::Array: {
            return sizeOfType(type->tryGetElementType())
                 * type->asArrayType()->size();
        }
        default: {
            assert(false);
            return -1;
        }
    }
}

std::string Generator::genGetElemPtrInst(GetElementPtrInst *inst) {
    //! NOTE: 2 more extra registers requried

    //! addr = base + i[0] * sizeof(type) + i[1] * sizeof(*type) + ...
    auto        baseType = inst->op<0>()->type()->tryGetElementType();
    std::string getelemcode;
    getelemcode += sprintln("@ %%%d:", inst->unwrap()->id());
    //! dest is initially assign to base addr
    auto dest = findVariable(inst->unwrap())->reg;
    if (auto var = findVariable(inst->op<0>()); var->is_alloca) {
        getelemcode += cgMov(dest, ARMGeneralRegs::SP);
        getelemcode +=
            cgAdd(dest, dest, generator_.stack->stackSize - var->stackpos);
    } else if (var->is_global) {
        addUsedGlobalVar(var);
        getelemcode += cgLdr(dest, var);
    } else {
        assert(!var->is_spilled);
        assert(!inst->op<0>()->isImmediate());
        getelemcode += cgMov(dest, var->reg);
    }

    decltype(dest) *tmp = nullptr;
    decltype(dest)  regPlaceholder{};
    bool            tmpIsAllocated = false;
    for (int i = 1; i < inst->totalOperands(); ++i) {
        auto op = inst->op()[i];
        if (op == nullptr) {
            assert(i > 1);
            break;
        }
        //! tmp <- index
        int multiplier = -1;
        if (op->isImmediate()) {
            auto imm   = static_cast<ConstantInt *>(op.value())->value;
            multiplier = imm;
            if (multiplier >= 1) {
                auto whitelist = generator_.allocator->getInstOperands(inst);
                regPlaceholder = generator_.allocator->allocateRegister(
                    true, whitelist, this, &getelemcode);
                tmp            = &regPlaceholder;
                tmpIsAllocated = true;
                if (multiplier > 1) { getelemcode += cgMov(*tmp, imm); }
            }
        } else if (op->isGlobal()) {
            //! FIXME: assume that reg of global variable is free to use
            addUsedGlobalVar(findVariable(op));
            auto var     = findVariable(op);
            tmp          = &var->reg;
            getelemcode += cgLdr(*tmp, var);
            getelemcode += cgLdr(*tmp, *tmp, 0);
        } else {
            auto var = findVariable(op);
            assert(var != nullptr);
            tmp = &var->reg;
        }

        //! offset <- stride * index
        const auto stride = sizeOfType(baseType);

        if (multiplier == -1) {
            auto whitelist = generator_.allocator->getInstOperands(inst);
            auto reg       = generator_.allocator->allocateRegister(
                true, whitelist, this, &getelemcode);
            getelemcode += cgMov(reg, stride);
            getelemcode += cgMul(reg, *tmp, reg);
            //! swap register: *tmp <- reg
            if (tmpIsAllocated) { generator_.allocator->releaseRegister(*tmp); }
            regPlaceholder = reg;
        } else if (multiplier > 0) {
            if (multiplier == 1) {
                getelemcode += cgMov(*tmp, stride);
            } else if (multiplier == 2) {
                getelemcode += cgMov(*tmp, stride);
                getelemcode += cgAdd(*tmp, *tmp, *tmp);
            } else {
                auto whitelist = generator_.allocator->getInstOperands(inst);
                auto reg       = generator_.allocator->allocateRegister(
                    true, whitelist, this, &getelemcode);
                getelemcode += cgMov(reg, stride);
                getelemcode += cgMul(*tmp, *tmp, reg);
                generator_.allocator->releaseRegister(reg);
            }
        }

        if (multiplier != 0) {
            //! dest <- base + offset
            getelemcode += cgAdd(dest, dest, *tmp);
        }

        if (tmpIsAllocated) {
            generator_.allocator->releaseRegister(*tmp);
            tmp            = nullptr;
            tmpIsAllocated = false;
        }

        if (i + 1 == inst->totalOperands() || inst->op()[i + 1] == nullptr) {
            break;
        }

        baseType = baseType->tryGetElementType();
    }
    return getelemcode;
}

std::string Generator::genAddInst(AddInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rn;
    std::string    addcode;
    auto           op1  = inst->useAt(0);
    auto           op2  = inst->useAt(1);
    addcode            += sprintln("@ %%%d:", inst->unwrap()->id());

    if (Allocator::isVariable(op1)) {
        rn = findVariable(op1)->reg;
    } else {
        rn = rd;
        addcode +=
            cgMov(rd, static_cast<ConstantInt *>(op1->asConstantData())->value);
    }

    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg  = findVariable(op2)->reg;
        addcode               += cgAdd(rd, rn, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        addcode += cgAdd(rd, rn, imm);
    }
    return addcode;
}

std::string Generator::genSubInst(SubInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    std::string    subcode;
    subcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        subcode += cgMov(rd, imm);
        rs       = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg  = findVariable(op2)->reg;
        subcode               += cgSub(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        subcode += cgSub(rd, rs, imm);
    }
    return subcode;
}

std::string Generator::genMulInst(MulInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    std::string    mulcode;
    mulcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        mulcode += cgMov(rd, imm);
        rs       = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg  = findVariable(op2)->reg;
        mulcode               += cgMul(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        mulcode += cgMul(rd, rs, imm);
    }
    return mulcode;
}

std::string Generator::genUDivInst(UDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genSDivInst(SDivInst *inst) {
    assert(!generator_.allocator->regAllocatedMap[0]);
    assert(!generator_.allocator->regAllocatedMap[1]);
    std::string sdivcode;
    sdivcode += sprintln("@ %%%d:", inst->unwrap()->id());
    for (int i = 0; i < inst->totalOperands(); i++) {
        if (inst->useAt(i)->isConstant()) {
            sdivcode += cgMov(
                static_cast<ARMGeneralRegs>(i),
                static_cast<ConstantInt *>(inst->useAt(i)->asConstantData())
                    ->value);
        } else {
            sdivcode += cgMov(
                static_cast<ARMGeneralRegs>(i),
                findVariable(inst->useAt(i))->reg);
        }
    }
    auto resultReg  = findVariable(inst->unwrap())->reg;
    sdivcode       += cgBl("__aeabi_idiv");
    sdivcode       += cgMov(resultReg, ARMGeneralRegs::R0);
    return sdivcode;
}

std::string Generator::genURemInst(URemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

//! NOTE: 用除法和乘法完成求余
std::string Generator::genSRemInst(SRemInst *inst) {
    auto        targetReg = findVariable(inst->unwrap())->reg;
    std::string sremcode;
    sremcode += sprintln("@ %%%d:", inst->unwrap()->id());
    for (int i = 0; i < inst->totalOperands(); i++) {
        if (inst->useAt(i)->isConstant()) {
            sremcode += cgMov(
                static_cast<ARMGeneralRegs>(i),
                static_cast<ConstantInt *>(inst->useAt(i)->asConstantData())
                    ->value);
        } else {
            sremcode += cgMov(
                static_cast<ARMGeneralRegs>(i),
                findVariable(inst->useAt(i))->reg);
        }
    }
    sremcode += cgBl("__aeabi_idiv");
    ARMGeneralRegs tmpReg;
    if (inst->useAt(1)->isConstant()) {
        sremcode += cgMov(
            ARMGeneralRegs::R1,
            static_cast<ConstantInt *>(inst->useAt(1)->asConstantData())
                ->value);
        tmpReg = ARMGeneralRegs::R1;
    } else {
        tmpReg = findVariable(inst->useAt(1))->reg;
    }
    sremcode += cgMul(targetReg, tmpReg, ARMGeneralRegs::R0);
    if (inst->useAt(0)->isConstant()) {
        sremcode += cgMov(
            ARMGeneralRegs::R0,
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())
                ->value);
        sremcode += cgSub(targetReg, ARMGeneralRegs::R0, targetReg);
    } else {
        sremcode +=
            cgSub(targetReg, findVariable(inst->useAt(0))->reg, targetReg);
    }
    return sremcode;
}

std::string Generator::genFNegInst(FNegInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFAddInst(FAddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFSubInst(FSubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFMulInst(FMulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFDivInst(FDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFRemInst(FRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genShlInst(ShlInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    std::string    shlcode;

    shlcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        shlcode += cgMov(rd, imm);
        rs       = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        assert(0);
        // += cgLsl(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        shlcode += cgLsl(rd, rs, imm);
    }
    return shlcode;
}

std::string Generator::genLShrInst(LShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genAShrInst(AShrInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    std::string    ashrcode;

    ashrcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        ashrcode += cgMov(rd, imm);
        rs        = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        assert(0);
        // += cgAsr(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        ashrcode += cgAsr(rd, rs, imm);
    }
    return ashrcode;
}

std::string Generator::genAndInst(AndInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    std::string    andcode;
    andcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        andcode += cgMov(rd, imm);
        rs       = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg  = findVariable(op2)->reg;
        andcode               += cgAnd(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        andcode += cgAnd(rd, rs, imm);
    }
    return andcode;
}

std::string Generator::genOrInst(OrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genXorInst(XorInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFPToUIInst(FPToUIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genFPToSIInst(FPToSIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genUIToFPInst(UIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genSIToFPInst(SIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

std::string Generator::genICmpInst(ICmpInst *inst) {
    auto        op1 = inst->useAt(0), op2 = inst->useAt(1);
    std::string icmpcode;

    icmpcode += sprintln("@ %%%d:", inst->unwrap()->id());
    if (Allocator::isVariable(op2)) {
        if (!Allocator::isVariable(op1)) {
            std::set<Variable *> *whitelist =
                generator_.allocator->getInstOperands(inst);
            auto tmpreg = generator_.allocator->allocateRegister(
                true, whitelist, this, &icmpcode);
            icmpcode += cgMov(
                tmpreg,
                static_cast<ConstantInt *>(op1->asConstantData())->value);
            icmpcode += cgCmp(tmpreg, findVariable(op2)->reg);
            generator_.allocator->releaseRegister(tmpreg);
        } else {
            Variable *lhs = findVariable(op1), *rhs = findVariable(op2);
            icmpcode += cgCmp(lhs->reg, rhs->reg);
        }
    } else {
        //! TODO: float
        assert(!op2->asConstantData()->type()->isFloat());
        Variable *lhs = findVariable(op1);
        int32_t imm  = static_cast<ConstantInt *>(op2->asConstantData())->value;
        icmpcode    += cgCmp(lhs->reg, imm);
    }

    auto result = findVariable(inst->unwrap());
    if (result->reg != ARMGeneralRegs::None) {
        icmpcode += cgMov(result->reg, 0);
        icmpcode += cgMov(result->reg, 1, inst->predicate());
        icmpcode += cgAnd(result->reg, result->reg, 1);
    }
    return icmpcode;
}

std::string Generator::genFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

//! TODO: optimise
std::string Generator::genZExtInst(ZExtInst *inst) {
    auto        extVar = findVariable(inst->unwrap());
    std::string zextcode;
    if (Allocator::isVariable(inst->useAt(0))) {
        zextcode += cgMov(extVar->reg, findVariable(inst->useAt(0))->reg);
    } else {
        int imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        zextcode += cgMov(extVar->reg, imm);
    }
    return zextcode;
}

std::string Generator::genCallInst(CallInst *inst) {
    std::string callcode;
    if (!inst->unwrap()->type()->isVoid())
        callcode += sprintln("@ %%%d:", inst->unwrap()->id());
    for (int i = 0; i < inst->totalParams(); i++) {
        if (i < 4) {
            assert(!generator_.allocator->regAllocatedMap[i]);
            if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
                assert(!inst->paramAt(i)->type()->isFloat());
                auto constant  = inst->paramAt(i)->asConstantData();
                callcode      += cgMov(
                    static_cast<ARMGeneralRegs>(i),
                    static_cast<ConstantInt *>(constant)->value);
            } else {
                assert(Allocator::isVariable(inst->paramAt(i)));
                Variable *var = findVariable(inst->paramAt(i));
                if (var->is_alloca) {
                    callcode += cgMov(
                        static_cast<ARMGeneralRegs>(i), ARMGeneralRegs::SP);
                    callcode += cgAdd(
                        static_cast<ARMGeneralRegs>(i),
                        static_cast<ARMGeneralRegs>(i),
                        generator_.stack->stackSize - var->stackpos);
                } else if (var->reg != static_cast<ARMGeneralRegs>(i)) {
                    assert(var->reg != ARMGeneralRegs::None);
                    callcode += cgMov(static_cast<ARMGeneralRegs>(i), var->reg);
                }
            }
        } else {
            assert(0);
            // 需要修改
            if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
                assert(!inst->paramAt(i)->type()->isFloat());
                generator_.stack->pushVar(
                    Variable::create(inst->paramAt(i)), 4);
            } else {
                assert(Allocator::isVariable(inst->paramAt(i)));
                Variable *var = findVariable(inst->paramAt(i).value());
                generator_.stack->pushVar(var, 4);
            }
        }
    }
    if (inst->unwrap()->uses().size() != 0) {
        if (inst->totalParams() == 0
            && generator_.allocator->regAllocatedMap[0]) {
            auto var =
                generator_.allocator->getVarOfAllocatedReg(ARMGeneralRegs::R0);
            generator_.allocator->releaseRegister(var);
        }
        generator_.allocator->regAllocatedMap[0] = true;
        auto var                                 = findVariable(inst->unwrap());
        var->reg                                 = ARMGeneralRegs::R0;
    }
    callcode += cgBl(inst->callee()->asFunction());
    return callcode;
}

std::string Generator::cgMov(
    ARMGeneralRegs rd, ARMGeneralRegs rs, ComparePredicationType cond) {
    switch (cond) {
        case ComparePredicationType::TRUE:
            return sprintln("    mov    %s, %s", reg2str(rd), reg2str(rs));
        case ComparePredicationType::EQ:
            return sprintln("    moveq  %s, %s", reg2str(rd), reg2str(rs));
        case ComparePredicationType::NE:
            return sprintln("    movne  %s, #%d", reg2str(rd), reg2str(rs));
        case ComparePredicationType::SLE:
            return sprintln("    movle  %s, #%d", reg2str(rd), reg2str(rs));
        case ComparePredicationType::SLT:
            return sprintln("    movlt  %s, #%d", reg2str(rd), reg2str(rs));
        case ComparePredicationType::SGT:
            return sprintln("    movgt  %s, #%d", reg2str(rd), reg2str(rs));
        case ComparePredicationType::SGE:
            return sprintln("    movge  %s, #%d", reg2str(rd), reg2str(rs));
        default:
            assert(0 && "unfinished comparative type");
    }
}

std::string Generator::cgMov(
    ARMGeneralRegs rd, int32_t imm, ComparePredicationType cond) {
    switch (cond) {
        case ComparePredicationType::TRUE:
            return sprintln("    mov    %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::EQ:
            return sprintln("    moveq  %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::NE:
            return sprintln("    movne  %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::SLE:
            return sprintln("    movle  %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::SLT:
            return sprintln("    movle  %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::SGE:
            return sprintln("    movge  %s, #%d", reg2str(rd), imm);
        case ComparePredicationType::SGT:
            return sprintln("    movgt  %s, #%d", reg2str(rd), imm);
        default:
            assert(0 && "unfinished comparative type");
    }
}

std::string Generator::cgLdr(
    ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(src), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(src));
    return sprintln("    ldr    %s, %s", reg2str(dst), tmpStr);
}

std::string Generator::cgLdr(ARMGeneralRegs dst, Variable *var) {
    assert(var->is_global);
    auto it = generator_.usedGlobalVars->find(var);
    if (it == generator_.usedGlobalVars->end())
        assert(0 && "it must be an error here.");
    return sprintln("    ldr    %s, %s", reg2str(dst), it->second.data());
}

std::string Generator::cgStr(
    ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(dst), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(dst));
    return sprintln("    str    %s, %s", reg2str(src), tmpStr);
}

std::string Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return sprintln(
        "    add    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    add    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return sprintln(
        "    sub    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    sub    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgMul(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return sprintln(
        "    mul    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgMul(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    mul    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgAnd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return sprintln(
        "    and    %s, %s, #%d", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgAnd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    and    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgLsl(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    lsl    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgAsr(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return sprintln("    asr    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgCmp(ARMGeneralRegs op1, ARMGeneralRegs op2) {
    return sprintln("    cmp    %s, %s", reg2str(op1), reg2str(op2));
}

std::string Generator::cgCmp(ARMGeneralRegs op1, int32_t op2) {
    return sprintln("    cmp    %s, #%d", reg2str(op1), op2);
}

std::string Generator::cgTst(ARMGeneralRegs op1, int32_t op2) {
    return sprintln("    tst    %s, #%d", reg2str(op1), op2);
}

std::string Generator::cgB(Value *brTarget, ComparePredicationType cond) {
    assert(brTarget->isLabel());
    size_t blockid = brTarget->id();
    switch (cond) {
        case ComparePredicationType::TRUE:
            return sprintln(
                "    b       .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::EQ:
            return sprintln(
                "    beq     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::NE:
            return sprintln(
                "    bne     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::SLT:
            // case ComparePredicationType::ULT:
            return sprintln(
                "    blt     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::SGT:
            return sprintln(
                "    bgt     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::SLE:
            return sprintln(
                "    ble     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        case ComparePredicationType::SGE:
            return sprintln(
                "    bge     .F%dBB.%d",
                generator_.cur_funcnum,
                getBlockNum(blockid));
        default:
            assert(0);
    }
}

std::string Generator::cgBx(ARMGeneralRegs rd) {
    return sprintln("    bx     %s", reg2str(rd));
}

std::string Generator::cgBl(Function *callee) {
    return sprintln("    bl     %s", callee->name().data());
}

std::string Generator::cgBl(const char *libfuncname) {
    assert(libfunc.find(libfuncname) != libfunc.end());
    return sprintln("    bl     %s", libfuncname);
}

std::string Generator::cgPush(RegList &reglist) {
    std::string pushcode;
    pushcode += "    push   {";
    for (auto reg : reglist) {
        pushcode += std::string(reg2str(reg));
        if (reg != reglist.tail()->value()) pushcode += std::string(",");
    }
    pushcode += sprintln("}");
    return pushcode;
}

std::string Generator::cgPop(RegList &reglist) {
    std::string popcode;
    popcode += std::string("    pop    {");
    for (auto reg : reglist) {
        popcode += std::string(reg2str(reg));
        if (reg != reglist.tail()->value()) popcode += std::string(",");
    }
    popcode += sprintln("}");
    return popcode;
}

} // namespace slime::backend
