#pragma once

#include "InstrAssign.h"

#include <slime/experimental/Utility.h>
#include <initializer_list>
#include <algorithm>
#include <type_traits>

namespace slime::experimental::backend::ARMv7A {

template <typename T>
inline void InstrOp::push(const T &regs) {
    using R                        = std::decay_t<T>;
    bool                  isFpRegs = false;
    std::vector<RegRange> ranges;
    if constexpr (is_iterable_as<R, GeneralRegister>) {
        ranges   = toRegRanges(regs);
        isFpRegs = false;
    } else if constexpr (is_iterable_as<R, FPRegister>) {
        ranges   = toRegRanges(regs);
        isFpRegs = true;
    } else if constexpr (std::is_same_v<R, GeneralRegister>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = false;
    } else if constexpr (std::is_same_v<R, FPRegister>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = true;
    } else {
        unreachable();
    }
    instrs_.push_back(
        createRegRange(isFpRegs ? InstrKind::VPUSH : InstrKind::PUSH, ranges));
}

template <typename T>
inline void InstrOp::pop(const T &regs) {
    using R                        = std::decay_t<T>;
    bool                  isFpRegs = false;
    std::vector<RegRange> ranges;
    if constexpr (is_iterable_as<R, GeneralRegister>) {
        ranges   = toRegRanges(regs);
        isFpRegs = false;
    } else if constexpr (is_iterable_as<R, FPRegister>) {
        ranges   = toRegRanges(regs);
        isFpRegs = true;
    } else if constexpr (std::is_same_v<R, GeneralRegister>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = false;
    } else if constexpr (std::is_same_v<R, FPRegister>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = true;
    } else {
        unreachable();
    }
    instrs_.push_back(
        createRegRange(isFpRegs ? InstrKind::VPOP : InstrKind::POP, ranges));
}

inline auto InstrOp::instrs() const -> const std::vector<InstrState> & {
    return instrs_;
}

template <typename T>
inline auto InstrOp::toRegRanges(const T &regs) -> std::vector<RegRange> {
    using E = std::decay_t<decltype(*regs.begin())>;
    static_assert(
        std::is_same_v<E, GeneralRegister> || std::is_same_v<E, FPRegister>);

    std::vector<Register> idents;
    for (auto reg : regs) { idents.push_back(reg); }

    std::vector<RegRange> resp;
    if (idents.empty()) { return resp; }
    std::sort(idents.begin(), idents.end());
    int first = 0, last = first;
    do {
        while (last + 1 < idents.size()) {
            if (idents[last + 1].id() <= idents[last].id() + 1) {
                ++last;
            } else {
                break;
            }
        }
        resp.push_back({idents[first], idents[last]});
        first = last + 1;
        last  = first;
    } while (last < idents.size());
    return resp;
}

template <typename R>
inline auto InstrOp::createReg(InstrKind kind, R reg) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::Reg;
    instr.regs[0] = reg;
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createReg2(InstrKind kind, R1 r1, R2 r2) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::Reg2;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    return instr;
}

template <typename R1, typename R2, typename R3>
inline auto InstrOp::createReg3(InstrKind kind, R1 r1, R2 r2, R3 r3)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::Reg3;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    instr.regs[2] = r3;
    return instr;
}

template <typename R, typename I>
inline auto InstrOp::createRegImm(InstrKind kind, R reg, I imm) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegImm;
    instr.regs[0] = reg;
    instr.isFpImm = std::is_floating_point_v<I>;
    instr.extra   = imm;
    return instr;
}

template <typename R>
inline auto InstrOp::createRegLabel(
    InstrKind kind, R reg, std::string_view label) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegLabel;
    instr.regs[0] = reg;
    instr.extra   = std::string{label};
    return instr;
}

template <typename R>
inline auto InstrOp::createRegImmExtend(InstrKind kind, R reg, int32_t imm)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegImmExtend;
    instr.regs[0] = reg;
    instr.isFpImm = false;
    instr.extra   = imm;
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createRegAddr(InstrKind kind, R1 r1, R2 r2) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegAddr;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createRegAddrImmOffset(
    InstrKind kind, R1 r1, R2 r2, int32_t imm) -> InstrState {
    if (imm == 0) { return createRegAddr(kind, r1, r2); }
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegAddrImmOffset;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    instr.isFpImm = false;
    instr.extra   = imm;
    return instr;
}

template <typename R1, typename R2, typename R3>
inline auto InstrOp::createRegAddrRegOffset(InstrKind kind, R1 r1, R2 r2, R3 r3)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegAddrRegOffset;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    instr.regs[2] = r3;
    return instr;
}

template <typename R1, typename R2, typename I>
inline auto InstrOp::createRegRegImm(InstrKind kind, R1 r1, R2 r2, I imm)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegRegImm;
    instr.regs[0] = r1;
    instr.regs[1] = r2;
    instr.isFpImm = std::is_floating_point_v<I>;
    instr.extra   = imm;
    return instr;
}

inline auto InstrOp::createRegRange(InstrKind kind, const RegRanges &ranges)
    -> InstrState {
    InstrState instr;
    instr.kind  = kind;
    instr.type  = InstrType::RegRange;
    instr.extra = ranges;
    return instr;
}

inline auto InstrOp::createLabel(InstrKind kind, std::string_view label)
    -> InstrState {
    InstrState instr;
    instr.kind  = kind;
    instr.type  = InstrType::Label;
    instr.extra = label;
    return instr;
}

} // namespace slime::experimental::backend::ARMv7A
