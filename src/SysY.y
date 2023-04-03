%{
#include "pch.h"
#include "utils.h"
#include "ast.h"
#include "parser_context.h"

#include <string_view>
#include <utility>

#define YYDEBUG 1

using ssyc::ParserContext;
using ContextFlag = ssyc::ParserContext::ContextFlag;
using namespace ssyc::ast;

void yyerror(ssyc::ParserContext &context, char* message);
extern "C" int yywrap();
int yylex();
%}

//! 指定解析器参数
%parse-param {ssyc::ParserContext &context}

%union {
	const char *token;
	int type;
	ssyc::ast::Program *program;
	ssyc::ast::DeclStatement *decl;
	ssyc::ast::FuncDef *func;
	ssyc::ast::Block *block;
	ssyc::ast::Statement *statement;
	ssyc::ast::Expr *expr;
	std::vector<ssyc::ast::Expr*> *elist;
}

%type <type> BType UnaryOp
%type <program> CompUnit
%type <decl> Decl ConstDecl VarDecl
%type <decl> ConstDefList ConstDef VarDefList VarDef
%type <func> FuncDef FuncFParams FuncFParam
%type <expr> FuncRParams
%type <block> Block BlockItemSequence
%type <statement> BlockItem
%type <statement> Stmt StmtBeforeElseStmt
%type <expr> Exp Cond LVal PrimaryExp Number ConstExp
%type <expr> UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <expr> ConstInitVal ConstInitValList InitVal InitValList
%type <elist> SubscriptChain SubscriptChainConst SubscriptChainVariant

//! #-- TOKEN 声明 BEGIN ---
//! 标识符
%token <token> IDENT

//! 常量
%token <token> CONST_INT CONST_FLOAT

//! 基本类型
%token <type> T_VOID T_INT T_FLOAT

//! 类型限定符
%token KW_CONST

//! 控制关键字
%token KW_IF KW_ELSE
%token KW_WHILE KW_BREAK KW_CONTINUE
%token KW_RETURN

//! 操作符
%token <type> OP_ASS
%token <type> OP_POS OP_NEG
%token <type> OP_ADD OP_SUB
%token <type> OP_MUL OP_DIV OP_MOD
%token <type> OP_LT OP_GT OP_LE OP_GE OP_EQ OP_NE
%token <type> OP_LNOT OP_LAND OP_LOR

//! 杂项
%token LPAREN RPAREN
%token LBRACKET RBRACKET
%token LBRACE RBRACE
%token COMMA SEMICOLON

//! #-- TOKEN 声明 END ---

%left OP_ADD OP_SUB OP_MUL OP_DIV
%left OP_LT OP_LE OP_GT OP_GE OP_EQ OP_NE
%left OP_LAND OP_LOR
%left COMMA
%right OP_LNOT
%right OP_ASS

%%

//! #-- SysY 2022 文法 BEGIN ---

%start CompUnit;

//! 编译单元
CompUnit
: Decl                                                                              { SSYC_PRINT_REDUCE(CompUnit, "Decl");
	const auto ok = context.testAndSetFlag(ContextFlag::SyntaxAnalysisDone);
	LOG_IF(ERROR, !ok) << "Parser: caught one more program unit";

	context.program->append($1);
}
| FuncDef                                                                           { SSYC_PRINT_REDUCE(CompUnit, "FuncDef");
	const auto ok = context.testAndSetFlag(ContextFlag::SyntaxAnalysisDone);
	LOG_IF(ERROR, !ok) << "Parser: caught one more program unit";

	context.program->append($1);
}
| CompUnit Decl                                                                     { SSYC_PRINT_REDUCE(CompUnit, "CompUnit Decl");
	context.program->append($2);
}
| CompUnit FuncDef                                                                  { SSYC_PRINT_REDUCE(CompUnit, "CompUnit FuncDef");
	context.program->append($2);
}
;

//! 声明
Decl
: ConstDecl                                                                         { SSYC_PRINT_REDUCE(Decl, "ConstDecl");
	$$ = $1;
}
| VarDecl                                                                           { SSYC_PRINT_REDUCE(Decl, "VarDecl");
	$$ = $1;
}
;

//! 常量声明
//! NOTE: ConstDefList := ConstDef { COMMA ConstDef }
ConstDecl
: KW_CONST BType ConstDefList SEMICOLON                                             { SSYC_PRINT_REDUCE(ConstDecl, "KW_CONST BType SEMICOLON");
	$$ = $3;

	TypeDecl::Type basicType{};
	switch ($2) {
		case T_INT: {
			basicType = TypeDecl::Type::Integer;
		} break;
		case T_FLOAT: {
			basicType = TypeDecl::Type::Float;
		} break;
		default: {
			LOG(ERROR) << "Parser: illegal type (" << static_cast<int>($2) << ") of declaration";
			std::unreachable();
		} break;
	}

	for (auto &decl : $$->declList) {
		auto &[type, expr] = decl;
		type->constant = true;
		type->type = basicType;
	}
}
;

//! 基本类型
//! #-- BType BEGIN ---
/*!
 * BType 和 FuncType 同时组成了 CompUnit 的前缀，产生 reduce/reduce 冲突
 * TODO: 解决 BType 和 FuncType 的 reduce/reduce 冲突
 * SOLUTION: 合并 BType 与 FuncType 将检查延迟到语义分析阶段
 *
 * BType
 * : T_INT
 * | T_FLOAT
 * ;
 */

BType
: T_VOID                                                                            { SSYC_PRINT_REDUCE(BType, "T_VOID");
	$$ = T_VOID;
}
| T_INT                                                                             { SSYC_PRINT_REDUCE(BType, "T_INT");
	$$ = T_INT;
}
| T_FLOAT                                                                           { SSYC_PRINT_REDUCE(BType, "T_FLOAT");
	$$ = T_FLOAT;
}
;

//! #-- BType END ---

//! 常数定义
//! NOTE: SubscriptChainConst := { LBRACKET ConstExp RBRACKET }
ConstDef
: IDENT OP_ASS ConstInitVal                                                         { SSYC_PRINT_REDUCE(ConstDef, "IDENT OP_ASS ConstInitVal");
	//! TODO: 检查各种合法性
	auto type = context.require<TypeDecl>();
	type->ident = $1;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, $3);
}
| IDENT SubscriptChainConst OP_ASS ConstInitVal                                     { SSYC_PRINT_REDUCE(ConstDef, "IDENT SubscriptChainConst OP_ASS ConstInitVal");
	auto type = context.require<TypeDecl>();
	type->ident = $1;
	type->optSubscriptList = std::move(*$2);
	delete $2;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, $4);
}
;

//! 常量初值
//! NOTE: ConstInitValList := ConstInitVal { COMMA ConstInitVal }
ConstInitVal
: ConstExp                                                                          { SSYC_PRINT_REDUCE(ConstInitVal, "ConstExp");
	$$ = $1;
}
| LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(ConstInitVal, "LBRACE RBRACE");
	auto list = context.require<InitializeList>();
	auto expr = context.require<OrphanExpr>();
	expr->ref = list;

	$$ = expr;
}
| LBRACE ConstInitValList RBRACE                                                    { SSYC_PRINT_REDUCE(ConstInitVal, "LBRACE ConstInitValList RBRACE");
	$$ = $2;
}
;

//! 变量声明
//! NOTE: VarDefList := VarDef { COMMA VarDef }
VarDecl
: BType VarDefList SEMICOLON                                                        { SSYC_PRINT_REDUCE(VarDecl, "BType VarDefList SEMICOLON");
	$$ = $2;

	TypeDecl::Type basicType{};
	switch ($1) {
		case T_INT: {
			basicType = TypeDecl::Type::Integer;
		} break;
		case T_FLOAT: {
			basicType = TypeDecl::Type::Float;
		} break;
		default: {
			LOG(ERROR) << "Parser: illegal type (" << static_cast<int>($1) << ") of declaration";
			std::unreachable();
		} break;
	}

	for (auto &decl : $$->declList) {
		auto &[type, expr] = decl;
		type->type = basicType;
	}
}
;

//! 变量定义
VarDef
: IDENT                                                                             { SSYC_PRINT_REDUCE(VarDef, "IDENT");
	//! TODO: 检查各种合法性
	auto type = context.require<TypeDecl>();
	type->ident = $1;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, nullptr);
}
| IDENT OP_ASS InitVal                                                              { SSYC_PRINT_REDUCE(VarDef, "IDENT OP_ASS InitVal");
	//! TODO: 检查各种合法性
	auto type = context.require<TypeDecl>();
	type->ident = $1;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, $3);
}
| IDENT SubscriptChainConst                                                         { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst");
	auto type = context.require<TypeDecl>();
	type->ident = $1;
	type->optSubscriptList = std::move(*$2);
	delete $2;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, nullptr);
}
| IDENT SubscriptChainConst OP_ASS InitVal                                          { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst OP_ASS InitVal");
	auto type = context.require<TypeDecl>();
	type->ident = $1;
	type->optSubscriptList = std::move(*$2);
	delete $2;

	$$ = context.require<DeclStatement>();
	$$->declList.emplace_back(type, $4);
}
;

//! 变量初值
//! NOTE: InitValList := InitVal { COMMA InitVal }
InitVal
: Exp                                                                               { SSYC_PRINT_REDUCE(InitVal, "Exp");
	$$ = $1;
}
| LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(InitVal, "LBRACE RBRACE");
	auto list = context.require<InitializeList>();
	auto expr = context.require<OrphanExpr>();
	expr->ref = list;

	$$ = expr;
}
| LBRACE InitValList RBRACE                                                         { SSYC_PRINT_REDUCE(InitVal, "LBRACE InitValList RBRACE");
	$$ = $2;
}
;

//! 函数定义
//! NOTE: FuncDef := FuncType Ident LPAREN [ FuncFParams ] RPAREN Block
//! NOTE: replace `FuncType` with `BType`
FuncDef
: BType IDENT LPAREN RPAREN Block                                                   { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN RPAREN Block");
	$$ = context.require<FuncDef>();
	$$->ident = $2;
	$$->body = $5;

	switch ($1) {
		case T_VOID: {
			$$->retType = FuncDef::Type::Void;
		} break;
		case T_INT: {
			$$->retType = FuncDef::Type::Integer;
		} break;
		case T_FLOAT: {
			$$->retType = FuncDef::Type::Float;
		} break;
	}
}
| BType IDENT LPAREN FuncFParams RPAREN Block                                       { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN FuncFParams RPAREN Block");
	$$ = $4;
	$$->ident = $2;
	$$->body = $6;

	switch ($1) {
		case T_VOID: {
			$$->retType = FuncDef::Type::Void;
		} break;
		case T_INT: {
			$$->retType = FuncDef::Type::Integer;
		} break;
		case T_FLOAT: {
			$$->retType = FuncDef::Type::Float;
		} break;
	}
}
;

//! 函数类型
//! #-- FuncType BEGIN ---
/*!
 * BType 和 FuncType 同时组成了 CompUnit 的前缀，产生 reduce/reduce 冲突
 * TODO: 解决 BType 和 FuncType 的 reduce/reduce 冲突
 * SOLUTION: 合并 BType 与 FuncType 将检查延迟到语义分析阶段
 *
 * FuncType
 * : T_VOID
 * | T_INT
 * | T_FLOAT
 * ;
 */

//! #-- FuncType END ---

//! 函数形参表
FuncFParams
: FuncFParam                                                                        { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParam");
	$$ = $1;
}
| FuncFParams COMMA FuncFParam                                                      { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParams COMMA FuncFParam");
	$$ = $1;
	$$->params.push_back($3->params[0]);
	//! TODO: 内存清理
}
;

//! 函数形参
//! NOTE: SubscriptChainVariant := LBRACKET RBRACKET [ { LBRACKET Exp RBRACKET } ]
FuncFParam
: BType IDENT                                                                       { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT");
	TypeDecl::Type basicType{};
	switch ($1) {
		case T_INT: {
			basicType = TypeDecl::Type::Integer;
		} break;
		case T_FLOAT: {
			basicType = TypeDecl::Type::Float;
		} break;
		default: {
			LOG(ERROR) << "Parser: illegal type (" << static_cast<int>($1) << ") of declaration";
			std::unreachable();
		} break;
	}

	auto type = context.require<TypeDecl>();
	type->ident = $2;
	type->type = basicType;

	$$ = context.require<FuncDef>();
	$$->params.push_back(type);
}
| BType IDENT SubscriptChainVariant                                                 { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT SubscriptChainVariant");
TypeDecl::Type basicType{};
	switch ($1) {
		case T_INT: {
			basicType = TypeDecl::Type::Integer;
		} break;
		case T_FLOAT: {
			basicType = TypeDecl::Type::Float;
		} break;
		default: {
			LOG(ERROR) << "Parser: illegal type (" << static_cast<int>($1) << ") of declaration";
			std::unreachable();
		} break;
	}

	auto type = context.require<TypeDecl>();
	type->ident = $2;
	type->type = basicType;
	type->optSubscriptList = std::move(*$3);
	delete $3;

	$$ = context.require<FuncDef>();
	$$->params.push_back(type);
}
;

//! 语句块
//! NOTE: BlockItemSequence := { BlockItem }
Block
: LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE");
	$$ = nullptr;
}
| LBRACE BlockItemSequence RBRACE                                                   { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE");
	//! TODO: 内存清理
	$$ = $2->statementList.size() == 0 ? nullptr : $2;
}
;

//! 语句块项
BlockItem
: Decl                                                                              { SSYC_PRINT_REDUCE(BlockItem, "Decl");
	$$ = $1;
}
| Stmt                                                                              { SSYC_PRINT_REDUCE(BlockItem, "Stmt");
	$$ = $1;
}
;

//! 语句
//! #-- Stmt BEGIN ---
/*!
 * 源文法使用 LR(0) 文法分析时对于 if-else 语句将产生 shift/reduce 冲突
 * TODO: 解决 if-else 语句的 shift/reduce 冲突
 * SOLUTION: 禁止 if 语句 shift 进 else 语句
 *
 * Stmt
 * : LVal OP_ASS Exp SEMICOLON
 * | SEMICOLON
 * | Exp SEMICOLON
 * | Block
 * | KW_IF LPAREN Cond RPAREN Stmt KW_ELSE Stmt
 * | KW_IF LPAREN Cond RPAREN Stmt
 * | KW_WHILE LPAREN Cond RPAREN Stmt
 * | KW_BREAK SEMICOLON
 * | KW_CONTINUE SEMICOLON
 * | KW_RETURN SEMICOLON
 * | KW_RETURN Exp SEMICOLON
 * ;
 */

Stmt
: LVal OP_ASS Exp SEMICOLON                                                         { SSYC_PRINT_REDUCE(Stmt, "LVal OP_ASS Exp SEMICOLON");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::ASSIGN;
	expr->lhs = $1;
	expr->rhs = $3;

	auto statement = context.require<ExprStatement>();
	statement->expr = expr;

	$$ = statement;
}
| SEMICOLON                                                                         { SSYC_PRINT_REDUCE(Stmt, "SEMICOLON");
	$$ = nullptr;
}
| Exp SEMICOLON                                                                     { SSYC_PRINT_REDUCE(Stmt, "Exp SEMICOLON");
	auto statement = context.require<ExprStatement>();
	statement->expr = $1;

	$$ = statement;
}
| Block                                                                             { SSYC_PRINT_REDUCE(Stmt, "Block");
	auto statement = context.require<CompoundStatement>();
	statement->block = $1;

	$$ = statement;
}
| KW_IF LPAREN Cond RPAREN Stmt                                                     { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN Stmt");
	auto statement = context.require<IfElseStatement>();
	statement->condition = $3;
	statement->trueRoute = $5;

	$$ = statement;
}
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt                          { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt");
	auto statement = context.require<IfElseStatement>();
	statement->condition = $3;
	statement->trueRoute = $5;
	statement->falseRoute = $7;

	$$ = statement;
}
| KW_WHILE LPAREN Cond RPAREN Stmt                                                  { SSYC_PRINT_REDUCE(Stmt, "KW_WHILE LPAREN Cond RPAREN Stmt");
	auto statement = context.require<WhileStatement>();
	statement->condition = $3;
	statement->body = $5;

	$$ = statement;
}
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(Stmt, "KW_BREAK SEMICOLON");
	$$ = context.require<BreakStatement>();
}
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(Stmt, "KW_CONTINUE SEMICOLON");
	$$ = context.require<ContinueStatement>();
}
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN SEMICOLON");
	$$ = context.require<ReturnStatement>();
}
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN Exp SEMICOLON");
	auto statement = context.require<ReturnStatement>();
	statement->retval = $2;

	$$ = statement;
}
;

StmtBeforeElseStmt
: LVal OP_ASS Exp SEMICOLON                                                         { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "LVal OP_ASS Exp SEMICOLON");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::ASSIGN;
	expr->lhs = $1;
	expr->rhs = $3;

	auto statement = context.require<ExprStatement>();
	statement->expr = expr;

	$$ = statement;
}
| SEMICOLON                                                                         { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "SEMICOLON");
	$$ = nullptr;
}
| Exp SEMICOLON                                                                     { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Exp SEMICOLON");
	auto statement = context.require<ExprStatement>();
	statement->expr = $1;

	$$ = statement;
}
| Block                                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Block");
	auto statement = context.require<CompoundStatement>();
	statement->block = $1;

	$$ = statement;
}
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt            { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt");
	auto statement = context.require<IfElseStatement>();
	statement->condition = $3;
	statement->trueRoute = $5;
	statement->falseRoute = $7;

	$$ = statement;
}
| KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt                                    { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt");
	auto statement = context.require<WhileStatement>();
	statement->condition = $3;
	statement->body = $5;

	$$ = statement;
}
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_BREAK SEMICOLON");
	$$ = context.require<BreakStatement>();
}
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_CONTINUE SEMICOLON");
	$$ = context.require<ContinueStatement>();
}
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN SEMICOLON");
	$$ = context.require<ReturnStatement>();
}
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN Exp SEMICOLON");
	auto statement = context.require<ReturnStatement>();
	statement->retval = $2;

	$$ = statement;
}
;

//! #-- Stmt END ---

//! 表达式
Exp
: AddExp                                                                            { SSYC_PRINT_REDUCE(Exp, "AddExp");
	$$ = $1;
}
;

//! 条件表达式
Cond
: LOrExp                                                                            { SSYC_PRINT_REDUCE(Cond, "LOrExp");
	$$ = $1;
}
;

//! 左值表达式
//! NOTE: SubscriptChain := { LBRACKET Exp RBRACKET }
LVal
: IDENT                                                                             { SSYC_PRINT_REDUCE(LVal, "IDENT");
	//! FIXME: type 应该通过查找符号表获取
	auto type = context.require<TypeDecl>();
	type->ident = $1;

	auto expr = context.require<OrphanExpr>();
	expr->ref = type;

	$$ = expr;
}
| IDENT SubscriptChain                                                              { SSYC_PRINT_REDUCE(LVal, "IDENT SubscriptChain");
	//! FIXME: type 应该通过查找符号表获取
	auto type = context.require<TypeDecl>();
	type->ident = $1;
	type->optSubscriptList = std::move(*$2);
	delete $2;

	auto expr = context.require<OrphanExpr>();
	expr->ref = type;

	$$ = expr;
}
;

//! 基本表达式
PrimaryExp
: LPAREN Exp RPAREN                                                                 { SSYC_PRINT_REDUCE(PrimaryExp, "LPAREN Exp RPAREN");
	$$ = $2;
}
| LVal                                                                              { SSYC_PRINT_REDUCE(PrimaryExp, "LVal");
	$$ = $1;
}
| Number                                                                            { SSYC_PRINT_REDUCE(PrimaryExp, "Number");
	$$ = $1;
}
;

//! 数值
Number
: CONST_INT                                                                         { SSYC_PRINT_REDUCE(Number, "CONST_INT");
	//! FIXME: 注意下这里的数值转换规范
	auto expr = context.require<ConstExprExpr>();
	expr->type = ConstExprExpr::Type::Integer;
	expr->value = static_cast<int32_t>(std::stoi($1));

	$$ = expr;
}
| CONST_FLOAT                                                                       { SSYC_PRINT_REDUCE(Number, "CONST_FLOAT");
	//! FIXME: 注意下这里的数值转换规范
	auto expr = context.require<ConstExprExpr>();
	expr->type = ConstExprExpr::Type::Float;
	expr->value = static_cast<float>(std::stof($1));

	$$ = expr;
}
;

//! 一元表达式
UnaryExp
: PrimaryExp                                                                        { SSYC_PRINT_REDUCE(UnaryExp, "PrimaryExp");
	$$ = $1;
}
| IDENT LPAREN RPAREN                                                               { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN RPAREN");
	//! FIXME: 这里的函数引用应该通过查找符号表得到
	auto func = context.require<FuncDef>();
	func->ident = $1;

	auto expr = context.require<FnCallExpr>();
	expr->func = func;

	$$ = expr;
}
| IDENT LPAREN FuncRParams RPAREN                                                   { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN FuncRParams RPAREN");
	//! FIXME: 这里的函数引用应该通过查找符号表得到
	auto func = context.require<FuncDef>();
	func->ident = $1;

	auto expr = dynamic_cast<FnCallExpr*>($3);
	expr->func = func;

	$$ = expr;
}
| UnaryOp UnaryExp                                                                  { SSYC_PRINT_REDUCE(UnaryExp, "UnaryOp UnaryExp");
	auto expr = context.require<UnaryExpr>();
	expr->operand = $2;

	switch ($1) {
		case OP_POS: {
			expr->op = UnaryExpr::Type::POS;
		} break;
		case OP_NEG: {
			expr->op = UnaryExpr::Type::NEG;
		} break;
		case OP_LNOT: {
			expr->op = UnaryExpr::Type::LNOT;
		} break;
	}

	$$ = expr;
}
;

//! 单目运算符
UnaryOp
: OP_ADD                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_ADD");
	$$ = OP_POS;
}
| OP_SUB                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_SUB");
	$$ = OP_NEG;
}
| OP_LNOT                                                                           { SSYC_PRINT_REDUCE(UnaryOp, "OP_LNOT");
	$$ = $1;
}
;

//! 函数实参表
FuncRParams
: Exp                                                                               { SSYC_PRINT_REDUCE(FuncRParams, "Exp");
	auto expr = context.require<FnCallExpr>();
	expr->params.push_back($1);

	$$ = expr;
}
| FuncRParams COMMA Exp                                                             { SSYC_PRINT_REDUCE(FuncRParams, "FuncRParams COMMA Exp");
	auto expr = dynamic_cast<FnCallExpr*>($1);
	expr->params.push_back($3);

	$$ = expr;
}
;

//! 乘除模表达式
MulExp
: UnaryExp                                                                          { SSYC_PRINT_REDUCE(MulExp, "UnaryExp");
	$$ = $1;
}
| MulExp OP_MUL UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MUL UnaryExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::MUL;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| MulExp OP_DIV UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_DIV UnaryExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::DIV;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| MulExp OP_MOD UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MOD UnaryExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::MOD;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 加减表达式
AddExp
: MulExp                                                                            { SSYC_PRINT_REDUCE(AddExp, "MulExp");
	$$ = $1;
}
| AddExp OP_ADD MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_ADD MulExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::ADD;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| AddExp OP_SUB MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_SUB MulExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::SUB;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 关系表达式
RelExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(RelExp, "AddExp");
	$$ = $1;
}
| RelExp OP_LT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LT AddExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::LT;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| RelExp OP_GT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GT AddExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::GT;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| RelExp OP_LE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LE AddExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::LE;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| RelExp OP_GE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GE AddExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::GE;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 相等性表达式
EqExp
: RelExp                                                                            { SSYC_PRINT_REDUCE(EqExp, "RelExp");
	$$ = $1;
}
| EqExp OP_EQ RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_EQ RelExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::EQ;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
| EqExp OP_NE RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_NE RelExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::NE;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 逻辑与表达式
LAndExp
: EqExp                                                                             { SSYC_PRINT_REDUCE(LAndExp, "EqExp");
	$$ = $1;
}
| LAndExp OP_LAND EqExp                                                             { SSYC_PRINT_REDUCE(LAndExp, "LAndExp OP_LAND EqExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::LAND;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 逻辑或表达式
LOrExp
: LAndExp                                                                           { SSYC_PRINT_REDUCE(LOrExp, "LAndExp");
	$$ = $1;
}
| LOrExp OP_LOR LAndExp                                                             { SSYC_PRINT_REDUCE(LOrExp, "LOrExp OP_LOR LAndExp");
	auto expr = context.require<BinaryExpr>();
	expr->op = BinaryExpr::Type::LOR;
	expr->lhs = $1;
	expr->rhs = $3;

	$$ = expr;
}
;

//! 常量表达式
ConstExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(ConstExp, "AddExp");
	$$ = $1;
}
;

//! #-- 辅助文法 BEGIN ---

//! 常量定义列表
ConstDefList
: ConstDef                                                                          { SSYC_PRINT_REDUCE(ConstDefList, "ConstDef");
	$$ = $1;
}
| ConstDefList COMMA ConstDef                                                       { SSYC_PRINT_REDUCE(ConstDefList, "ConstDefList COMMA ConstDef");
	//! TODO: 内存清理
	$$ = $1;
	$$->declList.push_back($3->declList[0]);
}
;

//! 常量下标索引链
SubscriptChainConst
: LBRACKET ConstExp RBRACKET                                                        { SSYC_PRINT_REDUCE(SubscriptChainConst, "LBRACKET ConstExp RBRACKET");
	$$ = new std::vector<Expr*>;
	$$->push_back($2);
}
| SubscriptChainConst LBRACKET ConstExp RBRACKET                                    { SSYC_PRINT_REDUCE(SubscriptChainConst, "SubscriptChainConst LBRACKET ConstExp RBRACKET");
	$$ = $1;
	$$->push_back($3);
}
;

//! 常量初值列表
ConstInitValList
: ConstInitVal                                                                      { SSYC_PRINT_REDUCE(ConstInitValList, "ConstInitVal");
	auto list = context.require<InitializeList>();
	list->valueList.push_back($1);

	auto expr = context.require<OrphanExpr>();
	expr->ref = list;

	$$ = expr;
}
| ConstInitValList COMMA ConstInitVal                                               { SSYC_PRINT_REDUCE(ConstInitValList, "ConstInitValList COMMA ConstInitVal");
	$$ = $1;

	auto expr = dynamic_cast<OrphanExpr*>($$);
	auto list = std::get<InitializeList*>(expr->ref);
	list->valueList.push_back($3);
}
;

//! 缺省下标索引链
SubscriptChainVariant
: LBRACKET RBRACKET                                                                 { SSYC_PRINT_REDUCE(SubscriptChainVariant, "LBRACKET RBRACKET");
	$$ = new std::vector<Expr*>;
	$$->push_back(nullptr);
}
| LBRACKET RBRACKET SubscriptChain                                                  { SSYC_PRINT_REDUCE(SubscriptChainVariant, "LBRACKET RBRACKET SubscriptChain");
	$$ = $3;
	$$->insert($$->begin(), nullptr);
}
;

//! 变量定义列表
VarDefList
: VarDef                                                                            { SSYC_PRINT_REDUCE(VarDefList, "VarDef");
	$$ = $1;
}
| VarDefList COMMA VarDef                                                           { SSYC_PRINT_REDUCE(VarDefList, "VarDefList COMMA VarDef");
	//! TODO: 内存清理
	$$ = $1;
	$$->declList.push_back($3->declList[0]);
}
;

//! 变量初值列表
InitValList
: InitVal                                                                           { SSYC_PRINT_REDUCE(InitValList, "InitVal");
	auto list = context.require<InitializeList>();
	list->valueList.push_back($1);

	auto expr = context.require<OrphanExpr>();
	expr->ref = list;

	$$ = expr;
}
| InitValList COMMA InitVal                                                         { SSYC_PRINT_REDUCE(InitValList, "InitValList COMMA InitVal");
	$$ = $1;

	auto expr = dynamic_cast<OrphanExpr*>($$);
	auto list = std::get<InitializeList*>(expr->ref);
	list->valueList.push_back($3);
}
;

//! 下标索引链
SubscriptChain
: LBRACKET Exp RBRACKET                                                             { SSYC_PRINT_REDUCE(SubscriptChain, "LBRACKET Exp RBRACKET");
	$$ = new std::vector<Expr*>;
	$$->push_back($2);
}
| SubscriptChain LBRACKET Exp RBRACKET                                              { SSYC_PRINT_REDUCE(SubscriptChain, "SubscriptChain LBRACKET Exp RBRACKET");
	$$ = $1;
	$$->push_back($3);
}
;

//! 语句块项序列
BlockItemSequence
: BlockItem                                                                         { SSYC_PRINT_REDUCE(BlockItemSequence, "BlockItem");
	$$ = context.require<Block>();
	if ($1 != nullptr) {
		$$->statementList.push_back($1);
	}
}
| BlockItemSequence BlockItem                                                       { SSYC_PRINT_REDUCE(BlockItemSequence, "BlockItemSequence BlockItem");
	$$ = $1;
	if ($2 != nullptr) {
		$$->statementList.push_back($2);
	}
}
;

//! #-- SysY 2022 文法 END ---

%%

void yyerror(ssyc::ParserContext &context, char* message) {
    LOG(ERROR) << "Parser: " << message;
    exit(-1);
}

int yywrap() {
    return 1;
}
