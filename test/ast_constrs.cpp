#include <ssyc/ast/ast.h>

using namespace ssyc::ast;

static_assert(ast_node<AbstractAstNode>, "derive from AbstractAstNode");
static_assert(ast_node<Type>, "derive from AbstractAstNode");
static_assert(ast_node<Decl>, "derive from AbstractAstNode");
static_assert(ast_node<Expr>, "derive from AbstractAstNode");
static_assert(ast_node<Stmt>, "derive from AbstractAstNode");
static_assert(ast_node<Program>, "derive from AbstractAstNode");
static_assert(ast_node<QualifiedType>, "derive from AbstractAstNode");
static_assert(ast_node<BuiltinType>, "derive from AbstractAstNode");
static_assert(ast_node<ArrayType>, "derive from AbstractAstNode");
static_assert(ast_node<ConstantArrayType>, "derive from AbstractAstNode");
static_assert(ast_node<VariableArrayType>, "derive from AbstractAstNode");
static_assert(ast_node<FunctionProtoType>, "derive from AbstractAstNode");
static_assert(ast_node<VarDecl>, "derive from AbstractAstNode");
static_assert(ast_node<ParamVarDecl>, "derive from AbstractAstNode");
static_assert(ast_node<FunctionDecl>, "derive from AbstractAstNode");
static_assert(ast_node<InitListExpr>, "derive from AbstractAstNode");
static_assert(ast_node<CallExpr>, "derive from AbstractAstNode");
static_assert(ast_node<ParenExpr>, "derive from AbstractAstNode");
static_assert(ast_node<UnaryOperatorExpr>, "derive from AbstractAstNode");
static_assert(ast_node<BinaryOperatorExpr>, "derive from AbstractAstNode");
static_assert(ast_node<DeclRef>, "derive from AbstractAstNode");
static_assert(ast_node<Literal>, "derive from AbstractAstNode");
static_assert(ast_node<IntegerLiteral>, "derive from AbstractAstNode");
static_assert(ast_node<FloatingLiteral>, "derive from AbstractAstNode");
static_assert(ast_node<StringLiteral>, "derive from AbstractAstNode");
static_assert(ast_node<NullStmt>, "derive from AbstractAstNode");
static_assert(ast_node<CompoundStmt>, "derive from AbstractAstNode");
static_assert(ast_node<DeclStmt>, "derive from AbstractAstNode");
static_assert(ast_node<IfElseStmt>, "derive from AbstractAstNode");
static_assert(ast_node<WhileStmt>, "derive from AbstractAstNode");
static_assert(ast_node<DoWhileStmt>, "derive from AbstractAstNode");
static_assert(ast_node<ForStmt>, "derive from AbstractAstNode");
static_assert(ast_node<ContinueStmt>, "derive from AbstractAstNode");
static_assert(ast_node<BreakStmt>, "derive from AbstractAstNode");
static_assert(ast_node<ReturnStmt>, "derive from AbstractAstNode");
