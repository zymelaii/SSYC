%{
#include "pch.h"
#include "utils.h"
#include "y.tab.hpp"
%}

WSP [[:blank:]]
EOL (\r\n)|\r|\n
LWSP ({WSP}|{EOL}{WSP})*

LINE_COMMENT \/\/[^\n]*

BLOCK_COMMENT_BEGIN \/\*
BLOCK_COMMENT_ELE .
BLOCK_COMMENT_LINE {EOL}
BLOCK_COMMENT_END \*\/
%x BLOCK_COMMENT

CONST_OCT 0[0-7]+
CONST_DEC 0|([1-9][0-9]*)
CONST_HEX 0[xX][0-9a-fA-F]+
CONST_INT {CONST_OCT}|{CONST_DEC}|{CONST_HEX}

FLOAT_EXP [eE][+-]?{CONST_DEC}
FLOAT_EXP_BIN [pP][+-]?{CONST_DEC}
FLOAT_FRACTIONAL ({CONST_DEC}?\.{CONST_DEC})|({CONST_DEC}\.)
FLOAT_FRACTIONAL_HEX ({CONST_HEX}?\.[0-9a-fA-F]+)|({CONST_HEX}\.)
CONST_FLOAT_DEC ({CONST_DEC}{FLOAT_EXP})|({FLOAT_FRACTIONAL}{FLOAT_EXP}?)
CONST_FLOAT_HEX ({CONST_HEX}|{FLOAT_FRACTIONAL_HEX}){FLOAT_EXP_BIN}
CONST_FLOAT {CONST_FLOAT_DEC}|{CONST_FLOAT_HEX}

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

"="   { SSYC_PRINT_TOKEN("found operator OP_ASS"); return OP_ASS;   }
"+"   { SSYC_PRINT_TOKEN("found operator OP_ADD"); return OP_ADD;   }
"-"   { SSYC_PRINT_TOKEN("found operator OP_SUB"); return OP_SUB;   }
"*"   { SSYC_PRINT_TOKEN("found operator OP_MUL"); return OP_MUL;   }
"/"   { SSYC_PRINT_TOKEN("found operator OP_DIV"); return OP_DIV;   }
"%"   { SSYC_PRINT_TOKEN("found operator OP_MOD"); return OP_MOD;   }
"<"   { SSYC_PRINT_TOKEN("found operator OP_LT"); return OP_LT;     }
">"   { SSYC_PRINT_TOKEN("found operator OP_GT"); return OP_GT;     }
"<="  { SSYC_PRINT_TOKEN("found operator OP_LE"); return OP_LE;     }
">="  { SSYC_PRINT_TOKEN("found operator OP_GE"); return OP_GE;     }
"=="  { SSYC_PRINT_TOKEN("found operator OP_EQ"); return OP_EQ;     }
"!="  { SSYC_PRINT_TOKEN("found operator OP_NE"); return OP_NE;     }
"!"   { SSYC_PRINT_TOKEN("found operator OP_LNOT"); return OP_LNOT; }
"&&"  { SSYC_PRINT_TOKEN("found operator OP_LAND"); return OP_LAND; }
"||"  { SSYC_PRINT_TOKEN("found operator OP_LOR"); return OP_LOR;   }

"(" { SSYC_PRINT_TOKEN("found LPAREN"); return LPAREN;       }
")" { SSYC_PRINT_TOKEN("found RPAREN"); return RPAREN;       }
"[" { SSYC_PRINT_TOKEN("found LBRACKET"); return LBRACKET;   }
"]" { SSYC_PRINT_TOKEN("found RBRACKET"); return RBRACKET;   }
"{" { SSYC_PRINT_TOKEN("found LBRACE"); return LBRACE;       }
"}" { SSYC_PRINT_TOKEN("found RBRACE"); return RBRACE;       }
"," { SSYC_PRINT_TOKEN("found COMMA"); return COMMA;         }
";" { SSYC_PRINT_TOKEN("found SEMICOLON"); return SEMICOLON; }

{CONST_INT}   { SSYC_PRINT_TOKEN("found integer literal"); return CONST_INT; }
{CONST_FLOAT} { SSYC_PRINT_TOKEN("found float literal"); return CONST_FLOAT; }

{IDENT} { SSYC_PRINT_TOKEN("found identifier"); return IDENT; }

. { SSYC_PRINT_TOKEN("caught unexpected char"); }

%%

void yyerror(char* message) {
    LOG(ERROR) << "parse error: " << message;
}

int yywrap() {
    return 1;
}
