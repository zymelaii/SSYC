#pragma once

#include <slime/experimental/Utility.h>
#include <stdint.h>
#include <assert.h>
#include <limits>
#include <string_view>
#include <variant>

namespace slime::experimental::backend::ARMv7A {

enum class GeneralRegister : uint8_t;
enum class FPRegister : uint8_t;

struct NonRegisterType {
    inline constexpr uint8_t id() const;
};

static constexpr inline NonRegisterType NonRegister;

class Register final
    : public std::variant<NonRegisterType, GeneralRegister, FPRegister> {
private:
    using base_type =
        std::variant<NonRegisterType, GeneralRegister, FPRegister>;

public:
    inline Register() = default;

    template <typename R>
    inline Register(R reg);

    template <typename R>
    inline Register& operator=(R reg);

    inline void reset();

    inline bool isValid() const;

    inline bool isGeneralRegister() const;
    inline bool isFPRegister() const;

    inline NonRegisterType asNonRegister() const;
    inline GeneralRegister asGeneralRegister() const;
    inline FPRegister      asFPRegister() const;

    template <typename R>
    inline bool operator==(R reg) const;

    template <typename R>
    inline bool operator!=(R reg) const;

    inline bool operator>(GeneralRegister reg) const;
    inline bool operator<(GeneralRegister reg) const;
    inline bool operator>=(GeneralRegister reg) const;
    inline bool operator<=(GeneralRegister reg) const;

    inline bool operator<(FPRegister reg) const;
    inline bool operator>(FPRegister reg) const;
    inline bool operator<=(FPRegister reg) const;
    inline bool operator>=(FPRegister reg) const;

    inline bool operator==(uint8_t id) const;
    inline bool operator!=(uint8_t id) const;

    uint8_t id() const;
    uint8_t uniqueId() const;

    static inline uint8_t           idOf(Register reg);
    static inline constexpr uint8_t idOf(NonRegisterType reg);
    static inline constexpr uint8_t idOf(GeneralRegister reg);
    static inline constexpr uint8_t idOf(FPRegister reg);

    static inline uint8_t           uniqueIdOf(Register reg);
    static inline constexpr uint8_t uniqueIdOf(NonRegisterType reg);
    static inline constexpr uint8_t uniqueIdOf(GeneralRegister reg);
    static inline constexpr uint8_t uniqueIdOf(FPRegister reg);

    static inline constexpr size_t totalValidRegisters();

    static inline std::string_view toString(const Register& reg);
    std::string_view               toString() const;
};

enum class GeneralRegister : uint8_t {
    R0    = 0,
    FIRST = R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    IP,
    SP,
    LR,
    PC,
    LAST = PC,
};

enum class FPRegister : uint8_t {
    S0    = 0,
    FIRST = S0,
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
    S8,
    S9,
    S10,
    S11,
    S12,
    S13,
    S14,
    S15,
    S16,
    S17,
    S18,
    S19,
    S20,
    S21,
    S22,
    S23,
    S24,
    S25,
    S26,
    S27,
    S28,
    S29,
    S30,
    S31,
    LAST = S31,
};

} // namespace slime::experimental::backend::ARMv7A

#include "RegFile.inl"
