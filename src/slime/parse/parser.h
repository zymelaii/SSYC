#pragma once

#include "../lex/lex.h"
#include "ast.h"

namespace slime {

namespace detail {
static constexpr size_t MAX_SYMTABLE_LENGTH = 512;
} // namespace detail

struct ParseState {};

struct syminfo {
    char* name;
    int   type;    // 类型(void/int)
    int   stype;   // var:0 function:1
    int   arrsize; // 数组大小
};

struct symtable {
    syminfo symbols[detail::MAX_SYMTABLE_LENGTH];
    int     sym_num;
} gsym{.sym_num = 0};

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
    void enterfunc();
    void leavefunc();

public:
    void decl();
    void vardef();
    void initlist();
    void func();
    void funcargs();
    void statement();
    void ifstat();
    void whilestat();
    void breakstat();
    void continuestat();
    void returnstat();
    void block();

    int add_globalsym(LexState& ls); // 添加一个全局变量符号到gsym，返回下标。
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
};

} // namespace slime
