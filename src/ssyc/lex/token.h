#pragma once

#include "../ast/ast.h"

#include <stdint.h>

namespace ssyc {

enum class TokenType : uint32_t {
    //! ident
    IDENT = 258,

    //! literal
    INT_LITERAL   = 259,
    FLOAT_LITERAL = 260,

    //! keyword
    VOID     = 261,
    INT      = 262,
    FLOAT    = 263,
    CONST    = 264,
    IF       = 265,
    ELSE     = 266,
    WHILE    = 267,
    BREAK    = 268,
    CONTINUE = 269,
    RETURN   = 270,

    //! operator
    ASS_OP   = 271,
    POS_OP   = 272,
    NEG_OP   = 273,
    ADD_OP   = 274,
    SUB_OP   = 275,
    MUL_OP   = 276,
    DIV_OP   = 277,
    MOD_OP   = 278,
    LT_OP    = 279,
    GT_OP    = 280,
    LE_OP    = 281,
    GE_OP    = 282,
    EQ_OP    = 283,
    NE_OP    = 284,
    LNOT_OP  = 285,
    LAND_OP  = 286,
    LOR_OP   = 287,
    COMMA_OP = 288,

    //! misc
    LPAREN    = 289,
    RPAREN    = 290,
    LBRACKET  = 291,
    RBRACKET  = 292,
    LBRACE    = 293,
    RBRACE    = 294,
    COMMA     = 295,
    SEMICOLON = 296,
};

}; // namespace ssyc
