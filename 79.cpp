#include "80.h"

#include "49.h"
#include <math.h>

namespace slime::pass {

using namespace ir;

void PeekholePass::runOnFunction(Function *target) {
    for (auto block : target->basicBlocks()) {
        auto it = block->instructions().begin();
        while (it != block->instructions().end()) {
            auto inst = *it++;
            if (auto user = inst->tryIntoUser<2>()) {
                if (user->lhs().value() != nullptr
                    && user->rhs().value() != nullptr) {
                    bool lhsImm = user->lhs()->isImmediate();
                    bool rhsImm = user->rhs()->isImmediate();
                    if (lhsImm && rhsImm) {
                        foldConstant(inst);
                    } else if (rhsImm) {
                        binUserPeekholeOptimize(user);
                    }
                }
            }
            if (auto user = inst->tryIntoUser<1>()) {
                if (user->operand().value() != nullptr) {
                    if (user->operand()->isImmediate()) { foldConstant(inst); }
                }
            }
        }
    }
}

void PeekholePass::foldConstant(Instruction *inst) {
    if (auto user = inst->tryIntoUser<1>()) {
        foldUnaryConstant(user);
    } else if (auto user = inst->tryIntoUser<2>()) {
        foldBinaryConstant(user);
    }
}

void PeekholePass::foldUnaryConstant(User<1> *inst) {
    auto   self  = inst->asInstruction();
    Value *value = nullptr;
    switch (self->id()) {
        case InstructionID::FNeg: {
            value = ConstantData::createF32(
                -static_cast<ConstantFloat *>(inst->operand().value())->value);
        } break;
        case InstructionID::FPToUI:
        case InstructionID::FPToSI: {
            value = ConstantData::createI32(static_cast<int32_t>(
                static_cast<ConstantFloat *>(inst->operand().value())->value));
        } break;
        case InstructionID::UIToFP:
        case InstructionID::SIToFP: {
            value = ConstantData::createF32(static_cast<float>(
                static_cast<ConstantInt *>(inst->operand().value())->value));
        } break;
        case InstructionID::ZExt: {
            value = ConstantData::createI32(
                static_cast<ConstantInt *>(inst->operand().value())->value);
        } break;
        default: {
        } break;
    }
    if (value != nullptr) {
        auto it = inst->uses().begin();
        while (it != inst->uses().end()) { (*it++)->reset(value); }
        auto ok = self->removeFromBlock(true);
        assert(ok);
    }
}

void PeekholePass::foldBinaryConstant(User<2> *inst) {
    auto   self  = inst->asInstruction();
    Value *value = nullptr;
    switch (self->id()) {
        case InstructionID::Add:
        case InstructionID::Sub:
        case InstructionID::Mul:
        case InstructionID::UDiv:
        case InstructionID::SDiv:
        case InstructionID::URem:
        case InstructionID::SRem:
        case InstructionID::Shl:
        case InstructionID::LShr:
        case InstructionID::AShr:
        case InstructionID::And:
        case InstructionID::Or:
        case InstructionID::Xor:
        case InstructionID::ICmp: {
            auto lhs = static_cast<ConstantInt *>(inst->lhs().value())->value;
            auto rhs = static_cast<ConstantInt *>(inst->rhs().value())->value;
            switch (self->id()) {
                case InstructionID::Add: {
                    value = ConstantInt::create(lhs + rhs);
                } break;
                case InstructionID::Sub: {
                    value = ConstantInt::create(lhs - rhs);
                } break;
                case InstructionID::Mul: {
                    value = ConstantInt::create(lhs * rhs);
                } break;
                case InstructionID::UDiv: {
                    value = ConstantInt::create(
                        static_cast<uint32_t>(lhs)
                        / static_cast<uint32_t>(rhs));
                } break;
                case InstructionID::SDiv: {
                    value = ConstantInt::create(lhs / rhs);
                } break;
                case InstructionID::URem: {
                    value = ConstantInt::create(
                        static_cast<uint32_t>(lhs)
                        % static_cast<uint32_t>(rhs));
                } break;
                case InstructionID::SRem: {
                    value = ConstantInt::create(lhs % rhs);
                } break;
                case InstructionID::Shl: {
                    value = ConstantInt::create(lhs << rhs);
                } break;
                case InstructionID::LShr: {
                    value =
                        ConstantInt::create(static_cast<uint32_t>(lhs) >> rhs);
                } break;
                case InstructionID::AShr: {
                    value = ConstantInt::create(lhs >> rhs);
                } break;
                case InstructionID::And: {
                    value = ConstantInt::create(lhs & rhs);
                } break;
                case InstructionID::Or: {
                    value = ConstantInt::create(lhs | rhs);
                } break;
                case InstructionID::Xor: {
                    value = ConstantInt::create(lhs ^ rhs);
                } break;
                case InstructionID::ICmp: {
                    switch (self->asICmp()->predicate()) {
                        case ComparePredicationType::EQ: {
                            value = ConstantData::getBoolean(lhs == rhs);
                        } break;
                        case ComparePredicationType::NE: {
                            value = ConstantData::getBoolean(lhs != rhs);
                        } break;
                        case ComparePredicationType::UGT: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                > static_cast<uint32_t>(rhs));
                        } break;
                        case ComparePredicationType::UGE: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                >= static_cast<uint32_t>(rhs));
                        } break;
                        case ComparePredicationType::ULT: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                <= static_cast<uint32_t>(rhs));
                        } break;
                        case ComparePredicationType::ULE: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                <= static_cast<uint32_t>(rhs));
                        } break;
                        case ComparePredicationType::SGT: {
                            value = ConstantData::getBoolean(lhs > rhs);
                        } break;
                        case ComparePredicationType::SGE: {
                            value = ConstantData::getBoolean(lhs >= rhs);
                        } break;
                        case ComparePredicationType::SLT: {
                            value = ConstantData::getBoolean(lhs < rhs);
                        } break;
                        case ComparePredicationType::SLE: {
                            value = ConstantData::getBoolean(lhs <= rhs);
                        } break;
                        default: {
                        } break;
                    }
                } break;
                default: {
                } break;
            }
        } break;
        case InstructionID::FAdd:
        case InstructionID::FSub:
        case InstructionID::FMul:
        case InstructionID::FDiv:
        case InstructionID::FRem:
        case InstructionID::FCmp: {
            auto lhs = static_cast<ConstantFloat *>(inst->lhs().value())->value;
            auto rhs = static_cast<ConstantFloat *>(inst->rhs().value())->value;
            switch (self->id()) {
                case InstructionID::FAdd: {
                    value = ConstantFloat::create(lhs + rhs);
                } break;
                case InstructionID::FSub: {
                    value = ConstantFloat::create(lhs - rhs);
                } break;
                case InstructionID::FMul: {
                    value = ConstantFloat::create(lhs * rhs);
                } break;
                case InstructionID::FDiv: {
                    value = ConstantFloat::create(lhs / rhs);
                } break;
                case InstructionID::FRem: {
                    //! FIXME: fmod may have precision issues
                    value = ConstantFloat::create(fmod(lhs, rhs));
                } break;
                case InstructionID::FCmp: {
                    switch (self->asFCmp()->predicate()) {
                        case ComparePredicationType::FALSE: {
                            value = ConstantData::getBoolean(false);
                        } break;
                        case ComparePredicationType::OEQ: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs == rhs);
                        } break;
                        case ComparePredicationType::OGT: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs > rhs);
                        } break;
                        case ComparePredicationType::OGE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs >= rhs);
                        } break;
                        case ComparePredicationType::OLT: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs < rhs);
                        } break;
                        case ComparePredicationType::OLE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs <= rhs);
                        } break;
                        case ComparePredicationType::ONE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs != rhs);
                        } break;
                        case ComparePredicationType::ORD: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs));
                        } break;
                        case ComparePredicationType::UEQ: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs) || lhs == rhs);
                        } break;
                        case ComparePredicationType::UNE: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs) || lhs != rhs);
                        } break;
                        case ComparePredicationType::UNO: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs));
                        } break;
                        case ComparePredicationType::TRUE: {
                            value = ConstantData::getBoolean(true);
                        }
                        default: {
                        } break;
                    }
                }
                default: {
                } break;
            }
        } break;
        default: {
        } break;
    }
    if (value != nullptr) {
        std::vector<Use *> uses(inst->uses().begin(), inst->uses().end());
        for (auto use : uses) { use->reset(value); }
        auto ok = self->removeFromBlock(true);
        assert(ok);
    }
}

void PeekholePass::binUserPeekholeOptimize(User<2> *inst) {
    auto       self         = inst->asInstruction();
    const auto op           = self->id();
    bool       shouldRemove = false;
    Value     *value        = nullptr;
    switch (op) {
        case InstructionID::Mul: {
            auto imm = static_cast<ConstantInt *>(inst->rhs().value())->value;
            if (int32_t exp = floor(log2(imm)); (1ull << exp) == imm) {
                value = ShlInst::create(inst->lhs(), ConstantInt::create(exp));
                value->asInstruction()->insertBefore(self);
                shouldRemove = true;
            }
        } break;
        case InstructionID::UDiv:
        case InstructionID::SDiv: {
            auto imm = static_cast<ConstantInt *>(inst->rhs().value())->value;
            if (imm != 0) {
                if (int32_t exp = floor(log2(imm)); (1ull << exp) == imm) {
                    value =
                        AShrInst::create(inst->lhs(), ConstantInt::create(exp));
                    value->asInstruction()->insertBefore(self);
                    shouldRemove = true;
                    break;
                }

                //! WARNING: to perform the algorithm, result of mul-inst must
                //! be 64-bit
                break;
                //! barrett reduction
                //! floor(N / M) = floor(N * X / 2^(b + s))
                //! where: s = floor(log2(M - 1)), X = ceil(2^(b + s) / M),
                //! NOTE: b is word size (32 here)
                int32_t b = 32;
                int32_t s = floor(log2(imm - 1));
                int32_t X = ceil((1ull << (b + s)) / imm);
                auto    inst1 =
                    MulInst::create(inst->lhs(), ConstantInt::create(X));
                inst1->insertBefore(self);
                auto inst2 =
                    AShrInst::create(inst1, ConstantInt::create(b + s));
                inst2->insertAfter(inst1);
                value        = inst2->unwrap();
                shouldRemove = true;
            }
        } break;
        case InstructionID::URem:
        case InstructionID::SRem: {
            auto imm = static_cast<ConstantInt *>(inst->rhs().value())->value;
            bool shouldReset = false;
            if (imm == 0) { break; }
            if (imm == 1) {
                value        = ConstantInt::create(0);
                shouldRemove = true;
            } else if (int32_t exp = floor(log2(imm)); (1ull << exp) == imm) {
                value =
                    AndInst::create(inst->lhs(), ConstantInt::create(imm - 1));
                value->asInstruction()->insertBefore(self);
                shouldRemove = true;
            }
        } break;
        case InstructionID::FDiv: {
            auto imm = static_cast<ConstantFloat *>(inst->rhs().value())->value;
            if (imm != 0.f) {
                imm = 1. / imm;
                value =
                    FMulInst::create(inst->lhs(), ConstantFloat::create(imm))
                        ->unwrap();
                value->asInstruction()->insertBefore(self);
                shouldRemove = true;
            }
        } break;
        case InstructionID::FCmp: {
            if (self->asFCmp()->predicate() == ComparePredicationType::TRUE) {
                value        = ConstantData::getBoolean(true);
                shouldRemove = true;
            } else if (
                self->asFCmp()->predicate() == ComparePredicationType::FALSE) {
                value        = ConstantData::getBoolean(false);
                shouldRemove = true;
            }
        } break;
        default: {
        } break;
    }
    if (shouldRemove) {
        auto it = inst->uses().begin();
        while (it != inst->uses().end()) { (*it++)->reset(value); }
        auto ok = self->removeFromBlock(true);
        assert(ok);
    }
}

} // namespace slime::pass
