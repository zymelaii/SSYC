#pragma once

#include "RegFile.h"

namespace slime::experimental::backend::ARMv7A {

inline constexpr uint8_t NonRegisterType::id() const {
    return std::numeric_limits<uint8_t>::max();
}

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

} // namespace slime::experimental::backend::ARMv7A
