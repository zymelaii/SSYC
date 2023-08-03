#pragma once

#include <limits>
#include <string_view>
#include <stdint.h>

namespace slime {

enum class TOKEN;

struct Token {
    TOKEN            id;     //<! token ID
    std::string_view detail; //<! raw content of token

    bool operator==(TOKEN token) const {
        return id == token;
    }

    bool operator!=(TOKEN token) const {
        return id != token;
    }

    operator TOKEN() const {
        return id;
    }

    operator std::string_view() const {
        return detail;
    }

    operator const char*() const {
        return detail.data();
    }

    inline bool isNone() const;
    inline bool isIdent() const;
    inline bool isComma() const;
    inline bool isSemicolon() const;
    inline bool isEOF() const;
    inline bool isComment() const;
    inline bool isSingleCharToken() const;
    inline bool isReserved() const;
    inline bool isLiteral() const;
    inline bool isBracket() const;
    inline bool isLeftBracket() const;
    inline bool isRightBracket() const;
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
    TK_STATIC,                        //<! `static`
    TK_INLINE,                        //<! `inline`
    TK_EXTERN,                        //<! `extern`
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

inline bool Token::isNone() const {
    return id == TOKEN::TK_NONE;
}

inline bool Token::isIdent() const {
    return id == TOKEN::TK_IDENT;
}

inline bool Token::isComma() const {
    return id == TOKEN::TK_COMMA;
}

inline bool Token::isSemicolon() const {
    return id == TOKEN::TK_SEMICOLON;
}

inline bool Token::isEOF() const {
    return id == TOKEN::TK_EOF;
}

inline bool Token::isComment() const {
    return id == TOKEN::TK_COMMENT || id == TOKEN::TK_MLCOMMENT;
}

inline bool Token::isSingleCharToken() const {
    auto v = static_cast<std::underlying_type_t<TOKEN>>(id);
    return v > 0 && v < 128;
}

inline bool Token::isReserved() const {
    auto v = static_cast<std::underlying_type_t<TOKEN>>(id);
    return v >= detail::FIRST_RESERVED && v <= detail::LAST_RESERVED;
}

inline bool Token::isLiteral() const {
    return id == TOKEN::TK_INTVAL || id == TOKEN::TK_FLTVAL
        || id == TOKEN::TK_STRING;
}

inline bool Token::isBracket() const {
    return isLeftBracket() || isRightBracket();
}

inline bool Token::isLeftBracket() const {
    return id == TOKEN::TK_LPAREN || id == TOKEN::TK_LBRACKET
        || id == TOKEN::TK_LBRACE;
}

inline bool Token::isRightBracket() const {
    return id == TOKEN::TK_RPAREN || id == TOKEN::TK_RBRACKET
        || id == TOKEN::TK_RBRACE;
}

} // namespace slime
