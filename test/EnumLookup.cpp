#include <gtest/gtest.h>
#include <slime/ir/instruction.h>
#include <slime/parse/priority.h>
#include <slime/visitor/IRDumpVisitor.h>

using Inst     = slime::ir::InstructionID;
using Dump     = slime::visitor::IRDumpVisitor;
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
    ASSERT_TRUE(Dump::lookupInstName(nullptr).empty());

    ASSERT_TRUE(Dump::lookupInstName(Inst::Alloca) == "alloca");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Load) == "load");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Store) == "store");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Ret) == "ret");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Br) == "br");
    ASSERT_TRUE(Dump::lookupInstName(Inst::GetElementPtr) == "getelementptr");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Add) == "add");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Sub) == "sub");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Mul) == "mul");
    ASSERT_TRUE(Dump::lookupInstName(Inst::UDiv) == "udiv");
    ASSERT_TRUE(Dump::lookupInstName(Inst::SDiv) == "sdiv");
    ASSERT_TRUE(Dump::lookupInstName(Inst::URem) == "urem");
    ASSERT_TRUE(Dump::lookupInstName(Inst::SRem) == "srem");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FNeg) == "fneg");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FAdd) == "fadd");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FSub) == "fsub");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FMul) == "fmul");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FDiv) == "fdiv");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FRem) == "frem");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Shl) == "shl");
    ASSERT_TRUE(Dump::lookupInstName(Inst::LShr) == "lshr");
    ASSERT_TRUE(Dump::lookupInstName(Inst::AShr) == "ashr");
    ASSERT_TRUE(Dump::lookupInstName(Inst::And) == "and");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Or) == "or");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Xor) == "xor");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FPToUI) == "fptoui");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FPToSI) == "fptosi");
    ASSERT_TRUE(Dump::lookupInstName(Inst::UIToFP) == "uitofp");
    ASSERT_TRUE(Dump::lookupInstName(Inst::SIToFP) == "sitofp");
    ASSERT_TRUE(Dump::lookupInstName(Inst::ZExt) == "zext");
    ASSERT_TRUE(Dump::lookupInstName(Inst::ICmp) == "icmp");
    ASSERT_TRUE(Dump::lookupInstName(Inst::FCmp) == "fcmp");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Phi) == "phi");
    ASSERT_TRUE(Dump::lookupInstName(Inst::Call) == "call");
}
