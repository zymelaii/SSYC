#pragma once

#include "value.h"
#include "user.h"
#include "instruction.def"
#include "../utils/traits.h"

#include <assert.h>

namespace slime::ir {

class BasicBlock;
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
class ICmpInst;
class FCmpInst;
class PhiInst;
class CallInst;

std::string_view getPredicateName(ComparePredicationType predicate);

class Instruction {
protected:
    Instruction(InstructionID id, Value *instruction)
        : id_{id}
        , instruction_{instruction} {
        instruction->setPatch(reinterpret_cast<void *>(this));
    }

public:
    InstructionID id() const {
        return id_;
    }

    Value *unwrap() const {
        return instruction_;
    }

    BasicBlock *parent() const {
        return parent_;
    }

    bool insertToHead(BasicBlock *block);
    bool insertToTail(BasicBlock *block);
    bool insertBefore(Instruction *inst);
    bool insertAfter(Instruction *inst);
    bool moveToPrev();
    bool moveToNext();
    bool removeFromBlock();

    static inline AllocaInst *createAlloca(Type *type);

    static inline LoadInst  *createLoad(Value *address);
    static inline StoreInst *createStore(Value *address, Value *value);

    static inline RetInst *createRet(Value *value = nullptr);
    static inline BrInst  *createBr(BasicBlock *block);
    static inline BrInst  *createBr(
         Value *condition, BasicBlock *branchIf, BasicBlock *branchElse);

    static inline GetElementPtrInst *createGetElementPtr(
        Value *address, Value *index);

    static inline AddInst  *createAdd(Value *lhs, Value *rhs);
    static inline SubInst  *createSub(Value *lhs, Value *rhs);
    static inline MulInst  *createMul(Value *lhs, Value *rhs);
    static inline UDivInst *createUDiv(Value *lhs, Value *rhs);
    static inline SDivInst *createSDiv(Value *lhs, Value *rhs);
    static inline URemInst *createURem(Value *lhs, Value *rhs);
    static inline SRemInst *createSRem(Value *lhs, Value *rhs);

    static inline FNegInst *createFNeg(Value *value);
    static inline FAddInst *createFAdd(Value *lhs, Value *rhs);
    static inline FSubInst *createFSub(Value *lhs, Value *rhs);
    static inline FMulInst *createFMul(Value *lhs, Value *rhs);
    static inline FDivInst *createFDiv(Value *lhs, Value *rhs);
    static inline FRemInst *createFRem(Value *lhs, Value *rhs);

    static inline ShlInst  *createShl(Value *lhs, Value *rhs);
    static inline LShrInst *createLShr(Value *lhs, Value *rhs);
    static inline AShrInst *createAShr(Value *lhs, Value *rhs);
    static inline AndInst  *createAnd(Value *lhs, Value *rhs);
    static inline OrInst   *createOr(Value *lhs, Value *rhs);
    static inline XorInst  *createXor(Value *lhs, Value *rhs);

    static inline FPToUIInst *createFPToUI(Value *value);
    static inline FPToSIInst *createFPToSI(Value *value);
    static inline UIToFPInst *createUIToFP(Value *value);
    static inline SIToFPInst *createSIToFP(Value *value);

    static inline ICmpInst *createICmp(
        ComparePredicationType op, Value *lhs, Value *rhs);
    static inline FCmpInst *createFCmp(
        ComparePredicationType op, Value *lhs, Value *rhs);

    static inline PhiInst *createPhi(Type *type);

    static inline CallInst *createCall(Function *callee);

private:
    const InstructionID    id_;
    Value *const           instruction_;
    BasicBlock            *parent_ = nullptr;
    BasicBlock::node_type *self_   = nullptr;
};

class AllocaInst final
    : public Instruction
    , public User<0>
    , public utils::BuildTrait<AllocaInst> {
public:
    AllocaInst(Type *type)
        : Instruction(InstructionID::Alloca, this)
        , User<0>(Type::createPointerType(type), ValueTag::Instruction | 0) {}
};

class LoadInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<LoadInst> {
public:
    LoadInst(Value *address)
        : Instruction(InstructionID::Load, this)
        , User<1>(
              address->type()->tryGetElementType(), ValueTag::Instruction | 0) {
        operand() = address;
    }
};

class StoreInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<StoreInst> {
public:
    StoreInst(Value *address, Value *value)
        : Instruction(InstructionID::Store, this)
        , User<2>(Type::getVoidType(), ValueTag::Instruction | 0) {
        lhs() = address;
        rhs() = value;
    }
};

class RetInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<RetInst> {
public:
    RetInst()
        : Instruction(InstructionID::Ret, this)
        , User<1>(Type::getVoidType(), ValueTag::Instruction | 0) {}

    RetInst(Value *value)
        : Instruction(InstructionID::Ret, this)
        , User<1>(value->type(), ValueTag::Instruction | 0) {
        assert(value != nullptr);
        operand() = value;
    }
};

class BrInst final
    : public Instruction
    , public User<3>
    , public utils::BuildTrait<BrInst> {
public:
    BrInst(BasicBlock *block)
        : Instruction(InstructionID::Br, this)
        , User<3>(Type::getVoidType(), ValueTag::Instruction | 0) {
        op<0>() = block;
    }

    BrInst(Value *condition, BasicBlock *branchIf, BasicBlock *branchElse)
        : Instruction(InstructionID::Br, this)
        , User<3>(Type::getVoidType(), ValueTag::Instruction | 0) {
        op<0>() = condition;
        op<1>() = branchIf;
        op<2>() = branchElse;
    }
};

class GetElementPtrInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<GetElementPtrInst> {
public:
    GetElementPtrInst(Value *address, Value *index)
        : Instruction(InstructionID::GetElementPtr, this)
        , User<2>(
              address->type()->tryGetElementType(), ValueTag::Instruction | 0) {
        lhs() = address;
        rhs() = index;
    }
};

class AddInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<AddInst> {
public:
    AddInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Add, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class SubInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<SubInst> {
public:
    SubInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Sub, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class MulInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<MulInst> {
public:
    MulInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Mul, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class UDivInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<UDivInst> {
public:
    UDivInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::UDiv, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class SDivInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<SDivInst> {
public:
    SDivInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::SDiv, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class URemInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<URemInst> {
public:
    URemInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::URem, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class SRemInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<SRemInst> {
public:
    SRemInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::SRem, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FNegInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<FNegInst> {
public:
    FNegInst(Value *value)
        : Instruction(InstructionID::FNeg, this)
        , User<1>(Type::getFloatType(), ValueTag::Instruction | 0) {
        operand() = value;
    }
};

class FAddInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FAddInst> {
public:
    FAddInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::FAdd, this)
        , User<2>(Type::getFloatType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FSubInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FSubInst> {
public:
    FSubInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::FSub, this)
        , User<2>(Type::getFloatType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FMulInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FMulInst> {
public:
    FMulInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::FMul, this)
        , User<2>(Type::getFloatType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FDivInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FDivInst> {
public:
    FDivInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::FDiv, this)
        , User<2>(Type::getFloatType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FRemInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FRemInst> {
public:
    FRemInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::FRem, this)
        , User<2>(Type::getFloatType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class ShlInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<ShlInst> {
public:
    ShlInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Shl, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class LShrInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<LShrInst> {
public:
    LShrInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::LShr, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class AShrInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<AShrInst> {
public:
    AShrInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::AShr, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class AndInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<AndInst> {
public:
    AndInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::And, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class OrInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<OrInst> {
public:
    OrInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Or, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class XorInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<XorInst> {
public:
    XorInst(Value *lhs, Value *rhs)
        : Instruction(InstructionID::Xor, this)
        , User<2>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }
};

class FPToUIInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<FPToUIInst> {
public:
    FPToUIInst(Value *value)
        : Instruction(InstructionID::FPToUI, this)
        , User<1>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        operand() = value;
    }
};

class FPToSIInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<FPToSIInst> {
public:
    FPToSIInst(Value *value)
        : Instruction(InstructionID::FPToSI, this)
        , User<1>(Type::getIntegerType(), ValueTag::Instruction | 0) {
        operand() = value;
    }
};

class UIToFPInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<UIToFPInst> {
public:
    UIToFPInst(Value *value)
        : Instruction(InstructionID::UIToFP, this)
        , User<1>(Type::getFloatType(), ValueTag::Instruction | 0) {
        operand() = value;
    }
};

class SIToFPInst final
    : public Instruction
    , public User<1>
    , public utils::BuildTrait<SIToFPInst> {
public:
    SIToFPInst(Value *value)
        : Instruction(InstructionID::SIToFP, this)
        , User<1>(Type::getFloatType(), ValueTag::Instruction | 0) {
        operand() = value;
    }
};

class ICmpInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<ICmpInst> {
public:
    ICmpInst(ComparePredicationType cmp, Value *lhs, Value *rhs)
        : Instruction(InstructionID::ICmp, this)
        , User<2>(Type::getIntegerType(), ValueTag::CompareInst | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }

    ComparePredicationType predicate() const {
        return predicate_;
    }

    static inline ICmpInst *createEQ(Value *lhs, Value *rhs);
    static inline ICmpInst *createNE(Value *lhs, Value *rhs);
    static inline ICmpInst *createUGT(Value *lhs, Value *rhs);
    static inline ICmpInst *createUGE(Value *lhs, Value *rhs);
    static inline ICmpInst *createULT(Value *lhs, Value *rhs);
    static inline ICmpInst *createULE(Value *lhs, Value *rhs);
    static inline ICmpInst *createSGT(Value *lhs, Value *rhs);
    static inline ICmpInst *createSGE(Value *lhs, Value *rhs);
    static inline ICmpInst *createSLT(Value *lhs, Value *rhs);
    static inline ICmpInst *createSLE(Value *lhs, Value *rhs);

private:
    ComparePredicationType predicate_;
};

class FCmpInst final
    : public Instruction
    , public User<2>
    , public utils::BuildTrait<FCmpInst> {
public:
    FCmpInst(ComparePredicationType cmp, Value *lhs, Value *rhs)
        : Instruction(InstructionID::FCmp, this)
        , User<2>(Type::getIntegerType(), ValueTag::CompareInst | 0) {
        this->lhs() = lhs;
        this->rhs() = rhs;
    }

    ComparePredicationType predicate() const {
        return predicate_;
    }

    static inline FCmpInst *createFALSE(Value *lhs, Value *rhs);
    static inline FCmpInst *createOEQ(Value *lhs, Value *rhs);
    static inline FCmpInst *createOGT(Value *lhs, Value *rhs);
    static inline FCmpInst *createOGE(Value *lhs, Value *rhs);
    static inline FCmpInst *createOLT(Value *lhs, Value *rhs);
    static inline FCmpInst *createOLE(Value *lhs, Value *rhs);
    static inline FCmpInst *createONE(Value *lhs, Value *rhs);
    static inline FCmpInst *createORD(Value *lhs, Value *rhs);
    static inline FCmpInst *createUEQ(Value *lhs, Value *rhs);
    static inline FCmpInst *createUGT(Value *lhs, Value *rhs);
    static inline FCmpInst *createUGE(Value *lhs, Value *rhs);
    static inline FCmpInst *createULT(Value *lhs, Value *rhs);
    static inline FCmpInst *createULE(Value *lhs, Value *rhs);
    static inline FCmpInst *createUNE(Value *lhs, Value *rhs);
    static inline FCmpInst *createUNO(Value *lhs, Value *rhs);
    static inline FCmpInst *createTRUE(Value *lhs, Value *rhs);

private:
    ComparePredicationType predicate_;
};

class PhiInst final
    : public Instruction
    , public User<-1>
    , public utils::BuildTrait<PhiInst> {
public:
    PhiInst(Type *type)
        : Instruction(InstructionID::Phi, this)
        , User<-1>(type, ValueTag::Instruction | 0) {}

    bool addIncomingValue(Value *value, BasicBlock *block);

    bool addIncomingValue(Instruction *value) {
        return addIncomingValue(value->unwrap(), value->parent());
    }

    bool removeIncomingValue(Instruction *value) {
        return removeValueFrom(value->parent());
    }

    bool removeValueFrom(BasicBlock *block);
};

class CallInst final
    : public Instruction
    , public User<-1>
    , public utils::BuildTrait<CallInst> {
public:
    CallInst(Function *callee)
        : Instruction(InstructionID::Call, this)
        , User<-1>(callee->proto()->returnType(), ValueTag::Instruction | 0) {
        resize(callee->proto()->totalParams() + 1);
        op()[0] = callee;
    }
};

inline AllocaInst *Instruction::createAlloca(Type *type) {
    return AllocaInst::create(type);
}

inline LoadInst *Instruction::createLoad(Value *address) {
    assert(address->type()->tryGetElementType() != nullptr);
    return LoadInst::create(address);
}

inline StoreInst *Instruction::createStore(Value *address, Value *value) {
    assert(address->type()->tryGetElementType() != nullptr);
    assert(address->type()->tryGetElementType()->equals(value->type()));
    return StoreInst::create(address, value);
}

inline RetInst *Instruction::createRet(Value *value) {
    return RetInst::create(value);
}

inline BrInst *Instruction::createBr(BasicBlock *block) {
    return BrInst::create(block);
}

inline BrInst *Instruction::createBr(
    Value *condition, BasicBlock *branchIf, BasicBlock *branchElse) {
    return BrInst::create(condition, branchIf, branchElse);
}

inline GetElementPtrInst *Instruction::createGetElementPtr(
    Value *address, Value *index) {
    assert(address->type()->tryGetElementType() != nullptr);
    assert(index->type()->equals(Type::getIntegerType()));
    return GetElementPtrInst::create(address, index);
}

inline AddInst *Instruction::createAdd(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return AddInst::create(lhs, rhs);
}

inline SubInst *Instruction::createSub(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return SubInst::create(lhs, rhs);
}

inline MulInst *Instruction::createMul(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return MulInst::create(lhs, rhs);
}

inline UDivInst *Instruction::createUDiv(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return UDivInst::create(lhs, rhs);
}

inline SDivInst *Instruction::createSDiv(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return SDivInst::create(lhs, rhs);
}

inline URemInst *Instruction::createURem(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return URemInst::create(lhs, rhs);
}

inline SRemInst *Instruction::createSRem(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return SRemInst::create(lhs, rhs);
}

inline FNegInst *Instruction::createFNeg(Value *value) {
    assert(value->type()->equals(Type::getFloatType()));
    return FNegInst::create(value);
}

inline FAddInst *Instruction::createFAdd(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FAddInst::create(lhs, rhs);
}

inline FSubInst *Instruction::createFSub(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FSubInst::create(lhs, rhs);
}

inline FMulInst *Instruction::createFMul(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FMulInst::create(lhs, rhs);
}

inline FDivInst *Instruction::createFDiv(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FDivInst::create(lhs, rhs);
}

inline FRemInst *Instruction::createFRem(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FRemInst::create(lhs, rhs);
}

inline ShlInst *Instruction::createShl(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return ShlInst::create(lhs, rhs);
}

inline LShrInst *Instruction::createLShr(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return LShrInst::create(lhs, rhs);
}

inline AShrInst *Instruction::createAShr(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return AShrInst::create(lhs, rhs);
}

inline AndInst *Instruction::createAnd(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return AndInst::create(lhs, rhs);
}

inline OrInst *Instruction::createOr(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return OrInst::create(lhs, rhs);
}

inline XorInst *Instruction::createXor(Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return XorInst::create(lhs, rhs);
}

inline FPToUIInst *Instruction::createFPToUI(Value *value) {
    assert(value->type()->equals(Type::getFloatType()));
    return FPToUIInst::create(value);
}

inline FPToSIInst *Instruction::createFPToSI(Value *value) {
    assert(value->type()->equals(Type::getFloatType()));
    return FPToSIInst::create(value);
}

inline UIToFPInst *Instruction::createUIToFP(Value *value) {
    assert(value->type()->equals(Type::getIntegerType()));
    return UIToFPInst::create(value);
}

inline SIToFPInst *Instruction::createSIToFP(Value *value) {
    assert(value->type()->equals(Type::getIntegerType()));
    return SIToFPInst::create(value);
}

inline ICmpInst *Instruction::createICmp(
    ComparePredicationType op, Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getIntegerType()));
    assert(rhs->type()->equals(Type::getIntegerType()));
    return ICmpInst::create(op, lhs, rhs);
}

inline FCmpInst *Instruction::createFCmp(
    ComparePredicationType op, Value *lhs, Value *rhs) {
    assert(lhs->type()->equals(Type::getFloatType()));
    assert(rhs->type()->equals(Type::getFloatType()));
    return FCmpInst::create(op, lhs, rhs);
}

inline PhiInst *Instruction::createPhi(Type *type) {
    return PhiInst::create(type);
}

inline CallInst *Instruction::createCall(Function *callee) {
    return CallInst::create(callee);
}

inline ICmpInst *ICmpInst::createEQ(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::EQ, lhs, rhs);
}

inline ICmpInst *ICmpInst::createNE(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::NE, lhs, rhs);
}

inline ICmpInst *ICmpInst::createUGT(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::UGT, lhs, rhs);
}

inline ICmpInst *ICmpInst::createUGE(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::UGE, lhs, rhs);
}

inline ICmpInst *ICmpInst::createULT(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::ULT, lhs, rhs);
}

inline ICmpInst *ICmpInst::createULE(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::ULE, lhs, rhs);
}

inline ICmpInst *ICmpInst::createSGT(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::SGT, lhs, rhs);
}

inline ICmpInst *ICmpInst::createSGE(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::SGE, lhs, rhs);
}

inline ICmpInst *ICmpInst::createSLT(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::SLT, lhs, rhs);
}

inline ICmpInst *ICmpInst::createSLE(Value *lhs, Value *rhs) {
    return ICmpInst::create(ComparePredicationType::SLE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createFALSE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::FALSE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createOEQ(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::OEQ, lhs, rhs);
}

inline FCmpInst *FCmpInst::createOGT(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::OGT, lhs, rhs);
}

inline FCmpInst *FCmpInst::createOGE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::OGE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createOLT(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::OLT, lhs, rhs);
}

inline FCmpInst *FCmpInst::createOLE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::OLE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createONE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::ONE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createORD(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::ORD, lhs, rhs);
}

inline FCmpInst *FCmpInst::createUEQ(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::UEQ, lhs, rhs);
}

inline FCmpInst *FCmpInst::createUGT(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::UGT, lhs, rhs);
}

inline FCmpInst *FCmpInst::createUGE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::UGE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createULT(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::ULT, lhs, rhs);
}

inline FCmpInst *FCmpInst::createULE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::ULE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createUNE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::UNE, lhs, rhs);
}

inline FCmpInst *FCmpInst::createUNO(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::UNO, lhs, rhs);
}

inline FCmpInst *FCmpInst::createTRUE(Value *lhs, Value *rhs) {
    return FCmpInst::create(ComparePredicationType::TRUE, lhs, rhs);
}

}; // namespace slime::ir
