#pragma once

#include <slime/ast/operators.def>
#include <array>
#include <stddef.h>
#include <assert.h>

namespace slime {

namespace detail {
static inline constexpr size_t TOTAL_OPERATORS =
    static_cast<int>(ast::UnaryOperator::Unreachable)
    + static_cast<int>(ast::BinaryOperator::Unreachable);
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
        //! UnaryOperator
        OperatorPriority(1, false), //<! Pos
        OperatorPriority(1, false), //<! Neg
        OperatorPriority(1, false), //<! Not
        OperatorPriority(1, false), //<! Int
        //! BinaryOperator
        OperatorPriority(12, false), //<! Assign
        OperatorPriority(3, true),   //<! Add
        OperatorPriority(3, true),   //<! Sub
        OperatorPriority(2, true),   //<! Mul
        OperatorPriority(2, true),   //<! Div
        OperatorPriority(2, true),   //<! Mod
        OperatorPriority(7, true),   //<! And
        OperatorPriority(9, true),   //<! Or
        OperatorPriority(8, true),   //<! Xor
        OperatorPriority(10, true),  //<! LAnd
        OperatorPriority(11, true),  //<! LOr
        OperatorPriority(5, true),   //<! LT
        OperatorPriority(5, true),   //<! LE
        OperatorPriority(5, true),   //<! GT
        OperatorPriority(5, true),   //<! GE
        OperatorPriority(6, true),   //<! EQ
        OperatorPriority(6, true),   //<! NE
        OperatorPriority(4, true),   //<! Shl
        OperatorPriority(4, true),   //<! Shr
    };

inline OperatorPriority lookupOperatorPriority(ast::UnaryOperator op) {
    assert(op != ast::UnaryOperator::Paren);
    return PRIORITIES[static_cast<int>(op)];
}

inline OperatorPriority lookupOperatorPriority(ast::BinaryOperator op) {
    return PRIORITIES
        [static_cast<int>(op)
         + static_cast<int>(ast::UnaryOperator::Unreachable)];
}

} // namespace slime
