#pragma once

#include <stdint.h>

namespace slime::lang {

struct TokenKind {
    static constexpr inline int32_t FIRST_RESERVED = 128;

    static constexpr inline int32_t TK_EOF       = -1;
    static constexpr inline int32_t TK_NUL       = '\0';
    static constexpr inline int32_t TK_WS        = ' ';
    static constexpr inline int32_t TK_TAB       = '\t';
    static constexpr inline int32_t TK_CR        = '\r';
    static constexpr inline int32_t TK_NL        = '\n';
    static constexpr inline int32_t TK_ASSIGN    = '=';
    static constexpr inline int32_t TK_ADD       = '+';
    static constexpr inline int32_t TK_SUB       = '-';
    static constexpr inline int32_t TK_STAR      = '*';
    static constexpr inline int32_t TK_DIV       = '/';
    static constexpr inline int32_t TK_MOD       = '%';
    static constexpr inline int32_t TK_LT        = '<';
    static constexpr inline int32_t TK_GT        = '>';
    static constexpr inline int32_t TK_OR        = '|';
    static constexpr inline int32_t TK_AND       = '&';
    static constexpr inline int32_t TK_XOR       = '^';
    static constexpr inline int32_t TK_INV       = '~';
    static constexpr inline int32_t TK_NOT       = '!';
    static constexpr inline int32_t TK_LPAREN    = '(';
    static constexpr inline int32_t TK_RPAREN    = ')';
    static constexpr inline int32_t TK_LBRACKET  = '[';
    static constexpr inline int32_t TK_RBRACKET  = ']';
    static constexpr inline int32_t TK_LBRACE    = '{';
    static constexpr inline int32_t TK_RBRACE    = '}';
    static constexpr inline int32_t TK_COMMA     = ',';
    static constexpr inline int32_t TK_QUESTION  = '?';
    static constexpr inline int32_t TK_COLON     = ':';
    static constexpr inline int32_t TK_SEMICOLON = ';';
    static constexpr inline int32_t TK_DOLLAR    = '$';

    enum {
        TK_VOID = FIRST_RESERVED, //<! `void`
        TK_BOOL,                  //<! `bool`
        TK_CHAR,                  //<! `char`
        TK_CHAR8,                 //<! `char8`
        TK_CHAR16,                //<! `char16`
        TK_CHAR32,                //<! `char32`
        TK_BYTE,                  //<! `byte`
        TK_SHORT,                 //<! `short`
        TK_INT,                   //<! `int`
        TK_LONG,                  //<! `long`
        TK_FLOAT,                 //<! `float`
        TK_DOUBLE,                //<! `double`
        TK_SIGNED,                //<! `signed`
        TK_UNSIGNED,              //<! `unsigned`
        TK_CONST,                 //<! `const`
        TK_STATIC,                //<! `static`
        TK_INLINE,                //<! `inline`
        TK_EXTERN,                //<! `extern`
        TK_CONSTEXPR,             //<! `constexpr`
        TK_IF,                    //<! `if`
        TK_ELSE,                  //<! `else`
        TK_SWITCH,                //<! `switch`
        TK_CASE,                  //<! `case`
        TK_DEFAULT,               //<! `default`
        TK_WHILE,                 //<! `while`
        TK_DO,                    //<! `do`
        TK_LOOP,                  //<! `loop`
        TK_FOR,                   //<! `for`
        TK_CONTINUE,              //<! `continue`
        TK_BREAK,                 //<! `break`
        TK_RETURN,                //<! `return`
    };

    static constexpr inline int32_t LAST_RESERVED  = TK_RETURN;
    static constexpr inline int32_t FIRST_COMPOUND = LAST_RESERVED + 1;

    enum {
        TK_EQ = FIRST_COMPOUND, //<! `==`
        TK_NE,                  //<! `!=`
        TK_LE,                  //<! `<=`
        TK_GE,                  //<! `>=`
        TK_LOR,                 //<! `||`
        TK_LAND,                //<! `&&`
        TK_SHL,                 //<! `<<`
        TK_SHR,                 //<! `>>`
        TK_INCR,                //<! `++`
        TK_DECR,                //<! `--`
        TK_ADDASS,              //<! `+=`
        TK_SUBASS,              //<! `-=`
        TK_MULASS,              //<! `*=`
        TK_DIVASS,              //<! `/=`
        TK_MODASS,              //<! `%=`
        TK_ORASS,               //<! `|=`
        TK_ANDASS,              //<! `&=`
        TK_XORASS,              //<! `^=`
        TK_SHLASS,              //<! `<<=`
        TK_SHRASS,              //<! `>>=`
    };

    static constexpr inline int32_t LAST_COMPOUND = TK_SHRASS;
    static constexpr inline int32_t FIRST_DYNAMIC = LAST_COMPOUND + 1;

    enum {
        TK_IDENT = FIRST_DYNAMIC, //<! identifier
        TK_ATTRIDENT,             //<! attribute identifier
        TK_CTXCONST,              //<! builtin contextual compile-time value
        TK_BOOLVAL,               //<! boolean literal
        TK_CHARVAL,               //<! character literal
        TK_INTVAL,                //<! integer literal
        TK_FLTVAL,                //<! float literal
        TK_STRVAL,                //<! string literal
        TK_RAWSTR,                //<! raw string literal
        TK_FMTSTR,                //<! formattable string literal
        TK_COMMENT,               //<! comment
    };

    static constexpr inline int32_t LAST_DYNAMIC = TK_COMMENT;
};

} // namespace slime::lang
