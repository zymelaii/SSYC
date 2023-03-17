#include "utils.h"
#include <assert.h>

static constexpr std::array<const char*, 39> TOKENTABLE{
    "TT_IDENT",  "TT_COMMENT", "TT_CONST_INT", "TT_CONST_FLOAT", "TT_T_VOID",
    "TT_T_INT",  "TT_T_FLOAT", "TT_CONST",     "TT_IF",          "TT_ELSE",
    "TT_WHILE",  "TT_BREAK",   "TT_CONTINUE",  "TT_RETURN",      "TT_OP_ASS",
    "TT_OP_POS", "TT_OP_NEG",  "TT_OP_ADD",    "TT_OP_SUB",      "TT_OP_MUL",
    "TT_OP_DIV", "TT_OP_MOD",  "TT_OP_LT",     "TT_OP_GT",       "TT_OP_LE",
    "TT_OP_GE",  "TT_OP_EQ",   "TT_OP_NE",     "TT_OP_LNOT",     "TT_OP_LAND",
    "TT_OP_LOR", "TT_LPAREN",  "TT_RPAREN",    "TT_LBRACKET",    "TT_RBRACKET",
    "TT_LBRACE", "TT_RBRACE",  "TT_COMMA",     "TT_SEMICOLON",
};

static constexpr std::array<const char*, 9> SYNTAXTABLE{
    "Program",
    "Decl",
    "FuncDef",
    "Block",
    "Expr",
    "Assign",
    "IfElse",
    "While",
    "Return",
};

std::string_view tokenEnumToString(ssyc::TokenType token) {
    const auto index = static_cast<size_t>(token);
    assert(index >= 0 && index < TOKENTABLE.size());
    return TOKENTABLE[index];
}

std::string_view syntaxEnumToString(ssyc::SyntaxType syntax) {
    const auto index = static_cast<size_t>(syntax);
    assert(index >= 0 && index < SYNTAXTABLE.size());
    return SYNTAXTABLE[index];
}
