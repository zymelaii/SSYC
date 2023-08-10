#include <slime/experimental/Utility.h>
#include <stdint.h>
#include <assert.h>
#include <limits>
#include <string_view>
#include <variant>

namespace slime::experimental::backend::ARMv7a {

enum class GeneralRegister : uint8_t;
enum class FPRegister : uint8_t;

struct NonRegisterType {
    constexpr uint8_t id() const {
        return std::numeric_limits<uint8_t>::max();
    }
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

} // namespace slime::experimental::backend::ARMv7a

namespace slime::experimental::backend::ARMv7a {

template <typename R>
inline Register::Register(R reg) {
    operator=(reg);
}

template <typename R>
inline Register& Register::operator=(R reg) {
    using value_type = std::decay_t<NonRegisterType>;
    static_assert(
        std::is_same_v<Register, value_type>
        || std::is_same_v<NonRegisterType, value_type>
        || std::is_same_v<GeneralRegister, value_type>
        || std::is_same_v<FPRegister, value_type>);
    *static_cast<base_type*>(this) = reg;
    return *this;
}

inline void Register::reset() {
    *this = NonRegister;
}

inline bool Register::isValid() const {
    return std::get_if<NonRegisterType>(this) != nullptr;
}

inline bool Register::isGeneralRegister() const {
    return std::get_if<GeneralRegister>(this) != nullptr;
}

inline bool Register::isFPRegister() const {
    return std::get_if<FPRegister>(this) != nullptr;
}

inline NonRegisterType Register::asNonRegister() const {
    return std::get<NonRegisterType>(*this);
}

inline GeneralRegister Register::asGeneralRegister() const {
    return std::get<GeneralRegister>(*this);
}

inline FPRegister Register::asFPRegister() const {
    return std::get<FPRegister>(*this);
}

template <typename R>
inline bool Register::operator==(R reg) const {
    using value_type = std::decay_t<NonRegisterType>;
    static_assert(
        std::is_same_v<Register, value_type>
        || std::is_same_v<NonRegisterType, value_type>
        || std::is_same_v<GeneralRegister, value_type>
        || std::is_same_v<FPRegister, value_type>);
    return uniqueId() == uniqueIdOf(reg);
}

template <typename R>
inline bool Register::operator!=(R reg) const {
    using value_type = std::decay_t<NonRegisterType>;
    static_assert(
        std::is_same_v<Register, value_type>
        || std::is_same_v<NonRegisterType, value_type>
        || std::is_same_v<GeneralRegister, value_type>
        || std::is_same_v<FPRegister, value_type>);
    return uniqueId() = uniqueIdOf(reg);
}

inline bool Register::operator>(GeneralRegister reg) const {
    assert(isGeneralRegister());
    return id() > idOf(reg);
}

inline bool Register::operator<(GeneralRegister reg) const {
    assert(isGeneralRegister());
    return id() < idOf(reg);
}

inline bool Register::operator>=(GeneralRegister reg) const {
    assert(isGeneralRegister());
    return id() >= idOf(reg);
}

inline bool Register::operator<=(GeneralRegister reg) const {
    assert(isGeneralRegister());
    return id() <= idOf(reg);
}

inline bool Register::operator<(FPRegister reg) const {
    assert(isFPRegister());
    return id() < idOf(reg);
}

inline bool Register::operator>(FPRegister reg) const {
    assert(isFPRegister());
    return id() > idOf(reg);
}

inline bool Register::operator<=(FPRegister reg) const {
    assert(isFPRegister());
    return id() <= idOf(reg);
}

inline bool Register::operator>=(FPRegister reg) const {
    assert(isFPRegister());
    return id() >= idOf(reg);
}

inline bool Register::operator==(uint8_t id) const {
    return uniqueId() == id;
}

inline bool Register::operator!=(uint8_t id) const {
    return uniqueId() != id;
}

inline uint8_t Register::idOf(Register reg) {
    return reg.id();
}

inline constexpr uint8_t Register::idOf(NonRegisterType reg) {
    return reg.id();
}

inline constexpr uint8_t Register::idOf(GeneralRegister reg) {
    return static_cast<std::underlying_type_t<GeneralRegister>>(reg);
}

inline constexpr uint8_t Register::idOf(FPRegister reg) {
    return static_cast<std::underlying_type_t<FPRegister>>(reg);
}

inline uint8_t Register::uniqueIdOf(Register reg) {
    return reg.uniqueId();
}

inline constexpr uint8_t Register::uniqueIdOf(NonRegisterType reg) {
    return idOf(reg);
}

inline constexpr uint8_t Register::uniqueIdOf(GeneralRegister reg) {
    return idOf(reg);
}

inline constexpr uint8_t Register::uniqueIdOf(FPRegister reg) {
    return idOf(reg) + idOf(GeneralRegister::LAST) + 1;
}

inline constexpr size_t Register::totalValidRegisters() {
    return idOf(GeneralRegister::LAST) + idOf(FPRegister::LAST) + 2;
}

static inline std::string_view toString(const Register& reg) {
    return reg.toString();
}

} // namespace slime::experimental::backend::ARMv7a
