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
    A_STMT,     // 连接多个语句
    A_ASSIGN,   // 15
    A_IDENT,    // 16
    A_FUNCTION, // 17
    A_RETURN,   // 18
    A_INTLIT,   // 19
    A_FLTLIT    // 20
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
    ASTVal32        val;
};

struct ASTNode *mkastnode(
    int op, ASTNode *left, ASTNode *mid, ASTNode *right, ASTVal32 val);
struct ASTNode *mkastleaf(int op, ASTVal32 val);
struct ASTNode *mkastunary(int op, ASTNode *left, ASTVal32 val);

int         tok2ast(TOKEN tok); //<! TOKEN类型转换为AST结点类型
const char *ast2str(int asttype);
void        inorder(ASTNode *n);

} // namespace slime
