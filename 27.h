#pragma once

#include "26.h"

namespace slime::experimental::ir {

class AllocaInst;
class LoadInst;
class StoreInst;
class RetInst;
class BrInst;
class GetElementPtrInst;
class AddInst;
class SubInst;
class MulInst;
class UDivInst;
class SDivInst;
class URemInst;
class SRemInst;
class FNegInst;
class FAddInst;
class FSubInst;
class FMulInst;
class FDivInst;
class FRemInst;
class ShlInst;
class LShrInst;
class AShrInst;
class AndInst;
class OrInst;
class XorInst;
class FPToUIInst;
class FPToSIInst;
class UIToFPInst;
class SIToFPInst;
class ZExtInst;
class TruncInst;
class ICmpInst;
class FCmpInst;
class PhiInst;
class CallInst;

class AllocaInst
    : public Instruction
    , public SpecificUser<0> {};

class LoadInst
    : public Instruction
    , public SpecificUser<1> {};

class StoreInst
    : public Instruction
    , public SpecificUser<2> {};

class RetInst
    : public Instruction
    , public SpecificUser<-1> {};

class BrInst
    : public Instruction
    , public SpecificUser<-1> {};

class GetElementPtrInst
    : public Instruction
    , public SpecificUser<-1> {};

class AddInst
    : public Instruction
    , public SpecificUser<2> {};

class SubInst
    : public Instruction
    , public SpecificUser<2> {};

class MulInst
    : public Instruction
    , public SpecificUser<2> {};

class UDivInst
    : public Instruction
    , public SpecificUser<2> {};

class SDivInst
    : public Instruction
    , public SpecificUser<2> {};

class URemInst
    : public Instruction
    , public SpecificUser<2> {};

class SRemInst
    : public Instruction
    , public SpecificUser<2> {};

class FNegInst
    : public Instruction
    , public SpecificUser<1> {};

class FAddInst
    : public Instruction
    , public SpecificUser<2> {};

class FSubInst
    : public Instruction
    , public SpecificUser<2> {};

class FMulInst
    : public Instruction
    , public SpecificUser<2> {};

class FDivInst
    : public Instruction
    , public SpecificUser<2> {};

class FRemInst
    : public Instruction
    , public SpecificUser<2> {};

class ShlInst
    : public Instruction
    , public SpecificUser<2> {};

class LShrInst
    : public Instruction
    , public SpecificUser<2> {};

class AShrInst
    : public Instruction
    , public SpecificUser<2> {};

class AndInst
    : public Instruction
    , public SpecificUser<2> {};

class OrInst
    : public Instruction
    , public SpecificUser<2> {};

class XorInst
    : public Instruction
    , public SpecificUser<2> {};

class FPToUIInst
    : public Instruction
    , public SpecificUser<1> {};

class FPToSIInst
    : public Instruction
    , public SpecificUser<1> {};

class UIToFPInst
    : public Instruction
    , public SpecificUser<1> {};

class SIToFPInst
    : public Instruction
    , public SpecificUser<1> {};

class ZExtInst
    : public Instruction
    , public SpecificUser<1> {};

class TruncInst
    : public Instruction
    , public SpecificUser<1> {};

class ICmpInst
    : public Instruction
    , public SpecificUser<2> {};

class FCmpInst
    : public Instruction
    , public SpecificUser<2> {};

class PhiInst
    : public Instruction
    , public SpecificUser<-1> {};

class CallInst
    : public Instruction
    , public SpecificUser<-1> {};

} // namespace slime::experimental::ir

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::AllocaInst,
    slime::experimental::ir::InstrID::Alloca);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::LoadInst, slime::experimental::ir::InstrID::Load);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::StoreInst,
    slime::experimental::ir::InstrID::Store);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::RetInst, slime::experimental::ir::InstrID::Ret);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::BrInst, slime::experimental::ir::InstrID::Br);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::GetElementPtrInst,
    slime::experimental::ir::InstrID::GetElementPtr);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::AddInst, slime::experimental::ir::InstrID::Add);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::SubInst, slime::experimental::ir::InstrID::Sub);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::MulInst, slime::experimental::ir::InstrID::Mul);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::UDivInst, slime::experimental::ir::InstrID::UDiv);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::SDivInst, slime::experimental::ir::InstrID::SDiv);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::URemInst, slime::experimental::ir::InstrID::URem);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::SRemInst, slime::experimental::ir::InstrID::SRem);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FNegInst, slime::experimental::ir::InstrID::FNeg);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FAddInst, slime::experimental::ir::InstrID::FAdd);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FSubInst, slime::experimental::ir::InstrID::FSub);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FMulInst, slime::experimental::ir::InstrID::FMul);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FDivInst, slime::experimental::ir::InstrID::FDiv);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FRemInst, slime::experimental::ir::InstrID::FRem);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::ShlInst, slime::experimental::ir::InstrID::Shl);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::LShrInst, slime::experimental::ir::InstrID::LShr);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::AShrInst, slime::experimental::ir::InstrID::AShr);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::AndInst, slime::experimental::ir::InstrID::And);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::OrInst, slime::experimental::ir::InstrID::Or);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::XorInst, slime::experimental::ir::InstrID::Xor);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FPToUIInst,
    slime::experimental::ir::InstrID::FPToUI);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FPToSIInst,
    slime::experimental::ir::InstrID::FPToSI);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::UIToFPInst,
    slime::experimental::ir::InstrID::UIToFP);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::SIToFPInst,
    slime::experimental::ir::InstrID::SIToFP);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::ZExtInst, slime::experimental::ir::InstrID::ZExt);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::TruncInst,
    slime::experimental::ir::InstrID::Trunc);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::ICmpInst, slime::experimental::ir::InstrID::ICmp);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::FCmpInst, slime::experimental::ir::InstrID::FCmp);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::PhiInst, slime::experimental::ir::InstrID::Phi);

emit(auto) slime::experimental::ir::InstructionBase::declareTryIntoItem(
    slime::experimental::ir::CallInst, slime::experimental::ir::InstrID::Call);
