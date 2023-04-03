#include <ssyc/ast/ast.h>
#include <gtest/gtest.h>

using namespace ssyc::ast;

void testAstTypeConstraint(ast_node auto foobar) {}

TEST(AstConstrs, all) {
    testAstTypeConstraint(AbstractAstNode{});

    testAstTypeConstraint(Type{});
    testAstTypeConstraint(Decl{});
    testAstTypeConstraint(Expr{});
    testAstTypeConstraint(Stmt{});

    testAstTypeConstraint(Program{});

    testAstTypeConstraint(QualifiedType{});
    testAstTypeConstraint(BuiltinType{});
    testAstTypeConstraint(ArrayType{});
    testAstTypeConstraint(ConstantArrayType{});
    testAstTypeConstraint(VariableArrayType{});
    testAstTypeConstraint(FunctionProtoType{});

    testAstTypeConstraint(VarDecl{});
    testAstTypeConstraint(ParamVarDecl{});
    testAstTypeConstraint(FunctionDecl{});

    testAstTypeConstraint(InitListExpr{});
    testAstTypeConstraint(CallExpr{});
    testAstTypeConstraint(ParenExpr{});
    testAstTypeConstraint(UnaryOperatorExpr{});
    testAstTypeConstraint(BinaryOperatorExpr{});
    testAstTypeConstraint(DeclRef{});
    testAstTypeConstraint(Literal{});
    testAstTypeConstraint(IntegerLiteral{});
    testAstTypeConstraint(FloatingLiteral{});
    testAstTypeConstraint(StringLiteral{});

    testAstTypeConstraint(NullStmt{});
    testAstTypeConstraint(CompoundStmt{});
    testAstTypeConstraint(DeclStmt{});
    testAstTypeConstraint(IfElseStmt{});
    testAstTypeConstraint(WhileStmt{});
    testAstTypeConstraint(DoWhileStmt{});
    testAstTypeConstraint(ForStmt{});
    testAstTypeConstraint(ContinueStmt{});
    testAstTypeConstraint(BreakStmt{});
    testAstTypeConstraint(ReturnStmt{});
}
