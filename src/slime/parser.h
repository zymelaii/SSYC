#pragma once

#include "lex.h"

struct ParseState {};

struct Expr {};

class Parser {
public:
    LexState   ls;
    ParseState ps;

    void next();
    bool expect(TOKEN token, const char *msg = nullptr);

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

    void primaryexpr();
    void postfixexpr();
    void unaryexpr();
    void mulexpr();
    void addexpr();
    void shiftexpr();
    void relexpr();
    void eqexpr();
    void andexpr();
    void xorexpr();
    void orexpr();
    void landexpr();
    void lorexpr();
    void condexpr();
    void assignexpr();
    void expr();

    void exprlist();
};
