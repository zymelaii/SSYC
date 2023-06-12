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
    A_ASSIGN,
    A_INTLIT,
    A_FLTLIT
};

union ASTVal32 {
    int   intvalue;
    float fltvalue;
    int   symindex; // 标识符在符号表中的下标
};

struct ASTNode {
    int             op;
    struct ASTNode *left;
    struct ASTNode *right;
    ASTVal32        val;
};

struct ASTNode *mkastnode(int op, ASTNode *left, ASTNode *right, ASTVal32 val);
struct ASTNode *mkastleaf(int op, ASTVal32 val);

int tok2ast(TOKEN tok); //<! TOKEN类型转换为AST结点类型

} // namespace slime
