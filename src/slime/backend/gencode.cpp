#include "gencode.h"
#include "regalloc.h"

#include "slime/ir/instruction.def"
#include "slime/ir/module.h"
#include "slime/ir/value.h"
#include "slime/ir/user.h"
#include "slime/ir/instruction.h"
#include "slime/utils/list.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace slime::backend {

Generator *Generator::generate() {
    return new Generator();
}

void Generator::genCode(FILE *fp, Module *module) {
    generator_.asmFile           = fp;
    generator_.allocator         = Allocator::create();
    generator_.stack             = generator_.allocator->stack;
    generator_.usedGlobalVars    = new UsedGlobalVars;
    GlobalObjectList *global_var = new GlobalObjectList;
    println("   .arch armv7a");
    println("   .text");
    for (auto e : *module) {
        if (e->type()->isFunction()) {
            generator_.allocator->initAllocator();
            generator_.cur_func = static_cast<Function *>(e);
            if (!strcmp(e->name().data(), "memset")) continue;
            genAssembly(static_cast<Function *>(e));
            println("   .pool\n");
        } else {
            if (e->tryIntoGlobalVariable() != nullptr)
                global_var->insertToTail(e);
        }
    }
    // generator constant pool
    genUsedGlobVars();
    for (auto e : *global_var) { genGlobalDef(e); }
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

void Generator::genGlobalDef(GlobalObject *obj) {
    if (obj->tryIntoFunction() != nullptr) {
        auto func = obj->asFunction();
        println("   .globl %s", func->name().data());
        println("   .p2align 2");
        println("   .type %s, %%function", func->name().data());
        println("%s:", func->name().data());
    } else if (obj->tryIntoGlobalVariable()) {
        auto globvar  = obj->asGlobalVariable();
        auto initData = globvar->type()->tryGetElementType();
        println("   .type %s, %%object", globvar->name().data());
        if (initData->isArray()) {
            uint32_t size = 1;
            auto     e    = initData;
            while (e != nullptr && e->isArray()) {
                size *= e->asArrayType()->size();
                e     = e->tryGetElementType();
            }
            println("   .comm %s, %d, %d", globvar->name().data(), size * 4, 4);
        } else {
            println("   .data");
            println("   .globl %s", globvar->name().data());
            println("   .p2align 2");
            println("%s:", globvar->name().data());
            //! TODO: float global var
            assert(!initData->isFloat());
            if (initData->isInteger()) {
                println(
                    "   .long   %d",
                    static_cast<const ConstantInt *>(globvar->data())->value);
                println("   .size %s, 4\n", globvar->name().data());
            }
        }
    }
}

void Generator::genUsedGlobVars() {
    for (auto e : *generator_.usedGlobalVars) {
        println("%s:", e.second.data());
        println("   .long %s", e.first->val->name().data());
    }
    println("");
}

void Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    genGlobalDef(func);
    int cnt = 0;
    for (auto block : func->basicBlocks()) {
        println("@ %%bb.%d:", cnt++); // label
        generator_.cur_block = block;
        genInstList(&block->instructions());
    }
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
    sprintf(str, "GlobAddr%d", nrGlobVar);
    generator_.usedGlobalVars->insert({var, str});
}

void Generator::saveCallerReg() {
    RegList regList;
    assert(generator_.allocator->usedRegs.empty());
    generator_.allocator->getUsedRegs(generator_.cur_func->basicBlocks());
    auto usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    cgPush(regList);
}

void Generator::restoreCallerReg() {
    RegList regList;
    auto    usedRegs = generator_.allocator->usedRegs;
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    cgPop(regList);
}

void Generator::genInstList(InstructionList *instlist) {
    int  allocaSize = 0;
    bool flag       = false;
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
            saveCallerReg();
            cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, allocaSize);
        }
        genInst(inst);
    }
}

void Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    assert(instID != InstructionID::Alloca);
    generator_.allocator->updateAllocation(
        this, generator_.cur_block, generator_.allocator->cur_inst);
    switch (inst->id()) {
        case InstructionID::Alloca:
            genAllocaInst(inst->asAlloca());
            break;
        case InstructionID::Load:
            genLoadInst(inst->asLoad());
            break;
        case InstructionID::Store:
            genStoreInst(inst->asStore());
            break;
        case InstructionID::Ret:
            genRetInst(inst->asRet());
            break;
        case InstructionID::Br:
            genBrInst(inst->asBr());
        case InstructionID::GetElementPtr:
            genGetElemPtrInst(inst->asGetElementPtr());
            break;
        case InstructionID::Add:
            genAddInst(inst->asAdd());
            break;
        case InstructionID::Sub:
            genSubInst(inst->asSub());
            break;
        case InstructionID::Mul:
            genMulInst(inst->asMul());
            break;
        case InstructionID::UDiv:
            genUDivInst(inst->asUDiv());
            break;
        case InstructionID::SDiv:
            genSDivInst(inst->asSDiv());
            break;
        case InstructionID::URem:
            genURemInst(inst->asURem());
            break;
        case InstructionID::SRem:
            genSRemInst(inst->asSRem());
            break;
        case InstructionID::FNeg:
            genFNegInst(inst->asFNeg());
            break;
        case InstructionID::FAdd:
            genFAddInst(inst->asFAdd());
            break;
        case InstructionID::FSub:
            genFSubInst(inst->asFSub());
            break;
        case InstructionID::FMul:
            genFMulInst(inst->asFMul());
            break;
        case InstructionID::FDiv:
            genFDivInst(inst->asFDiv());
            break;
        case InstructionID::FRem:
            genFRemInst(inst->asFRem());
            break;
        case InstructionID::Shl:
            genShlInst(inst->asShl());
            break;
        case InstructionID::LShr:
            genLShrInst(inst->asLShr());
            break;
        case InstructionID::AShr:
            genAShrInst(inst->asAShr());
            break;
        case InstructionID::And:
            genAndInst(inst->asAnd());
            break;
        case InstructionID::Or:
            genOrInst(inst->asOr());
            break;
        case InstructionID::Xor:
            genXorInst(inst->asXor());
            break;
        case InstructionID::FPToUI:
            genFPToUIInst(inst->asFPToUI());
            break;
        case InstructionID::FPToSI:
            genFPToSIInst(inst->asFPToSI());
            break;
        case InstructionID::UIToFP:
            genUIToFPInst(inst->asUIToFP());
            break;
        case InstructionID::SIToFP:
            genSIToFPInst(inst->asSIToFP());
            break;
        case InstructionID::ICmp:
            genICmpInst(inst->asICmp());
            break;
        case InstructionID::FCmp:
            genFCmpInst(inst->asFCmp());
            break;
        case InstructionID::Phi:
            genPhiInst(inst->asPhi());
            break;
        case InstructionID::Call:
            genCallInst(inst->asCall());
            break;
        default:
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            exit(-1);
    }
}

int Generator::genAllocaInst(AllocaInst *inst) {
    int          totalSize;
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

void Generator::genLoadInst(LoadInst *inst) {
    auto           targetVar = findVariable(inst->unwrap());
    auto           source    = inst->useAt(0);
    int            offset;
    ARMGeneralRegs sourceReg;
    if (source->isLabel()) {
        //! TODO: label
        assert(0);
    } else {
        assert(source->type()->isPointer());
        auto sourceVar = findVariable(source);

        if (sourceVar->is_alloca) {
            sourceReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize
                   - generator_.stack->lookupOnStackVar(sourceVar);
        } else if (sourceVar->is_global) {
            assert(sourceVar->reg == ARMGeneralRegs::None);
            addUsedGlobalVar(sourceVar);
            cgLdr(targetVar->reg, sourceVar);
            sourceReg = targetVar->reg;
            offset    = 0;
        }
        cgLdr(targetVar->reg, sourceReg, offset);
    }
}

void Generator::genStoreInst(StoreInst *inst) {
    auto           target = inst->useAt(0);
    auto           source = inst->useAt(1);
    ARMGeneralRegs sourceReg, targetReg;
    int            offset;
    if (source.value()->isConstant()) {
        if (source.value()->type()->isInteger()) {
            sourceReg   = generator_.allocator->allocateRegister();
            int32_t imm = static_cast<ConstantInt *>(source.value())->value;
            if (sourceReg == ARMGeneralRegs::None) {
                //! TODO: spill something
                assert(0 && "TODO!");
            }
            cgMov(sourceReg, imm);
        } else if (source.value()->type()->isFloat()) {
            //! TODO: float
            assert(0);
        }
    } else {
        auto srcVar = findVariable(source);
        if (srcVar->is_spilled) {
            //! TODO: handle spilled variable
            assert(0);
        }
        sourceReg = srcVar->reg;
    }
    auto tarVar = findVariable(target);
    if (tarVar->is_global) {
        //! TODO: global variable
        targetReg = generator_.allocator->allocateRegister();
        addUsedGlobalVar(tarVar);
        if (targetReg == ARMGeneralRegs::None) {
            //! TODO: spill something
            assert(0 && "TODO!");
        }
        cgLdr(targetReg, tarVar);
        cgStr(sourceReg, targetReg, 0);
    } else {
        if (tarVar->is_alloca) {
            targetReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize
                   - generator_.stack->lookupOnStackVar(tarVar);
        }
        cgStr(sourceReg, targetReg, offset);
    }
    if (source.value()->isConstant())
        generator_.allocator->releaseRegister(sourceReg);
}

void Generator::genRetInst(RetInst *inst) {
    if (inst->totalOperands() != 0) {
        auto operand = inst->useAt(0).value();
        if (operand->isImmediate()) {
            assert(operand->type()->isInteger());
            cgMov(
                ARMGeneralRegs::R0, static_cast<ConstantInt *>(operand)->value);
        } else {
            assert(Allocator::isVariable(operand));
            auto var = findVariable(operand);
            if (var->reg != ARMGeneralRegs::R0)
                cgMov(ARMGeneralRegs::R0, var->reg);
        }
    }
    if (generator_.stack->stackSize > 0)
        cgAdd(
            ARMGeneralRegs::SP,
            ARMGeneralRegs::SP,
            generator_.stack->stackSize);
    restoreCallerReg();
    cgBx(ARMGeneralRegs::LR);
}

void Generator::genBrInst(BrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genGetElemPtrInst(GetElementPtrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAddInst(AddInst *inst) {
    ARMGeneralRegs rd  = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs  = findVariable(inst->useAt(0))->reg;
    auto           op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        cgAdd(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgAdd(rd, rs, imm);
    }
}

void Generator::genSubInst(SubInst *inst) {
    ARMGeneralRegs rd  = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs  = findVariable(inst->useAt(0))->reg;
    auto           op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        cgSub(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        cgSub(rd, rs, imm);
    }
}

void Generator::genMulInst(MulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genUDivInst(UDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSDivInst(SDivInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genURemInst(URemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSRemInst(SRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFNegInst(FNegInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFAddInst(FAddInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFSubInst(FSubInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFMulInst(FMulInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFDivInst(FDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    assert(0 && "unfinished yet!\n");
}

void Generator::genFRemInst(FRemInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genShlInst(ShlInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genLShrInst(LShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAShrInst(AShrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genAndInst(AndInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genOrInst(OrInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genXorInst(XorInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFPToUIInst(FPToUIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFPToSIInst(FPToSIInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genUIToFPInst(UIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genSIToFPInst(SIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genICmpInst(ICmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genPhiInst(PhiInst *inst) {
    assert(0 && "unfinished yet!\n");
}

void Generator::genCallInst(CallInst *inst) {
    for (int i = 0; i < inst->totalParams(); i++) {
        if (i < 4) {
            assert(!generator_.allocator->regAllocatedMap[i]);
            if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
                assert(!inst->paramAt(i)->type()->isFloat());
                auto constant = inst->paramAt(i)->asConstantData();
                cgMov(
                    static_cast<ARMGeneralRegs>(i),
                    static_cast<ConstantInt *>(constant)->value);
            } else {
                assert(Allocator::isVariable(inst->paramAt(i)));
                Variable *var = findVariable(inst->paramAt(i));
                if (var->reg != static_cast<ARMGeneralRegs>(i)) {
                    assert(var->reg != ARMGeneralRegs::None);
                    cgMov(static_cast<ARMGeneralRegs>(i), var->reg);
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
        generator_.allocator->regAllocatedMap[0] = true;
        auto var                                 = findVariable(inst->unwrap());
        var->reg                                 = ARMGeneralRegs::R0;
    }
    cgBl(inst->callee()->asFunction());
}

void Generator::cgMov(ARMGeneralRegs rd, ARMGeneralRegs rs) {
    println("    mov    %s, %s", reg2str(rd), reg2str(rs));
}

void Generator::cgMov(ARMGeneralRegs rd, int32_t imm) {
    println("    mov    %s, #%d", reg2str(rd), imm);
}

void Generator::cgLdr(ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(src), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(src));
    println("    ldr    %s, %s", reg2str(dst), tmpStr);
}

void Generator::cgLdr(ARMGeneralRegs dst, Variable *var) {
    assert(var->is_global);
    auto it = generator_.usedGlobalVars->find(var);
    if (it == generator_.usedGlobalVars->end())
        assert(0 && "it must be an error here.");
    println("    ldr    %s, %s", reg2str(dst), it->second.data());
}

void Generator::cgStr(ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset) {
    static char tmpStr[10];
    if (offset != 0)
        sprintf(tmpStr, "[%s, #%d]", reg2str(dst), offset);
    else
        sprintf(tmpStr, "[%s]", reg2str(dst));
    println("    str    %s, %s", reg2str(src), tmpStr);
}

void Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    add    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgAdd(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    add    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    println("    sub    %s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

void Generator::cgSub(ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    println("    sub    %s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

void Generator::cgBx(ARMGeneralRegs rd) {
    println("    bx     %s", reg2str(rd));
}

void Generator::cgBl(Function *callee) {
    println("    bl     %s", callee->name().data());
}

void Generator::cgPush(RegList &reglist) {
    printf("    push   {");
    for (auto reg : reglist) {
        printf("%s", reg2str(reg));
        if (reg != reglist.tail()->value()) printf(", ");
    }
    println("}");
}

void Generator::cgPop(RegList &reglist) {
    printf("    pop    {");
    for (auto reg : reglist) {
        printf("%s", reg2str(reg));
        if (reg != reglist.tail()->value()) printf(", ");
    }
    println("}");
}

} // namespace slime::backend