#include "16.h"

#include <iostream>
#include <sstream>

namespace slime::backend {

std::string InstrOp::InstrState::dump() const {
    constexpr auto opcodeWidth     = 5;
    constexpr auto longOpcodeWidth = 14;
    constexpr auto indent          = 4;
    constexpr auto space           = 2;

    constexpr auto N     = 64;
    auto           instr = InstrOp::toString(kind);
    char           buffer[N]{}, *p = buffer;

    int written = sprintf(
        p,
        "%*s%-s%*s",
        indent,
        "",
        instr.size() <= opcodeWidth ? opcodeWidth : longOpcodeWidth,
        instr.data(),
        space,
        "");
    assert(written < N);

    p        += written;
    int size = N - written;

    switch (type) {
        case InstrType::Reg: {
            snprintf(p, size, "%s", InstrOp::toString(regs[0]).data());
        } break;
        case InstrType::Reg2: {
            written = snprintf(
                p,
                size,
                "%s, %s",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data());
        } break;
        case InstrType::Reg3: {
            written = snprintf(
                p,
                size,
                "%s, %s, %s",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data(),
                InstrOp::toString(regs[2]).data());
        } break;
        case InstrType::RegImm: {
            assert(!isFpImm);
            written = snprintf(
                p,
                size,
                "%s, #%d",
                InstrOp::toString(regs[0]).data(),
                std::get<int32_t>(extra));
        } break;
        case InstrType::RegLabel: {
            written = snprintf(
                p,
                size,
                "%s, %s",
                InstrOp::toString(regs[0]).data(),
                std::get<std::string_view>(extra).data());
        } break;
        case InstrType::RegImmExtend: {
            written = snprintf(
                p,
                size,
                "%s, =%d",
                InstrOp::toString(regs[0]).data(),
                std::get<int32_t>(extra));
        } break;
        case InstrType::RegAddr: {
            written = snprintf(
                p,
                size,
                "%s, [%s]",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data());
        } break;
        case InstrType::RegAddrImmOffset: {
            written = snprintf(
                p,
                size,
                "%s, [%s, #%d]",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data(),
                std::get<int32_t>(extra));
        } break;
        case InstrType::RegAddrRegOffset: {
            written = snprintf(
                p,
                size,
                "%s, [%s, %s]",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data(),
                InstrOp::toString(regs[2]).data());
        } break;
        case InstrType::RegRegImm: {
            written = snprintf(
                p,
                size,
                "%s, %s, #%d",
                InstrOp::toString(regs[0]).data(),
                InstrOp::toString(regs[1]).data(),
                std::get<int32_t>(extra));
        } break;
        case InstrType::RegRange: {
            const char *origin = p;
            *p++               = '{';
            auto &ranges       = std::get<RegRanges>(extra);
            for (int i = 0; i < ranges.size(); ++i) {
                auto [first, last] = ranges[i];
                if (first == last) {
                    p += sprintf(p, "%s", InstrOp::toString(first).data());
                } else {
                    p += sprintf(
                        p,
                        "%s-%s",
                        InstrOp::toString(first).data(),
                        InstrOp::toString(last).data());
                }
                if (i + 1 != ranges.size()) {
                    *p++ = ',';
                    *p++ = ' ';
                }
            }
            *p++    = '}';
            *p++    = '\0';
            written = p - origin;
        } break;
        case InstrType::Label: {
            written = snprintf(
                p, size, "%s", std::get<std::string_view>(extra).data());
        } break;
        default: {
            unreachable();
        } break;
    }

    assert(written < N - size);
    return {buffer};
}

void InstrOp::move(ARMGeneralRegs rd, ARMGeneralRegs rs) {
    instrs_.push_back(createReg2(InstrKind::MOV, rd, rs));
}

void InstrOp::move(ARMGeneralRegs rd, int32_t imm) {
    instrs_.push_back(createRegImm(InstrKind::MOV, rd, imm));
}

void InstrOp::move(ARMFloatRegs rd, ARMFloatRegs rs) {
    instrs_.push_back(createReg2(InstrKind::VMOV, rd, rs));
}

void InstrOp::move(ARMFloatRegs rd, float imm) {
    instrs_.push_back(createRegImm(InstrKind::VMOV, rd, imm));
}

void InstrOp::move(ARMFloatRegs rd, ARMGeneralRegs rs) {
    instrs_.push_back(createReg2(InstrKind::VMOV, rd, rs));
}

void InstrOp::move(ARMGeneralRegs rd, ARMFloatRegs rs) {
    instrs_.push_back(createReg2(InstrKind::VMOV, rd, rs));
}

void InstrOp::moveIf(ARMGeneralRegs rd, ARMGeneralRegs rs, Predicate pred) {
    InstrKind kind{};

    switch (pred) {
        case Predicate::TRUE: {
            kind = InstrKind::MOV;
        } break;
        case Predicate::EQ: {
            kind = InstrKind::MOVEQ;
        } break;
        case Predicate::NE: {
            kind = InstrKind::MOVNE;
        } break;
        case Predicate::SLE: {
            kind = InstrKind::MOVLE;
        } break;
        case Predicate::SLT: {
            kind = InstrKind::MOVLT;
        } break;
        case Predicate::SGE: {
            kind = InstrKind::MOVGE;
        } break;
        case Predicate::SGT: {
            kind = InstrKind::MOVGT;
        } break;
        default: {
            unreachable();
        } break;
    }

    instrs_.push_back(createReg2(kind, rd, rs));
}

void InstrOp::moveIf(ARMGeneralRegs rd, int32_t imm, Predicate pred) {
    InstrKind kind{};

    switch (pred) {
        case Predicate::TRUE: {
            kind = InstrKind::MOV;
        } break;
        case Predicate::EQ: {
            kind = InstrKind::MOVEQ;
        } break;
        case Predicate::NE: {
            kind = InstrKind::MOVNE;
        } break;
        case Predicate::SLE: {
            kind = InstrKind::MOVLE;
        } break;
        case Predicate::SLT: {
            kind = InstrKind::MOVLT;
        } break;
        case Predicate::SGE: {
            kind = InstrKind::MOVGE;
        } break;
        case Predicate::SGT: {
            kind = InstrKind::MOVGT;
        } break;
        default: {
            unreachable();
        } break;
    }

    instrs_.push_back(createRegImm(kind, rd, imm));
}

void InstrOp::load(ARMGeneralRegs rd, ARMGeneralRegs base, int32_t offset) {
    instrs_.push_back(createRegAddrImmOffset(InstrKind::LDR, rd, base, offset));
}

void InstrOp::load(
    ARMGeneralRegs rd, ARMGeneralRegs base, ARMGeneralRegs offset) {
    instrs_.push_back(createRegAddrRegOffset(InstrKind::LDR, rd, base, offset));
}

void InstrOp::load(ARMGeneralRegs rd, Variable *var) {
    const auto &globvars = parent_->generator_.usedGlobalVars;
    assert(globvars->count(var));
    instrs_.push_back(createRegLabel(InstrKind::LDR, rd, globvars->at(var)));
}

void InstrOp::load(ARMFloatRegs rd, ARMGeneralRegs base, int32_t offset) {
    instrs_.push_back(
        createRegAddrImmOffset(InstrKind::VLDR, rd, base, offset));
}

void InstrOp::load(
    ARMFloatRegs rd, ARMGeneralRegs base, ARMGeneralRegs offset) {
    instrs_.push_back(
        createRegAddrRegOffset(InstrKind::VLDR, rd, base, offset));
}

void InstrOp::load(ARMFloatRegs rd, Variable *var) {
    const auto &globvars = parent_->generator_.usedGlobalVars;
    assert(globvars->count(var));
    instrs_.push_back(createRegLabel(InstrKind::VLDR, rd, globvars->at(var)));
}

void InstrOp::store(ARMGeneralRegs rs, ARMGeneralRegs base, int32_t offset) {
    instrs_.push_back(createRegAddrImmOffset(InstrKind::STR, rs, base, offset));
}

void InstrOp::store(
    ARMGeneralRegs rs, ARMGeneralRegs base, ARMGeneralRegs offset) {
    instrs_.push_back(createRegAddrRegOffset(InstrKind::STR, rs, base, offset));
}

void InstrOp::store(ARMFloatRegs rs, ARMGeneralRegs base, int32_t offset) {
    instrs_.push_back(
        createRegAddrImmOffset(InstrKind::VSTR, rs, base, offset));
}

void InstrOp::store(
    ARMFloatRegs rs, ARMGeneralRegs base, ARMGeneralRegs offset) {
    instrs_.push_back(
        createRegAddrRegOffset(InstrKind::VSTR, rs, base, offset));
}

void InstrOp::neg(ARMGeneralRegs rd, ARMGeneralRegs rs) {
    sub(rd, 0, rs);
}

void InstrOp::neg(ARMFloatRegs rd, ARMFloatRegs rs) {
    instrs_.push_back(createReg2(InstrKind::VNEG_F32, rd, rs));
}

void InstrOp::add(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::ADD, rd, lhs, rhs));
}

void InstrOp::add(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::ADD, rd, lhs, rhs));
}

void InstrOp::add(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::VADD_F32, rd, lhs, rhs));
}

void InstrOp::sub(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::SUB, rd, lhs, rhs));
}

void InstrOp::sub(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::SUB, rd, lhs, rhs));
}

void InstrOp::sub(ARMGeneralRegs rd, int32_t lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::RSB, rd, rhs, lhs));
}

void InstrOp::sub(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::VSUB_F32, rd, lhs, rhs));
}

void InstrOp::mul(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::MUL, rd, lhs, rhs));
}

void InstrOp::mul(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::MUL, rd, lhs, rhs));
}

void InstrOp::mul(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::VMUL_F32, rd, lhs, rhs));
}

void InstrOp::div(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    unreachable();
}

void InstrOp::div(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    unreachable();
}

void InstrOp::div(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::VDIV_F32, rd, lhs, rhs));
}

void InstrOp::mod(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    unreachable();
}

void InstrOp::mod(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    unreachable();
}

void InstrOp::divmod(
    ARMGeneralRegs rd1,
    ARMGeneralRegs rd2,
    ARMGeneralRegs lhs,
    ARMGeneralRegs rhs) {
    unreachable();
}

void InstrOp::divmod(
    ARMGeneralRegs rd1, ARMGeneralRegs rd2, ARMGeneralRegs lhs, int32_t rhs) {
    unreachable();
}

void InstrOp::bitwiseAnd(
    ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::AND, rd, lhs, rhs));
}

void InstrOp::bitwiseAnd(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::AND, rd, lhs, rhs));
}

void InstrOp::logicalShl(
    ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::LSL, rd, lhs, rhs));
}

void InstrOp::logicalShl(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::LSL, rd, lhs, rhs));
}

void InstrOp::arithShr(
    ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg3(InstrKind::ASR, rd, lhs, rhs));
}

void InstrOp::arithShr(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegRegImm(InstrKind::ASR, rd, lhs, rhs));
}

void InstrOp::compare(ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg2(InstrKind::CMP, lhs, rhs));
}

void InstrOp::compare(ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegImm(InstrKind::CMP, lhs, rhs));
}

void InstrOp::compare(ARMFloatRegs lhs, ARMFloatRegs rhs) {
    unreachable();
}

void InstrOp::test(ARMGeneralRegs lhs, ARMGeneralRegs rhs) {
    instrs_.push_back(createReg2(InstrKind::TST, lhs, rhs));
}

void InstrOp::test(ARMGeneralRegs lhs, int32_t rhs) {
    instrs_.push_back(createRegImm(InstrKind::TST, lhs, rhs));
}

void InstrOp::call(Function *dest) {
    call(dest->name().data());
}

void InstrOp::call(const char *dest) {
    instrs_.push_back(createLabel(InstrKind::BL, dest));
}

void InstrOp::call(ARMGeneralRegs dest) {
    instrs_.push_back(createReg(InstrKind::BL, dest));
}

void InstrOp::ret() {
    ret(ARMGeneralRegs::LR);
}

void InstrOp::ret(ARMGeneralRegs dest) {
    instrs_.push_back(createReg(InstrKind::BX, dest));
}

void InstrOp::jmp(const char *dest) {
    instrs_.push_back(createLabel(InstrKind::B, dest));
}

void InstrOp::jmp(ARMGeneralRegs dest) {
    instrs_.push_back(createReg(InstrKind::B, dest));
}

void InstrOp::jmp(const char *dest, Predicate pred) {
    InstrKind kind{};

    switch (pred) {
        case Predicate::TRUE: {
            kind = InstrKind::B;
        } break;
        case Predicate::EQ: {
            kind = InstrKind::BEQ;
        } break;
        case Predicate::OEQ:
        case Predicate::NE: {
            kind = InstrKind::BNE;
        } break;
        case Predicate::SLT: {
            kind = InstrKind::BLT;
        } break;
        case Predicate::OGE:
        case Predicate::SGT: {
            kind = InstrKind::BGT;
        } break;
        case Predicate::OGT:
        case Predicate::SLE: {
            kind = InstrKind::BLE;
        } break;
        case Predicate::SGE: {
            kind = InstrKind::BGE;
        } break;
        case Predicate::OLT: {
            kind = InstrKind::BPL;
        } break;
        case Predicate::OLE: {
            kind = InstrKind::BHI;
        } break;
        case Predicate::ONE: {
            kind = InstrKind::BVS;
        } break;
        default: {
            unreachable();
        } break;
    }

    instrs_.push_back(createLabel(kind, dest));
}

void InstrOp::jmp(ARMGeneralRegs dest, Predicate pred) {
    InstrKind kind{};

    switch (pred) {
        case Predicate::TRUE: {
            kind = InstrKind::B;
        } break;
        case Predicate::EQ: {
            kind = InstrKind::BEQ;
        } break;
        case Predicate::OEQ:
        case Predicate::NE: {
            kind = InstrKind::BNE;
        } break;
        case Predicate::SLT: {
            kind = InstrKind::BLT;
        } break;
        case Predicate::OGE:
        case Predicate::SGT: {
            kind = InstrKind::BGT;
        } break;
        case Predicate::OGT:
        case Predicate::SLE: {
            kind = InstrKind::BLE;
        } break;
        case Predicate::SGE: {
            kind = InstrKind::BGE;
        } break;
        case Predicate::OLT: {
            kind = InstrKind::BPL;
        } break;
        case Predicate::OLE: {
            kind = InstrKind::BHI;
        } break;
        case Predicate::ONE: {
            kind = InstrKind::BVS;
        } break;
        default: {
            unreachable();
        } break;
    }

    instrs_.push_back(createReg(kind, dest));
}

void InstrOp::convert(ARMFloatRegs rd, ARMGeneralRegs rs, bool isSigned) {
    instrs_.push_back(createReg2(
        isSigned ? InstrKind::VCVT_F32_S32 : InstrKind::VCVT_F32_U32, rd, rs));
}

void InstrOp::convert(ARMGeneralRegs rd, ARMFloatRegs rs, bool isSigned) {
    instrs_.push_back(createReg2(
        isSigned ? InstrKind::VCVT_S32_F32 : InstrKind::VCVT_U32_F32, rd, rs));
}

void InstrOp::convert(ARMGeneralRegs rd, float imm, bool isSigned) {
    unreachable();
}

void InstrOp::convert(ARMFloatRegs rd, int32_t imm) {
    instrs_.push_back(createRegImm(InstrKind::VCVT_F32_S32, rd, imm));
}

void InstrOp::convert(ARMFloatRegs rd, uint32_t imm) {
    instrs_.push_back(
        createRegImm(InstrKind::VCVT_F32_U32, rd, static_cast<int32_t>(imm)));
}

std::string InstrOp::dumpAll() const {
    std::stringstream ss;
    dumpAll(ss);
    return ss.str();
}

void InstrOp::dumpAll(std::ostream &os) const {
    for (auto instr : instrs()) { os << instr.dump() << std::endl; }
}

} // namespace slime::backend
