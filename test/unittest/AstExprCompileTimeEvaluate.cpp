#include <gtest/gtest.h>
#include <slime/visitor/ASTExprSimplifier.h>

using namespace slime::ast;

TEST(AstExprCompileTimeEvaluate, SimpleBinaryOp) {
    ASSERT_EQ(
        *BinaryExpr::createAdd(
             ConstantExpr::createI32(1), ConstantExpr::createI32(3))
             ->tryEvaluate(),
        4);

    ASSERT_EQ(
        *BinaryExpr::createSub(
             ConstantExpr::createI32(1), ConstantExpr::createI32(3))
             ->tryEvaluate(),
        -2);

    ASSERT_EQ(
        *BinaryExpr::createMul(
             ConstantExpr::createI32(4), ConstantExpr::createI32(3))
             ->tryEvaluate(),
        12);

    ASSERT_EQ(
        *BinaryExpr::createDiv(
             ConstantExpr::createI32(4), ConstantExpr::createI32(3))
             ->tryEvaluate(),
        1);

    ASSERT_EQ(
        *BinaryExpr::createMod(
             ConstantExpr::createI32(5), ConstantExpr::createI32(3))
             ->tryEvaluate(),
        2);
}
