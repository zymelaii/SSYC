#include "14.h"
#include "16.h"

#include "21.h"
#include "46.h"
#include "49.h"
#include "53.h"
#include "51.h"
#include "47.h"
#include "88.h"
#include <set>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
    modulecode += sprintln("    .arch armv7a");
    modulecode += sprintln("    .text");
    for (auto e : *module) {
        if (e->type()->isFunction()) {
            generator_.allocator->initAllocator();
            generator_.cur_func = static_cast<Function *>(e);
            if (libfunc.find(e->name().data()) != libfunc.end()) continue;
            modulecode += genAssembly(static_cast<Function *>(e));
            generator_.cur_funcnum++;
            modulecode += sprintln("    .pool\n");
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

const char *Generator::reg2str(ARMFloatRegs reg) {
    switch (reg) {
        case ARMFloatRegs::S0:
            return "s0";
        case ARMFloatRegs::S1:
            return "s1";
        case ARMFloatRegs::S2:
            return "s2";
        case ARMFloatRegs::S3:
            return "s3";
        case ARMFloatRegs::S4:
            return "s4";
        case ARMFloatRegs::S5:
            return "s5";
        case ARMFloatRegs::S6:
            return "s6";
        case ARMFloatRegs::S7:
            return "s7";
        case ARMFloatRegs::S8:
            return "s8";
        case ARMFloatRegs::S9:
            return "s9";
        case ARMFloatRegs::S10:
            return "s10";
        case ARMFloatRegs::S11:
            return "s11";
        case ARMFloatRegs::S12:
            return "s12";
        case ARMFloatRegs::S13:
            return "s13";
        case ARMFloatRegs::S14:
            return "s14";
        case ARMFloatRegs::S15:
            return "s15";
        case ARMFloatRegs::S16:
            return "s16";
        case ARMFloatRegs::S17:
            return "s17";
        case ARMFloatRegs::S18:
            return "s18";
        case ARMFloatRegs::S19:
            return "s19";
        case ARMFloatRegs::S20:
            return "s20";
        case ARMFloatRegs::S21:
            return "s21";
        case ARMFloatRegs::S22:
            return "s22";
        case ARMFloatRegs::S23:
            return "s23";
        case ARMFloatRegs::S24:
            return "s24";
        case ARMFloatRegs::S25:
            return "s25";
        case ARMFloatRegs::S26:
            return "s26";
        case ARMFloatRegs::S27:
            return "s27";
        case ARMFloatRegs::S28:
            return "s28";
        case ARMFloatRegs::S29:
            return "s29";
        case ARMFloatRegs::S30:
            return "s30";
        case ARMFloatRegs::S31:
            return "s31";
        case ARMFloatRegs::None:
            fprintf(stderr, "Invalid Register!");
            exit(-1);
    }
}

std::string Generator::genGlobalArrayInitData(
    ConstantArray *globarr, uint32_t baseSize) {
    std::string body;
    if (baseSize == 0) { return body; }

    auto type     = globarr->type()->asArrayType();
    auto dataType = type->elementType();

    if (globarr->size() == 0) {
        body = sprintln("    .zero %d", baseSize * type->size());
        return body;
    }

    auto left = type->size() - globarr->size();

    if (dataType->isArray()) {
        for (int i = 0; i < globarr->size(); ++i) {
            body += genGlobalArrayInitData(
                static_cast<ConstantArray *>(globarr->at(i)),
                baseSize / dataType->asArrayType()->size());
        }
        if (left > 0) { body += sprintln("    .zero %d", baseSize * left); }
        return body;
    }

    if (auto type = dataType->tryIntoIntegerType()) {
        if (type->isI32()) {
            for (int i = 0; i < globarr->size(); ++i) {
                uint32_t value =
                    static_cast<ConstantInt *>(globarr->at(i))->value;
                body += sprintln("    .long %d", value);
            }
        } else if (type->isI8()) {
            if (left > 8) {
                for (int i = 0; i < globarr->size(); ++i) {
                    uint32_t value =
                        static_cast<ConstantInt *>(globarr->at(i))->value;
                    body += sprintln("    .byte %d", value);
                }
            } else {
                std::string ascii;
                for (int i = 0; i < globarr->size(); ++i) {
                    uint32_t value =
                        static_cast<ConstantInt *>(globarr->at(i))->value;
                    if (value == '\\') {
                        ascii += "\\\\";
                    } else if (value >= 0x20 && value <= 0x7e) {
                        ascii += static_cast<char>(value);
                    } else {
                        char octal[5]{};
                        sprintf(octal, "\\%03o", value);
                        ascii += octal;
                    }
                }
                for (int i = 0; i < left; ++i) { ascii += "\\000"; }
                body += sprintln("    .asciz \"%s\"", ascii.c_str());
                left = 0;
            }
        } else {
            unreachable();
        }
        if (left > 0) { body += sprintln("    .zero %d", baseSize * left); }
        return body;
    }

    if (dataType->isFloat()) {
        for (int i = 0; i < globarr->size(); ++i) {
            float value = static_cast<ConstantFloat *>(globarr->at(i))->value;
            body        += sprintln(
                "    .long 0x%08x", *reinterpret_cast<uint32_t *>(&value));
        }
        if (left > 0) { body += sprintln("    .zero %d", baseSize * left); }
        return body;
    }

    if (auto type = dataType->asPointerType()) {
        //! TODO: complete pointer data
        unreachable();
        if (left > 0) { body += sprintln("    .zero %d", baseSize * left); }
        return body;
    }

    unreachable();
}

std::string Generator::genGlobalDef(GlobalObject *obj) {
    std::string globdefs;
    globdefs += sprintln("    .globl %s", obj->name().data());

    if (auto func = obj->tryIntoFunction()) {
        globdefs += sprintln("    .type %s, %%function", func->name().data());
        globdefs += sprintln("    .p2align 1");
        globdefs += sprintln("%s:", func->name().data());
        return globdefs;
    }

    if (auto var = obj->tryIntoGlobalVariable()) {
        std::string body;
        int         align = -1; //<! no alignment
        int         size  = 0;
        auto        type  = var->type()->tryGetElementType();
        if (auto arrayType = type->tryIntoArrayType()) {
            Type *tmp = arrayType;
            size      = 1;
            while (tmp->isArray()) {
                size *= tmp->asArrayType()->size();
                tmp  = tmp->tryGetElementType();
            }
            if (auto type = tmp->tryIntoIntegerType()) {
                if (type->isI32()) {
                    size  *= 4;
                    align = 2;
                }
            } else if (tmp->isFloat()) {
                size  *= 4;
                align = 2;
            } else if (tmp->isPointer()) {
                size  *= 4;
                align = 2;
            } else {
                unreachable();
            }
            auto data = static_cast<ConstantArray *>(
                const_cast<ConstantData *>(var->data()));
            body = genGlobalArrayInitData(
                data, arrayType->size() == 0 ? size : size / arrayType->size());
        } else if (type->isBuiltinType()) {
            if (auto intType = type->tryIntoIntegerType()) {
                uint32_t value =
                    static_cast<const ConstantInt *>(var->data())->value;
                if (intType->isI8()) {
                    size = 1;
                    body = sprintln("    .byte %d", value);
                } else {
                    assert(intType->isI32());
                    align = 2;
                    size  = 4;
                    body  = sprintln("    .long %d", value);
                }
            } else if (type->isFloat()) {
                size  = 4;
                align = 2;
                float value =
                    static_cast<const ConstantFloat *>(var->data())->value;
                body = sprintln(
                    "    .long 0x%08x", *reinterpret_cast<uint32_t *>(&value));
            } else if (type->isPointer()) {
                //! TODO: pointer data
                unreachable();
            }
        } else {
            //! TODO: handle pointer type
            unreachable();
        }

        globdefs += sprintln("    .type %s, %%object", var->name().data());
        globdefs += sprintln("    .data");
        if (align != -1) { globdefs += sprintln("    .p2align %d", align); }
        globdefs += sprintln("%s:", var->name().data());
        globdefs += body;
        // globdefs += sprintln("    .size %d", size);
        globdefs += sprintln("");
        return globdefs;
    }

    unreachable();
}

std::string Generator::genUsedGlobVars() {
    std::string globvars;
    for (auto e : *generator_.usedGlobalVars) {
        globvars += sprintln("%s:", e.second.data());
        globvars += sprintln("    .long %s", e.first->val->name().data());
    }
    globvars += sprintln("");
    return globvars;
}

std::string Generator::genAssembly(Function *func) {
    generator_.allocator->computeInterval(func);
    std::string   funccode;
    BlockCodeList blockCodeList;

    funccode += genGlobalDef(func);
    for (auto block : func->basicBlocks()) {
        BlockCode *blockcodes = new BlockCode;
        blockcodes->code      += sprintln(
            ".F%dBB.%d:",
            generator_.cur_funcnum,
            getBlockNum(block->id())); // label
        generator_.cur_block  = block;
        blockcodes->instcodes = genInstList(&block->instructions());
        blockCodeList.insertToTail(blockcodes);
    }
    checkStackChanges(blockCodeList);
    if (func->totalParams() > 4) {
        funccode += cgMov(ARMGeneralRegs::R11, ARMGeneralRegs::SP);
    }
    funccode += saveCallerReg();
    funccode += unpackBlockCodeList(blockCodeList);
    return funccode;
}

Variable *Generator::findVariable(Value *val) {
    auto blockVarTable = generator_.allocator->blockVarTable;
    auto valVarTable   = blockVarTable->find(generator_.cur_block)->second;
    auto it            = valVarTable->find(val);
    if (it != valVarTable->end()) { return it->second; }
    //! use of a anonymous variable (like string literal)
    assert(val->isReadOnly());
    return generator_.allocator->createVariable(val);
}

bool Generator::isImmediateValid(uint32_t imm) {
    uint32_t bitmask = 1;
    uint32_t cnt     = 0;
    int      i, j;
    if (imm <= 255) return true;
    for (i = 0; i < 31; i++) {
        if (imm & (bitmask << i))
            break;
        else
            cnt++;
    }
    bitmask = 0x80000000;
    for (j = 0; j < 31 - i; j++) {
        if (imm & (bitmask >> j))
            break;
        else
            cnt++;
    }
    if (cnt < 24)
        return false;
    else {
        uint32_t rotate_cnt = 0;
        while (imm >> 8 != 0) {
            imm        = (imm >> 2) | (imm << 30);
            rotate_cnt += 2;
            if (rotate_cnt > 30) return false;
        }
        return true;
    }
}

void Generator::checkStackChanges(BlockCodeList &blockCodeList) {
    auto it = blockCodeList.begin();
    while (it != blockCodeList.end()) {
        auto it2 = (*it)->instcodes->begin();
        while (it2 != (*it)->instcodes->end()) {
            auto inst = (*it2)->inst;
            if ((*it2)->inst->id() == InstructionID::Ret) {
                (*it2)->code = (*it2)->code + restoreCallerReg();
            }
            it2++;
        }
        it++;
    }
    // TODO: 处理通过栈传入的函数参数
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
    if (generator_.allocator->max_funcargs > 4)
        usedRegs.insert(ARMGeneralRegs::R11);
    usedRegs.erase(ARMGeneralRegs::R0);
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    return cgPush(regList);
}

std::string Generator::restoreCallerReg() {
    RegList regList;
    auto    usedRegs = generator_.allocator->usedRegs;
    if (generator_.allocator->max_funcargs > 4)
        usedRegs.insert(ARMGeneralRegs::R11);
    usedRegs.erase(ARMGeneralRegs::R0);
    for (auto reg : usedRegs) { regList.insertToTail(reg); }
    regList.insertToTail(ARMGeneralRegs::LR);
    return cgPop(regList) + cgBx(ARMGeneralRegs::LR);
}

std::string Generator::unpackInstCodeList(InstCodeList &instCodeList) {
    static std::string unpackedInstcodes;
    unpackedInstcodes.clear();
    auto it = instCodeList.begin();
    while (it != instCodeList.end()) {
        unpackedInstcodes += (*it)->code;
        it++;
    }
    return unpackedInstcodes;
}

std::string Generator::unpackBlockCodeList(BlockCodeList &blockCodeList) {
    static std::string unpackedBlockcodes;
    unpackedBlockcodes.clear();
    auto it = blockCodeList.begin();
    while (it != blockCodeList.end()) {
        unpackedBlockcodes += (*it)->code;
        unpackedBlockcodes += unpackInstCodeList(*(*it)->instcodes);
        it++;
    }
    return unpackedBlockcodes;
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

void Generator::attachIRInformation(InstCode *instcode) {
    assert(instcode != nullptr);
    assert(instcode->inst != nullptr);
    if (instcode->code.empty()) { return; }

    auto       inst = instcode->inst;
    const auto id   = inst->unwrap()->id();

    constexpr auto align = 40;
    char           ident[32]{};

    do {
        if (id != 0) {
            sprintf(ident, "%%%d", id);
            break;
        }

        assert(inst->unwrap()->type()->isVoid());
        const char *name = getInstructionName(inst).data();

        if (inst->id() == InstructionID::Store) {
            sprintf(ident, "%s %%%d", name, inst->asStore()->lhs()->id());
            break;
        }

        if (inst->id() == InstructionID::Ret) {
            if (auto retval = inst->asRet()->operand(); !retval) {
                sprintf(ident, "%s", name);
            } else if (retval->isImmediate()) {
                sprintf(ident, "%s $", name);
            } else {
                sprintf(ident, "%s %%%d", name, retval->id());
            }
            break;
        }

        if (inst->id() == InstructionID::Br) {
            auto block = inst->asBr()->parent();
            assert(!block->isTerminal());
            assert(block->isLinear() || block->isBranched());
            if (block->isLinear()) {
                sprintf(ident, "%s <%%%d>", name, block->branch()->id());
            } else if (auto control = block->control();
                       control->isImmediate()) {
                sprintf(
                    ident,
                    "%s $, <%%%d>, <%%%d>",
                    name,
                    block->branch()->id(),
                    block->branchElse()->id());
            } else {
                sprintf(
                    ident,
                    "%s %%%d, <%%%d>, <%%%d>",
                    name,
                    block->control()->id(),
                    block->branch()->id(),
                    block->branchElse()->id());
            }
            break;
        }

        if (inst->id() == InstructionID::Call) {
            auto fn = inst->asCall()->callee();
            assert(fn->isFunction());
            sprintf(ident, "%s %s", name, fn->name().data());
            break;
        }

        unreachable();
    } while (0);

    auto            &code = instcode->code;
    std::vector<int> linePos{0};
    auto             pos = code.find_first_of('\n');
    assert(pos != code.npos);
    while (pos + 1 != code.size()) {
        linePos.push_back(pos + 1);
        pos = code.find_first_of('\n', pos + 1);
    }

    int lineInIndex = -1;
    int lineOut     = linePos.back();
    for (int i = 0; i < linePos.size(); ++i) {
        if (code[linePos[i]] != '#') {
            lineInIndex = i;
            break;
        }
    }
    assert(lineInIndex != -1);
    assert(linePos[lineInIndex] <= lineOut);

    auto lineOutOpcode = code.substr(lineOut, code.size() - lineOut - 1);
    code.erase(lineOut);
    code += sprintln("%-*s@ --> %s", align, lineOutOpcode.c_str(), ident);

    if (auto lineIn = linePos[lineInIndex]; lineIn != lineOut) {
        assert(lineInIndex + 1 < linePos.size());
        auto endpos       = linePos[lineInIndex + 1] - 1;
        auto lineInOpcode = code.substr(lineIn, endpos - lineIn);
        code.erase(lineIn, endpos - lineIn + 1);
        code.insert(
            lineIn,
            sprintln("%-*s@ <-- %s", align, lineInOpcode.c_str(), ident));
    }
}

InstCodeList *Generator::genInstList(InstructionList *instlist) {
    InstCodeList *instCodeList = new InstCodeList();
    int           allocaSize   = 0;
    bool          flag         = false;
    InstCode     *allocacode   = new InstCode(nullptr);
    for (auto inst : *instlist) {
        generator_.allocator->cur_inst++;
        if (inst->id() == InstructionID::Alloca) {
            allocaSize       += genAllocaInst(inst->asAlloca());
            allocacode->inst = inst;
            flag             = true;
            continue;
        } else if (
            (flag || generator_.allocator->cur_inst == 1)
            && inst->id() != InstructionID::Alloca) {
            flag = false;
            if (inst->parent()
                    == generator_.cur_func->basicBlocks().head()->value()
                && allocaSize != 0) {
                if (!isImmediateValid(allocaSize)) {
                    ARMGeneralRegs tmpreg =
                        generator_.allocator->allocateRegister();
                    assert(tmpreg != ARMGeneralRegs::None);
                    allocacode->code += cgLdr(tmpreg, allocaSize);
                    allocacode->code +=
                        cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, tmpreg);
                    generator_.allocator->releaseRegister(tmpreg);
                } else
                    allocacode->code += cgSub(
                        ARMGeneralRegs::SP, ARMGeneralRegs::SP, allocaSize);
                instCodeList->insertToTail(allocacode);
            }
        }
        instCodeList->insertToTail(genInst(inst));
    }
    return instCodeList;
}

InstCode *Generator::genInst(Instruction *inst) {
    InstructionID instID = inst->id();
    assert(instID != InstructionID::Alloca);
    InstCode *instcode = nullptr;
    InstCode  tmpcode(nullptr);
    generator_.allocator->updateAllocation(
        this, &tmpcode, generator_.cur_block, inst);

    switch (instID) {
        case InstructionID::Alloca: {
            assert(false && "unexpected alloca");
            unreachable();
        } break;
        case InstructionID::Load: {
            instcode = genLoadInst(inst->asLoad());
        } break;
        case InstructionID::Store: {
            instcode = genStoreInst(inst->asStore());
        } break;
        case InstructionID::Ret: {
            instcode = genRetInst(inst->asRet());
        } break;
        case InstructionID::Br: {
            instcode = genBrInst(inst->asBr());
        } break;
        case InstructionID::GetElementPtr: {
            instcode = genGetElemPtrInst(inst->asGetElementPtr());
        } break;
        case InstructionID::Add: {
            instcode = genAddInst(inst->asAdd());
        } break;
        case InstructionID::Sub: {
            instcode = genSubInst(inst->asSub());
        } break;
        case InstructionID::Mul: {
            instcode = genMulInst(inst->asMul());
        } break;
        case InstructionID::UDiv: {
            instcode = genUDivInst(inst->asUDiv());
        } break;
        case InstructionID::SDiv: {
            instcode = genSDivInst(inst->asSDiv());
        } break;
        case InstructionID::URem: {
            instcode = genURemInst(inst->asURem());
        } break;
        case InstructionID::SRem: {
            instcode = genSRemInst(inst->asSRem());
        } break;
        case InstructionID::FNeg: {
            instcode = genFNegInst(inst->asFNeg());
        } break;
        case InstructionID::FAdd: {
            instcode = genFAddInst(inst->asFAdd());
        } break;
        case InstructionID::FSub: {
            instcode = genFSubInst(inst->asFSub());
        } break;
        case InstructionID::FMul: {
            instcode = genFMulInst(inst->asFMul());
        } break;
        case InstructionID::FDiv: {
            instcode = genFDivInst(inst->asFDiv());
        } break;
        case InstructionID::FRem: {
            instcode = genFRemInst(inst->asFRem());
        } break;
        case InstructionID::Shl: {
            instcode = genShlInst(inst->asShl());
        } break;
        case InstructionID::LShr: {
            instcode = genLShrInst(inst->asLShr());
        } break;
        case InstructionID::AShr: {
            instcode = genAShrInst(inst->asAShr());
        } break;
        case InstructionID::And: {
            instcode = genAndInst(inst->asAnd());
        } break;
        case InstructionID::Or: {
            instcode = genOrInst(inst->asOr());
        } break;
        case InstructionID::Xor: {
            instcode = genXorInst(inst->asXor());
        } break;
        case InstructionID::FPToUI: {
            instcode = genFPToUIInst(inst->asFPToUI());
        } break;
        case InstructionID::FPToSI: {
            instcode = genFPToSIInst(inst->asFPToSI());
        } break;
        case InstructionID::UIToFP: {
            instcode = genUIToFPInst(inst->asUIToFP());
        } break;
        case InstructionID::SIToFP: {
            instcode = genSIToFPInst(inst->asSIToFP());
        } break;
        case InstructionID::ICmp: {
            instcode = genICmpInst(inst->asICmp());
        } break;
        case InstructionID::FCmp: {
            instcode = genFCmpInst(inst->asFCmp());
        } break;
        case InstructionID::ZExt: {
            instcode = genZExtInst(inst->asZExt());
        } break;
        case InstructionID::Call: {
            instcode = genCallInst(inst->asCall());
        } break;
        default: {
            fprintf(stderr, "Unkown ir inst type:%d!\n", instID);
            unreachable();
        } break;
    }
    assert(instcode != nullptr);

    generator_.allocator->checkLiveInterval(&instcode->code);
    if (!tmpcode.code.empty()) {
        instcode->code = tmpcode.code + instcode->code;
    }

    attachIRInformation(instcode);
    return instcode;
}

int Generator::genAllocaInst(AllocaInst *inst) {
    Instruction *pinst = inst;
    auto         e     = inst->unwrap()->type()->tryGetElementType();
    int          size  = 1;
    while (e != nullptr && e->isArray()) {
        size *= e->asArrayType()->size();
        e    = e->tryGetElementType();
    }
    auto var       = findVariable(inst->unwrap());
    var->is_alloca = true;
    generator_.stack->spillVar(var, size * 4);
    return size * 4;
}

InstCode *Generator::genLoadInst(LoadInst *inst) {
    auto           targetVar = findVariable(inst->unwrap());
    auto           source    = inst->useAt(0);
    int            offset;
    InstCode      *loadcode = new InstCode(inst);
    ARMGeneralRegs sourceReg, tmpreg = ARMGeneralRegs::None;
    if (source->isLabel()) {
        //! TODO: label
        assert(0);
    } else {
        assert(source->type()->isPointer());
        auto sourceVar = findVariable(source);

        if (sourceVar->is_alloca) {
            if (sourceVar->is_funcparam) {
                sourceReg = ARMGeneralRegs::R11;
                offset    = sourceVar->stackpos;
            } else {
                sourceReg = ARMGeneralRegs::SP;
                offset    = generator_.stack->stackSize - sourceVar->stackpos;
            }
        } else if (sourceVar->is_global) {
            addUsedGlobalVar(sourceVar);
            offset         = 0;
            loadcode->code += cgLdr(targetVar->reg, sourceVar);
            sourceReg      = targetVar->reg;
            // if (sourceVar->reg == ARMGeneralRegs::None) {
            //     cgLdr(targetVar->reg, sourceVar);
            //     sourceReg = targetVar->reg;
            // } else
            //     sourceReg = sourceVar->reg;
        } else {
            sourceReg = sourceVar->reg;
            offset    = 0;
        }

        if (!isImmediateValid(offset)) {
            //! TODO: simpify invalid immediate number
            // by using "LDR rd [rs, rn , lsl #imm]"
            auto whitelist = generator_.allocator->getInstOperands(inst);
            tmpreg         = generator_.allocator->allocateRegister(
                true, whitelist, this, loadcode);
            loadcode->code += cgLdr(tmpreg, offset);
            loadcode->code += cgLdr(targetVar->reg, sourceReg, tmpreg);
        } else
            loadcode->code += cgLdr(targetVar->reg, sourceReg, offset);
    }

    if (tmpreg != ARMGeneralRegs::None)
        generator_.allocator->releaseRegister(tmpreg);
    return loadcode;
}

InstCode *Generator::genStoreInst(StoreInst *inst) {
    auto           target = inst->useAt(0);
    auto           source = inst->useAt(1);
    ARMGeneralRegs sourceReg, targetReg;
    int            offset;
    InstCode      *storecode = new InstCode(inst);
    auto           whitelist = generator_.allocator->getInstOperands(inst);

    ARMGeneralRegs tmpreg = ARMGeneralRegs::None;
    if (source.value()->isConstant()) {
        if (source.value()->type()->isInteger()) {
            sourceReg = generator_.allocator->allocateRegister(
                true, whitelist, this, storecode);
            int32_t imm     = static_cast<ConstantInt *>(source.value())->value;
            storecode->code += cgLdr(sourceReg, imm);
        } else if (source.value()->type()->isFloat()) {
            //! TODO: float
            assert(0);
        }
    } else {
        auto sourceVar = findVariable(source);
        if (sourceVar->is_alloca) {
            //! NOTE: only funcparams could reach here
            assert(sourceVar->is_funcparam);
            int32_t offset = sourceVar->stackpos;
            tmpreg         = generator_.allocator->allocateRegister(
                true, whitelist, this, storecode);
            storecode->code += cgLdr(tmpreg, ARMGeneralRegs::R11, offset);
            sourceReg       = tmpreg;
        } else
            sourceReg = sourceVar->reg;
    }
    auto targetVar = findVariable(target);
    if (targetVar->is_global) {
        assert(targetVar->reg != ARMGeneralRegs::None);
        if (targetVar->reg != ARMGeneralRegs::None) {
            targetReg = targetVar->reg;
            addUsedGlobalVar(targetVar);
            storecode->code += cgLdr(targetReg, targetVar);
            storecode->code += cgStr(sourceReg, targetReg, 0);
            generator_.allocator->releaseRegister(targetVar);
        }
    } else {
        if (targetVar->is_alloca) {
            //! NOTE: funcparams can't reach here
            assert(!targetVar->is_funcparam);
            targetReg = ARMGeneralRegs::SP;
            offset    = generator_.stack->stackSize - targetVar->stackpos;
        } else {
            targetReg = targetVar->reg;
            offset    = 0;
        }
        if (!isImmediateValid(offset)) {
            auto tmpreg2 = generator_.allocator->allocateRegister(
                true, whitelist, this, storecode);
            storecode->code += cgLdr(tmpreg2, offset);
            storecode->code += cgStr(sourceReg, targetReg, tmpreg2);
            generator_.allocator->releaseRegister(tmpreg2);
        } else
            storecode->code += cgStr(sourceReg, targetReg, offset);
    }
    if (source.value()->isConstant())
        generator_.allocator->releaseRegister(sourceReg);
    if (tmpreg != ARMGeneralRegs::None)
        generator_.allocator->releaseRegister(tmpreg);
    return storecode;
}

InstCode *Generator::genRetInst(RetInst *inst) {
    InstCode *retcode = new InstCode(inst);
    if (inst->totalOperands() != 0 && inst->useAt(0).value()) {
        auto operand = inst->useAt(0).value();
        if (operand->isImmediate()) {
            assert(operand->type()->isInteger());
            retcode->code += cgLdr(
                ARMGeneralRegs::R0, static_cast<ConstantInt *>(operand)->value);
        } else {
            assert(Allocator::isVariable(operand));
            auto var = findVariable(operand);
            if (var->reg != ARMGeneralRegs::R0)
                retcode->code += cgMov(ARMGeneralRegs::R0, var->reg);
        }
    }
    if (generator_.stack->stackSize > 0) {
        if (!isImmediateValid(generator_.stack->stackSize)) {
            ARMGeneralRegs tmpreg = generator_.allocator->allocateRegister();
            assert(tmpreg != ARMGeneralRegs::None);
            retcode->code += cgLdr(tmpreg, generator_.stack->stackSize);
            retcode->code +=
                cgAdd(ARMGeneralRegs::SP, ARMGeneralRegs::SP, tmpreg);
            generator_.allocator->releaseRegister(tmpreg);
        } else
            retcode->code += cgAdd(
                ARMGeneralRegs::SP,
                ARMGeneralRegs::SP,
                generator_.stack->stackSize);
    }
    // caller registers' restoration will be handled in funciton 'genAssembly'
    return retcode;
}

InstCode *Generator::genBrInst(BrInst *inst) {
    InstCode   *brcode    = new InstCode(inst);
    BasicBlock *nextblock = getNextBlock();
    if (generator_.cur_block->isLinear()) {
        Value *target = inst->useAt(0);
        if (!nextblock || target->id() != nextblock->id())
            brcode->code += cgB(inst->useAt(0));
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
            brcode->code += cgTst(cond->reg, 1);
            brcode->code += cgB(target2, ComparePredicationType::EQ);
            brcode->code += cgB(target1);
            return brcode;
        }
        // if (nextblock->id() == target1->id()) {
        //     cgB(target2, ComparePredicationType::EQ);
        // } else if (nextblock->id() == target2->id()) {
        //     cgB(target1, ComparePredicationType::NE);
        // } else {
        brcode->code += cgB(target1, predict);
        brcode->code += cgB(target2);
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

InstCode *Generator::genGetElemPtrInst(GetElementPtrInst *inst) {
    //! NOTE: 2 more extra registers requried

    //! addr = base + i[0] * sizeof(type) + i[1] * sizeof(*type) + ...
    auto      baseType    = inst->op<0>()->type()->tryGetElementType();
    InstCode *getelemcode = new InstCode(inst);
    //! dest is initially assign to base addr
    auto dest = findVariable(inst->unwrap())->reg;
    if (auto var = findVariable(inst->op<0>()); var->is_alloca) {
        int            offset;
        ARMGeneralRegs sourceReg;
        if (var->is_funcparam) {
            offset    = var->stackpos;
            sourceReg = ARMGeneralRegs::R11;
        } else {
            offset    = generator_.stack->stackSize - var->stackpos;
            sourceReg = ARMGeneralRegs::SP;
        }
        if (!isImmediateValid(offset)) {
            getelemcode->code += cgLdr(dest, offset);
            getelemcode->code += cgAdd(dest, sourceReg, dest);
        } else {
            getelemcode->code += cgAdd(dest, sourceReg, offset);
        }
    } else if (var->is_global) {
        addUsedGlobalVar(var);
        getelemcode->code += cgLdr(dest, var);
    } else {
        assert(!var->is_spilled);
        assert(!inst->op<0>()->isImmediate());
        getelemcode->code += cgMov(dest, var->reg);
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
                    true, whitelist, this, getelemcode);
                tmp            = &regPlaceholder;
                tmpIsAllocated = true;
                if (multiplier > 1) { getelemcode->code += cgLdr(*tmp, imm); }
            }
        } else if (op->isGlobal()) {
            //! FIXME: assume that reg of global variable is free to use
            addUsedGlobalVar(findVariable(op));
            auto var          = findVariable(op);
            tmp               = &var->reg;
            getelemcode->code += cgLdr(*tmp, var);
            getelemcode->code += cgLdr(*tmp, *tmp, 0);
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
                true, whitelist, this, getelemcode);
            getelemcode->code += cgLdr(reg, stride);
            getelemcode->code += cgMul(*tmp, *tmp, reg);
            //! swap register: *tmp <- reg
            if (tmpIsAllocated) { generator_.allocator->releaseRegister(*tmp); }
            generator_.allocator->releaseRegister(reg);
        } else if (multiplier > 0) {
            if (multiplier == 1) {
                getelemcode->code += cgLdr(*tmp, stride);
            } else if (multiplier == 2) {
                getelemcode->code += cgLdr(*tmp, stride);
                getelemcode->code += cgAdd(*tmp, *tmp, *tmp);
            } else {
                auto whitelist = generator_.allocator->getInstOperands(inst);
                auto reg       = generator_.allocator->allocateRegister(
                    true, whitelist, this, getelemcode);
                getelemcode->code += cgLdr(reg, stride);
                getelemcode->code += cgMul(*tmp, *tmp, reg);
                generator_.allocator->releaseRegister(reg);
            }
        }

        if (multiplier != 0) {
            //! dest <- base + offset
            getelemcode->code += cgAdd(dest, dest, *tmp);
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

InstCode *Generator::genAddInst(AddInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rn;
    InstCode      *addcode = new InstCode(inst);
    auto           op1     = inst->useAt(0);
    auto           op2     = inst->useAt(1);

    if (Allocator::isVariable(op1)) {
        rn = findVariable(op1)->reg;
    } else {
        rn = rd;
        addcode->code +=
            cgLdr(rd, static_cast<ConstantInt *>(op1->asConstantData())->value);
    }

    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        addcode->code         += cgAdd(rd, rn, op2reg);
    } else {
        assert(Allocator::isVariable(op1));
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        if (!isImmediateValid(imm)) {
            addcode->code += cgLdr(rd, imm);
            addcode->code += cgAdd(rd, rn, rd);
        } else
            addcode->code += cgAdd(rd, rn, imm);
    }
    return addcode;
}

InstCode *Generator::genSubInst(SubInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    InstCode      *subcode = new InstCode(inst);
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        subcode->code += cgMov(rd, imm);
        rs            = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        subcode->code         += cgSub(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        subcode->code += cgSub(rd, rs, imm);
    }
    return subcode;
}

InstCode *Generator::genMulInst(MulInst *inst) {
    ARMGeneralRegs rd      = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs      = ARMGeneralRegs::None;
    InstCode      *mulcode = new InstCode(inst);
    auto           op1 = inst->useAt(0), op2 = inst->useAt(1);

    if (!Allocator::isVariable(op1)) {
        uint32_t imm = static_cast<ConstantInt *>(op1->asConstantData())->value;
        mulcode->code += cgLdr(rd, imm);
        rs            = rd;
    } else
        rs = findVariable(op1)->reg;

    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        mulcode->code         += cgMul(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        if (!Allocator::isVariable(op1)) {
            auto whitelist        = generator_.allocator->getInstOperands(inst);
            ARMGeneralRegs tmpreg = generator_.allocator->allocateRegister(
                true, whitelist, this, mulcode);
            mulcode->code += cgLdr(
                tmpreg,
                static_cast<ConstantInt *>(op2->asConstantData())->value);
            mulcode->code += cgMul(rd, rs, tmpreg);
            generator_.allocator->releaseRegister(tmpreg);
        } else {
            mulcode->code += cgLdr(rd, imm);
            mulcode->code += cgMul(rd, rs, rd);
        }
    }

    return mulcode;
}

InstCode *Generator::genUDivInst(UDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

//! FIXME: a variable may lose when it holds RO-R3
InstCode *Generator::genSDivInst(SDivInst *inst) {
    assert(!generator_.allocator->regAllocatedMap[0]);
    assert(!generator_.allocator->regAllocatedMap[1]);
    InstCode *sdivcode = new InstCode(inst);
    for (int i = 0; i < inst->totalOperands(); i++) {
        generator_.allocator->usedRegs.insert(static_cast<ARMGeneralRegs>(i));
        if (inst->useAt(i)->isConstant()) {
            sdivcode->code += cgLdr(
                static_cast<ARMGeneralRegs>(i),
                static_cast<ConstantInt *>(inst->useAt(i)->asConstantData())
                    ->value);
        } else {
            sdivcode->code += cgMov(
                static_cast<ARMGeneralRegs>(i),
                findVariable(inst->useAt(i))->reg);
        }
    }
    auto resultReg = findVariable(inst->unwrap())->reg;
    sdivcode->code += cgBl("__aeabi_idiv");
    sdivcode->code += cgMov(resultReg, ARMGeneralRegs::R0);
    return sdivcode;
}

InstCode *Generator::genURemInst(URemInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

//! NOTE: 用除法和乘法完成求余
InstCode *Generator::genSRemInst(SRemInst *inst) {
    auto      targetReg = findVariable(inst->unwrap())->reg;
    InstCode *sremcode  = new InstCode(inst);
    for (int i = 0; i < inst->totalOperands(); i++) {
        generator_.allocator->usedRegs.insert(static_cast<ARMGeneralRegs>(i));
        if (inst->useAt(i)->isConstant()) {
            sremcode->code += cgLdr(
                static_cast<ARMGeneralRegs>(i),
                static_cast<ConstantInt *>(inst->useAt(i)->asConstantData())
                    ->value);
        } else {
            sremcode->code += cgMov(
                static_cast<ARMGeneralRegs>(i),
                findVariable(inst->useAt(i))->reg);
        }
    }
    sremcode->code += cgBl("__aeabi_idiv");
    ARMGeneralRegs tmpReg;
    if (inst->useAt(1)->isConstant()) {
        sremcode->code += cgLdr(
            ARMGeneralRegs::R1,
            static_cast<ConstantInt *>(inst->useAt(1)->asConstantData())
                ->value);
        tmpReg = ARMGeneralRegs::R1;
    } else {
        tmpReg = findVariable(inst->useAt(1))->reg;
    }
    sremcode->code += cgMul(targetReg, tmpReg, ARMGeneralRegs::R0);
    if (inst->useAt(0)->isConstant()) {
        sremcode->code += cgLdr(
            ARMGeneralRegs::R0,
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())
                ->value);
        sremcode->code += cgSub(targetReg, ARMGeneralRegs::R0, targetReg);
    } else {
        sremcode->code +=
            cgSub(targetReg, findVariable(inst->useAt(0))->reg, targetReg);
    }
    return sremcode;
}

InstCode *Generator::genFNegInst(FNegInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFAddInst(FAddInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFSubInst(FSubInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFMulInst(FMulInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFDivInst(FDivInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFRemInst(FRemInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genShlInst(ShlInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    InstCode      *shlcode = new InstCode(inst);

    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        shlcode->code += cgLdr(rd, imm);
        rs            = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        assert(0);
        // += cgLsl(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        shlcode->code += cgLsl(rd, rs, imm);
    }
    return shlcode;
}

InstCode *Generator::genLShrInst(LShrInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genAShrInst(AShrInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    InstCode      *ashrcode = new InstCode(inst);

    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        ashrcode->code += cgLdr(rd, imm);
        rs             = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        assert(0);
        // += cgAsr(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        ashrcode->code += cgAsr(rd, rs, imm);
    }
    return ashrcode;
}

InstCode *Generator::genAndInst(AndInst *inst) {
    ARMGeneralRegs rd = findVariable(inst->unwrap())->reg;
    ARMGeneralRegs rs;
    InstCode      *andcode = new InstCode(inst);
    if (!Allocator::isVariable(inst->useAt(0))) {
        uint32_t imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        andcode->code += cgLdr(rd, imm);
        rs            = rd;
    } else
        rs = findVariable(inst->useAt(0))->reg;

    auto op2 = inst->useAt(1);
    if (Allocator::isVariable(op2)) {
        ARMGeneralRegs op2reg = findVariable(op2)->reg;
        andcode->code         += cgAnd(rd, rs, op2reg);
    } else {
        uint32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        andcode->code += cgAnd(rd, rs, imm);
    }
    return andcode;
}

InstCode *Generator::genOrInst(OrInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genXorInst(XorInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFPToUIInst(FPToUIInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genFPToSIInst(FPToSIInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genUIToFPInst(UIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genSIToFPInst(SIToFPInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

InstCode *Generator::genICmpInst(ICmpInst *inst) {
    auto      op1 = inst->useAt(0), op2 = inst->useAt(1);
    auto      result    = findVariable(inst->unwrap());
    InstCode *icmpcode  = new InstCode(inst);
    auto      whitelist = generator_.allocator->getInstOperands(inst);

    if (Allocator::isVariable(op2)) {
        if (!Allocator::isVariable(op1)) {
            auto tmpreg = generator_.allocator->allocateRegister(
                true, whitelist, this, icmpcode);
            icmpcode->code += cgLdr(
                tmpreg,
                static_cast<ConstantInt *>(op1->asConstantData())->value);
            icmpcode->code += cgCmp(tmpreg, findVariable(op2)->reg);
            generator_.allocator->releaseRegister(tmpreg);
        } else {
            Variable *lhs = findVariable(op1), *rhs = findVariable(op2);
            icmpcode->code += cgCmp(lhs->reg, rhs->reg);
        }
    } else if (Allocator::isVariable(op1)) {
        //! TODO: float
        assert(!op2->asConstantData()->type()->isFloat());
        Variable *lhs = findVariable(op1);
        int32_t imm = static_cast<ConstantInt *>(op2->asConstantData())->value;
        if (isImmediateValid(imm))
            icmpcode->code += cgCmp(lhs->reg, imm);
        else {
            auto tmpreg = generator_.allocator->allocateRegister(
                true, whitelist, this, icmpcode);
            icmpcode->code += cgLdr(tmpreg, imm);
            icmpcode->code += cgCmp(lhs->reg, tmpreg);
            generator_.allocator->releaseRegister(tmpreg);
        }
    } else {
        int32_t imm  = static_cast<ConstantInt *>(op1->asConstantData())->value;
        int32_t imm2 = static_cast<ConstantInt *>(op2->asConstantData())->value;
        if (result->reg != ARMGeneralRegs::None) {
            icmpcode->code += cgMov(result->reg, imm);
            icmpcode->code += cgMov(result->reg, imm2, inst->predicate());
            return icmpcode;
        } else {
            assert(false);
        }
    }

    if (result->reg != ARMGeneralRegs::None) {
        icmpcode->code += cgMov(result->reg, 0);
        icmpcode->code += cgMov(result->reg, 1, inst->predicate());
        icmpcode->code += cgAnd(result->reg, result->reg, 1);
    }
    return icmpcode;
}

InstCode *Generator::genFCmpInst(FCmpInst *inst) {
    assert(0 && "unfinished yet!\n");
    unreachable();
}

//! TODO: optimize
InstCode *Generator::genZExtInst(ZExtInst *inst) {
    auto      extVar   = findVariable(inst->unwrap());
    InstCode *zextcode = new InstCode(inst);
    if (Allocator::isVariable(inst->useAt(0))) {
        zextcode->code += cgMov(extVar->reg, findVariable(inst->useAt(0))->reg);
    } else {
        int imm =
            static_cast<ConstantInt *>(inst->useAt(0)->asConstantData())->value;
        zextcode->code += cgLdr(extVar->reg, imm);
    }
    return zextcode;
}

InstCode *Generator::genCallInst(CallInst *inst) {
    InstCode             *callcode = new InstCode(inst);
    std::string           commentCode;
    std::set<Variable *> *whitelist = nullptr;
    if (!inst->unwrap()->type()->isVoid())
        commentCode += sprintln("@ %%%d:", inst->unwrap()->id());

    //! NOTE: This variable is only use to avoid stack spaces allocated to
    //! params being treated as fragment by Stack
    Variable *onStackParams;
    int       stackParamSize = 0;
    int       regArgNum = inst->totalParams() > 4 ? 4 : inst->totalParams();

    for (int i = 0; i < regArgNum; i++) {
        // param register has already been allocated
        generator_.allocator->usedRegs.insert(static_cast<ARMGeneralRegs>(i));
        if (generator_.allocator->regAllocatedMap[i]) {
            // allocate a new one
            ARMGeneralRegs newreg = generator_.allocator->allocateRegister(
                true, whitelist, this, callcode);
            auto var = generator_.allocator->getVarOfAllocatedReg(
                static_cast<ARMGeneralRegs>(i));
            callcode->code += cgMov(newreg, var->reg);
            var->reg        = newreg;
            generator_.allocator->releaseRegister(
                static_cast<ARMGeneralRegs>(i));
        }
        generator_.allocator->regAllocatedMap[i] = true;
        if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
            assert(!inst->paramAt(i)->type()->isFloat());
            auto constant   = inst->paramAt(i)->asConstantData();
            callcode->code += cgLdr(
                static_cast<ARMGeneralRegs>(i),
                static_cast<ConstantInt *>(constant)->value);
        } else {
            assert(Allocator::isVariable(inst->paramAt(i)));
            Variable *var = findVariable(inst->paramAt(i));
            if (var->is_alloca || var->is_spilled) {
                int            offset;
                ARMGeneralRegs srcReg;
                if (var->is_funcparam) {
                    srcReg = ARMGeneralRegs::R11;
                    offset = var->stackpos;
                } else {
                    srcReg = ARMGeneralRegs::SP;
                    offset =
                        generator_.allocator->stack->stackSize - var->stackpos;
                }
                if (!isImmediateValid(offset)) {
                    callcode->code +=
                        cgLdr(static_cast<ARMGeneralRegs>(i), offset);
                    if (var->is_spilled) {
                        callcode->code += sprintln(
                            "# Use spilled %%%d as argument", var->val->id());
                        callcode->code += cgLdr(
                            static_cast<ARMGeneralRegs>(i),
                            ARMGeneralRegs::SP,
                            static_cast<ARMGeneralRegs>(i));
                    } else {
                        callcode->code += cgAdd(
                            static_cast<ARMGeneralRegs>(i),
                            srcReg,
                            static_cast<ARMGeneralRegs>(i));
                    }
                } else {
                    if (var->is_spilled) {
                        callcode->code += sprintln(
                            "# Use spilled %%%d as argument", var->val->id());
                        callcode->code += cgLdr(
                            static_cast<ARMGeneralRegs>(i), srcReg, offset);
                    } else {
                        callcode->code += cgAdd(
                            static_cast<ARMGeneralRegs>(i), srcReg, offset);
                    }
                }

            } else if (var->reg != static_cast<ARMGeneralRegs>(i)) {
                assert(var->reg != ARMGeneralRegs::None);
                callcode->code +=
                    cgMov(static_cast<ARMGeneralRegs>(i), var->reg);
            }
        }
    }

    ARMGeneralRegs tmpreg =
        generator_.allocator->allocateRegister(true, whitelist, this, callcode);
    const char *callee_name = inst->callee()->asFunction()->name().data();

    RegList saveRegs;
    //if (!strncmp(callee_name, "get", 3)
    //    && libfunc.find(callee_name) != libfunc.end()) {
        for (int i = 1; i <= 3 - inst->totalParams(); i++) {
            auto it = generator_.allocator->usedRegs.find(
                static_cast<ARMGeneralRegs>(i));
            if (it != generator_.allocator->usedRegs.end())
                saveRegs.insertToTail(static_cast<ARMGeneralRegs>(i));
        }
    //}

    callcode->code += cgPush(saveRegs);

    if (inst->totalParams() > 4) {
        stackParamSize = (inst->totalParams() - 4) * 4;
        callcode->code +=
            cgSub(ARMGeneralRegs::SP, ARMGeneralRegs::SP, stackParamSize);
        generator_.stack->pushVar(onStackParams, stackParamSize);
    }
    for (int i = 4; i < inst->totalParams(); i++) {
        //! NOTE: untested code
        if (inst->paramAt(i)->tryIntoConstantData() != nullptr) {
            assert(!inst->paramAt(i)->type()->isFloat());
            uint32_t imm =
                static_cast<ConstantInt *>(inst->paramAt(i)->asConstantData())
                    ->value;
            callcode->code += cgLdr(tmpreg, imm);
            callcode->code += cgStr(
                tmpreg,
                ARMGeneralRegs::SP,
                (inst->totalParams() - (i + 1)) * 4);
        } else {
            assert(Allocator::isVariable(inst->paramAt(i)));
            Variable *var = findVariable(inst->paramAt(i).value());
            if (var->is_alloca || var->is_spilled) {
                int            offset;
                ARMGeneralRegs srcReg;
                if (var->is_funcparam) {
                    offset = var->stackpos;
                    srcReg = ARMGeneralRegs::R11;
                } else {
                    offset = generator_.stack->stackSize - var->stackpos;
                    srcReg = ARMGeneralRegs::SP;
                }
                if (!isImmediateValid(offset)) {
                    callcode->code += cgLdr(tmpreg, offset);
                    callcode->code += cgAdd(tmpreg, tmpreg, srcReg);
                } else if (var->is_alloca) {
                    callcode->code += cgAdd(tmpreg, srcReg, offset);
                } else if (var->is_spilled) {
                    callcode->code += sprintln(
                        "# Use spilled %%%d as argument", var->val->id());
                    callcode->code += cgLdr(tmpreg, srcReg, offset);
                }
                callcode->code += cgStr(
                    tmpreg,
                    ARMGeneralRegs::SP,
                    (inst->totalParams() - (i + 1)) * 4);
            } else {
                assert(var->reg != ARMGeneralRegs::None);
                callcode->code += cgStr(
                    var->reg,
                    ARMGeneralRegs::SP,
                    (inst->totalParams() - (i + 1)) * 4);
            }
        }
    }
    generator_.allocator->releaseRegister(tmpreg);

    int usedRegNum = inst->totalParams() > 4 ? 4 : inst->totalParams();
    for (int i = 0; i < usedRegNum; i++) {
        generator_.allocator->releaseRegister(static_cast<ARMGeneralRegs>(i));
    }
    if (inst->unwrap()->uses().size() != 0) {
        if (generator_.allocator->regAllocatedMap[0]) {
            auto var =
                generator_.allocator->getVarOfAllocatedReg(ARMGeneralRegs::R0);
            // assert(var->val->asInstruction()->id() == InstructionID::Call);
            ARMGeneralRegs newreg = generator_.allocator->allocateRegister(
                true, whitelist, this, callcode);
            callcode->code += cgMov(newreg, var->reg);
            var->reg        = newreg;
        }
        generator_.allocator->regAllocatedMap[0] = true;
        auto var                                 = findVariable(inst->unwrap());
        if (var->reg != ARMGeneralRegs::None && var->reg != ARMGeneralRegs::R0)
            generator_.allocator->releaseRegister(var);
        var->reg = ARMGeneralRegs::R0;
    }
    callcode->code += cgBl(inst->callee()->asFunction());

    callcode->code = callcode->code + cgPop(saveRegs);

    if (inst->totalParams() > 4) {
        callcode->code +=
            cgAdd(ARMGeneralRegs::SP, ARMGeneralRegs::SP, stackParamSize);
        generator_.stack->popVar(onStackParams, stackParamSize);
    }
    callcode->code = commentCode + callcode->code;

    return callcode;
}

std::string Generator::cgMov(
    ARMGeneralRegs rd, ARMGeneralRegs rs, ComparePredicationType cond) {
    const char *instr = nullptr;
    switch (cond) {
        case ComparePredicationType::TRUE: {
            instr = "mov";
        } break;
        case ComparePredicationType::EQ: {
            instr = "moveq";
        } break;
        case ComparePredicationType::NE: {
            instr = "movne";
        } break;
        case ComparePredicationType::SLE: {
            instr = "movle";
        } break;
        case ComparePredicationType::SLT: {
            instr = "movlt";
        } break;
        case ComparePredicationType::SGE: {
            instr = "movge";
        } break;
        case ComparePredicationType::SGT: {
            instr = "movgt";
        } break;
        default: {
            assert(0 && "unfinished comparative type");
            unreachable();
        } break;
    }
    assert(instr != nullptr);
    return instrln(instr, "%s, %s", reg2str(rd), reg2str(rs));
}

std::string Generator::cgMov(
    ARMGeneralRegs rd, int32_t imm, ComparePredicationType cond) {
    const char *instr = nullptr;
    switch (cond) {
        case ComparePredicationType::TRUE: {
            instr = "mov";
        } break;
        case ComparePredicationType::EQ: {
            instr = "moveq";
        } break;
        case ComparePredicationType::NE: {
            instr = "movne";
        } break;
        case ComparePredicationType::SLE: {
            instr = "movle";
        } break;
        case ComparePredicationType::SLT: {
            instr = "movlt";
        } break;
        case ComparePredicationType::SGE: {
            instr = "movge";
        } break;
        case ComparePredicationType::SGT: {
            instr = "movgt";
        } break;
        default: {
            assert(0 && "unfinished comparative type");
            unreachable();
        } break;
    }
    assert(instr != nullptr);
    return instrln(instr, "%s, #%d", reg2str(rd), imm);
}

std::string Generator::cgLdr(
    ARMGeneralRegs dst, ARMGeneralRegs src, int32_t offset) {
    if (offset != 0) {
        return instrln(
            "ldr", "%s, [%s, #%d]", reg2str(dst), reg2str(src), offset);
    } else {
        return instrln("ldr", "%s, [%s]", reg2str(dst), reg2str(src));
    }
}

std::string Generator::cgLdr(
    ARMGeneralRegs dst, ARMGeneralRegs src, ARMGeneralRegs offset) {
    return instrln(
        "ldr", "%s, [%s, %s]", reg2str(dst), reg2str(src), reg2str(offset));
}

std::string Generator::cgLdr(ARMGeneralRegs dst, int32_t imm) {
    return instrln("ldr", "%s, =%d", reg2str(dst), imm);
}

std::string Generator::cgLdr(ARMGeneralRegs dst, Variable *var) {
    assert(var->is_global);
    assert(generator_.usedGlobalVars->count(var));
    const auto &source = generator_.usedGlobalVars->at(var);
    return instrln("ldr", "%s, %s", reg2str(dst), source.c_str());
}

std::string Generator::cgStr(
    ARMGeneralRegs src, ARMGeneralRegs dst, int32_t offset) {
    if (offset != 0) {
        return instrln(
            "str", "%s, [%s, #%d]", reg2str(src), reg2str(dst), offset);
    } else {
        return instrln("str", "%s, [%s]", reg2str(src), reg2str(dst));
    }
}

std::string Generator::cgStr(
    ARMGeneralRegs src, ARMGeneralRegs dst, ARMGeneralRegs offset) {
    return instrln(
        "str", "%s, [%s, %s]", reg2str(src), reg2str(dst), reg2str(offset));
}

std::string Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return instrln("add", "%s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgAdd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("add", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return instrln("sub", "%s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgSub(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("sub", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgMul(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return instrln("mul", "%s, %s, %s", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgMul(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("mul", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgAnd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, ARMGeneralRegs op2) {
    return instrln(
        "and", "%s, %s, #%d", reg2str(rd), reg2str(rn), reg2str(op2));
}

std::string Generator::cgAnd(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("and", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgLsl(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("lsl", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgAsr(
    ARMGeneralRegs rd, ARMGeneralRegs rn, int32_t op2) {
    return instrln("asr", "%s, %s, #%d", reg2str(rd), reg2str(rn), op2);
}

std::string Generator::cgCmp(ARMGeneralRegs op1, ARMGeneralRegs op2) {
    return instrln("cmp", "%s, %s", reg2str(op1), reg2str(op2));
}

std::string Generator::cgCmp(ARMGeneralRegs op1, int32_t op2) {
    return instrln("cmp", "%s, #%d", reg2str(op1), op2);
}

std::string Generator::cgTst(ARMGeneralRegs op1, int32_t op2) {
    return instrln("tst", "%s, #%d", reg2str(op1), op2);
}

std::string Generator::cgVmov(
    ARMFloatRegs rd, float imm, ComparePredicationType cond) {
    return instrln("vcmp.f32", "%s, %s", reg2str(rd), imm);
}

std::string Generator::cgVadd(
    ARMFloatRegs sd, ARMFloatRegs sn, ARMFloatRegs sm) {
    return instrln("vadd.f32", "%s, %s", reg2str(sd), reg2str(sn), reg2str(sm));
}

std::string Generator::cgVsub(
    ARMFloatRegs sd, ARMFloatRegs sn, ARMFloatRegs sm) {
    return instrln("vsub.f32", "%s, %s", reg2str(sd), reg2str(sn), reg2str(sm));
}

std::string Generator::cgVldr(
    ARMFloatRegs dst, ARMGeneralRegs src, int32_t offset) {
    if (offset != 0) {
        return instrln("vldr", "%s, [%s]", reg2str(dst), reg2str(src));
    } else {
        return instrln(
            "vldr", "%s, [%s, %d]", reg2str(dst), reg2str(src), offset);
    }
}

std::string Generator::cgVstr(
    ARMFloatRegs src, ARMFloatRegs dst, int32_t offset) {
    if (offset != 0) {
        return instrln("vstr", "%s, [%s]", reg2str(dst), reg2str(src));
    } else {
        return instrln(
            "vstr", "%s, [%s, %d]", reg2str(dst), reg2str(src), offset);
    }
}

std::string Generator::cgVmul(
    ARMFloatRegs sd, ARMFloatRegs sn, ARMFloatRegs sm) {
    return instrln("vmul.f32", "%s, %s", reg2str(sd), reg2str(sn), reg2str(sm));
}

std::string Generator::cgVcmp(ARMFloatRegs op1, ARMFloatRegs op2) {
    return instrln("vcmp.fa2", "%s, %s", reg2str(op1), reg2str(op2));
}

// direction 1:float to int/uint 0: int/uint to float
std::string Generator::cgVcvt(
    ARMFloatRegs sd, ARMFloatRegs sm, bool direction, bool sextflag) {
    if (direction) {
        if (sextflag) {
            return instrln("vcvt.f32.s32", "%s, %s", reg2str(sd), reg2str(sm));
        } else {
            return instrln("vcvt.f32.u32", "%s, %s", reg2str(sd), reg2str(sm));
        }
    } else {
        if (sextflag) {
            return instrln("vcvt.s32.f32", "%s, %s", reg2str(sd), reg2str(sm));
        } else {
            return instrln("vcvt.u32.f32", "%s, %s", reg2str(sd), reg2str(sm));
        }
    }
}

std::string Generator::cgVneg(ARMFloatRegs sd, ARMFloatRegs sm) {
    return instrln("vneg.f32", "%s, %s", reg2str(sd), reg2str(sm));
}

std::string Generator::cgB(Value *brTarget, ComparePredicationType cond) {
    assert(brTarget->isLabel());
    size_t      blockid = brTarget->id();
    const char *instr   = nullptr;
    switch (cond) {
        case ComparePredicationType::TRUE: {
            instr = "b";
        } break;
        case ComparePredicationType::EQ: {
            instr = "beq";
        } break;
        case ComparePredicationType::NE: {
            instr = "bne";
        } break;
        case ComparePredicationType::SLT: {
            instr = "blt";
        } break;
        case ComparePredicationType::SGT: {
            instr = "bgt";
        } break;
        case ComparePredicationType::SLE: {
            instr = "ble";
        } break;
        case ComparePredicationType::SGE: {
            instr = "bge";
        } break;
        default: {
            unreachable();
        } break;
    }
    assert(instr != nullptr);
    return instrln(
        instr, ".F%dBB.%d", generator_.cur_funcnum, getBlockNum(blockid));
}

std::string Generator::cgBx(ARMGeneralRegs rd) {
    return instrln("bx", "%s", reg2str(rd));
}

std::string Generator::cgBl(Function *callee) {
    return instrln("bl", "%s", callee->name().data());
}

std::string Generator::cgBl(const char *libfuncname) {
    assert(libfunc.count(libfuncname));
    return instrln("bl", "%s", libfuncname);
}

std::string Generator::cgPush(RegList &reglist) {
    if (reglist.size() == 0) { return ""; }
    std::string regs;
    for (auto reg : reglist) {
        regs += reg2str(reg);
        if (reg != reglist.tail()->value()) { regs.push_back(','); };
    }
    return instrln("push", "{%s}", regs.c_str());
}

std::string Generator::cgPop(RegList &reglist) {
    if (reglist.size() == 0) { return ""; }
    std::string regs;
    for (auto reg : reglist) {
        regs += reg2str(reg);
        if (reg != reglist.tail()->value()) { regs.push_back(','); };
    }
    return instrln("pop", "{%s}", regs.c_str());
}

} // namespace slime::backend
