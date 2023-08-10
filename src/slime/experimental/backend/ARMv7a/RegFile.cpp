#include "RegFile.h"

#include <array>

namespace slime::experimental::backend::ARMv7a {

uint8_t Register::id() const {
    if (!isValid()) {
        return idOf(asNonRegister());
    } else if (isGeneralRegister()) {
        return idOf(asGeneralRegister());
    } else if (isFPRegister()) {
        return idOf(asFPRegister());
    } else {
        unreachable();
    }
}

uint8_t Register::uniqueId() const {
    if (!isValid()) {
        return uniqueIdOf(asNonRegister());
    } else if (isGeneralRegister()) {
        return uniqueIdOf(asGeneralRegister());
    } else if (isFPRegister()) {
        return uniqueIdOf(asFPRegister());
    } else {
        unreachable();
    }
}

std::string_view Register::toString() const {
    static constexpr std::array<std::string_view, totalValidRegisters()>
        REG_NAME_TABLE{
            "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
            "r8",  "r9",  "r10", "r11", "ip",  "sp",  "lr",  "pc",
            "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
            "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
            "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
            "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
        };
    assert(uniqueId() < totalValidRegisters());
    return REG_NAME_TABLE[uniqueId()];
}

} // namespace slime::experimental::backend::ARMv7a
