#pragma once

#include "5.h"
#include <string_view>

namespace slime {

struct Diagnosis {
    //! expect: always false
    static void expectAlwaysFalse(std::string_view message);

    //! expect: the given expression is true
    static inline void expectTrue(bool value, std::string_view message);

    //! assert: always false
    [[noreturn]] static void assertAlwaysFalse(std::string_view message);

    //! assert: the given expression is true
    static inline void assertTrue(bool value, std::string_view message);

    //! check: pointer is not null
    static inline bool checkNotNull(void *ptr);

    //! check: type 'from' is convertible to type 'to'
    static bool checkTypeConvertible(ast::Type *from, ast::Type *to);

    //! assert: the given conditional expression is convertible to 'bool'
    static void assertConditionalExpression(ast::Expr *expr);

    //! assert: the return statement matches the function proto
    static void assertWellFormedReturnStatement(
        ast::ReturnStmt *stmt, ast::FunctionProtoType *proto);

    //! assert: break statement appears in a loop statement
    static void assertWellFormedBreakStatement(ast::BreakStmt *stmt);

    //! assert: continue statement appears in a loop statement
    static void assertWellFormedContinueStatement(ast::ContinueStmt *stmt);

    //! assert: for statement is well-formed
    static void assertWellFormedForStatement(ast::ForStmt *stmt);

    //! assert: comma is not left alone in a comma expression
    static void assertWellFormedCommaExpression(ast::Expr *expr);

    //! assert: the value being assigned is not constant
    static void assertNoAssignToConstQualifiedValue(
        ast::Expr *value, ast::BinaryOperator op = ast::BinaryOperator::Assign);

    //! assert: the given array type is well-formed
    static void assertWellFormedArrayType(ast::ArrayType *type);

    //! assert: the subscripted value is valid, e.g. array, pointer...
    static void assertSubscriptableValue(ast::Expr *value);

    //! assert: the initialization is well formed
    static void assertWellFormedInitialization(
        ast::Type *type, ast::Expr *value);

    //! assert: the given callable is valid
    static void assertCallable(ast::Expr *callable);

    //! assert: the function is valid
    static void assertWellFormedFunctionCall(
        ast::FunctionProtoType *proto, ast::ExprList *arguments);
};

inline void Diagnosis::expectTrue(bool value, std::string_view message) {
    if (!value) { expectAlwaysFalse(message); }
}

inline void Diagnosis::assertTrue(bool value, std::string_view message) {
    if (!value) { assertAlwaysFalse(message); }
}

inline bool Diagnosis::checkNotNull(void *ptr) {
    return ptr != nullptr;
}

} // namespace slime