#pragma once

#include "RegFile.h"
#include "InstrSet.h"

#include <slime/ir/instruction.h>
#include <slime/experimental/Utility.h>
#include <stdint.h>
#include <stddef.h>
#include <variant>
#include <string_view>
#include <array>
#include <ostream>
#include <vector>
#include <string>
#include <type_traits>
#include <initializer_list>
#include <algorithm>

namespace slime::experimental::backend::ARMv7A {

class Variable;

enum class InstrType : uint8_t {
    Reg,              //<! instr reg
    Reg2,             //<! instr reg, reg
    Reg3,             //<! instr reg, reg, reg
    RegImm,           //<! instr reg, imm
    RegLabel,         //<! instr reg, label
    RegImmExtend,     //<! instr reg, =imm
    RegAddr,          //<! instr reg, [reg]
    RegAddrImmOffset, //<! instr reg, [reg, imm]
    RegAddrRegOffset, //<! instr reg, [reg, reg]
    RegRegImm,        //<! instr reg, reg, imm
    RegRange,         //<! instr {regs...}
    Label,            //<! instr imm
};

class InstrOp {
public:
    struct RegRange {
        Register first;
        Register last;
    };

    using RegRanges = std::vector<RegRange>;

    struct InstrState {
        using ExtraArg = std::variant<
            std::monostate,
            int32_t,
            float,
            std::string_view,
            RegRanges>;

        std::string dump() const;

        InstrKind kind;
        InstrType type;
        bool      isFpImm;
        Register  regs[3];
        ExtraArg  extra;
    };

    static inline std::string_view toString(InstrKind instr);

protected:
    static constexpr std::
        array<std::string_view, static_cast<size_t>(InstrKind::LAST_INSTR) + 1>
            INSTR_NAME_TABLE{
                "mov",
                "moveq",
                "movne",
                "movle",
                "movlt",
                "movge",
                "movgt",
                "add",
                "and",
                "asr",
                "b",
                "beq",
                "bge",
                "bgt",
                "bhi",
                "bl",
                "ble",
                "blt",
                "bne",
                "bpl",
                "bvs",
                "bx",
                "cmp",
                "ldr",
                "lsl",
                "mul",
                "pop",
                "push",
                "str",
                "sub",
                "rsb",
                "tst",
                "vadd.f32",
                "vcmp.f32",
                "vcvt.f32.s32",
                "vcvt.f32.u32",
                "vcvt.s32.f32",
                "vcvt.u32.f32",
                "vdiv.f32",
                "vldr",
                "vmov",
                "vmul.f32",
                "vneg.f32",
                "vpop",
                "vpush",
                "vstr",
                "vsub.f32"};

public:
    using Predicate = ir::ComparePredicationType;

    void move(GeneralRegister rd, GeneralRegister rs);
    void move(GeneralRegister rd, int32_t imm);
    void move(FPRegister rd, FPRegister rs);
    void move(FPRegister rd, float imm);
    void move(FPRegister rd, GeneralRegister rs);
    void move(GeneralRegister rd, FPRegister rs);

    void moveIf(GeneralRegister rd, GeneralRegister rs, Predicate pred);
    void moveIf(GeneralRegister rd, int32_t imm, Predicate pred);

    void load(GeneralRegister rd, GeneralRegister base, int32_t offset);
    void load(GeneralRegister rd, GeneralRegister base, GeneralRegister offset);
    void load(GeneralRegister rd, Variable *var);
    void load(FPRegister rd, GeneralRegister base, int32_t offset);
    void load(FPRegister rd, GeneralRegister base, GeneralRegister offset);
    void load(FPRegister rd, Variable *var);

    void store(GeneralRegister rs, GeneralRegister base, int32_t offset);
    void store(
        GeneralRegister rs, GeneralRegister base, GeneralRegister offset);
    void store(FPRegister rs, GeneralRegister base, int32_t offset);
    void store(FPRegister rs, GeneralRegister base, GeneralRegister offset);

    void neg(GeneralRegister rd, GeneralRegister rs);
    void neg(FPRegister rd, FPRegister rs);

    void add(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void add(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);
    void add(FPRegister rd, FPRegister lhs, FPRegister rhs);

    void sub(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void sub(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);
    void sub(GeneralRegister rd, int32_t lhs, GeneralRegister rhs);
    void sub(FPRegister rd, FPRegister lhs, FPRegister rhs);

    void mul(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void mul(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);
    void mul(FPRegister rd, FPRegister lhs, FPRegister rhs);

    void div(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void div(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);
    void div(FPRegister rd, FPRegister lhs, FPRegister rhs);

    void mod(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void mod(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);

    void divmod(
        GeneralRegister rd1,
        GeneralRegister rd2,
        GeneralRegister lhs,
        GeneralRegister rhs);
    void divmod(
        GeneralRegister rd1,
        GeneralRegister rd2,
        GeneralRegister lhs,
        int32_t         rhs);

    void bitwiseAnd(
        GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void bitwiseAnd(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);

    void logicalShl(
        GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void logicalShl(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);

    void arithShr(GeneralRegister rd, GeneralRegister lhs, GeneralRegister rhs);
    void arithShr(GeneralRegister rd, GeneralRegister lhs, int32_t rhs);

    void compare(GeneralRegister lhs, GeneralRegister rhs);
    void compare(GeneralRegister lhs, int32_t rhs);
    void compare(FPRegister lhs, FPRegister rhs);

    void test(GeneralRegister lhs, GeneralRegister rhs);
    void test(GeneralRegister lhs, int32_t rhs);

    void call(ir::Function *dest);
    void call(const char *dest);
    void call(GeneralRegister dest);

    void ret();
    void ret(GeneralRegister dest);

    void jmp(const char *dest);
    void jmp(GeneralRegister dest);

    void jmp(const char *dest, Predicate pred);
    void jmp(GeneralRegister dest, Predicate pred);

    void convert(FPRegister rd, GeneralRegister rs, bool isSigned);
    void convert(GeneralRegister rd, FPRegister rs, bool isSigned);
    void convert(GeneralRegister rd, float imm, bool isSigned);
    void convert(FPRegister rd, int32_t imm);
    void convert(FPRegister rd, uint32_t imm);

    template <typename T>
    void push(const T &regs);

    template <typename T>
    void pop(const T &regs);

    inline const std::vector<InstrState> &instrs() const;

    std::string dumpAll() const;
    void        dumpAll(std::ostream &os) const;

public:
    template <typename T>
    static std::vector<RegRange> toRegRanges(const T &regs);

    template <typename R>
    static inline InstrState createReg(InstrKind, R);

    template <typename R1, typename R2>
    static inline InstrState createReg2(InstrKind, R1, R2);

    template <typename R1, typename R2, typename R3>
    static inline InstrState createReg3(InstrKind, R1, R2, R3);

    template <typename R, typename I>
    static inline InstrState createRegImm(InstrKind, R, I);

    template <typename R>
    static inline InstrState createRegLabel(InstrKind, R, std::string_view);

    template <typename R>
    static inline InstrState createRegImmExtend(InstrKind, R, int32_t);

    template <typename R1, typename R2>
    static inline InstrState createRegAddr(InstrKind, R1, R2);

    template <typename R1, typename R2>
    static inline InstrState createRegAddrImmOffset(InstrKind, R1, R2, int32_t);

    template <typename R1, typename R2, typename R3>
    static inline InstrState createRegAddrRegOffset(InstrKind, R1, R2, R3);

    template <typename R1, typename R2, typename I>
    static inline InstrState createRegRegImm(InstrKind, R1, R2, I);

    static inline InstrState createRegRange(InstrKind, const RegRanges &);

    static inline InstrState createLabel(InstrKind, std::string_view);

private:
    std::vector<InstrState> instrs_;
};

} // namespace slime::experimental::backend::ARMv7A

namespace slime::experimental::backend::ARMv7A {

inline std::string_view InstrOp::toString(InstrKind instr) {
    using value_type     = std::underlying_type_t<InstrType>;
    auto           index = static_cast<value_type>(instr);
    constexpr auto last  = static_cast<value_type>(InstrKind::LAST_INSTR);
    assert(index >= 0 && index <= last);
    return INSTR_NAME_TABLE[index];
}

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
