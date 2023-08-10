#pragma once

#include "32.h"
#include "36.h"

#include "40.h"

namespace slime::experimental::ir {

enum class InstrID {
    Alloca,        //<! alloca
    Load,          //<! load
    Store,         //<! store
    Ret,           //<! return
    Br,            //<! branch
    GetElementPtr, //<! getelementptr
    Add,           //<! add
    Sub,           //<! sub
    Mul,           //<! mul
    UDiv,          //<! div (unsigned)
    SDiv,          //<! div (signed)
    URem,          //<! remainder (unsigned)
    SRem,          //<! remainder (signed)
    FNeg,          //<! neg (float)
    FAdd,          //<! add (float)
    FSub,          //<! sub (float)
    FMul,          //<! mul (float)
    FDiv,          //<! div (float)
    FRem,          //<! remainder (float)
    Shl,           //<! shl
    LShr,          //<! shr (logical)
    AShr,          //<! shr (arithmetic)
    And,           //<! bitwise and
    Or,            //<! bitwise or
    Xor,           //<! xor
    FPToUI,        //<! floating point -> unsigned int
    FPToSI,        //<! floating point -> signed int
    UIToFP,        //<! unsigned int -> floating point
    SIToFP,        //<! signed int -> floating point
    ZExt,          //<! zext
    Trunc,         //<! trunc
    ICmp,          //<! cmp (int)
    FCmp,          //<! cmp (float)
    Phi,           //<! Phi node instruction
    Call,          //<! call
    LAST_INSTR = Call,
};

class InstructionBaseImpl {
protected:
    inline InstructionBaseImpl(InstrID id, User* self);

public:
    inline InstrID id() const;
    inline User*   unwrap() const;

private:
    InstrID                     id_;
    User* const                 source_;
    BasicBlock*                 parent_;
    InstructionList::node_type* self_;
};

using InstructionBase =
    EnumBasedTryIntoTraitWrapper<InstructionBaseImpl, &InstructionBaseImpl::id>;

class Instruction : public InstructionBase {
protected:
    inline Instruction(InstrID id, User* self);
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline InstructionBaseImpl::InstructionBaseImpl(InstrID id, User* source)
    : id_{id}
    , source_{source->withDelegator(this)}
    , parent_{nullptr}
    , self_{nullptr} {}

inline InstrID InstructionBaseImpl::id() const {
    return id_;
}

inline User* InstructionBaseImpl::unwrap() const {
    return source_;
}

inline Instruction::Instruction(InstrID id, User* self)
    : InstructionBase(id, self) {}

} // namespace slime::experimental::ir

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::Instruction>() const {
    return isInstruction();
}

emit(auto) slime::experimental::ir::ValueBase::as<
    slime::experimental::ir::Instruction>() {
    auto delegator = as<experimental::ir::User>()->delegator();
    return reinterpret_cast<slime::experimental::ir::Instruction*>(delegator);
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::User>() const {
    return true;
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::SpecificUser<-1>>() const {
    return unwrap()->is<slime::experimental::ir::SpecificUser<-1>>();
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::SpecificUser<0>>() const {
    return unwrap()->is<slime::experimental::ir::SpecificUser<0>>();
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::SpecificUser<1>>() const {
    return unwrap()->is<slime::experimental::ir::SpecificUser<1>>();
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::SpecificUser<2>>() const {
    return unwrap()->is<slime::experimental::ir::SpecificUser<2>>();
}

emit(bool) slime::experimental::ir::InstructionBase::is<
    slime::experimental::ir::SpecificUser<3>>() const {
    return unwrap()->is<slime::experimental::ir::SpecificUser<3>>();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::User>() {
    return unwrap();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::SpecificUser<-1>>() {
    return unwrap()->as<slime::experimental::ir::SpecificUser<-1>>();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::SpecificUser<0>>() {
    return unwrap()->as<slime::experimental::ir::SpecificUser<0>>();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::SpecificUser<1>>() {
    return unwrap()->as<slime::experimental::ir::SpecificUser<1>>();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::SpecificUser<2>>() {
    return unwrap()->as<slime::experimental::ir::SpecificUser<2>>();
}

emit(auto) slime::experimental::ir::InstructionBase::as<
    slime::experimental::ir::SpecificUser<3>>() {
    return unwrap()->as<slime::experimental::ir::SpecificUser<3>>();
}
