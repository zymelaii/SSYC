#pragma once

#include <queue>

#include "../lex/lex.h"
#include "ast.h"

namespace slime {

namespace detail {
static constexpr size_t MAX_SYMTABLE_LENGTH = 512;
} // namespace detail


struct node_type{
    node_type *next;
    node_type *prev;
    ASTNode *node;
};

struct ParseState {
    int cur_func;       //<! index in gsym of current parsing function(-1 if not in a function)
};

enum{
    S_VARIABLE, S_FUNCTION
};

enum{
    TYPE_VOID, TYPE_INT, TYPE_FLOAT
};

struct syminfo {
    char* name;
    int   type;    // 类型(void/int/float)
    int   stype;   // var:0 function:1
    int   arrsize; // 数组大小
};

struct symtable {
    syminfo symbols[detail::MAX_SYMTABLE_LENGTH];
    int     sym_num;
};

class Parser {
public:
    LexState   ls;
    ParseState ps;

    void next();
    bool expect(TOKEN token, const char* msg = nullptr);

protected:
    void enterblock();       
    void leaveblock();
    void enterdecl();
    void leavedecl();
    int  enterfunc();   //返回函数名在全局符号表中的下标
    void leavefunc();

public:
    ASTNode* decl();
    ASTNode* vardef(int type);
    void initlist();
    ASTNode* func();
    void funcargs();
    ASTNode* statement();
    void ifstat();
    void whilestat();
    void breakstat();
    void continuestat();
    ASTNode* returnstat();
    ASTNode* block();

    int  add_globalsym(LexState& ls, int type, int stype);      //add a symbol to g_sym list, return its index
    int  find_globalsym(const char *name);                      //search a symbol in g_sym and return its index(-1 if failed)
    void add_localsym();

    struct ASTNode* primaryexpr();
    struct ASTNode* postfixexpr();
    struct ASTNode* unaryexpr();
    struct ASTNode* mulexpr();
    struct ASTNode* addexpr();
    struct ASTNode* shiftexpr();
    struct ASTNode* relexpr();
    struct ASTNode* eqexpr();
    struct ASTNode* andexpr();
    struct ASTNode* xorexpr();
    struct ASTNode* orexpr();
    struct ASTNode* landexpr();
    struct ASTNode* lorexpr();
    struct ASTNode* condexpr();
    struct ASTNode* assignexpr();
    struct ASTNode* expr();

    void exprlist();
    void traverseAST(ASTNode *root);

    //输出AST（后序遍历）
};

} // namespace slime
