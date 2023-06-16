#pragma once

#include <queue>

#include "../lex/lex.h"
#include "ast.h"

namespace slime {

namespace detail {
//!NOTE: unused now
static constexpr size_t MAX_SYMTABLE_LENGTH = 512;
} // namespace detail

struct node_type {
    node_type* next;
    node_type* prev;
    ASTNode*   node;
};

struct ParseState {
    int cur_func; //<! index in gsym of current parsing function(-1 if not in a
                  // function)
    paramtable *cur_funcparam;  // point to current function's parameter table
    blockinfo* cur_block;// pointer to current block
};

enum {
    S_VARIABLE,
    S_FUNCTION
};

enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_FLOAT
};

class Parser {
public:
    LexState   ls;
    ParseState ps{.cur_func = -1, .cur_funcparam = NULL,.cur_block = NULL};

    void next();
    bool expect(TOKEN token, const char* msg = nullptr);

protected:
    void enterblock();      //add a new blcok to b_info and return its index
    void leaveblock();
    void enterdecl();
    void leavedecl(int tag);
    int  enterfunc(int type);   //add func symbol to g_sym and return its index
    void leavefunc();

public:
    ASTNode* global_parse();

    ASTNode* decl();
    ASTNode* vardef(int type);
    void     initlist();
    ASTNode* func(int type);
    paramtable* funcargs();
    ASTNode* statement();
    void     ifstat();
    void     whilestat();
    void     breakstat();
    void     continuestat();
    ASTNode* returnstat();
    ASTNode* block();

    int add_globalsym(
        int       type,
        int       stype); // add a symbol to g_sym list, return its index
    int find_globalsym(const char* name); // search a symbol in g_sym and return
                                          // its index(-1 if failed)
    int find_localsym(const char *name, blockinfo **pblock); //search a local symbol in a block and return its index in l_sym.
                                                            // pblock will point to that block if it is not NULL
    int  add_localsym(int type, int stype);
    int  add_localsym(int type, int stype, char *name, blockinfo *block);


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

    ASTNode *exprlist(int funcindex);
    void traverseAST(ASTNode* root);
    void inorder(ASTNode *n);
    void displaySymInfo(int index, blockinfo *block);

    // 输出AST（后序遍历）
};

} // namespace slime
