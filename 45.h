#pragma once

#include "47.h"

#include <set>
#include <memory>
#include <istream>

namespace slime {

struct LexStatePrivate;

class LexState {
public:
    std::unique_ptr<LexStatePrivate>        d;        //<! private data
    std::shared_ptr<std::set<const char *>> strtable; //<! shared string set

    int32_t cur;        //<! current character (consider utf-8 code)
    size_t  line;       //<! number of current line
    size_t  lastline;   //<! line of last consumed token
    size_t  column;     //<! number of curreng column
    size_t  lastcolumn; //<! column of last consumed token
    Token   token;      //<! current token
    Token   nexttoken;  //<! lookahead token

    LexState();
    ~LexState();

    LexState(LexState &&other);
    LexState& operator=(LexState &&other);

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

} // namespace slime