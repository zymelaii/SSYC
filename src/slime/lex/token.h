#pragma once

#include <limits>
#include <string_view>

namespace slime {

enum class TOKEN;

struct Token {
    TOKEN            id;     //<! token ID
    std::string_view detail; //<! raw content of token
};

namespace detail {
static constexpr int FIRST_RESERVED = std::numeric_limits<char>::max();
} // namespace detail

enum class TOKEN : int {
    TK_NONE = 0, //<! reserved token for lex
    //! NOTE: single-char tokens are represented by their char codes
    TK_ASS       = '=',
    TK_ADD       = '+',
    TK_SUB       = '-',
    TK_MUL       = '*',
    TK_DIV       = '/',
    TK_MOD       = '%',
    TK_LT        = '<',
    TK_GT        = '>',
    TK_OR        = '|',
    TK_AND       = '&',
    TK_XOR       = '^',
    TK_INV       = '~',
    TK_NOT       = '!',
    TK_LPAREN    = '(',
    TK_RPAREN    = ')',
    TK_LBRACKET  = '[',
    TK_RBRACKET  = ']',
    TK_LBRACE    = '{',
    TK_RBRACE    = '}',
    TK_COMMA     = ',',
    TK_SEMICOLON = ';',
    //! reserved keywords
    TK_VOID = detail::FIRST_RESERVED, //<! `void`
    TK_INT,                           //<! `int`
    TK_FLOAT,                         //<! `float`
    TK_CONST,                         //<! `const`
    TK_IF,                            //<! `if`
    TK_ELSE,                          //<! `else`
    TK_FOR,                           //<! `for`
    TK_DO,                            //<! `do`
    TK_WHILE,                         //<! `while`
    TK_SWITCH,                        //<! `switch`
    TK_CASE,                          //<! `case`
    TK_DEFAULT,                       //<! `default`
    TK_BREAK,                         //<! `break`
    TK_CONTINUE,                      //<! `continue`
    TK_RETURN,                        //<! `return`
    LAST_RESERVED = TK_RETURN,
    //! other terminal tokens
    TK_EQ,        //<! `==`
    TK_NE,        //<! `!=`
    TK_LE,        //<! `<=`
    TK_GE,        //<! `>=`
    TK_LOR,       //<! `||`
    TK_LAND,      //<! `&&`
    TK_SHL,       //<! `<<`
    TK_SHR,       //<! `>>`
    TK_IDENT,     //<! identifier
    TK_INTVAL,    //<! integer constant
    TK_FLTVAL,    //<! floating constant
    TK_STRING,    //<! string constant
    TK_COMMENT,   //<! single comment
    TK_MLCOMMENT, //<! multi-line comment
    TK_EOF,       //<! end of input stream
};

namespace detail {
static constexpr int LAST_RESERVED = static_cast<int>(TOKEN::LAST_RESERVED);
} // namespace detail

} // namespace slime
