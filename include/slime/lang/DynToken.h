#pragma once

#include <cstddef>
#include <string>
#include <memory>

namespace slime::lang {

enum class ContextualConstantKind;
class FormattableString;

//! identifier token
struct IdentifierTok {
    std::string value;
};

//! attribute identifier token
struct AttributeIdentifierTok {
    std::string value;
};

//! contextual constant token
struct ContextualConstantTok {
    ContextualConstantKind value;
};

//! boolean literal token
struct BooleanLiteralTok {
    bool value;
};

//! character literal token
struct CharLiteralTok {
    char32_t value;
};

//! integer literal token
struct IntegerLiteralTok {
    std::byte value[8];
    bool      isSigned;
    size_t    bitWidth;
};

//! floating-point literal token
struct FloatLiteralTok {
    std::byte value[16];
    size_t    bitWidth;
};

//! string literal token
struct StringLiteralTok {
    std::string value;
};

//! raw string literal token
struct RawStringLiteralTok {
    std::string value;
};

//! formattable string literal token
struct FormattableStringLiteralTok {
    std::unique_ptr<FormattableString> value;
};

//! comment token
struct CommentTok {
    std::string value;
};

}; // namespace slime::lang
