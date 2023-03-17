%{
#include "pch.h"
#include "utils.h"
#include "ast.h"

#include <stdint.h>

#define YYSTYPE ssyc::AstNode*
#include "y.tab.hpp"

using ssyc::AstNode;
using ssyc::SyntaxType;
using ssyc::TokenType;
%}

WSP  [[:blank:]]
EOL  (\r\n)|\r|\n
LWSP ({WSP}|{EOL}{WSP})*

LINE_COMMENT        \/\/[^\n]*
BLOCK_COMMENT_BEGIN \/\*
BLOCK_COMMENT_ELE   .
BLOCK_COMMENT_LINE  {EOL}
BLOCK_COMMENT_END   \*\/
%x BLOCK_COMMENT

CONST_OCT 0[0-7]+
CONST_DEC 0|([1-9][0-9]*)
CONST_HEX 0[xX][0-9a-fA-F]+
CONST_INT {CONST_OCT}|{CONST_DEC}|{CONST_HEX}

FLOAT_EXP            [eE][+-]?[0-9]+
FLOAT_EXP_BIN        [pP][+-]?[0-9]+
FLOAT_FRAC           ([0-9]*\.[0-9]+)|([0-9]+\.)
FLOAT_FRAC_HEX       ([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.)
CONST_FLOAT_DEC      ({FLOAT_FRAC}{FLOAT_EXP}?)|([0-9]+{FLOAT_EXP})
CONST_FLOAT_HEX      0[xX](({FLOAT_FRAC_HEX}{FLOAT_EXP_BIN})|([0-9a-zA-Z]+{FLOAT_EXP_BIN}))
CONST_FLOAT          {CONST_FLOAT_DEC}|{CONST_FLOAT_HEX}

IDENT [a-zA-Z_][0-9a-zA-Z_]*

%%

{WSP}  { /* IGNORE */ }
{EOL}  { /* IGNORE */ }
{LWSP} { /* IGNORE */ }

{LINE_COMMENT}  { SSYC_PRINT_TOKEN("skip LINE_COMMENT"); }

{BLOCK_COMMENT_BEGIN} { BEGIN BLOCK_COMMENT; }
<BLOCK_COMMENT>{BLOCK_COMMENT_ELE}  { /* IGNORE */ }
<BLOCK_COMMENT>{BLOCK_COMMENT_LINE} { /* IGNORE */ }
<BLOCK_COMMENT>{BLOCK_COMMENT_END} {
    LOG(INFO) << "skip BLOCK_COMMENT: /* ... */";
    BEGIN INITIAL;
}

"const" { SSYC_PRINT_TOKEN("found qualifier"); return KW_CONST; }

"void" 	{ SSYC_PRINT_TOKEN("found type keyword"); return T_VOID;  }
"int" 	{ SSYC_PRINT_TOKEN("found type keyword"); return T_INT;   }
"float" { SSYC_PRINT_TOKEN("found type keyword"); return T_FLOAT; }

"if"       { SSYC_PRINT_TOKEN("found control keyword"); return KW_IF;       }
"else"     { SSYC_PRINT_TOKEN("found control keyword"); return KW_ELSE;     }
"while"    { SSYC_PRINT_TOKEN("found control keyword"); return KW_WHILE;    }
"break"    { SSYC_PRINT_TOKEN("found control keyword"); return KW_BREAK;    }
"continue" { SSYC_PRINT_TOKEN("found control keyword"); return KW_CONTINUE; }
"return"   { SSYC_PRINT_TOKEN("found control keyword"); return KW_RETURN;   }

"="  { SSYC_PRINT_TOKEN("found operator OP_ASS"); return OP_ASS;   }
"+"  { SSYC_PRINT_TOKEN("found operator OP_ADD"); return OP_ADD;   }
"-"  { SSYC_PRINT_TOKEN("found operator OP_SUB"); return OP_SUB;   }
"*"  { SSYC_PRINT_TOKEN("found operator OP_MUL"); return OP_MUL;   }
"/"  { SSYC_PRINT_TOKEN("found operator OP_DIV"); return OP_DIV;   }
"%"  { SSYC_PRINT_TOKEN("found operator OP_MOD"); return OP_MOD;   }
"<"  { SSYC_PRINT_TOKEN("found operator OP_LT"); return OP_LT;     }
">"  { SSYC_PRINT_TOKEN("found operator OP_GT"); return OP_GT;     }
"<=" { SSYC_PRINT_TOKEN("found operator OP_LE"); return OP_LE;     }
">=" { SSYC_PRINT_TOKEN("found operator OP_GE"); return OP_GE;     }
"==" { SSYC_PRINT_TOKEN("found operator OP_EQ"); return OP_EQ;     }
"!=" { SSYC_PRINT_TOKEN("found operator OP_NE"); return OP_NE;     }
"!"  { SSYC_PRINT_TOKEN("found operator OP_LNOT"); return OP_LNOT; }
"&&" { SSYC_PRINT_TOKEN("found operator OP_LAND"); return OP_LAND; }
"||" { SSYC_PRINT_TOKEN("found operator OP_LOR"); return OP_LOR;   }

"(" { SSYC_PRINT_TOKEN("found LPAREN"); return LPAREN;       }
")" { SSYC_PRINT_TOKEN("found RPAREN"); return RPAREN;       }
"[" { SSYC_PRINT_TOKEN("found LBRACKET"); return LBRACKET;   }
"]" { SSYC_PRINT_TOKEN("found RBRACKET"); return RBRACKET;   }
"{" { SSYC_PRINT_TOKEN("found LBRACE"); return LBRACE;       }
"}" { SSYC_PRINT_TOKEN("found RBRACE"); return RBRACE;       }
"," { SSYC_PRINT_TOKEN("found COMMA"); return COMMA;         }
";" { SSYC_PRINT_TOKEN("found SEMICOLON"); return SEMICOLON; }

{CONST_INT}   { SSYC_PRINT_TOKEN("found integer literal");
    yylval = new AstNode;
    yylval->token = TokenType::TT_CONST_INT;
    yylval->FLAGS_ConstEval = true;
    yylval->FLAGS_Constant = true;
    yylval->FLAGS_Arithmetical = true;

    //! TODO: 检查合法性
    yylval->value = static_cast<int32_t>(std::stoi(yytext));

    return CONST_INT;
}
{CONST_FLOAT} { SSYC_PRINT_TOKEN("found float literal");
    yylval = new AstNode;
    yylval->token = TokenType::TT_CONST_FLOAT;
    yylval->FLAGS_ConstEval = true;
    yylval->FLAGS_Constant = true;
    yylval->FLAGS_Arithmetical = true;

    //! TODO: 检查合法性
    yylval->value = static_cast<float>(std::stof(yytext));

    return CONST_FLOAT;
}

{IDENT} { SSYC_PRINT_TOKEN("found identifier");
    yylval = new AstNode;
    yylval->token = TokenType::TT_IDENT;
    yylval->value = yytext;
    return IDENT;
}

. { SSYC_PRINT_TOKEN("caught unexpected char"); }

%%
