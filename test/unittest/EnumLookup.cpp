#include <gtest/gtest.h>
#include <slime/ir/instruction.h>
#include <slime/parse/priority.h>

namespace ir   = slime::ir;
using Inst     = slime::ir::InstructionID;
using UnaryOp  = slime::ast::UnaryOperator;
using BinaryOp = slime::ast::BinaryOperator;

TEST(EnumLookup, OperatorPriority) {
    slime::OperatorPriority result{0, false};

    result = slime::lookupOperatorPriority(UnaryOp::Pos);
    ASSERT_EQ(result.priority, 1);
    ASSERT_EQ(result.assoc, false);
    result = slime::lookupOperatorPriority(UnaryOp::Neg);
    ASSERT_EQ(result.priority, 1);
    ASSERT_EQ(result.assoc, false);
    result = slime::lookupOperatorPriority(UnaryOp::Not);
    ASSERT_EQ(result.priority, 1);
    ASSERT_EQ(result.assoc, false);
    result = slime::lookupOperatorPriority(UnaryOp::Inv);
    ASSERT_EQ(result.priority, 1);
    ASSERT_EQ(result.assoc, false);
    result = slime::lookupOperatorPriority(BinaryOp::Assign);
    ASSERT_EQ(result.priority, 12);
    ASSERT_EQ(result.assoc, false);
    result = slime::lookupOperatorPriority(BinaryOp::Add);
    ASSERT_EQ(result.priority, 3);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Sub);
    ASSERT_EQ(result.priority, 3);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Mul);
    ASSERT_EQ(result.priority, 2);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Div);
    ASSERT_EQ(result.priority, 2);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Mod);
    ASSERT_EQ(result.priority, 2);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::And);
    ASSERT_EQ(result.priority, 7);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Or);
    ASSERT_EQ(result.priority, 9);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Xor);
    ASSERT_EQ(result.priority, 8);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::LAnd);
    ASSERT_EQ(result.priority, 10);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::LOr);
    ASSERT_EQ(result.priority, 11);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::LT);
    ASSERT_EQ(result.priority, 5);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::LE);
    ASSERT_EQ(result.priority, 5);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::GT);
    ASSERT_EQ(result.priority, 5);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::GE);
    ASSERT_EQ(result.priority, 5);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::EQ);
    ASSERT_EQ(result.priority, 6);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::NE);
    ASSERT_EQ(result.priority, 6);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Shl);
    ASSERT_EQ(result.priority, 4);
    ASSERT_EQ(result.assoc, true);
    result = slime::lookupOperatorPriority(BinaryOp::Shr);
    ASSERT_EQ(result.priority, 4);
    ASSERT_EQ(result.assoc, true);
}

TEST(EnumLookup, InstructionName) {
    ASSERT_TRUE(ir::getInstructionName(nullptr).empty());

    ASSERT_TRUE(ir::getInstructionName(Inst::Alloca) == "alloca");
    ASSERT_TRUE(ir::getInstructionName(Inst::Load) == "load");
    ASSERT_TRUE(ir::getInstructionName(Inst::Store) == "store");
    ASSERT_TRUE(ir::getInstructionName(Inst::Ret) == "ret");
    ASSERT_TRUE(ir::getInstructionName(Inst::Br) == "br");
    ASSERT_TRUE(ir::getInstructionName(Inst::GetElementPtr) == "getelementptr");
    ASSERT_TRUE(ir::getInstructionName(Inst::Add) == "add");
    ASSERT_TRUE(ir::getInstructionName(Inst::Sub) == "sub");
    ASSERT_TRUE(ir::getInstructionName(Inst::Mul) == "mul");
    ASSERT_TRUE(ir::getInstructionName(Inst::UDiv) == "udiv");
    ASSERT_TRUE(ir::getInstructionName(Inst::SDiv) == "sdiv");
    ASSERT_TRUE(ir::getInstructionName(Inst::URem) == "urem");
    ASSERT_TRUE(ir::getInstructionName(Inst::SRem) == "srem");
    ASSERT_TRUE(ir::getInstructionName(Inst::FNeg) == "fneg");
    ASSERT_TRUE(ir::getInstructionName(Inst::FAdd) == "fadd");
    ASSERT_TRUE(ir::getInstructionName(Inst::FSub) == "fsub");
    ASSERT_TRUE(ir::getInstructionName(Inst::FMul) == "fmul");
    ASSERT_TRUE(ir::getInstructionName(Inst::FDiv) == "fdiv");
    ASSERT_TRUE(ir::getInstructionName(Inst::FRem) == "frem");
    ASSERT_TRUE(ir::getInstructionName(Inst::Shl) == "shl");
    ASSERT_TRUE(ir::getInstructionName(Inst::LShr) == "lshr");
    ASSERT_TRUE(ir::getInstructionName(Inst::AShr) == "ashr");
    ASSERT_TRUE(ir::getInstructionName(Inst::And) == "and");
    ASSERT_TRUE(ir::getInstructionName(Inst::Or) == "or");
    ASSERT_TRUE(ir::getInstructionName(Inst::Xor) == "xor");
    ASSERT_TRUE(ir::getInstructionName(Inst::FPToUI) == "fptoui");
    ASSERT_TRUE(ir::getInstructionName(Inst::FPToSI) == "fptosi");
    ASSERT_TRUE(ir::getInstructionName(Inst::UIToFP) == "uitofp");
    ASSERT_TRUE(ir::getInstructionName(Inst::SIToFP) == "sitofp");
    ASSERT_TRUE(ir::getInstructionName(Inst::ZExt) == "zext");
    ASSERT_TRUE(ir::getInstructionName(Inst::ICmp) == "icmp");
    ASSERT_TRUE(ir::getInstructionName(Inst::FCmp) == "fcmp");
    ASSERT_TRUE(ir::getInstructionName(Inst::Phi) == "phi");
    ASSERT_TRUE(ir::getInstructionName(Inst::Call) == "call");
}
