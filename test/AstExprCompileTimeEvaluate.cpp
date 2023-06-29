#include <gtest/gtest.h>
#include <slime/visitor/ASTToIRVisitor.h>

using slime::visitor::ASTToIRVisitor;
using namespace slime::ast;
namespace ir = slime::ir;

TEST(AstExprCompileTimeEvaluate, SimpleBinaryOp) {
    ASSERT_EQ(
        3,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Add,
                ConstantExpr::createI32(1),
                ConstantExpr::createI32(2))))
            ->value);

    ASSERT_EQ(
        -1,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Sub,
                ConstantExpr::createI32(1),
                ConstantExpr::createI32(2))))
            ->value);

    ASSERT_EQ(
        12,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Mul,
                ConstantExpr::createI32(4),
                ConstantExpr::createI32(3))))
            ->value);

    ASSERT_EQ(
        1,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Div,
                ConstantExpr::createI32(4),
                ConstantExpr::createI32(3))))
            ->value);

    ASSERT_EQ(
        2,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Mod,
                ConstantExpr::createI32(5),
                ConstantExpr::createI32(3))))
            ->value);
}
