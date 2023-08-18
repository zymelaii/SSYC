#pragma once

#include <string>
#include <vector>
#include <stddef.h>

namespace slime::lang {

namespace syntax {
class Expr;
}; // namespace syntax

class FormattableString {
private:
    struct ExprItem {
        size_t        location;
        syntax::Expr *value;
    };

private:
    std::string           format_;
    std::vector<ExprItem> subItems_;
};

} // namespace slime::lang
