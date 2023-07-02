#pragma once

#include "../ast/expr.h"

#include <stddef.h>

namespace slime::visitor {

class ASTExprSimplifier {
public:
    static ast::Expr*         trySimplify(ast::Expr* expr);
    static ast::Expr*         tryEvaluateCompileTimeExpr(ast::Expr* expr);
    static ast::ConstantExpr* tryEvaluateCompileTimeUnaryExpr(ast::Expr* expr);
    static ast::ConstantExpr* tryEvaluateCompileTimeBinaryExpr(ast::Expr* expr);
    static bool               isFunctionCallCompileTimeEvaluable(
                      ast::FunctionDecl* function, size_t maxStmtAllowed = 64);
    static ast::ConstantExpr* tryEvaluateFunctionCall(ast::CallExpr* call);
    static ast::InitListExpr* regulateInitListForArray(
        ast::ArrayType* array, ast::InitListExpr* list);
};

} // namespace slime::visitor
