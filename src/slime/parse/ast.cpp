#include "ast.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace slime {

ASTNode *mkastnode(
    int op, ASTNode *left, ASTNode *mid, ASTNode *right, ASTVal32 val) {
    ASTNode *n;
    n = (ASTNode *)malloc(sizeof(ASTNode));
    if (!n) {
        fprintf(stderr, "Unable to malloc in mkastnode!\n");
        exit(-1);
    }
    n->op    = op;
    n->left  = left;
    n->mid   = mid;
    n->right = right;
    n->val   = val;

    return n;
}

ASTNode *mkastleaf(int op, ASTVal32 val) {
    return mkastnode(op, NULL, NULL, NULL, val);
}

ASTNode *mkastunary(int op, ASTNode *left, ASTVal32 val) {
    return mkastnode(op, left, NULL, NULL, val);
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

const char *ast2str(int asttype) {
    switch (asttype) {
        case (A_ADD):
            return "<A_ADD>";
        case (A_SUBTRACT):
            return "<A_SUBTRACT>";
        case (A_MULTIPLY):
            return "<A_MULTIPLY>";
        case (A_DIVIDE):
            return "<A_DIVIDE>";
        case (A_MOD):
            return "<A_MOD>";
        case (A_LOWTO):
            return "<A_LOWTO>";
        case (A_GREATTO):
            return "<A_GREATTO>";
        case (A_OR):
            return "<A_OR>";
        case (A_AND):
            return "<A_AND>";
        case (A_XOR):
            return "<A_XOR>";
        case (A_PLUS):
            return "<A_PLUS>";
        case (A_MINUS):
            return "<A_MINUS>";
        case (A_INV):
            return "<A_INV>";
        case (A_NOT):
            return "<A_NOT>";
        case (A_STMT):
            return "<A_STMT>";
        case (A_ASSIGN):
            return "<A_ASSIGN>";
        case (A_IDENT):
            return "<A_IDENT>";
        case (A_FUNCTION):
            return "<A_FUNCTION>";
        case (A_RETURN):
            return "<A_RETURN>";
        case (A_INTLIT):
            return "<A_INTLIT>";
        case (A_FLTLIT):
            return "A_FLTLIT";
        default:
            fprintf(stderr, "Unknown AST node type:%d!\n");
            return NULL;
    }
}

void inorder(ASTNode *n) {
    if (!n) return;
    if (n->left) inorder(n->left);
    printf("%s", ast2str(n->op));
    if (n->op == A_INTLIT)
        printf(": %d", n->val.intvalue);
    else if (n->op == A_FLTLIT)
        printf(": %f", n->val.fltvalue);
    else if (n->op == A_IDENT || n->op == A_FUNCTION)
        printf(" index: %d", n->val.symindex);
    printf("\n");
    if (n->mid) inorder(n->mid);
    if (n->right) inorder(n->right);
}

} // namespace slime
