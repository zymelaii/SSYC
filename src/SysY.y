%{
#include "utils.h"
#include <iostream>
#include <string.h>

void yyerror(char* message);
extern "C" int yywrap();
int yylex();
%}

//! #-- TOKEN 声明 BEGIN ---
//! 标识符
%token IDENT

//! 常量
%token INT_LITERAL FLOAT_LITERAL

//! 基本类型
%token VOID_TYPE INT_TYPE FLOAT_TYPE

//! 类型限定符
%token CONST_KEYWORD

//! 控制关键字
%token IF_KEYWORD ELSE_KEYWORD
%token WHILE_KEYWORD BREAK_KEYWORD CONTINUE_KEYWORD
%token RETURN_KEYWORD

//! 操作符
%token ASSIGN_OP
%token ADD_OP SUB_OP
%token MUL_OP DIV_OP MOD_OP
%token LT_OP GT_OP LE_OP GE_OP EQ_OP NE_OP
%token LNOT_OP LAND_OP LOR_OP

//! 杂项
%token LPAREN RPAREN
%token LBRACKET RBRACKET
%token LBRACE RBRACE
%token COMMA SEMICOLON

//! #-- TOKEN 声明 END ---

%left ADD_OP SUB_OP MUL_OP DIV_OP 
%left LT_OP LE_OP GT_OP GE_OP EQ_OP NE_OP
%left LAND_OP LOR_OP
%left COMMA
%right LNOT_OP
%right ASSIGN_OP

%%

/*! 文法单元测试
 * [TODO] %start CompUnit;
 * [TODO] %start Decl;
 * [PASS] %start ConstDecl;
 * - [PASS] CONST_KEYWORD BType ConstDefList SEMICOLON
 * [PASS] %start BType;
 * - [PASS] VOID_TYPE
 * - [PASS] INT_TYPE
 * - [PASS] FLOAT_TYPE
 * [PASS] %start ConstDef;
 * - [PASS] IDENT ASSIGN_OP ConstInitVal
 * - [PASS] IDENT SubscriptChainConst ASSIGN_OP ConstInitVal
 * [TODO] %start ConstInitVal;
 * [TODO] %start VarDecl;
 * [TODO] %start VarDef;
 * [TODO] %start InitVal;
 * [TODO] %start FuncDef;
 * [TODO] %start FuncFParams;
 * [TODO] %start FuncFParam;
 * [TODO] %start Block;
 * [TODO] %start BlockItem;
 * [TODO] %start Stmt;
 * [TODO] %start StmtBeforeElseStmt;
 * [TODO] %start Exp;
 * [TODO] %start Cond;
 * [PASS] %start LVal;
 * - [PASS] IDENT SubscriptChain
 * [PASS] %start PrimaryExp;
 * - [PASS] LPAREN Exp RPAREN
 * - [PASS] LVal
 * - [PASS] Number
 * [PASS] %start Number;
 * - [PASS] INT_LITERAL
 * - [PASS] FLOAT_LITERAL
 * [PASS] %start UnaryExp;
 * - [PASS] PrimaryExp
 * - [PASS] IDENT LPAREN RPAREN
 * - [PASS] IDENT LPAREN FuncRParams RPAREN
 * - [PASS] UnaryOp UnaryExp
 * [PASS] %start UnaryOp;
 * - [PASS] ADD_OP
 * - [PASS] SUB_OP
 * - [PASS] LNOT_OP
 * [TODO] %start FuncRParams;
 * [PASS] %start MulExp;
 * - [PASS] UnaryExp
 * - [PASS] MulExp MUL_OP UnaryExp
 * - [PASS] MulExp DIV_OP UnaryExp
 * - [PASS] MulExp MOD_OP UnaryExp
 * [PASS] %start AddExp;
 * - [PASS] MulExp
 * - [PASS] AddExp ADD_OP MulExp
 * - [PASS] AddExp SUB_OP MulExp
 * [TODO] %start RelExp;
 * [TODO] %start EqExp;
 * [TODO] %start LAndExp;
 * [TODO] %start LOrExp;
 * [TODO] %start ConstExp;
 * [TODO] %start ConstDefList;
 * [TODO] %start SubscriptChainConst;
 * [TODO] %start ConstInitValList;
 * [TODO] %start VarDefList;
 * [TODO] %start InitValList;
 * [PASS] %start SubscriptChain;
 * - [PASS] LBRACKET Exp RBRACKET
 * - [PASS] SubscriptChain LBRACKET Exp RBRACKET
 * [TODO] %start BlockItemSequence;
 */

//! #-- SysY 2022 文法 BEGIN ---

%start CompUnit;

//! 编译单元
CompUnit
: Decl                                                                              { SSYC_PRINT_REDUCE(CompUnit, "Decl"); }
| FuncDef                                                                           { SSYC_PRINT_REDUCE(CompUnit, "FuncDef"); }
| CompUnit Decl                                                                     { SSYC_PRINT_REDUCE(CompUnit, "CompUnit Decl"); }
| CompUnit FuncDef                                                                  { SSYC_PRINT_REDUCE(CompUnit, "CompUnit FuncDef"); }
;

//! 声明
Decl
: ConstDecl                                                                         { SSYC_PRINT_REDUCE(Decl, "ConstDecl"); }
| VarDecl                                                                           { SSYC_PRINT_REDUCE(Decl, "VarDecl"); }
;

//! 常量声明
//! NOTE: ConstDefList := ConstDef { COMMA ConstDef }
ConstDecl
: CONST_KEYWORD BType ConstDefList SEMICOLON                                        { SSYC_PRINT_REDUCE(ConstDecl, "CONST_KEYWORD BType SEMICOLON"); }
;

//! 基本类型
//! #-- BType BEGIN ---
/*!
 * BType 和 FuncType 同时组成了 CompUnit 的前缀，产生 reduce/reduce 冲突
 * TODO: 解决 BType 和 FuncType 的 reduce/reduce 冲突
 * SOLUTION: 合并 BType 与 FuncType 将检查延迟到语义分析阶段
 *
 * BType
 * : INT_TYPE
 * | FLOAT_TYPE
 * ;
 */

BType
: VOID_TYPE                                                                         { SSYC_PRINT_REDUCE(BType, "VOID_TYPE"); }
| INT_TYPE                                                                          { SSYC_PRINT_REDUCE(BType, "INT_TYPE"); }
| FLOAT_TYPE                                                                        { SSYC_PRINT_REDUCE(BType, "FLOAT_TYPE"); }
;

//! #-- BType END ---

//! 常数定义
//! NOTE: SubscriptChainConst := { LBRACKET ConstExp RBRACKET }
ConstDef
: IDENT ASSIGN_OP ConstInitVal                                                      { SSYC_PRINT_REDUCE(ConstDef, "IDENT ASSIGN_OP ConstInitVal"); }
| IDENT SubscriptChainConst ASSIGN_OP ConstInitVal                                  { SSYC_PRINT_REDUCE(ConstDef, "IDENT SubscriptChainConst ASSIGN_OP ConstInitVal"); }
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
: BType VarDefList SEMICOLON                                                        { SSYC_PRINT_REDUCE(VarDecl, "BType VarDefList SEMICOLON"); }
;

//! 变量定义
VarDef
: IDENT SubscriptChainConst                                                         { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst"); }
| IDENT SubscriptChainConst ASSIGN_OP InitVal                                       { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst ASSIGN_OP InitVal"); }
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
: BType IDENT LPAREN RPAREN Block                                                   { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN RPAREN Block"); }
| BType IDENT LPAREN FuncFParams RPAREN Block                                       { SSYC_PRINT_REDUCE(FuncDef, "BType IDENT LPAREN FuncFParams RPAREN Block"); }
;

//! 函数类型
//! #-- FuncType BEGIN ---
/*!
 * BType 和 FuncType 同时组成了 CompUnit 的前缀，产生 reduce/reduce 冲突
 * TODO: 解决 BType 和 FuncType 的 reduce/reduce 冲突
 * SOLUTION: 合并 BType 与 FuncType 将检查延迟到语义分析阶段
 *
 * FuncType
 * : VOID_TYPE
 * | INT_TYPE
 * | FLOAT_TYPE
 * ;
 */

//! #-- FuncType END ---

//! 函数形参表
FuncFParams
: FuncFParam                                                                        { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParam"); }
| FuncFParams COMMA FuncFParam                                                      { SSYC_PRINT_REDUCE(FuncFParams, "FuncFParams COMMA FuncFParam"); }
;

//! 函数形参
//! NOTE: SubscriptChain := { LBRACKET Exp RBRACKET }
FuncFParam
: BType IDENT                                                                       { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT"); }
| BType IDENT LBRACKET RBRACKET SubscriptChain                                      { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT LBRACKET RBRACKET SubscriptChain"); }
;

//! 语句块
//! NOTE: BlockItemSequence := { BlockItem }
Block
: LBRACE RBRACE                                                                     { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE"); }
| LBRACE BlockItemSequence RBRACE                                                   { SSYC_PRINT_REDUCE(Block, "LBRACE BlockItemSequence RBRACE"); }
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
 * : LVal ASSIGN_OP Exp SEMICOLON
 * | Block
 * | IF_KEYWORD LPAREN Cond RPAREN Stmt ELSE_KEYWORD Stmt
 * | IF_KEYWORD LPAREN Cond RPAREN Stmt
 * | WHILE_KEYWORD LPAREN Cond RPAREN Stmt
 * | BREAK_KEYWORD SEMICOLON
 * | CONTINUE_KEYWORD SEMICOLON
 * | RETURN_KEYWORD SEMICOLON
 * | RETURN_KEYWORD Exp SEMICOLON
 * ;
 */

Stmt
: LVal ASSIGN_OP Exp SEMICOLON                                                      { SSYC_PRINT_REDUCE(Stmt, "LVal ASSIGN_OP Exp SEMICOLON"); }
| Block                                                                             { SSYC_PRINT_REDUCE(Stmt, "Block"); }
| IF_KEYWORD LPAREN Cond RPAREN Stmt                                                { SSYC_PRINT_REDUCE(Stmt, "IF_KEYWORD LPAREN Cond RPAREN Stmt"); }
| IF_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt ELSE_KEYWORD Stmt                { SSYC_PRINT_REDUCE(Stmt, "IF_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt ELSE_KEYWORD Stmt"); }
| WHILE_KEYWORD LPAREN Cond RPAREN Stmt                                             { SSYC_PRINT_REDUCE(Stmt, "WHILE_KEYWORD LPAREN Cond RPAREN Stmt"); }
| BREAK_KEYWORD SEMICOLON                                                           { SSYC_PRINT_REDUCE(Stmt, "BREAK_KEYWORD SEMICOLON"); }
| CONTINUE_KEYWORD SEMICOLON                                                        { SSYC_PRINT_REDUCE(Stmt, "CONTINUE_KEYWORD SEMICOLON"); }
| RETURN_KEYWORD SEMICOLON                                                          { SSYC_PRINT_REDUCE(Stmt, "RETURN_KEYWORD SEMICOLON"); }
| RETURN_KEYWORD Exp SEMICOLON                                                      { SSYC_PRINT_REDUCE(Stmt, "RETURN_KEYWORD Exp SEMICOLON"); }
;

StmtBeforeElseStmt
: LVal ASSIGN_OP Exp SEMICOLON                                                      { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "LVal ASSIGN_OP Exp SEMICOLON"); }
| Block                                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Block"); }
| IF_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt ELSE_KEYWORD StmtBeforeElseStmt  { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "IF_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt ELSE_KEYWORD StmtBeforeElseStmt"); }
| WHILE_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt                               { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "WHILE_KEYWORD LPAREN Cond RPAREN StmtBeforeElseStmt"); }
| BREAK_KEYWORD SEMICOLON                                                           { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "BREAK_KEYWORD SEMICOLON"); }
| CONTINUE_KEYWORD SEMICOLON                                                        { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "CONTINUE_KEYWORD SEMICOLON"); }
| RETURN_KEYWORD SEMICOLON                                                          { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "RETURN_KEYWORD SEMICOLON"); }
| RETURN_KEYWORD Exp SEMICOLON                                                      { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "RETURN_KEYWORD Exp SEMICOLON"); }
;

//! #-- Stmt END ---

//! 表达式
Exp
: AddExp                                                                            { SSYC_PRINT_REDUCE(Exp, "AddExp"); }
;

//! 条件表达式
Cond
: LOrExp                                                                            { SSYC_PRINT_REDUCE(Cond, "LOrExp"); }
;

//! 左值表达式
LVal
: IDENT                                                                             { SSYC_PRINT_REDUCE(LVal, "IDENT"); }
| IDENT SubscriptChain                                                              { SSYC_PRINT_REDUCE(LVal, "IDENT SubscriptChain"); }
;

//! 基本表达式
PrimaryExp
: LPAREN Exp RPAREN                                                                 { SSYC_PRINT_REDUCE(PrimaryExp, "LPAREN Exp RPAREN"); }
| LVal                                                                              { SSYC_PRINT_REDUCE(PrimaryExp, "LVal"); }
| Number                                                                            { SSYC_PRINT_REDUCE(PrimaryExp, "Number"); }
;

//! 数值
Number
: INT_LITERAL                                                                       { SSYC_PRINT_REDUCE(Number, "INT_LITERAL"); }
| FLOAT_LITERAL                                                                     { SSYC_PRINT_REDUCE(Number, "FLOAT_LITERAL"); }
;

//! 一元表达式
UnaryExp
: PrimaryExp                                                                        { SSYC_PRINT_REDUCE(UnaryExp, "PrimaryExp"); }
| IDENT LPAREN RPAREN                                                               { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN RPAREN"); }
| IDENT LPAREN FuncRParams RPAREN                                                   { SSYC_PRINT_REDUCE(UnaryExp, "IDENT LPAREN FuncRParams RPAREN"); }
| UnaryOp UnaryExp                                                                  { SSYC_PRINT_REDUCE(UnaryExp, "UnaryOp UnaryExp"); }
;

//! 单目运算符
UnaryOp
: ADD_OP                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "ADD_OP"); }
| SUB_OP                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "SUB_OP"); }
| LNOT_OP                                                                           { SSYC_PRINT_REDUCE(UnaryOp, "LNOT_OP"); }
;

//! 函数实参表
FuncRParams
: Exp                                                                               { SSYC_PRINT_REDUCE(FuncRParams, "Exp"); }
| FuncRParams COMMA Exp                                                             { SSYC_PRINT_REDUCE(FuncRParams, "FuncRParams COMMA Exp"); }
;

//! 乘除模表达式
MulExp
: UnaryExp                                                                          { SSYC_PRINT_REDUCE(MulExp, "UnaryExp"); }
| MulExp MUL_OP UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp MUL_OP UnaryExp"); }
| MulExp DIV_OP UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp DIV_OP UnaryExp"); }
| MulExp MOD_OP UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp MOD_OP UnaryExp"); }
;

//! 加减表达式
AddExp
: MulExp                                                                            { SSYC_PRINT_REDUCE(AddExp, "MulExp"); }
| AddExp ADD_OP MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp ADD_OP MulExp"); }
| AddExp SUB_OP MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp SUB_OP MulExp"); }
;

//! 关系表达式
RelExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(RelExp, "AddExp"); }
| RelExp LT_OP AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp LT_OP AddExp"); }
| RelExp GT_OP AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp GT_OP AddExp"); }
| RelExp LE_OP AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp LE_OP AddExp"); }
| RelExp GE_OP AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp GE_OP AddExp"); }
;

//! 相等性表达式
EqExp
: RelExp                                                                            { SSYC_PRINT_REDUCE(EqExp, "RelExp"); }
| EqExp EQ_OP RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp EQ_OP RelExp"); }
| EqExp NE_OP RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp NE_OP RelExp"); }
;

//! 逻辑与表达式
LAndExp
: EqExp                                                                             { SSYC_PRINT_REDUCE(LAndExp, "EqExp"); }
| LAndExp LAND_OP EqExp                                                             { SSYC_PRINT_REDUCE(LAndExp, "LAndExp LAND_OP EqExp"); }
;

//! 逻辑或表达式
LOrExp
: LAndExp                                                                           { SSYC_PRINT_REDUCE(LOrExp, "LAndExp"); }
| LOrExp LOR_OP LAndExp                                                             { SSYC_PRINT_REDUCE(LOrExp, "LOrExp LOR_OP LAndExp"); }
;

//! 常量表达式
ConstExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(ConstExp, "AddExp"); }
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

int main(int argc, char* argv[]) {
    FLAGS_colorlogtostderr = true;
	FLAGS_logtostderr	   = true;
	google::SetStderrLogging(google::GLOG_INFO);
	google::InitGoogleLogging(argv[0]);

    yyparse();
    
    return 0;
}
