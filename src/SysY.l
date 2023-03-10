%{
#include "pch.h"
#include "utils.h"
#include "y.tab.hpp"
%}

WSP [[:blank:]]
EOL (\r\n)|\r|\n
LWSP ({WSP}|{EOL}{WSP})*

LINE_COMMENT \/\/[^\n]*
BLOCK_COMMENT \/\*[\n\s\S]*\*\/

OCT_LITERAL 0[0-7]+
DEC_LITERAL 0|([1-9][0-9]*)
HEX_LITERAL 0[xX][0-9a-fA-F]+
INT_LITERAL {OCT_LITERAL}|{DEC_LITERAL}|{HEX_LITERAL}

FLOAT_EXP [eE][+-]?{DEC_LITERAL}
FLOAT_EXP_BIN [pP][+-]?{DEC_LITERAL}
FLOAT_FRACTIONAL ({DEC_LITERAL}?\.{DEC_LITERAL})|({DEC_LITERAL}\.)
FLOAT_FRACTIONAL_HEX ({HEX_LITERAL}?\.[0-9a-fA-F]+)|({HEX_LITERAL}\.)
FLOAT_DEC_LITERAL ({DEC_LITERAL}{FLOAT_EXP})|({FLOAT_FRACTIONAL}{FLOAT_EXP}?)
FLOAT_HEX_LITERAL ({HEX_LITERAL}|{FLOAT_FRACTIONAL_HEX}){FLOAT_EXP_BIN}
FLOAT_LITERAL {FLOAT_DEC_LITERAL}|{FLOAT_HEX_LITERAL}

IDENT [a-zA-Z_][0-9a-zA-Z_]*

%%

{WSP}  { /* IGNORE */ }
{EOL}  { /* IGNORE */ }
{LWSP} { /* IGNORE */ }

{LINE_COMMENT}  { SSYC_PRINT_TOKEN("skip LINE_COMMENT"); }
{BLOCK_COMMENT} { printf("skip BLOCK_COMMENT: /* ... */ Content-Length=%d", yyleng - 4); }

"const" { SSYC_PRINT_TOKEN("found qualifier"); return CONST_KEYWORD; }

"void" 	{ SSYC_PRINT_TOKEN("found type keyword"); return VOID_TYPE;  }
"int" 	{ SSYC_PRINT_TOKEN("found type keyword"); return INT_TYPE;   }
"float" { SSYC_PRINT_TOKEN("found type keyword"); return FLOAT_TYPE; }

"if"       { SSYC_PRINT_TOKEN("found control keyword"); return IF_KEYWORD;       }
"else"     { SSYC_PRINT_TOKEN("found control keyword"); return ELSE_KEYWORD;     }
"while"    { SSYC_PRINT_TOKEN("found control keyword"); return WHILE_KEYWORD;    }
"break"    { SSYC_PRINT_TOKEN("found control keyword"); return BREAK_KEYWORD;    }
"continue" { SSYC_PRINT_TOKEN("found control keyword"); return CONTINUE_KEYWORD; }
"return"   { SSYC_PRINT_TOKEN("found control keyword"); return RETURN_KEYWORD;   }

"="   { SSYC_PRINT_TOKEN("found operator ASSIGN_OP"); return ASSIGN_OP; }
"+"   { SSYC_PRINT_TOKEN("found operator ADD_OP"); return ADD_OP;       }
"-"   { SSYC_PRINT_TOKEN("found operator SUB_OP"); return SUB_OP;       }
"*"   { SSYC_PRINT_TOKEN("found operator MUL_OP"); return MUL_OP;       }
"/"   { SSYC_PRINT_TOKEN("found operator DIV_OP"); return DIV_OP;       }
"%"   { SSYC_PRINT_TOKEN("found operator MOD_OP"); return MOD_OP;       }
"<"   { SSYC_PRINT_TOKEN("found operator LT_OP"); return LT_OP;         }
">"   { SSYC_PRINT_TOKEN("found operator GT_OP"); return GT_OP;         }
"<="  { SSYC_PRINT_TOKEN("found operator LE_OP"); return LE_OP;         }
">="  { SSYC_PRINT_TOKEN("found operator GE_OP"); return GE_OP;         }
"=="  { SSYC_PRINT_TOKEN("found operator EQ_OP"); return EQ_OP;         }
"!="  { SSYC_PRINT_TOKEN("found operator NE_OP"); return NE_OP;         }
"!"   { SSYC_PRINT_TOKEN("found operator LNOT_OP"); return LNOT_OP;     }
"&&"  { SSYC_PRINT_TOKEN("found operator LAND_OP"); return LAND_OP;     }
"||"  { SSYC_PRINT_TOKEN("found operator LOR_OP"); return LOR_OP;       }

"(" { SSYC_PRINT_TOKEN("found LPAREN"); return LPAREN;       }
")" { SSYC_PRINT_TOKEN("found RPAREN"); return RPAREN;       }
"[" { SSYC_PRINT_TOKEN("found LBRACKET"); return LBRACKET;   }
"]" { SSYC_PRINT_TOKEN("found RBRACKET"); return RBRACKET;   }
"{" { SSYC_PRINT_TOKEN("found LBRACE"); return LBRACE;       }
"}" { SSYC_PRINT_TOKEN("found RBRACE"); return RBRACE;       }
"," { SSYC_PRINT_TOKEN("found COMMA"); return COMMA;         }
";" { SSYC_PRINT_TOKEN("found SEMICOLON"); return SEMICOLON; }

{INT_LITERAL}   { SSYC_PRINT_TOKEN("found integer literal"); return INT_LITERAL; }
{FLOAT_LITERAL} { SSYC_PRINT_TOKEN("found float literal"); return FLOAT_LITERAL; }

{IDENT} { SSYC_PRINT_TOKEN("found identifier"); return IDENT; }

. { SSYC_PRINT_TOKEN("caught unexpected char"); }

%%

void yyerror(char* message) {
    LOG(ERROR) << "parse error: " << message;
}

int yywrap() {
    return 1;
}
