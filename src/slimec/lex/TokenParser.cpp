#include <slimec/lex/TokenParser.h>
#include <slimec/lex/LexObject.h>
#include <slime/lang/Token.h>

using namespace slime::lang;

namespace slimec {

template <>
void parseDynamicToken<IdentifierTok>(LexObject& token) {
    auto code = token.code();
    if (!std::isalpha(code) && code != '_') {
        token.reject("mal-formed identifier");
    }
    do { code = token.next(); } while (std::isalnum(code) || code == '_');

    IdentifierTok result;
    result.value = token.str();
    token.accept(result);
}

template <>
void parseDynamicToken<AttributeIdentifierTok>(LexObject& token) {}

template <>
void parseDynamicToken<ContextualConstantTok>(LexObject& token) {}

template <>
void parseDynamicToken<BooleanLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<CharLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<IntegerLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<FloatLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<StringLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<RawStringLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<FormattableStringLiteralTok>(LexObject& token) {}

template <>
void parseDynamicToken<CommentTok>(LexObject& token) {}

} // namespace slimec
