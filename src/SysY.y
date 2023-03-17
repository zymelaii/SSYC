%{
#include "pch.h"
#include "utils.h"
#include "ast.h"
#include "parser_context.h"

#define YYSTYPE ssyc::AstNode*

using ssyc::AstNode;
using ssyc::ParserContext;
using ssyc::SyntaxType;
using ssyc::TokenType;
using ContextFlag = ssyc::ParserContext::ContextFlag;

void yyerror(ssyc::ParserContext &context, char* message);
extern "C" int yywrap();
int yylex();
%}

//! 指定解析器参数
%parse-param {ssyc::ParserContext &context}

//! #-- TOKEN 声明 BEGIN ---
//! 标识符
%token IDENT

//! 常量
%token CONST_INT CONST_FLOAT

//! 基本类型
%token T_VOID T_INT T_FLOAT

//! 类型限定符
%token KW_CONST

//! 控制关键字
%token KW_IF KW_ELSE
%token KW_WHILE KW_BREAK KW_CONTINUE
%token KW_RETURN

//! 操作符
%token OP_ASS
%token OP_ADD OP_SUB
%token OP_MUL OP_DIV OP_MOD
%token OP_LT OP_GT OP_LE OP_GE OP_EQ OP_NE
%token OP_LNOT OP_LAND OP_LOR

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
	$$ = context.require();
	$$->left = $1;
	$$->syntax = SyntaxType::Program;

	const auto ok = context.testOrSetFlag(ContextFlag::SyntaxAnalysisDone);
	LOG_IF(ERROR, !ok) << "Parser: caught one more program unit";

	context.program = $$;
}
| FuncDef                                                                           { SSYC_PRINT_REDUCE(CompUnit, "FuncDef");
	$$ = context.require();
	$$->left = $1;
	$$->syntax = SyntaxType::Program;

	$1->parent = $$;

	const auto ok = context.testOrSetFlag(ContextFlag::SyntaxAnalysisDone);
	LOG_IF(ERROR, !ok) << "Parser: caught one more program unit";

	context.program = $$;
}
| CompUnit Decl                                                                     { SSYC_PRINT_REDUCE(CompUnit, "CompUnit Decl");
	$$ = context.require();
	$$->left = $1;
	$$->right = $2;
	$$->syntax = SyntaxType::Program;

	$1->parent = $$;
	$2->parent = $$;
}
| CompUnit FuncDef                                                                  { SSYC_PRINT_REDUCE(CompUnit, "CompUnit FuncDef");
	$$ = context.require();
	$$->left = $1;
	$$->right = $2;
	$$->syntax = SyntaxType::Program;

	$1->parent = $$;
	$2->parent = $$;
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
	LOG_IF(ERROR, $2->token == TokenType::TT_T_VOID)
		<< "Parser: declaration cannot be of type `void'";

	//! TODO: 待补全
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
	$$ = context.require();
	$$->token = TokenType::TT_T_VOID;
}
| T_INT                                                                             { SSYC_PRINT_REDUCE(BType, "T_INT");
	$$ = context.require();
	$$->token = TokenType::TT_T_INT;
}
| T_FLOAT                                                                           { SSYC_PRINT_REDUCE(BType, "T_FLOAT");
	$$ = context.require();
	$$->token = TokenType::TT_T_FLOAT;
}
;

//! #-- BType END ---

//! 常数定义
//! NOTE: SubscriptChainConst := { LBRACKET ConstExp RBRACKET }
ConstDef
: IDENT OP_ASS ConstInitVal                                                         { SSYC_PRINT_REDUCE(ConstDef, "IDENT OP_ASS ConstInitVal");
	LOG_IF(ERROR, $3->FLAGS_ArrayBlock)
		<< "Parser: non-array variable cannot be initialized with array block";
}
| IDENT SubscriptChainConst OP_ASS ConstInitVal                                     { SSYC_PRINT_REDUCE(ConstDef, "IDENT SubscriptChainConst OP_ASS ConstInitVal");
	LOG_IF(ERROR, !$4->FLAGS_ArrayBlock)
		<< "Parser: array variable cannot be initialized with single value";
}
;

//! 常量初值
//! NOTE: ConstInitValList := ConstInitVal { COMMA ConstInitVal }
ConstInitVal
: ConstExp                                                                          { SSYC_PRINT_REDUCE(ConstInitVal, "ConstExp"); }
| LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(ConstInitVal, "LBRACE RBRACE"); }
| LBRACE ConstInitValList RBRACE                                                    { SSYC_PRINT_REDUCE(ConstInitVal, "LBRACE ConstInitValList RBRACE"); }
;

//! 变量声明
//! NOTE: VarDefList := VarDef { COMMA VarDef }
VarDecl
: BType VarDefList SEMICOLON                                                        { SSYC_PRINT_REDUCE(VarDecl, "BType VarDefList SEMICOLON");
	LOG_IF(ERROR, $2->token == TokenType::TT_T_VOID)
		<< "Parser: declaration cannot be of type `void'";

	//! TODO: 待补全
}
;

//! 变量定义
VarDef
: IDENT                                                                             { SSYC_PRINT_REDUCE(VarDef, "IDENT");
	$$ = context.require();
	//! TODO: 待补全
}
| IDENT OP_ASS InitVal                                                              { SSYC_PRINT_REDUCE(VarDef, "IDENT OP_ASS InitVal");
	$$ = context.require();
	//! TODO: 待补全
}
| IDENT SubscriptChainConst                                                         { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst");
	$$ = context.require();
	//! TODO: 待补全
}
| IDENT SubscriptChainConst OP_ASS InitVal                                          { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst OP_ASS InitVal");
	$$ = context.require();
	//! TODO: 待补全
}
;

//! 变量初值
//! NOTE: InitValList := InitVal { COMMA InitVal }
InitVal
: Exp                                                                               { SSYC_PRINT_REDUCE(InitVal, "Exp"); }
| LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(InitVal, "LBRACE RBRACE"); }
| LBRACE InitValList RBRACE                                                         { SSYC_PRINT_REDUCE(InitVal, "LBRACE InitValList RBRACE"); }
;

//! 函数定义
//! NOTE: FuncDef := FuncType Ident LPAREN [ FuncFParams ] RPAREN Block
//! NOTE: replace `FuncType` with `BType`
FuncDef
: BType IDENT LPAREN RPAREN Block                                                   { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN RPAREN Block");
	$$ = context.require();
	$$->right = $5;
	$$->syntax = SyntaxType::FuncDef;
	$$->value = $2->value;

	$5->parent = $$;

	context.deleteLater($2);
}
| BType IDENT LPAREN FuncFParams RPAREN Block                                       { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN FuncFParams RPAREN Block");

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
: FuncFParam                                                                        { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParam"); }
| FuncFParams COMMA FuncFParam                                                      { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParams COMMA FuncFParam"); }
;

//! 函数形参
//! NOTE: SubscriptChainVariant := LBRACKET RBRACKET [ { LBRACKET Exp RBRACKET } ]
FuncFParam
: BType IDENT                                                                       { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT");
	LOG_IF(ERROR, $2->token == TokenType::TT_T_VOID)
		<< "Parser: declaration cannot be of type `void'";

	//! TODO: 待补全
}
| BType IDENT SubscriptChainVariant                                                 { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT SubscriptChainVariant");
	LOG_IF(ERROR, $2->token == TokenType::TT_T_VOID)
		<< "Parser: declaration cannot be of type `void'";

	//! TODO: 待补全
}
;

//! 语句块
//! NOTE: BlockItemSequence := { BlockItem }
Block
: LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE");
	$$ = context.require();
	$$->syntax = SyntaxType::Block;
	$$->FLAGS_EmptyBlock = true;
	$$->FLAGS_Nested = true;

	LOG(WARNING) << "Parser: caught empty block";
}
| LBRACE BlockItemSequence RBRACE                                                   { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE");
	$$ = context.require();
	$$->syntax = SyntaxType::Block;

	//! FIXME: 完成 BlockItemSequence 后去除
	$$->FLAGS_Nested = true;

	//! TODO: 待补全
}
;

//! 语句块项
BlockItem
: Decl                                                                              { SSYC_PRINT_REDUCE(BlockItem, "Decl"); }
| Stmt                                                                              { SSYC_PRINT_REDUCE(BlockItem, "Stmt"); }
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
	$$ = context.require();
	$$->left = $1;
	$$->right = $3;
	$$->syntax = SyntaxType::Assign;

	$1->parent = $$;
	$3->parent = $$;
}
| SEMICOLON                                                                         { SSYC_PRINT_REDUCE(Stmt, "SEMICOLON");
	$$ = nullptr;
}
| Exp SEMICOLON                                                                     { SSYC_PRINT_REDUCE(Stmt, "Exp SEMICOLON");
	$$ = $1;
}
| Block                                                                             { SSYC_PRINT_REDUCE(Stmt, "Block");
	$$ = $1;
}
| KW_IF LPAREN Cond RPAREN Stmt                                                     { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN Stmt");
	//! TODO: 调整 if-else 结构
	$$ = context.require();
	$$->left = $5;
	$$->syntax = SyntaxType::IfElse;

	$5->parent = $$;
}
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt                          { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt");
	//! TODO: 调整 if-else 结构
	$$ = context.require();
	$$->left = $5;
	$$->right = $7;
	$$->syntax = SyntaxType::IfElse;

	$5->parent = $$;
	$7->parent = $$;
}
| KW_WHILE LPAREN Cond RPAREN Stmt                                                  { SSYC_PRINT_REDUCE(Stmt, "KW_WHILE LPAREN Cond RPAREN Stmt");
	$$ = context.require();
	$$->left = $2;
	$$->right = $5;
	$$->syntax = SyntaxType::While;

	$2->parent = $$;
	$5->parent = $$;
}
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(Stmt, "KW_BREAK SEMICOLON");
	$$ = context.require();
	$$->token = TokenType::TT_BREAK;
}
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(Stmt, "KW_CONTINUE SEMICOLON");
	$$ = context.require();
	$$->token = TokenType::TT_CONTINUE;
}
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN SEMICOLON");
	$$ = context.require();
	$$->syntax = SyntaxType::Return;
	$$->FLAGS_Nested = true;
}
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN Exp SEMICOLON");
	$$ = context.require();
	$$->left = $1;
	$$->syntax = SyntaxType::Return;

	$1->parent = $$;
}
;

StmtBeforeElseStmt
: LVal OP_ASS Exp SEMICOLON                                                         { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "LVal OP_ASS Exp SEMICOLON");
	$$ = context.require();
	$$->left = $1;
	$$->right = $3;
	$$->syntax = SyntaxType::Assign;

	$1->parent = $$;
	$3->parent = $$;
}
| SEMICOLON                                                                         { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "SEMICOLON");
	$$ = nullptr;
}
| Exp SEMICOLON                                                                     { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Exp SEMICOLON");
	$$ = $1;
}
| Block                                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Block");
	$$ = $1;
}
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt            { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt");
	//! TODO: 调整 if-else 结构
	$$ = context.require();
	$$->left = $5;
	$$->right = $7;
	$$->syntax = SyntaxType::IfElse;

	$5->parent = $$;
	$7->parent = $$;
}
| KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt                                    { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt");
	$$ = context.require();
	$$->left = $2;
	$$->right = $5;
	$$->syntax = SyntaxType::While;

	$2->parent = $$;
	$5->parent = $$;
}
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_BREAK SEMICOLON");
	$$ = context.require();
	$$->token = TokenType::TT_BREAK;
}
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_CONTINUE SEMICOLON");
	$$ = context.require();
	$$->token = TokenType::TT_CONTINUE;
}
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN SEMICOLON");
	$$ = context.require();
	$$->syntax = SyntaxType::Return;
	$$->FLAGS_Nested = true;
}
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN Exp SEMICOLON");
	$$ = context.require();
	$$->left = $1;
	$$->syntax = SyntaxType::Return;

	$1->parent = $$;
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
	$$ = $1;
}
| IDENT SubscriptChain                                                              { SSYC_PRINT_REDUCE(LVal, "IDENT SubscriptChain"); }
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
	$$ = $1;
}
| CONST_FLOAT                                                                       { SSYC_PRINT_REDUCE(Number, "CONST_FLOAT");
	$$ = $1;
}
;

//! 一元表达式
UnaryExp
: PrimaryExp                                                                        { SSYC_PRINT_REDUCE(UnaryExp, "PrimaryExp");
	$$ = $1;
}
| IDENT LPAREN RPAREN                                                               { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN RPAREN"); }
| IDENT LPAREN FuncRParams RPAREN                                                   { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN FuncRParams RPAREN"); }
| UnaryOp UnaryExp                                                                  { SSYC_PRINT_REDUCE(UnaryExp, "UnaryOp UnaryExp");
	$$ = $1;
	$1->left = $2;
	LOG(INFO) << "Parser: caught unary expression";
}
;

//! 单目运算符
UnaryOp
: OP_ADD                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_ADD");
	$$ = context.require();
	$$->token = TokenType::TT_OP_POS;
}
| OP_SUB                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_SUB");
	$$ = context.require();
	$$->token = TokenType::TT_OP_NEG;
}
| OP_LNOT                                                                           { SSYC_PRINT_REDUCE(UnaryOp, "OP_LNOT");
	$$ = context.require();
	$$->token = TokenType::TT_OP_LNOT;
}
;

//! 函数实参表
FuncRParams
: Exp                                                                               { SSYC_PRINT_REDUCE(FuncRParams, "Exp"); }
| FuncRParams COMMA Exp                                                             { SSYC_PRINT_REDUCE(FuncRParams, "FuncRParams COMMA Exp"); }
;

//! 乘除模表达式
MulExp
: UnaryExp                                                                          { SSYC_PRINT_REDUCE(MulExp, "UnaryExp");
	$$ = $1;
}
| MulExp OP_MUL UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MUL UnaryExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_MUL;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| MulExp OP_DIV UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_DIV UnaryExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_DIV;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| MulExp OP_MOD UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MOD UnaryExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_MOD;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 加减表达式
AddExp
: MulExp                                                                            { SSYC_PRINT_REDUCE(AddExp, "MulExp");
	$$ = $1;
}
| AddExp OP_ADD MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_ADD MulExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_ADD;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| AddExp OP_SUB MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_SUB MulExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_SUB;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 关系表达式
RelExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(RelExp, "AddExp");
	$$ = $1;
}
| RelExp OP_LT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LT AddExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_LT;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| RelExp OP_GT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GT AddExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_GT;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| RelExp OP_LE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LE AddExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_LE;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| RelExp OP_GE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GE AddExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_GE;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 相等性表达式
EqExp
: RelExp                                                                            { SSYC_PRINT_REDUCE(EqExp, "RelExp");
	$$ = $1;
}
| EqExp OP_EQ RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_EQ RelExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_EQ;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
| EqExp OP_NE RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_NE RelExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_NE;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 逻辑与表达式
LAndExp
: EqExp                                                                             { SSYC_PRINT_REDUCE(LAndExp, "EqExp");
	$$ = $1;
}
| LAndExp OP_LAND EqExp                                                             { SSYC_PRINT_REDUCE(LAndExp, "LAndExp OP_LAND EqExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_LAND;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 逻辑或表达式
LOrExp
: LAndExp                                                                           { SSYC_PRINT_REDUCE(LOrExp, "LAndExp");
	$$ = $1;
}
| LOrExp OP_LOR LAndExp                                                             { SSYC_PRINT_REDUCE(LOrExp, "LOrExp OP_LOR LAndExp");
	$$ = context.require();
	$$->token = TokenType::TT_OP_LOR;
	$$->left = $1;
	$$->right = $2;

	$1->parent = $$;
	$2->parent = $$;
}
;

//! 常量表达式
ConstExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(ConstExp, "AddExp");
	//! NOTE: 在常量/变量的数组定义中，必须保证 ConstExp 是严格的常量
	//! NOTE: FLAGS_ConstEval 表明目标可被编译期求值，不应当被直接作为判断是否为常量的依据
	$$ = $1;
}
;

//! #-- 辅助文法 BEGIN ---

//! 常量定义列表
ConstDefList
: ConstDef                                                                          { SSYC_PRINT_REDUCE(ConstDefList, "ConstDef"); }
| ConstDefList COMMA ConstDef                                                       { SSYC_PRINT_REDUCE(ConstDefList, "ConstDefList COMMA ConstDef"); }
;

//! 常量下标索引链
SubscriptChainConst
: LBRACKET ConstExp RBRACKET                                                        { SSYC_PRINT_REDUCE(SubscriptChainConst, "LBRACKET ConstExp RBRACKET"); }
| SubscriptChainConst LBRACKET ConstExp RBRACKET                                    { SSYC_PRINT_REDUCE(SubscriptChainConst, "SubscriptChainConst LBRACKET ConstExp RBRACKET"); }
;

//! 常量初值列表
ConstInitValList
: ConstInitVal                                                                      { SSYC_PRINT_REDUCE(ConstInitValList, "ConstInitVal"); }
| ConstInitValList COMMA ConstInitVal                                               { SSYC_PRINT_REDUCE(ConstInitValList, "ConstInitValList COMMA ConstInitVal"); }
;

//! 缺省下标索引链
SubscriptChainVariant
: LBRACKET RBRACKET                                                                 { SSYC_PRINT_REDUCE(SubscriptChainVariant, "LBRACKET RBRACKET"); }
| LBRACKET RBRACKET SubscriptChain                                                  { SSYC_PRINT_REDUCE(SubscriptChainVariant, "LBRACKET RBRACKET SubscriptChain"); }
;

//! 变量定义列表
VarDefList
: VarDef                                                                            { SSYC_PRINT_REDUCE(VarDefList, "VarDef"); }
| VarDefList COMMA VarDef                                                           { SSYC_PRINT_REDUCE(VarDefList, "VarDefList COMMA VarDef"); }
;

//! 变量初值列表
InitValList
: InitVal                                                                           { SSYC_PRINT_REDUCE(InitValList, "InitVal"); }
| InitValList COMMA InitVal                                                         { SSYC_PRINT_REDUCE(InitValList, "InitValList COMMA InitVal"); }
;

//! 下标索引链
SubscriptChain
: LBRACKET Exp RBRACKET                                                             { SSYC_PRINT_REDUCE(SubscriptChain, "LBRACKET Exp RBRACKET"); }
| SubscriptChain LBRACKET Exp RBRACKET                                              { SSYC_PRINT_REDUCE(SubscriptChain, "SubscriptChain LBRACKET Exp RBRACKET"); }
;

//! 语句块项序列
BlockItemSequence
: BlockItem                                                                         { SSYC_PRINT_REDUCE(BlockItemSequence, "BlockItem"); }
| BlockItemSequence BlockItem                                                       { SSYC_PRINT_REDUCE(BlockItemSequence, "BlockItemSequence BlockItem"); }
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
