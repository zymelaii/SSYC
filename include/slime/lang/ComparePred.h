#pragma once

namespace slime::lang {

enum class ComparePred {
    AlwaysTrue,
    AlwaysFalse,
    Ordered,
    Unordered,
    Equal,
    NotEqual,
    SignedGT,
    SignedGE,
    SignedLT,
    SignedLE,
    UnsignedGT,
    UnsignedGE,
    UnsignedLT,
    UnsignedLE,
    OrderedEQ,
    OrderedNE,
    OrderedGT,
    OrderedGE,
    OrderedLT,
    OrderedLE,
    UnorderedEQ,
    UnorderedNE,
    UnorderedGT,
    UnorderedGE,
    UnorderedLT,
    UnorderedLE,
};

} // namespace slime::lang
