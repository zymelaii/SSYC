#pragma once

namespace slime::ast {

enum class UnaryOperator {
    Pos, //<! `+` positive
    Neg, //<! `-` negative
    Not, //<! `!` logical not
    Inv, //<! `~` bitwise inverse
    Unreachable,
    Paren = Unreachable, //<! `()`
};

enum class BinaryOperator {
    Assign, //<! `=` assign
    Add,    //<! `+` add
    Sub,    //<! `-` sub
    Mul,    //<! `*` mul
    Div,    //<! `/` div
    Mod,    //<! `%` remainder
    And,    //<! `&` bitwise and
    Or,     //<! `|` bitwise or
    Xor,    //<! `^` xor
    LAnd,   //<! `&&` logical and
    LOr,    //<! `||` logical or
    LT,     //<! `<` less than
    LE,     //<! `<=` less than or equal
    GT,     //<! `>` greater than
    GE,     //<! `>=` greater than or equal
    EQ,     //<! `==` equal
    NE,     //<! `!=` not equal
    Shl,    //<! `<<` bitwise shift left
    Shr,    //<! `>>` bitwise shift right
    Unreachable,
    Comma     = Unreachable, //<! `,`
    Subscript = Unreachable, //<! `[]`
};

} // namespace slime::ast
