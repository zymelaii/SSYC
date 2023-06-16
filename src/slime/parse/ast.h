#pragma once

#include "../lex/token.h"

namespace slime {

enum ASTNodeType {
    A_ADD,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,
    A_MOD,
    A_LOWTO,
    A_GREATTO,
    A_OR,
    A_AND,
    A_XOR,
    A_PLUS,
    A_MINUS,
    A_INV,
    A_NOT,
    A_EXPR,
    A_STMT,     // 连接多个语句
    A_ASSIGN,   // 16
    A_BLOCK,    // 17
    A_IDENT,    // 18
    A_FUNCTION, // 19
    A_RETURN,   // 20
    A_FUNCCALL, // 21
    A_INTLIT,   // 22
    A_FLTLIT    // 23
};

struct ASTNode;
struct symtable;
struct paramtable;

struct syminfo {
    char* name;
    int   type;    // 类型(void/int/float)
    int   stype;   // var:0 function:1
    union {
        paramtable *funcparams;
        ASTNode  *initlist;
    }content;
};

struct paraminfo : syminfo{
    int lsym_index;
};

struct symtable {
    syminfo *symbols;
    int     sym_num;
};

struct paramtable {
    paraminfo *params;
    int param_num;
};

struct blockinfo{
    symtable l_sym;
    blockinfo *prev_head;// pointer to the block embedding the currentblock
    blockinfo *head;    // pointer to the block embedded in it
    blockinfo *next;    // pointer to the next block in current block
    int block_num;      
};

union ASTVal32 {
    int   intvalue;
    float fltvalue;
    int   symindex; // 标识符在符号表中的下标
};

struct ASTNode {
    int             op;
    struct ASTNode *left;
    struct ASTNode *mid;
    struct ASTNode *right;
    blockinfo      *block;
    ASTVal32        val;
};

struct ASTNode *mkastnode(
    int op, ASTNode *left, ASTNode *mid, ASTNode *right, ASTVal32 val);
struct ASTNode *mkastleaf(int op, ASTVal32 val);
struct ASTNode *mkastunary(int op, ASTNode *left, ASTVal32 val);

int         tok2ast(TOKEN tok); //<! TOKEN类型转换为AST结点类型
const char *ast2str(int asttype);

} // namespace slime
