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

    //! 添加一个全局变量符号到gsym，返回下标
    int  add_globalsym(LexState& ls, int type, int stype);
    int  find_globalsym(const char *name);
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
