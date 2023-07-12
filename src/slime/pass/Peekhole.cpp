#include "Peekhole.h"

#include <slime/ir/instruction.h>
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
                        binUserPeekholeOptmize(user);
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
        case ir::InstructionID::Add:
        case ir::InstructionID::Sub:
        case ir::InstructionID::Mul:
        case ir::InstructionID::UDiv:
        case ir::InstructionID::SDiv:
        case ir::InstructionID::URem:
        case ir::InstructionID::SRem:
        case ir::InstructionID::Shl:
        case ir::InstructionID::LShr:
        case ir::InstructionID::AShr:
        case ir::InstructionID::And:
        case ir::InstructionID::Or:
        case ir::InstructionID::Xor:
        case ir::InstructionID::ICmp: {
            auto lhs = static_cast<ConstantInt *>(inst->lhs().value())->value;
            auto rhs = static_cast<ConstantInt *>(inst->rhs().value())->value;
            switch (self->id()) {
                case ir::InstructionID::Add: {
                    value = ConstantInt::create(lhs + rhs);
                } break;
                case ir::InstructionID::Sub: {
                    value = ConstantInt::create(lhs - rhs);
                } break;
                case ir::InstructionID::Mul: {
                    value = ConstantInt::create(lhs * rhs);
                } break;
                case ir::InstructionID::UDiv: {
                    value = ConstantInt::create(
                        static_cast<uint32_t>(lhs)
                        / static_cast<uint32_t>(rhs));
                } break;
                case ir::InstructionID::SDiv: {
                    value = ConstantInt::create(lhs / rhs);
                } break;
                case ir::InstructionID::URem: {
                    value = ConstantInt::create(
                        static_cast<uint32_t>(lhs)
                        % static_cast<uint32_t>(rhs));
                } break;
                case ir::InstructionID::SRem: {
                    value = ConstantInt::create(lhs % rhs);
                } break;
                case ir::InstructionID::Shl: {
                    value = ConstantInt::create(lhs << rhs);
                } break;
                case ir::InstructionID::LShr: {
                    value =
                        ConstantInt::create(static_cast<uint32_t>(lhs) >> rhs);
                } break;
                case ir::InstructionID::AShr: {
                    value = ConstantInt::create(lhs >> rhs);
                } break;
                case ir::InstructionID::And: {
                    value = ConstantInt::create(lhs & rhs);
                } break;
                case ir::InstructionID::Or: {
                    value = ConstantInt::create(lhs | rhs);
                } break;
                case ir::InstructionID::Xor: {
                    value = ConstantInt::create(lhs ^ rhs);
                } break;
                case ir::InstructionID::ICmp: {
                    switch (self->asICmp()->predicate()) {
                        case ir::ComparePredicationType::EQ: {
                            value = ConstantData::getBoolean(lhs == rhs);
                        } break;
                        case ir::ComparePredicationType::NE: {
                            value = ConstantData::getBoolean(lhs != rhs);
                        } break;
                        case ir::ComparePredicationType::UGT: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                > static_cast<uint32_t>(rhs));
                        } break;
                        case ir::ComparePredicationType::UGE: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                >= static_cast<uint32_t>(rhs));
                        } break;
                        case ir::ComparePredicationType::ULT: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                <= static_cast<uint32_t>(rhs));
                        } break;
                        case ir::ComparePredicationType::ULE: {
                            value = ConstantData::getBoolean(
                                static_cast<uint32_t>(lhs)
                                <= static_cast<uint32_t>(rhs));
                        } break;
                        case ir::ComparePredicationType::SGT: {
                            value = ConstantData::getBoolean(lhs > rhs);
                        } break;
                        case ir::ComparePredicationType::SGE: {
                            value = ConstantData::getBoolean(lhs >= rhs);
                        } break;
                        case ir::ComparePredicationType::SLT: {
                            value = ConstantData::getBoolean(lhs < rhs);
                        } break;
                        case ir::ComparePredicationType::SLE: {
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
        case ir::InstructionID::FAdd:
        case ir::InstructionID::FSub:
        case ir::InstructionID::FMul:
        case ir::InstructionID::FDiv:
        case ir::InstructionID::FRem:
        case ir::InstructionID::FCmp: {
            auto lhs = static_cast<ConstantFloat *>(inst->lhs().value())->value;
            auto rhs = static_cast<ConstantFloat *>(inst->rhs().value())->value;
            switch (self->id()) {
                case ir::InstructionID::FAdd:
                case ir::InstructionID::FSub:
                case ir::InstructionID::FMul:
                case ir::InstructionID::FDiv:
                case ir::InstructionID::FRem:
                case ir::InstructionID::FCmp: {
                    switch (self->asFCmp()->predicate()) {
                        case ir::ComparePredicationType::FALSE: {
                            value = ConstantData::getBoolean(false);
                        } break;
                        case ir::ComparePredicationType::OEQ: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs == rhs);
                        } break;
                        case ir::ComparePredicationType::OGT: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs > rhs);
                        } break;
                        case ir::ComparePredicationType::OGE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs >= rhs);
                        } break;
                        case ir::ComparePredicationType::OLT: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs < rhs);
                        } break;
                        case ir::ComparePredicationType::OLE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs <= rhs);
                        } break;
                        case ir::ComparePredicationType::ONE: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs) && lhs != rhs);
                        } break;
                        case ir::ComparePredicationType::ORD: {
                            value = ConstantData::getBoolean(
                                !isnan(lhs) && !isnan(rhs));
                        } break;
                        case ir::ComparePredicationType::UEQ: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs) || lhs == rhs);
                        } break;
                        case ir::ComparePredicationType::UNE: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs) || lhs != rhs);
                        } break;
                        case ir::ComparePredicationType::UNO: {
                            value = ConstantData::getBoolean(
                                isnan(lhs) || isnan(rhs));
                        } break;
                        case ir::ComparePredicationType::TRUE: {
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
        auto it = inst->uses().begin();
        while (it != inst->uses().end()) { (*it++)->reset(value); }
        auto ok = self->removeFromBlock(true);
        assert(ok);
    }
}

void PeekholePass::binUserPeekholeOptmize(User<2> *inst) {
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
