#include "lex.h"
#include "preproc.h"

#include <string.h>
#include <set>
#include <regex>
#include <string>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

namespace slime {

#ifndef __STDC_LIB_EXT1__
static void strcpy_s(char* dest, size_t n, const char* src) {
    char*       p = dest;
    const char* q = src;
    while (--n >= 0 && *q != '\0') { *p++ = *q++; }
    *p = '\0';
}
#endif

struct Buffer {
    char*  buf;
    int    n;
    size_t size;
};

struct LexStatePrivate {
    InputStreamTransformer                 stream;   //<! input stream
    std::shared_ptr<std::set<const char*>> strtable; //<! buffered string table
    Buffer                                 buffer;   //<! raw input buffer
    bool                                   bufenabled; //<! save chars to buffer
    std::string linebuffer; //<! input buffer of currnet line

    LexStatePrivate()
        : bufenabled{false}
        , strtable{std::make_shared<std::set<const char*>>()} {
        buffer.size = 256;
        buffer.buf  = (char*)malloc(buffer.size + 1);
        buffer.n    = 0;
    }

    ~LexStatePrivate() {
        free(buffer.buf);
        buffer.n    = 0;
        buffer.size = 0;
    }

    const char* savestr(char* s) {
        auto [it, ok] = strtable->insert(s);
        if (!ok) { free(s); }
        return *it;
    }

    void bufreset() {
        buffer.n = 0;
    }

    void bufsave(char ch) {
        if (buffer.n >= buffer.size) {
            buffer.size += buffer.size / 2;
            buffer.buf  = (char*)realloc(buffer.buf, buffer.size + 1);
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

} // namespace slime

using slime::BufferGuard;
using slime::LexState;
using slime::TOKEN;
using slime::detail::FIRST_RESERVED;
using slime::detail::LAST_RESERVED;

void  lexerror(LexState& ls, const char* msg, TOKEN token);
TOKEN lex(LexState& ls, std::string_view& raw);

inline bool isnewline(char c) {
    return c == '\n' || c == '\r';
}

inline bool isreserved(TOKEN token) {
    const auto e = static_cast<int>(token);
    return e >= FIRST_RESERVED && e <= LAST_RESERVED;
}

bool isintval(const char* s) {
    auto              _ = R"(
o integer-constant: ((0[0-7]*)|([1-9][0-9]*)|(0[xX][0-9a-fA-F]+))(([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)?
x integer-suffix:   (([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)
)";
    static std::regex pattern(
        R"(((0[0-7]*)|(0[xX][0-9a-fA-F]+)|([1-9][0-9]*))(([uU](l{0,2}|L{0,2})?)|(l{1,2}|L{1,2})[uU]?)?)");
    return std::regex_match(s, pattern);
}

bool isfltval(const char* s) {
    auto              _ = R"(
o floating-constant:               ((((([0-9]*\.[0-9]+)|([0-9]+\.)([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+)))|(0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+))[flFL]?
o decimal-floating-constant:       (((([0-9]*\.[0-9]+)|([0-9]+\.))([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+))[flFL]?
o hexadecimal-floating-constant:   0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+[flFL]?
x exponent-part:                   [eE][+-]?[0-9]+
x binary-exponent-part:            [pP][+-]?[0-9]+
x fractional-constant:             ([0-9]*\.[0-9]+)|([0-9]+\.)
x hexadecimal-fractional-constant: ([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.)
)";
    static std::regex pattern(
        R"((((((([0-9]*\.[0-9]+)|([0-9]+\.))([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+)))|(0[xX]((([0-9a-fA-F]*\.[0-9a-fA-F]+)|([0-9a-fA-F]+\.))|([0-9a-fA-F]+))[pP][+-]?[0-9]+))[flFL]?)");
    return std::regex_match(s, pattern);
}

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
        case TOKEN::TK_STATIC: {
            return "static";
        } break;
        case TOKEN::TK_INLINE: {
            return "inline";
        } break;
        case TOKEN::TK_EXTERN: {
            return "extern";
        } break;
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
        case TOKEN::TK_BREAK: {
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

TOKEN to_reserved(const char* s) {
    auto e = FIRST_RESERVED;
    while (e <= LAST_RESERVED) {
        const auto token = static_cast<TOKEN>(e);
        if (strcmp(s, tok2str(token)) == 0) { return token; }
        ++e;
    }
    return TOKEN::TK_NONE;
}

//! read next character
inline void next(LexState& ls) {
    if (ls.cur != '\0') { ls.d->linebuffer.push_back(ls.cur); }
    if (ls.d->bufenabled) { ls.d->bufsave(ls.cur); }
    ls.cur = ls.d->stream.get();
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
    ls.d->linebuffer.clear();
    ls.d->linebuffer.push_back(ls.cur);
}

//! escape string
int try_escape(LexState& ls, char* s) {
    char*          origin = s;
    constexpr int  N      = 11;
    constexpr char ESCAPE_TABLE[N][2]{
        {'\'', '\''},
        {'"',  '"' },
        {'?',  '?' },
        {'\\', '\\'},
        {'a',  '\a'},
        {'b',  '\b'},
        {'f',  '\f'},
        {'n',  '\n'},
        {'r',  '\r'},
        {'t',  '\t'},
        {'v',  '\v'},
    };
    int   escaped = 0;
    bool  err     = false;
    char *p = s, *q = p;
    while (true) {
        while (*p != '\0' && *p != '\\') { *s++ = *p++; }
        if (*p == '\0') { break; }
        q     = p + 1;
        int i = 0;
        while (i < N) {
            if (ESCAPE_TABLE[i][0] == *q) {
                *s++ = ESCAPE_TABLE[i][1];
                ++escaped;
                p = q + 1;
                break;
            }
            ++i;
        }
        if (i < N) { continue; }
        if (*q >= '0' && *q <= '7') { //<! arbitrary octal value
            int value = *q - '0';
            int n     = 1;
            while (*++q != '\0' && n < 3) {
                if (!(*q >= '0' && *q <= '7')) { break; }
                value = value * 8 + *q - '0';
                ++n;
            }
            if (value > 0xff) {
                err = true;
                lexerror(
                    ls, "octal escape sequence out of range", TOKEN::TK_STRING);
            }
            *s++ = value % 0xff;
            ++escaped;
            p = q;
        } else if (*q == 'x') { //<! arbitrary hexadecimal value
            int n     = 0;
            int value = 0;
            while (*++q != '\0' && n < 2) {
                if (!isxdigit(*q)) { break; }
                value =
                    value * 16 + tolower(*q) - (isdigit(*q) ? '0' : 'a' - 10);
                ++n;
            }
            if (n == 0) {
                err = true;
                lexerror(
                    ls,
                    "\\x used with no following hex digits",
                    TOKEN::TK_STRING);
            }
            *s++ = value;
            ++escaped;
            p = q;
        } else if (*q == '\0') {
            err = true;
            lexerror(ls, "no characters to escape", TOKEN::TK_STRING);
            *s++ = '\\';
            ++escaped;
            break;
        } else {
            err = true;
            char msg[64]{};
            sprintf(msg, "unknown escape sequence '\\%c'", *q);
            lexerror(ls, msg, TOKEN::TK_STRING);
            *s++ = *q;
            ++escaped;
            p = q + 1;
        }
    }
    *s = '\0';
    return err ? -escaped : escaped;
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
        } else if (isalnum(ls.cur) || ls.cur == '.') {
            //! NOTE: consider hex integer, using isalnum instead of isnumber
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
    next(ls);
    BufferGuard guard(ls);
    bool        escape = false;
    bool        ok     = false;
    while (ls.cur != EOF) {
        if (isnewline(ls.cur)) { break; }
        if (escape) {
            next(ls);
            escape = false;
        } else if (nextif(ls, "\\")) {
            continue;
        } else if (nextif(ls, "\"")) {
            ok = true;
            break;
        } else {
            next(ls);
        }
    }
    if (!ok) {
        lexerror(ls, "unclosed string literal", TOKEN::TK_STRING);
    } else {
        ls.d->buffer.n -= 1;
    }
    char* s   = ls.d->bufdup();
    int   ret = try_escape(ls, s);
    if (ok && ret < 0) {
        lexerror(ls, "mal-formed string literal", TOKEN::TK_STRING);
    }
    raw = ls.d->savestr(s);
    return TOKEN::TK_STRING;
}

//! read identifier
TOKEN read_symbol(LexState& ls, std::string_view& raw) {
    assert(isalpha(ls.cur) || ls.cur == '_');
    BufferGuard guard(ls);
    next(ls);
    while (isalnum(ls.cur) || ls.cur == '_') { next(ls); }
    raw         = ls.d->savestr(ls.d->bufdup());
    auto result = to_reserved(raw.data());
    return result == TOKEN::TK_NONE ? TOKEN::TK_IDENT : result;
}

//! error handler for lex
void lexerror(LexState& ls, const char* msg, TOKEN token) {
    fprintf(
        stderr,
        "%zu:%zu: error: %s\n%s\n%*s^\n",
        ls.line,
        ls.column,
        msg,
        ls.d->linebuffer.c_str(),
        std::max(static_cast<int>(ls.column) - 1, 0),
        "");
    //! TODO: introduce custom error handlers
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
                        } else if (isnewline(ls.cur)) {
                            into_newline(ls);
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
                return nextif(ls, "=") ? TOKEN::TK_GE
                     : nextif(ls, ">") ? TOKEN::TK_SHR
                                       : TOKEN::TK_GT;
            } break;
            case '<': {
                next(ls);
                return nextif(ls, "=") ? TOKEN::TK_LE
                     : nextif(ls, "<") ? TOKEN::TK_SHL
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
                return read_string(ls, raw);
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

namespace slime {

LexState::LexState()
    : d{std::make_unique<LexStatePrivate>()} {
    strtable = d->strtable;
}

LexState::~LexState() = default;

LexState::LexState(LexState&& other) {
    cur        = other.cur;
    line       = other.line;
    lastline   = other.lastline;
    column     = other.column;
    lastcolumn = other.lastcolumn;
    token      = other.token;
    nexttoken  = other.nexttoken;
    d.reset(other.d.release());
    strtable = other.strtable;
    other.strtable.reset();
}

LexState& LexState::operator=(LexState&& other) {
    new (this) LexState(std::move(other));
    return *this;
}

void LexState::resetstream(std::istream* input, std::string_view source) {
    assert(!input->eof());
    d->stream.reset(input, source);
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
        auto colno   = column;
        auto lineno  = line;
        nexttoken.id = lex(*this, nexttoken.detail);
        lastcolumn   = column;
        lastline     = line;
        column       = colno;
        line         = lineno;
    }
    return nexttoken.id;
}

//! format token into string buffer
const char* tok2str(TOKEN token, char* buffer, size_t len) {
    auto itok = static_cast<int>(token);
    if (itok > 0 && itok < FIRST_RESERVED) {
        snprintf(buffer, len, "%c", static_cast<char>(itok));
    } else {
        strcpy_s(buffer, len, ::tok2str(token));
    }
    return buffer;
}

//! format token into pretty string
const char* pretty_tok2str(Token token, char* buffer, size_t len) {
    if (isreserved(token.id)) { return tok2str(token.id, buffer, len); }
    switch (token.id) {
        case TOKEN::TK_IDENT:
        case TOKEN::TK_INTVAL:
        case TOKEN::TK_FLTVAL: {
            char buf[16]{};
            auto id = tok2str(token.id, buf);
            snprintf(buffer, len, "%s %s", id, token.detail.data());
        } break;
        case TOKEN::TK_STRING: {
            snprintf(buffer, len, "<string> length: %llu", token.detail.size());
        } break;
        case TOKEN::TK_EOF: {
            strcpy_s(buffer, len, "<eof>");
        } break;
        case TOKEN::TK_COMMENT: {
            snprintf(buffer, len, "<comment> //%s", token.detail.data());
        } break;
        case TOKEN::TK_MLCOMMENT: {
            strcpy_s(buffer, len, "<comment> /*...*/");
        } break;
        case TOKEN::TK_NONE: {
            buffer[0] = '\0';
        } break;
        default: {
            const auto itok = static_cast<int>(token.id);
            char       buf[4]{};
            if (itok > 0 && itok < FIRST_RESERVED) {
                buf[0] = static_cast<char>(itok);
            } else {
                tok2str(token.id, buf);
            }
            snprintf(buffer, len, "\"%s\"", buf);
        } break;
    }
    return buffer;
}

} // namespace slime
