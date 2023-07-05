#pragma once

#include "lex.h"

#include <iostream>
#include <array>
#include <memory>
#include <type_traits>

namespace slime {

namespace detail {

template <TOKEN... Tokens>
constexpr bool isUniqueTokenSequence() {
    constexpr std::array tokens{Tokens...};
    for (int i = 0; i < tokens.size(); ++i) {
        for (int j = i + 1; j < tokens.size(); ++j) {
            if (tokens[i] == tokens[j]) { return false; }
        }
    }
    return true;
}

template <typename P, TOKEN... Ts>
class IncompleteLexerTrait;

template <TOKEN... Ts>
class IncompleteLexerTrait<void, Ts...> {
public:
    static constexpr inline bool isDiscard(TOKEN token) {
        if constexpr (discards_.size() == 0) {
            return false;
        } else if constexpr (discards_.size() == 1) {
            return token == discards_[0];
        } else if constexpr (discards_.size() == 2) {
            return token == discards_[0] || token == discards_[1];
        } else {
            for (auto e : discards_) {
                if (token == e) { return true; }
            }
            return false;
        }
    }

public:
    IncompleteLexerTrait()
        : state_() {}

    IncompleteLexerTrait(IncompleteLexerTrait&& other)
        : state_(std::move(other.state_)) {}

    IncompleteLexerTrait& operator=(IncompleteLexerTrait&& other) {
        new (this) IncompleteLexerTrait(std::move(other));
    }

    IncompleteLexerTrait(const IncompleteLexerTrait&)            = delete;
    IncompleteLexerTrait& operator=(const IncompleteLexerTrait&) = delete;

public:
    const Token& this_token() const {
        return state_.token;
    }

    const Token& exact_next() const {
        state_.next();
        return state_.token;
    }

    const Token& exact_lookahead() const {
        if (state_.nexttoken.isNone()) { state_.lookahead(); }
        return state_.nexttoken;
    }

    const Token& next() const {
        do { state_.next(); } while (isDiscard(state_.token.id));
        char buffer[64]{};
        std::cout << pretty_tok2str(this_token(), buffer) << std::endl;
        return state_.token;
    }

    const Token& lookahead() const {
        if (isDiscard(state_.nexttoken.id) || state_.nexttoken.isNone()) {
            do {
                state_.nexttoken.id = TOKEN::TK_NONE;
                state_.lookahead();
            } while (isDiscard(state_.nexttoken.id));
        }
        return state_.nexttoken;
    }

    auto& strtable() const {
        return state_.strtable;
    }

    template <typename T>
    void reset(T& stream) {
        state_.reset(stream);
        state_.next();
    }

private:
    static constexpr std::array discards_{Ts...};

    mutable LexState state_;
};

} // namespace detail

template <TOKEN... Ts>
using LexerTrait = detail::IncompleteLexerTrait<
    std::enable_if_t<detail::isUniqueTokenSequence<Ts...>()>,
    Ts...>;

using Lexer = LexerTrait<TOKEN::TK_COMMENT, TOKEN::TK_MLCOMMENT>;

} // namespace slime
