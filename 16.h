#include "14.h"
#include "18.h"

#include "49.h"
#include "23.h"
#include <initializer_list>
#include <type_traits>
#include <array>
#include <variant>
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <string_view>
#include <ostream>

namespace slime::backend {

enum class InstrKind : uint32_t {
    MOV = 0,      //<! mov
    MOVEQ,        //<! moveq
    MOVNE,        //<! movne
    MOVLE,        //<! movle
    MOVLT,        //<! movlt
    MOVGE,        //<! movge
    MOVGT,        //<! movgt
    ADD,          //<! add
    AND,          //<! and
    ASR,          //<! asr
    B,            //<! b
    BEQ,          //<! beq
    BGE,          //<! bge
    BGT,          //<! bgt
    BHI,          //<! bhi
    BL,           //<! bl
    BLE,          //<! ble
    BLT,          //<! blt
    BNE,          //<! bne
    BPL,          //<! bpl
    BVS,          //<! bvs
    BX,           //<! bx
    CMP,          //<! cmp
    LDR,          //<! ldr
    LSL,          //<! lsl
    MUL,          //<! mul
    POP,          //<! pop
    PUSH,         //<! push
    STR,          //<! str
    SUB,          //<! sub
    RSB,          //<! rsb
    TST,          //<! tst
    VADD_F32,     //<! vadd.f32
    VCMP_F32,     //<! vcmp.f32
    VCVT_F32_S32, //<! vcvt.f32.s32
    VCVT_F32_U32, //<! vcvt.f32.u32
    VCVT_S32_F32, //<! vcvt.s32.f32
    VCVT_U32_F32, //<! vcvt.u32.f32
    VDIV_F32,     //<! vdiv.f32
    VLDR,         //<! vldr
    VMOV,         //<! vmov
    VMUL_F32,     //<! vmul.f32
    VNEG_F32,     //<! vneg.f32
    VPOP,         //<! vpop
    VPUSH,        //<! vpush
    VSTR,         //<! vstr
    VSUB_F32,     //<! vsub.f32
    LAST_INSTR = VSUB_F32,
};

enum class InstrType {
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
    static constexpr auto totalGeneralRegs = 16;
    static constexpr auto totalFpRegs      = 32;

    using RegIdent = int32_t;

    struct RegRange {
        RegIdent first;
        RegIdent last;
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
        RegIdent  regs[3];
        ExtraArg  extra;
    };

    static inline RegIdent toRegIdent(ARMGeneralRegs reg);
    static inline RegIdent toRegIdent(ARMFloatRegs reg);

    static inline std::string_view toString(RegIdent reg);
    static inline std::string_view toString(InstrKind instr);

protected:
    static constexpr std::
        array<std::string_view, totalGeneralRegs + totalFpRegs>
            REG_NAME_TABLE{
                "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
                "r8",  "r9",  "r10", "r11", "ip",  "sp",  "lr",  "pc",
                "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
                "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
                "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
                "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
            };

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

    void move(ARMGeneralRegs rd, ARMGeneralRegs rs);
    void move(ARMGeneralRegs rd, int32_t imm);
    void move(ARMFloatRegs rd, ARMFloatRegs rs);
    void move(ARMFloatRegs rd, float imm);
    void move(ARMFloatRegs rd, ARMGeneralRegs rs);
    void move(ARMGeneralRegs rd, ARMFloatRegs rs);

    void moveIf(ARMGeneralRegs rd, ARMGeneralRegs rs, Predicate pred);
    void moveIf(ARMGeneralRegs rd, int32_t imm, Predicate pred);

    void load(ARMGeneralRegs rd, ARMGeneralRegs base, int32_t offset);
    void load(ARMGeneralRegs rd, ARMGeneralRegs base, ARMGeneralRegs offset);
    void load(ARMGeneralRegs rd, Variable *var);
    void load(ARMFloatRegs rd, ARMGeneralRegs base, int32_t offset);
    void load(ARMFloatRegs rd, ARMGeneralRegs base, ARMGeneralRegs offset);
    void load(ARMFloatRegs rd, Variable *var);

    void store(ARMGeneralRegs rs, ARMGeneralRegs base, int32_t offset);
    void store(ARMGeneralRegs rs, ARMGeneralRegs base, ARMGeneralRegs offset);
    void store(ARMFloatRegs rs, ARMGeneralRegs base, int32_t offset);
    void store(ARMFloatRegs rs, ARMGeneralRegs base, ARMGeneralRegs offset);

    void neg(ARMGeneralRegs rd, ARMGeneralRegs rs);
    void neg(ARMFloatRegs rd, ARMFloatRegs rs);

    void add(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void add(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);
    void add(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs);

    void sub(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void sub(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);
    void sub(ARMGeneralRegs rd, int32_t lhs, ARMGeneralRegs rhs);
    void sub(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs);

    void mul(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void mul(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);
    void mul(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs);

    void div(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void div(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);
    void div(ARMFloatRegs rd, ARMFloatRegs lhs, ARMFloatRegs rhs);

    void mod(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void mod(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);

    void divmod(
        ARMGeneralRegs rd1,
        ARMGeneralRegs rd2,
        ARMGeneralRegs lhs,
        ARMGeneralRegs rhs);
    void divmod(
        ARMGeneralRegs rd1,
        ARMGeneralRegs rd2,
        ARMGeneralRegs lhs,
        int32_t        rhs);

    void bitwiseAnd(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void bitwiseAnd(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);

    void logicalShl(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void logicalShl(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);

    void arithShr(ARMGeneralRegs rd, ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void arithShr(ARMGeneralRegs rd, ARMGeneralRegs lhs, int32_t rhs);

    void compare(ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void compare(ARMGeneralRegs lhs, int32_t rhs);
    void compare(ARMFloatRegs lhs, ARMFloatRegs rhs);

    void test(ARMGeneralRegs lhs, ARMGeneralRegs rhs);
    void test(ARMGeneralRegs lhs, int32_t rhs);

    void call(Function *dest);
    void call(const char *dest);
    void call(ARMGeneralRegs dest);

    void ret();
    void ret(ARMGeneralRegs dest);

    void jmp(const char *dest);
    void jmp(ARMGeneralRegs dest);

    void jmp(const char *dest, Predicate pred);
    void jmp(ARMGeneralRegs dest, Predicate pred);

    void convert(ARMFloatRegs rd, ARMGeneralRegs rs, bool isSigned);
    void convert(ARMGeneralRegs rd, ARMFloatRegs rs, bool isSigned);
    void convert(ARMGeneralRegs rd, float imm, bool isSigned);
    void convert(ARMFloatRegs rd, int32_t imm);
    void convert(ARMFloatRegs rd, uint32_t imm);

    template <typename T>
    void push(const T &regs);

    template <typename T>
    void pop(const T &regs);

    inline const std::vector<InstrState> &instrs() const;

    std::string dumpAll() const;
    void        dumpAll(std::ostream& os) const;

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
    Generator              *parent_;
};

} // namespace slime::backend

namespace slime::backend {

inline auto InstrOp::toRegIdent(ARMGeneralRegs reg) -> RegIdent {
    assert(reg != ARMGeneralRegs::None);
    return static_cast<RegIdent>(reg);
}

inline auto InstrOp::toRegIdent(ARMFloatRegs reg) -> RegIdent {
    assert(reg != ARMFloatRegs::None);
    return static_cast<RegIdent>(reg) + totalGeneralRegs;
}

inline std::string_view InstrOp::toString(RegIdent reg) {
    assert(reg >= 0 && reg < totalGeneralRegs + totalFpRegs);
    return REG_NAME_TABLE[reg];
}

inline std::string_view InstrOp::toString(InstrKind instr) {
    using value_type     = std::underlying_type_t<TypeKind>;
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
    if constexpr (is_iterable_as<R, ARMGeneralRegs>) {
        ranges   = toRegRanges(regs);
        isFpRegs = false;
    } else if constexpr (is_iterable_as<R, ARMFloatRegs>) {
        ranges   = toRegRanges(regs);
        isFpRegs = true;
    } else if constexpr (std::is_same_v<R, ARMGeneralRegs>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = false;
    } else if constexpr (std::is_same_v<R, ARMFloatRegs>) {
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
    if constexpr (is_iterable_as<R, ARMGeneralRegs>) {
        ranges   = toRegRanges(regs);
        isFpRegs = false;
    } else if constexpr (is_iterable_as<R, ARMFloatRegs>) {
        ranges   = toRegRanges(regs);
        isFpRegs = true;
    } else if constexpr (std::is_same_v<R, ARMGeneralRegs>) {
        ranges   = toRegRanges(std::initializer_list{regs});
        isFpRegs = false;
    } else if constexpr (std::is_same_v<R, ARMFloatRegs>) {
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
        std::is_same_v<E, ARMGeneralRegs> || std::is_same_v<E, ARMFloatRegs>);

    std::vector<RegIdent> idents;
    for (auto reg : regs) { idents.push_back(toRegIdent(reg)); }

    std::vector<RegRange> resp;
    if (idents.empty()) { return resp; }
    std::sort(idents.begin(), idents.end());
    int first = 0, last = first;
    do {
        while (last + 1 < idents.size()) {
            if (idents[last + 1] <= idents[last] + 1) {
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
    instr.regs[0] = toRegIdent(reg);
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createReg2(InstrKind kind, R1 r1, R2 r2) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::Reg2;
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
    return instr;
}

template <typename R1, typename R2, typename R3>
inline auto InstrOp::createReg3(InstrKind kind, R1 r1, R2 r2, R3 r3)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::Reg3;
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
    instr.regs[2] = toRegIdent(r3);
    return instr;
}

template <typename R, typename I>
inline auto InstrOp::createRegImm(InstrKind kind, R reg, I imm) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegImm;
    instr.regs[0] = toRegIdent(reg);
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
    instr.regs[0] = toRegIdent(reg);
    instr.extra   = std::string{label};
    return instr;
}

template <typename R>
inline auto InstrOp::createRegImmExtend(InstrKind kind, R reg, int32_t imm)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegImmExtend;
    instr.regs[0] = toRegIdent(reg);
    instr.isFpImm = false;
    instr.extra   = imm;
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createRegAddr(InstrKind kind, R1 r1, R2 r2) -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegAddr;
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
    return instr;
}

template <typename R1, typename R2>
inline auto InstrOp::createRegAddrImmOffset(
    InstrKind kind, R1 r1, R2 r2, int32_t imm) -> InstrState {
    if (imm == 0) { return createRegAddr(kind, r1, r2); }
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegAddrImmOffset;
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
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
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
    instr.regs[2] = toRegIdent(r3);
    return instr;
}

template <typename R1, typename R2, typename I>
inline auto InstrOp::createRegRegImm(InstrKind kind, R1 r1, R2 r2, I imm)
    -> InstrState {
    InstrState instr;
    instr.kind    = kind;
    instr.type    = InstrType::RegRegImm;
    instr.regs[0] = toRegIdent(r1);
    instr.regs[1] = toRegIdent(r2);
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
    instr.extra = std::string{label};
    return instr;
}

} // namespace slime::backend
