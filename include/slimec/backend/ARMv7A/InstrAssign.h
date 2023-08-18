#pragma once

#include "RegFile.h"
#include "InstrSet.h"

#include <slime/lang/ComparePred.h>
#include <stdint.h>
#include <variant>
#include <string_view>
#include <string>
#include <vector>
#include <ostream>

namespace slimec::ir {
class Function;
} // namespace slimec::ir

namespace slimec::backend::ARMv7A {

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

public:
    using Predicate = slime::lang::ComparePred;

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

} // namespace slimec::backend::ARMv7A

#include "InstrAssign.inl" // IWYU pragma: export
