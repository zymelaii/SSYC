#include "ast.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace slime {

ASTNode *mkastnode(int op, ASTNode *left, ASTNode *right, ASTVal32 val) {
    ASTNode *n;
    n = (ASTNode *)malloc(sizeof(ASTNode));
    if (!n) {
        fprintf(stderr, "Unable to malloc in mkastnode!\n");
        exit(-1);
    }
    n->op    = op;
    n->left  = left;
    n->right = right;
    n->val   = val;

    return n;
}

ASTNode *mkastleaf(int op, ASTVal32 val) {
    return mkastnode(op, NULL, NULL, val);
}

int tok2ast(TOKEN tok) {
    switch (tok) {
        case TOKEN::TK_ADD:
            return A_ADD;
        case TOKEN::TK_SUB:
            return A_SUBTRACT;
        case TOKEN::TK_MUL:
            return A_MULTIPLY;
        case TOKEN::TK_DIV:
            return A_DIVIDE;
        case TOKEN::TK_MOD:
            return A_MOD;
        case TOKEN::TK_LT:
            return A_LOWTO;
        case TOKEN::TK_GT:
            return A_GREATTO;
        case TOKEN::TK_OR:
            return A_OR;
        case TOKEN::TK_AND:
            return A_AND;
        case TOKEN::TK_XOR:
            return A_AND;
        case TOKEN::TK_INV:
            return A_INV;
        case TOKEN::TK_NOT:
            return A_NOT;
        default:
            printf("Unsupported yet token in tok2ast: %d\n", tok);
            exit(-1);
    }
}

} // namespace slime
