#include "lex.h"

#include <string.h>
#include <set>
#include <regex>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

void  lexerror(LexState& ls, const char* msg, TOKEN token);
TOKEN lex(LexState& ls, std::string_view& raw);

struct Buffer {
    char*  buf;
    int    n;
    size_t size;
};

struct LexStatePrivate {
    std::unique_ptr<std::istream> stream;     //<! input stream
    std::set<const char*>         strtable;   //<! buffered string table
    Buffer                        buffer;     //<! raw input buffer
    bool                          bufenabled; //<! save chars to buffer

    LexStatePrivate()
        : bufenabled{false} {
        buffer.size = 256;
        buffer.buf  = (char*)malloc(buffer.size + 1);
        buffer.n    = 0;
    }

    ~LexStatePrivate() {
        free(buffer.buf);
        buffer.n    = 0;
        buffer.size = 0;

        for (const auto& ptr : strtable) { delete ptr; }
        strtable.clear();
    }

    const char* savestr(char* s) {
        auto [it, ok] = strtable.insert(s);
        if (!ok) { free(s); }
        return *it;
    }

    void bufreset() {
        buffer.n = 0;
    }

    void bufsave(char ch) {
        if (buffer.n >= buffer.size) {
            buffer.size += buffer.size / 2;
            realloc(buffer.buf, buffer.size + 1);
        }
        buffer.buf[buffer.n++] = ch;
    }

    char* bufdup() {
        buffer.buf[buffer.n] = '\0';
        return strdup(buffer.buf);
    }

    void bufenable(bool enabled) {
        bufenabled = enabled;
        if (bufenabled) { bufreset(); }
    }
};

struct BufferGuard {
    BufferGuard(LexState& ls)
        : state{ls} {
        state.d->bufenable(true);
    }

    ~BufferGuard() {
        state.d->bufenable(false);
    }

    LexState& state;
};

//! format token to string
const char* tok2str(TOKEN token) {
    switch (token) {
        case TOKEN::TK_NONE: {
            return "<none>";
        } break;
        case TOKEN::TK_VOID: {
            return "void";
        }
        case TOKEN::TK_INT: {
            return "int";
        }
        case TOKEN::TK_FLOAT: {
            return "float";
        }
        case TOKEN::TK_CONST: {
            return "const";
        }
        case TOKEN::TK_IF: {
            return "if";
        }
        case TOKEN::TK_ELSE: {
            return "else";
        }
        case TOKEN::TK_FOR: {
            return "for";
        }
        case TOKEN::TK_DO: {
            return "do";
        }
        case TOKEN::TK_WHILE: {
            return "while";
        }
        case TOKEN::TK_SWITCH: {
            return "switch";
        }
        case TOKEN::TK_CASE: {
            return "case";
        }
        case TOKEN::TK_DEFAULT: {
            return "default";
        }
        case TOKEN::TK_BERAK: {
            return "break";
        }
        case TOKEN::TK_CONTINUE: {
            return "continue";
        }
        case TOKEN::TK_RETURN: {
            return "return";
        }
        case TOKEN::TK_EQ: {
            return "==";
        }
        case TOKEN::TK_NE: {
            return "!=";
        }
        case TOKEN::TK_LE: {
            return "<=";
        }
        case TOKEN::TK_GE: {
            return ">=";
        }
        case TOKEN::TK_LOR: {
            return "||";
        }
        case TOKEN::TK_LAND: {
            return "&&";
        }
        case TOKEN::TK_SHL: {
            return "<<";
        }
        case TOKEN::TK_SHR: {
            return ">>";
        }
        case TOKEN::TK_IDENT: {
            return "<ident>";
        }
        case TOKEN::TK_INTVAL: {
            return "<integer>";
        }
        case TOKEN::TK_FLTVAL: {
            return "<float>";
        }
        case TOKEN::TK_STRING: {
            return "<string>";
        }
        case TOKEN::TK_EOF: {
            return "<eof>";
        }
        case TOKEN::TK_COMMENT:
        case TOKEN::TK_MLCOMMENT: {
            return "<comment>";
        }
        default: {
            return "<single>";
        }
    }
}

//! format token into string buffer
void tok2str(TOKEN token, char* buffer, size_t len) {
    auto itok = static_cast<int>(token);
    if (itok > 0 && itok < FIRST_RESERVED) {
        snprintf(buffer, len, "%c", static_cast<char>(itok));
    } else {
        strcpy_s(buffer, len, tok2str(token));
    }
}

inline bool isnewline(char c) {
    return c == '\n' || c == '\r';
}

TOKEN isreserved(const char* s) {
    auto e = FIRST_RESERVED;
    while (e <= LAST_RESERVED) {
        const auto token = static_cast<TOKEN>(e);
        if (strcmp(s, tok2str(token)) == 0) { return token; }
        ++e;
    }
    return TOKEN::TK_NONE;
}

bool isintval(const char* s) {
    auto              _ = R"(
o integer-constant: ((0[0-7]*)|([1-9][0-9]*)|(0[xX][0-9a-fA-F]+))(([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)?
x integer-suffix:   (([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)
)";
    static std::regex pattern(
        R"(((0[0-7]*)|([1-9][0-9]*)|(0[xX][0-9a-fA-F]+))(([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)?)");
    return std::regex_match(s, pattern);
}

bool isfltval(const char* s) {
    auto              _ = R"(
o floating-constant:               ((((([0-9]*\.[0-9]+)|([0-9]+\.)([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+)))|(0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+))[flFL]?
o decimal-floating-constant:       ((([0-9]*\.[0-9]+)|([0-9]+\.)([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+))[flFL]?
o hexadecimal-floating-constant:   0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+[flFL]?
x exponent-part:                   [eE][+-]?[0-9]+
x binary-exponent-part:            [pP][+-]?[0-9]+
x fractional-constant:             ([0-9]*\.[0-9]+)|([0-9]+\.)
x hexadecimal-fractional-constant: ([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.)
)";
    static std::regex pattern(
        R"(((((([0-9]*\.[0-9]+)|([0-9]+\.)([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+)))|(0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+))[flFL]?)");
    return std::regex_match(s, pattern);
}

//! read next character
inline void next(LexState& ls) {
    if (ls.d->bufenabled) { ls.d->bufsave(ls.cur); }
    ls.cur = ls.d->stream->get();
    ++ls.column;
}

//! read next if the current char is in the given set
template <size_t N>
inline bool nextif(LexState& ls, const char (&set)[N]) {
    static_assert(N > 1);
    bool ok = false;
    if constexpr (N == 2) {
        ok = ls.cur == set[0];
    } else if constexpr (N == 3) {
        ok = ls.cur == set[0] || ls.cur == set[1];
    } else {
        for (int i = 0; i < N - 1 && !ok; ++i) { ok = ls.cur == set[i]; }
    }
    if (ok) { next(ls); }
    return ok;
}

inline bool nextif(LexState& ls, const char* set) {
    const char* p = set;
    while (*p != '\0') {
        if (ls.cur == *p++) {
            next(ls);
            return true;
        }
    }
    return false;
}

//! entry newline
void into_newline(LexState& ls) {
    assert(isnewline(ls.cur));
    const auto ch = ls.cur;
    next(ls);
    if (isnewline(ls.cur) && ls.cur != ch) {
        next(ls); //<! skip `\n\r` or `\r\n`
    }
    const auto num = ls.line;
    if (++ls.line < num) { lexerror(ls, "too many lines", TOKEN::TK_NONE); }
    ls.column = 1;
}

//! read number constant (integer and float)
TOKEN read_number(LexState& ls, std::string_view& raw) {
    assert(isdigit(ls.cur) || ls.cur == '.');
    BufferGuard guard(ls);
    const char* exp = "eE";
    if (ls.cur == '0') {
        next(ls);
        if (nextif(ls, "xX")) { exp = "pP"; }
    }
    while (true) {
        if (nextif(ls, exp)) {
            nextif(ls, "-+");
        } else if (isdigit(ls.cur) || ls.cur == '.') {
            next(ls);
        } else {
            break;
        }
    }
    nextif(ls, "flFL");
    raw = ls.d->savestr(ls.d->bufdup());
    if (isintval(raw.data())) { return TOKEN::TK_INTVAL; }
    if (!isfltval(raw.data())) {
        lexerror(ls, "mal-formed floating constant", TOKEN::TK_FLTVAL);
    }
    return TOKEN::TK_FLTVAL;
}

//! read string constant
TOKEN read_string(LexState& ls, std::string_view& raw) {
    assert(ls.cur == '"');
    //! TODO: to be completed
    return TOKEN::TK_STRING;
}

//! read identifier
TOKEN read_symbol(LexState& ls, std::string_view& raw) {
    assert(isalpha(ls.cur) || ls.cur == '_');
    BufferGuard guard(ls);
    next(ls);
    while (isalnum(ls.cur) || ls.cur == '_') { next(ls); }
    raw         = ls.d->savestr(ls.d->bufdup());
    auto result = isreserved(raw.data());
    return result == TOKEN::TK_NONE ? TOKEN::TK_IDENT : result;
}

//! error handler for lex
void lexerror(LexState& ls, const char* msg, TOKEN token) {
    //! TODO: to be completed
}

//! main lex method
TOKEN lex(LexState& ls, std::string_view& raw) {
    raw = "";
    while (true) {
        switch (ls.cur) {
            //! initial
            case '\0':
            //! whitespace
            case ' ':
            case '\t': {
                next(ls);
                continue;
            } break;

            //! line breaks
            case '\n':
            case '\r': {
                into_newline(ls);
                continue;
            } break;
            //! comment
            case '/': {
                next(ls);
                if (nextif(ls, "/")) {
                    BufferGuard guard(ls);
                    while (!isnewline(ls.cur) && ls.cur != EOF) { next(ls); }
                    raw = ls.d->savestr(ls.d->bufdup());
                    return TOKEN::TK_COMMENT;
                } else if (nextif(ls, "*")) {
                    BufferGuard guard(ls);
                    while (ls.cur != EOF) {
                        if (nextif(ls, "*")) {
                            if (nextif(ls, "/")) { break; }
                        } else {
                            next(ls);
                        }
                    }
                    if (ls.cur == EOF) {
                        lexerror(
                            ls,
                            "unclosed multi-line comment",
                            TOKEN::TK_MLCOMMENT);
                    } else {
                        ls.d->buffer.n -= 2;
                    }
                    raw = ls.d->savestr(ls.d->bufdup());
                    return TOKEN::TK_MLCOMMENT;
                } else {
                    return TOKEN::TK_DIV;
                }
            } break;
            //! multi-char tokens
            case '=': {
                next(ls);
                return nextif(ls, "=") ? TOKEN::TK_EQ : TOKEN::TK_ASS;
            } break;
            case '!': {
                next(ls);
                return nextif(ls, "=") ? TOKEN::TK_NE : TOKEN::TK_NOT;
            } break;
            case '>': {
                next(ls);
                return nextif(ls, "=")
                         ? nextif(ls, ">") ? TOKEN::TK_SHR : TOKEN::TK_GE
                         : TOKEN::TK_GT;
            } break;
            case '<': {
                next(ls);
                return nextif(ls, "=")
                         ? nextif(ls, "<") ? TOKEN::TK_SHL : TOKEN::TK_LE
                         : TOKEN::TK_LT;
            } break;
            case '|': {
                next(ls);
                return nextif(ls, "|") ? TOKEN::TK_LOR : TOKEN::TK_OR;
            } break;
            case '&': {
                next(ls);
                return nextif(ls, "&") ? TOKEN::TK_LAND : TOKEN::TK_AND;
            } break;
            //! string constant
            case '"': {
            } break;
            //! EOF
            case EOF: {
                return TOKEN::TK_EOF;
            } break;
        }

        //! number constant
        if (isdigit(ls.cur) || ls.cur == '.') { return read_number(ls, raw); }

        //! reserved keyword or identifier
        if (isalpha(ls.cur) || ls.cur == '_') { return read_symbol(ls, raw); }

        //! single-char tokens
        auto token = static_cast<TOKEN>(ls.cur);
        next(ls);
        return token;
    }
}

LexState::LexState()
    : d{std::make_unique<LexStatePrivate>()} {}

LexState::~LexState() = default;

void LexState::resetstream(std::istream* input) {
    assert(!input->eof());
    d->stream.reset(input);
}

void LexState::next() {
    lastline   = line;
    lastcolumn = column;
    if (nexttoken.id != TOKEN::TK_NONE) {
        token        = nexttoken;
        nexttoken.id = TOKEN::TK_NONE;
    } else {
        token.id = lex(*this, token.detail);
    }
}

TOKEN LexState::lookahead() {
    if (nexttoken.id == TOKEN::TK_NONE) {
        auto col     = column;
        nexttoken.id = lex(*this, nexttoken.detail);
        lastcolumn   = column;
        column       = col;
    }
    return nexttoken.id;
}
