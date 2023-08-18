#pragma once

#include <slime/lang/DynToken.h>

namespace slimec {

class LexObject;

template <typename T>
void parseDynamicToken(LexObject& token);

template <>
void parseDynamicToken<slime::lang::IdentifierTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::AttributeIdentifierTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::ContextualConstantTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::BooleanLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::CharLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::IntegerLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::FloatLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::StringLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::RawStringLiteralTok>(LexObject& token);

template <>
void parseDynamicToken<slime::lang::FormattableStringLiteralTok>(
    LexObject& token);

template <>
void parseDynamicToken<slime::lang::CommentTok>(LexObject& token);

} // namespace slimec
