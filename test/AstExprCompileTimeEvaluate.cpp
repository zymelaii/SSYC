#include <gtest/gtest.h>
#include <slime/visitor/ASTToIRVisitor.h>

using slime::visitor::ASTToIRVisitor;
using namespace slime::ast;
namespace ir = slime::ir;

TEST(AstExprCompileTimeEvaluate, test_name) {
    ASSERT_EQ(
        3,
        static_cast<ir::ConstantInt*>(
            ASTToIRVisitor::evaluateCompileTimeAstExpr(new BinaryExpr(
                BuiltinType::getIntType(),
                BinaryOperator::Add,
                ConstantExpr::createI32(1),
                ConstantExpr::createI32(2))))
            ->value);
}
