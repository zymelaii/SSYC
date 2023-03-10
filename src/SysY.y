%{
#include "utils.h"

void yyerror(char* message);
extern "C" int yywrap();
int yylex();
%}

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
: KW_CONST BType ConstDefList SEMICOLON                                             { SSYC_PRINT_REDUCE(ConstDecl, "KW_CONST BType SEMICOLON"); }
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
: T_VOID                                                                            { SSYC_PRINT_REDUCE(BType, "T_VOID"); }
| T_INT                                                                             { SSYC_PRINT_REDUCE(BType, "T_INT"); }
| T_FLOAT                                                                           { SSYC_PRINT_REDUCE(BType, "T_FLOAT"); }
;

//! #-- BType END ---

//! 常数定义
//! NOTE: SubscriptChainConst := { LBRACKET ConstExp RBRACKET }
ConstDef
: IDENT OP_ASS ConstInitVal                                                         { SSYC_PRINT_REDUCE(ConstDef, "IDENT OP_ASS ConstInitVal"); }
| IDENT SubscriptChainConst OP_ASS ConstInitVal                                     { SSYC_PRINT_REDUCE(ConstDef, "IDENT SubscriptChainConst OP_ASS ConstInitVal"); }
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
: IDENT                                                                             { SSYC_PRINT_REDUCE(VarDef, "IDENT"); }
| IDENT OP_ASS InitVal                                                              { SSYC_PRINT_REDUCE(VarDef, "IDENT OP_ASS InitVal"); }
| IDENT SubscriptChainConst                                                         { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst"); }
| IDENT SubscriptChainConst OP_ASS InitVal                                          { SSYC_PRINT_REDUCE(VarDef, "IDENT SubscriptChainConst OP_ASS InitVal"); }
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
: BType IDENT                                                                       { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT"); }
| BType IDENT SubscriptChainVariant                                                 { SSYC_PRINT_REDUCE(FuncFParam, "BType IDENT SubscriptChainVariant"); }
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
 * : LVal OP_ASS Exp SEMICOLON
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
: LVal OP_ASS Exp SEMICOLON                                                         { SSYC_PRINT_REDUCE(Stmt, "LVal OP_ASS Exp SEMICOLON"); }
| Block                                                                             { SSYC_PRINT_REDUCE(Stmt, "Block"); }
| KW_IF LPAREN Cond RPAREN Stmt                                                     { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN Stmt"); }
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt                          { SSYC_PRINT_REDUCE(Stmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE Stmt"); }
| KW_WHILE LPAREN Cond RPAREN Stmt                                                  { SSYC_PRINT_REDUCE(Stmt, "KW_WHILE LPAREN Cond RPAREN Stmt"); }
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(Stmt, "KW_BREAK SEMICOLON"); }
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(Stmt, "KW_CONTINUE SEMICOLON"); }
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN SEMICOLON"); }
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(Stmt, "KW_RETURN Exp SEMICOLON"); }
;

StmtBeforeElseStmt
: LVal OP_ASS Exp SEMICOLON                                                         { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "LVal OP_ASS Exp SEMICOLON"); }
| Block                                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "Block"); }
| KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt            { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_IF LPAREN Cond RPAREN StmtBeforeElseStmt KW_ELSE StmtBeforeElseStmt"); }
| KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt                                    { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_WHILE LPAREN Cond RPAREN StmtBeforeElseStmt"); }
| KW_BREAK SEMICOLON                                                                { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_BREAK SEMICOLON"); }
| KW_CONTINUE SEMICOLON                                                             { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_CONTINUE SEMICOLON"); }
| KW_RETURN SEMICOLON                                                               { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN SEMICOLON"); }
| KW_RETURN Exp SEMICOLON                                                           { SSYC_PRINT_REDUCE(StmtBeforeElseStmt, "KW_RETURN Exp SEMICOLON"); }
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
//! NOTE: SubscriptChain := { LBRACKET Exp RBRACKET }
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
: CONST_INT                                                                         { SSYC_PRINT_REDUCE(Number, "CONST_INT"); }
| CONST_FLOAT                                                                       { SSYC_PRINT_REDUCE(Number, "CONST_FLOAT"); }
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
: OP_ADD                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_ADD"); }
| OP_SUB                                                                            { SSYC_PRINT_REDUCE(UnaryOp, "OP_SUB"); }
| OP_LNOT                                                                           { SSYC_PRINT_REDUCE(UnaryOp, "OP_LNOT"); }
;

//! 函数实参表
FuncRParams
: Exp                                                                               { SSYC_PRINT_REDUCE(FuncRParams, "Exp"); }
| FuncRParams COMMA Exp                                                             { SSYC_PRINT_REDUCE(FuncRParams, "FuncRParams COMMA Exp"); }
;

//! 乘除模表达式
MulExp
: UnaryExp                                                                          { SSYC_PRINT_REDUCE(MulExp, "UnaryExp"); }
| MulExp OP_MUL UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MUL UnaryExp"); }
| MulExp OP_DIV UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_DIV UnaryExp"); }
| MulExp OP_MOD UnaryExp                                                            { SSYC_PRINT_REDUCE(MulExp, "MulExp OP_MOD UnaryExp"); }
;

//! 加减表达式
AddExp
: MulExp                                                                            { SSYC_PRINT_REDUCE(AddExp, "MulExp"); }
| AddExp OP_ADD MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_ADD MulExp"); }
| AddExp OP_SUB MulExp                                                              { SSYC_PRINT_REDUCE(AddExp, "AddExp OP_SUB MulExp"); }
;

//! 关系表达式
RelExp
: AddExp                                                                            { SSYC_PRINT_REDUCE(RelExp, "AddExp"); }
| RelExp OP_LT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LT AddExp"); }
| RelExp OP_GT AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GT AddExp"); }
| RelExp OP_LE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_LE AddExp"); }
| RelExp OP_GE AddExp                                                               { SSYC_PRINT_REDUCE(RelExp, "RelExp OP_GE AddExp"); }
;

//! 相等性表达式
EqExp
: RelExp                                                                            { SSYC_PRINT_REDUCE(EqExp, "RelExp"); }
| EqExp OP_EQ RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_EQ RelExp"); }
| EqExp OP_NE RelExp                                                                { SSYC_PRINT_REDUCE(EqExp, "EqExp OP_NE RelExp"); }
;

//! 逻辑与表达式
LAndExp
: EqExp                                                                             { SSYC_PRINT_REDUCE(LAndExp, "EqExp"); }
| LAndExp OP_LAND EqExp                                                             { SSYC_PRINT_REDUCE(LAndExp, "LAndExp OP_LAND EqExp"); }
;

//! 逻辑或表达式
LOrExp
: LAndExp                                                                           { SSYC_PRINT_REDUCE(LOrExp, "LAndExp"); }
| LOrExp OP_LOR LAndExp                                                             { SSYC_PRINT_REDUCE(LOrExp, "LOrExp OP_LOR LAndExp"); }
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

int main(int argc, char* argv[]) {
    FLAGS_colorlogtostderr = true;
	FLAGS_logtostderr	   = true;
	google::SetStderrLogging(google::GLOG_INFO);
	google::InitGoogleLogging(argv[0]);

    yyparse();
    
    return 0;
}
