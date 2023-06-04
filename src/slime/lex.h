#pragma once

#include <stdint.h>
#include <memory>
#include <limits>
#include <string_view>
#include <istream>

static constexpr int FIRST_RESERVED = std::numeric_limits<char>::max();

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
    TK_VOID = FIRST_RESERVED, //<! `void`
    TK_INT,                   //<! `int`
    TK_FLOAT,                 //<! `float`
    TK_CONST,                 //<! `const`
    TK_IF,                    //<! `if`
    TK_ELSE,                  //<! `else`
    TK_FOR,                   //<! `for`
    TK_DO,                    //<! `do`
    TK_WHILE,                 //<! `while`
    TK_SWITCH,                //<! `switch`
    TK_CASE,                  //<! `case`
    TK_DEFAULT,               //<! `default`
    TK_BERAK,                 //<! `break`
    TK_CONTINUE,              //<! `continue`
    TK_RETURN,                //<! `return`
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

static constexpr int LAST_RESERVED = static_cast<int>(TOKEN::LAST_RESERVED);

inline size_t totalReserved() {
    const auto LAST_RESERVED = static_cast<int>(TOKEN::LAST_RESERVED);
    return LAST_RESERVED - FIRST_RESERVED + 1;
}

struct Token {
    TOKEN            id;     //<! token ID
    std::string_view detail; //<! raw content of token
};

struct LexStatePrivate;

class LexState {
public:
    int32_t cur;        //<! current character (consider utf-8 code)
    size_t  line;       //<! number of current line
    size_t  lastline;   //<! line of last consumed token
    size_t  column;     //<! number of curreng column
    size_t  lastcolumn; //<! column of last consumed token
    Token   token;      //<! current token
    Token   nexttoken;  //<! lookahead token

    std::unique_ptr<LexStatePrivate> d; //<! private data

    LexState();
    ~LexState();

    LexState(const LexState &)            = delete;
    LexState &operator=(const LexState &) = delete;

    //! reset lex state and set input stream
    template <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<std::istream, T>>>
    void reset(T &input) {
        resetstream(new T(std::move(input)));
        cur          = '\0';
        line         = 1;
        column       = 0;
        lastline     = line;
        token.id     = TOKEN::TK_NONE;
        nexttoken.id = TOKEN::TK_NONE;
    }

    //! move to next token
    void next();

    //! lookahead one token
    //! FIXME: lookahead() changes column as well
    TOKEN lookahead();

protected:
    void resetstream(std::istream *input);
};

const char *tok2str(TOKEN token, char *buffer, size_t len);
const char *pretty_tok2str(Token token, char *buffer, size_t len);

template <size_t N>
inline const char *tok2str(TOKEN token, char (&buffer)[N]) {
    return tok2str(token, buffer, N);
}

template <size_t N>
inline const char *pretty_tok2str(Token token, char (&buffer)[N]) {
    return pretty_tok2str(token, buffer, N);
}
