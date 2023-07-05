#pragma once

#include "../ast/operators.def"

#include <array>
#include <stddef.h>
#include <assert.h>

namespace slime {

namespace detail {
static inline constexpr size_t TOTAL_OPERATORS = 25;
} // namespace detail

struct OperatorPriority {
    constexpr OperatorPriority(int priority, bool assoc)
        : priority{priority}
        , assoc{assoc} {}

    int priority;

    //! false: right-assoc
    //! true: left-assoc
    bool assoc;
};

static constexpr auto PRIORITIES =
    std::array<OperatorPriority, detail::TOTAL_OPERATORS>{
        //! UnaryOperator (except Paren)
        OperatorPriority(1, false),
        OperatorPriority(1, false),
        OperatorPriority(1, false),
        OperatorPriority(1, false),
        //! Subscript '[]'
        OperatorPriority(0, true),
        //! BinaryOperator
        //! Mul Div Mod
        OperatorPriority(2, true),
        OperatorPriority(2, true),
        OperatorPriority(2, true),
        //! Add Sub
        OperatorPriority(3, true),
        OperatorPriority(3, true),
        //! Shl Shr
        OperatorPriority(4, true),
        OperatorPriority(4, true),
        //! Comparaison Operator
        OperatorPriority(5, true),
        OperatorPriority(5, true),
        OperatorPriority(5, true),
        OperatorPriority(5, true),
        //! EQ NE
        OperatorPriority(6, true),
        OperatorPriority(6, true),
        //! And
        OperatorPriority(7, true),
        //! Xor
        OperatorPriority(8, true),
        //! Or
        OperatorPriority(9, true),
        //! Logic And
        OperatorPriority(10, true),
        //! Logic Or
        OperatorPriority(11, true),
        //! Assign
        OperatorPriority(12, false),
        //! Comma
        OperatorPriority(13, true),
    };

inline OperatorPriority lookupOperatorPriority(ast::UnaryOperator op) {
    assert(op != ast::UnaryOperator::Paren);
    return PRIORITIES[static_cast<int>(op)];
}

inline OperatorPriority lookupOperatorPriority(ast::BinaryOperator op) {
    return PRIORITIES
        [static_cast<int>(op) + static_cast<int>(ast::UnaryOperator::Paren)];
}

} // namespace slime
