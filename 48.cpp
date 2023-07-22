#include "49.h"

#include "2.h"
#include "10.h"
#include "78.h"
#include <iostream>
#include <stdlib.h>

namespace slime {

using namespace ast;
using visitor::ASTExprSimplifier;

void Diagnosis::expectAlwaysFalse(std::string_view message) {
    std::cerr << message << "\n";
}

void Diagnosis::assertAlwaysFalse(std::string_view message) {
    std::cerr << message << "\n";
    exit(-1);
}

bool Diagnosis::checkTypeConvertible(Type *from, Type *to) {
    if (auto builtin = to->tryIntoBuiltin()) {
        auto type = from->tryIntoBuiltin();
        return !(builtin->isVoid() || type->isVoid());
    }
    if (auto ptr = to->tryIntoIncompleteArray()) {
        auto array = from->tryIntoArray();
        if (ptr->size() + 1 != array->size()) { return false; }
        auto it = array->begin();
        for (auto e : *ptr) {
            auto u = ASTExprSimplifier::tryEvaluateCompileTimeExpr(*++it);
            auto v = ASTExprSimplifier::tryEvaluateCompileTimeExpr(e);
            if (!u || !v || u->asConstant()->i32 != v->asConstant()->i32) {
                return false;
            }
        }
        return true;
    }
    //! TODO: check function proto conversion
    return false;
}

void Diagnosis::assertConditionalExpression(Expr *expr) {
    assert(expr != nullptr);
    if (auto builtin = expr->implicitValueType()->tryIntoBuiltin();
        !builtin || builtin->isVoid()) {
        assertAlwaysFalse("value is not contextually convertible to bool");
    }
}

void Diagnosis::assertWellFormedReturnStatement(
    ast::ReturnStmt *stmt, ast::FunctionProtoType *proto) {
    assert(stmt != nullptr);
    assert(proto != nullptr);
    assert(proto->returnType->tryIntoBuiltin() != nullptr);
    auto type     = stmt->implicitValueType()->tryIntoBuiltin();
    auto expected = proto->returnType->asBuiltin();
    if (type == nullptr) {
        assertAlwaysFalse("invalid return type");
    } else if (type->isVoid() && !expected->isVoid()) {
        assertAlwaysFalse("missing return value for non-void function");
    } else if (!type->isVoid() && expected->isVoid()) {
        assertAlwaysFalse("void function should not return a value");
    }
}

void Diagnosis::assertWellFormedBreakStatement(BreakStmt *stmt) {
    assert(stmt != nullptr);
    auto loop = stmt->parent;
    if (!loop || !loop->tryIntoWhileStmt()) {
        assertAlwaysFalse("break appears out of a loop");
    }
}

void Diagnosis::assertWellFormedContinueStatement(ContinueStmt *stmt) {
    assert(stmt != nullptr);
    auto loop = stmt->parent;
    if (!loop || !loop->tryIntoWhileStmt()) {
        assertAlwaysFalse("continue appears out of a loop");
    }
}

void Diagnosis::assertWellFormedForStatement(ast::ForStmt *stmt) {
    assert(stmt != nullptr);
    bool isTypeOk = stmt->init->tryIntoNullStmt()
                 || stmt->init->tryIntoExprStmt()
                 || stmt->init->tryIntoDeclStmt();
    isTypeOk = isTypeOk
            && (stmt->condition->tryIntoNullStmt()
                || stmt->condition->tryIntoExprStmt());
    isTypeOk = isTypeOk && stmt->increment->tryIntoNullStmt()
            || stmt->increment->tryIntoExprStmt();
    if (!isTypeOk) { assertAlwaysFalse("ill-formed for statement"); }
    if (auto condition = stmt->condition->tryIntoExprStmt()) {
        assertConditionalExpression(condition->unwrap());
    }
}

void Diagnosis::assertWellFormedCommaExpression(Expr *expr) {
    assert(expr != nullptr);
    if (!checkNotNull(expr)) {
        assertAlwaysFalse("expect expression before comma");
    }
}

void Diagnosis::assertNoAssignToConstQualifiedValue(
    Expr *value, BinaryOperator op) {
    assert(value != nullptr);
    //! FIXME: value can be non-decl lvalue
    if (op == BinaryOperator::Assign) {
        if (auto decl = value->tryIntoDeclRef()) {
            const auto &var = decl->source;
            if (var->specifier->isConst() && op == BinaryOperator::Assign) {
                assertAlwaysFalse("assign to const-qualified variable");
            }
        }
    }
}

void Diagnosis::assertWellFormedArrayType(ArrayType *type) {
    assert(type != nullptr);
    bool pass = type->tryIntoIncompleteArray()
             || type->tryIntoArray() && type->size() > 0;
    for (auto e : *type) {
        if (!pass) { break; }
        auto expr = ASTExprSimplifier::tryEvaluateCompileTimeExpr(e);
        pass      = expr != nullptr && expr->tryIntoConstant() != nullptr
            && expr->asConstant()->type == ConstantType::i32
            && expr->asConstant()->i32 > 0;
    }
    if (!pass) { assertAlwaysFalse("mal-formed array type"); }
}

void Diagnosis::assertSubscriptableValue(Expr *value) {
    assert(value != nullptr);
    assertTrue(value->valueType->isArrayLike(), "subscript invalid value");
}

void Diagnosis::assertCallable(Expr *callable) {
    assert(callable != nullptr);
    if (!callable->valueType->tryIntoFunctionProto()) {
        assertAlwaysFalse("call to uncallable object");
    }
}

void Diagnosis::assertWellFormedFunctionCall(
    FunctionProtoType *proto, ExprList *arguments) {
    assert(proto != nullptr);
    assert(arguments != nullptr);
    bool matched = proto->size() == arguments->size();
    if (matched) {
        auto it = arguments->begin();
        for (auto type : *proto) {
            auto valueType = (*it)->valueType;
            switch (type->typeId) {
                case TypeID::None:
                case TypeID::Unresolved:
                case TypeID::Array: {
                    matched = false;
                } break;
                case TypeID::Builtin: {
                    matched =
                        !(valueType->asBuiltin()->isVoid()
                          || type->asBuiltin()->isVoid());
                } break;
                case TypeID::IncompleteArray:
                case TypeID::FunctionProto: {
                } break;
            }
            if (!matched) { break; }
            ++it;
        }
    }
    if (!matched) {
        assertAlwaysFalse("mismatched arguments for the function call");
    }
}

} // namespace slime