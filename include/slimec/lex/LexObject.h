#pragma once

#include <string_view>
#include <any>

namespace slimec {

class Lex;

class LexObject {
public:
    template <typename T>
    [[noreturn]] void accept(T token = {});
    [[noreturn]] void reject(std::string_view msg = "");

    char32_t code() const;
    char32_t next();

    std::string_view str() const;

private:
    bool     done_;
    Lex*     parent_;
    std::any result_;
};

} // namespace slimec
