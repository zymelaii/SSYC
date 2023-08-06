#pragma once

#include "88.h"
#include "89.h"

namespace slime::ast {

struct Type;
struct NoneType;
struct UnresolvedType;
struct BuiltinType;
struct ArrayType;
struct IncompleteArrayType;
struct FunctionProtoType;

struct Decl;
struct NamedDecl;
struct VarLikeDecl;
struct VarDecl;
struct ParamVarDecl;
struct FunctionDecl;

struct Stmt;
struct NullStmt;
struct DeclStmt;
struct ExprStmt;
struct CompoundStmt;
struct IfStmt;
struct DoStmt;
struct WhileStmt;
struct ForStmt;
struct BreakStmt;
struct ContinueStmt;
struct ReturnStmt;

struct Expr;
struct DeclRefExpr;
struct ConstantExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CommaExpr;
struct ParenExpr;
struct StmtExpr;
struct CallExpr;
struct SubscriptExpr;
struct InitListExpr;
struct NoInitExpr;

using TopLevelDeclList = utils::ListTrait<Decl*>;

struct TranslationUnit
    : public TopLevelDeclList
    , public utils::BuildTrait<TranslationUnit> {};

} // namespace slime::ast
