#include "ASTExprSimplifier.h"
#include "../utils/list.h"

namespace slime::visitor {

using namespace ast;

Expr* ASTExprSimplifier::trySimplify(Expr* expr) {
    if (expr->exprId == ExprID::InitList) {
        for (auto& e : *expr->asInitList()) { e = trySimplify(e); }
        return expr;
    }
    auto e = tryEvaluateCompileTimeExpr(expr);
    //! TODO: algebra simplification
    return !e ? expr : e;
}

Expr* ASTExprSimplifier::tryEvaluateCompileTimeExpr(Expr* expr) {
    switch (expr->exprId) {
        case ast::ExprID::DeclRef: {
            auto e = expr->asDeclRef();
            if (!e->source->specifier->isConst()) { return nullptr; }
            switch (e->source->declId) {
                case ast::DeclID::Var: {
                    auto var = e->source->asVarDecl();
                    if (auto array = var->type()->tryIntoArray()) {
                        return expr;
                    } else if (auto builtin = var->type()->tryIntoBuiltin()) {
                        return tryEvaluateCompileTimeExpr(var->initValue);
                    } else {
                        return nullptr;
                    }
                } break;
                case ast::DeclID::ParamVar: {
                    return nullptr;
                } break;
                case ast::DeclID::Function: {
                    return expr;
                }
            }
        } break;
        case ast::ExprID::Constant: {
            return expr;
        } break;
        case ast::ExprID::Unary: {
            return tryEvaluateCompileTimeUnaryExpr(expr->asUnary());
        } break;
        case ast::ExprID::Binary: {
            return tryEvaluateCompileTimeBinaryExpr(expr->asBinary());
        } break;
        case ast::ExprID::Comma: {
            return tryEvaluateCompileTimeExpr(expr->asComma()->tail()->value());
        } break;
        case ast::ExprID::Paren: {
            return tryEvaluateCompileTimeExpr(expr->asParen()->inner);
        } break;
        case ast::ExprID::Stmt: {
            assert(false && "unsupported StmtExpr");
            return nullptr;
        } break;
        case ast::ExprID::Call: {
            return tryEvaluateFunctionCall(expr->asCall());
        } break;
        case ast::ExprID::Subscript: {
            auto subscript = expr->asSubscript();
            if (subscript->lhs->exprId != ExprID::DeclRef) { return nullptr; }
            auto seqlike = tryEvaluateCompileTimeExpr(subscript->lhs);
            if (seqlike == nullptr
                || seqlike->valueType->typeId != ast::TypeID::Array) {
                return nullptr;
            }
            auto initval =
                trySimplify(
                    seqlike->asDeclRef()->source->asVarDecl()->initValue)
                    ->asInitList();
            if (initval == nullptr) { return nullptr; }
            if (initval->size() == 0) {
                auto builtin = seqlike->valueType->asBuiltin();
                if (builtin->isInt()) {
                    return ConstantExpr::createI32(0);
                } else if (builtin->isFloat()) {
                    return ConstantExpr::createF32(0.f);
                } else {
                    return nullptr;
                }
            }
            utils::ListTrait<ConstantExpr*> indices;
            while (subscript != nullptr) {
                if (auto index = tryEvaluateCompileTimeExpr(subscript->rhs)) {
                    indices.insertToHead(index->asConstant());
                } else {
                    return nullptr;
                }
                subscript = subscript->lhs->tryIntoSubscript();
            }
            //! TODO: complete element index
            return nullptr;
        } break;
        case ast::ExprID::InitList: {
            return nullptr;
        } break;
        case ast::ExprID::NoInit: {
            return nullptr;
        } break;
    }
}

ConstantExpr* ASTExprSimplifier::tryEvaluateCompileTimeUnaryExpr(Expr* expr) {
    auto e = expr->tryIntoUnary();
    if (!e) { return nullptr; }
    auto value = tryEvaluateCompileTimeExpr(e->operand)->tryIntoConstant();
    if (!value) { return nullptr; }
    switch (e->op) {
        case ast::UnaryOperator::Pos: {
            return value;
        } break;
        case ast::UnaryOperator::Neg: {
            if (auto builtin = value->valueType->tryIntoBuiltin()) {
                auto c = value->asConstant();
                if (builtin->isInt()) {
                    c->setData(-c->i32);
                } else if (builtin->isFloat()) {
                    c->setData(-c->f32);
                } else {
                    return nullptr;
                }
                return value;
            } else {
                return nullptr;
            }
        }
        case ast::UnaryOperator::Not: {
            if (auto builtin = value->valueType->tryIntoBuiltin()) {
                auto c = value->asConstant();
                if (builtin->isInt()) {
                    c->setData(!c->i32);
                } else if (builtin->isFloat()) {
                    c->setData(!c->f32);
                } else {
                    return nullptr;
                }
                return value;
            } else {
                return nullptr;
            }
        } break;
        case ast::UnaryOperator::Inv: {
            if (auto builtin = value->valueType->tryIntoBuiltin();
                builtin->isInt()) {
                value->asConstant()->setData(~value->asConstant()->i32);
                return value;
            } else {
                return nullptr;
            }
        } break;
        case ast::UnaryOperator::Paren: {
            assert(false && "ParenExpr is unreachable in UnaryExpr");
            return nullptr;
        } break;
    }
}

ConstantExpr* ASTExprSimplifier::tryEvaluateCompileTimeBinaryExpr(Expr* expr) {
    auto e = expr->tryIntoBinary();
    if (!e) { return nullptr; }
    auto lhs = tryEvaluateCompileTimeExpr(e->lhs);
    auto rhs = tryEvaluateCompileTimeExpr(e->rhs);
    if (!lhs || !rhs) { return nullptr; }
    switch (e->op) {
        case BinaryOperator::Assign: {
            return nullptr;
        } break;
        case BinaryOperator::Add:
        case BinaryOperator::Sub:
        case BinaryOperator::Mul:
        case BinaryOperator::Div:
        case BinaryOperator::LT:
        case BinaryOperator::LE:
        case BinaryOperator::GT:
        case BinaryOperator::GE:
        case BinaryOperator::EQ:
        case BinaryOperator::NE: {
            auto lbuiltin = lhs->valueType->tryIntoBuiltin();
            if (lbuiltin == nullptr || lbuiltin->isVoid()) { return nullptr; }
            auto rbuiltin = rhs->valueType->tryIntoBuiltin();
            if (rbuiltin == nullptr || rbuiltin->isVoid()) { return nullptr; }
            if (lbuiltin->isFloat() || rbuiltin->isFloat()) {
                auto lval = lbuiltin->isFloat()
                              ? lhs->asConstant()->f32
                              : static_cast<float>(lhs->asConstant()->i32);
                auto rval = rbuiltin->isFloat()
                              ? rhs->asConstant()->f32
                              : static_cast<float>(rhs->asConstant()->i32);
                switch (e->op) {
                    case BinaryOperator::Add: {
                        return ConstantExpr::createF32(lval + rval);
                    } break;
                    case BinaryOperator::Sub: {
                        return ConstantExpr::createF32(lval - rval);
                    } break;
                    case BinaryOperator::Mul: {
                        return ConstantExpr::createF32(lval * rval);
                    } break;
                    case BinaryOperator::Div: {
                        return ConstantExpr::createF32(lval / rval);
                    } break;
                    case BinaryOperator::LT: {
                        return ConstantExpr::createI32(lval < rval);
                    } break;
                    case BinaryOperator::LE: {
                        return ConstantExpr::createI32(lval <= rval);
                    } break;
                    case BinaryOperator::GT: {
                        return ConstantExpr::createI32(lval > rval);
                    } break;
                    case BinaryOperator::GE: {
                        return ConstantExpr::createI32(lval >= rval);
                    } break;
                    case BinaryOperator::EQ: {
                        return ConstantExpr::createI32(lval == rval);
                    } break;
                    case BinaryOperator::NE: {
                        return ConstantExpr::createI32(lval != rval);
                    } break;
                    default: {
                        return nullptr;
                    } break;
                }
            } else {
                auto lval = lhs->asConstant()->i32;
                auto rval = rhs->asConstant()->i32;
                switch (e->op) {
                    case BinaryOperator::Add: {
                        return ConstantExpr::createI32(lval + rval);
                    } break;
                    case BinaryOperator::Sub: {
                        return ConstantExpr::createI32(lval - rval);
                    } break;
                    case BinaryOperator::Mul: {
                        return ConstantExpr::createI32(lval * rval);
                    } break;
                    case BinaryOperator::Div: {
                        return ConstantExpr::createI32(lval / rval);
                    } break;
                    case BinaryOperator::LT: {
                        return ConstantExpr::createI32(lval < rval);
                    } break;
                    case BinaryOperator::LE: {
                        return ConstantExpr::createI32(lval <= rval);
                    } break;
                    case BinaryOperator::GT: {
                        return ConstantExpr::createI32(lval > rval);
                    } break;
                    case BinaryOperator::GE: {
                        return ConstantExpr::createI32(lval >= rval);
                    } break;
                    case BinaryOperator::EQ: {
                        return ConstantExpr::createI32(lval == rval);
                    } break;
                    case BinaryOperator::NE: {
                        return ConstantExpr::createI32(lval != rval);
                    } break;
                    default: {
                        return nullptr;
                    } break;
                }
            }
        } break;
        case BinaryOperator::Mod:
        case BinaryOperator::And:
        case BinaryOperator::Or:
        case BinaryOperator::Xor:
        case BinaryOperator::Shl:
        case BinaryOperator::Shr: {
            auto builtin = lhs->valueType->tryIntoBuiltin();
            if (builtin == nullptr || !builtin->isInt()) { return nullptr; }
            builtin = rhs->valueType->tryIntoBuiltin();
            if (builtin == nullptr || !builtin->isInt()) { return nullptr; }
            auto lval = lhs->asConstant()->i32;
            auto rval = rhs->asConstant()->i32;
            switch (e->op) {
                case BinaryOperator::Mod: {
                    return ConstantExpr::createI32(lval % rval);
                } break;
                case BinaryOperator::And: {
                    return ConstantExpr::createI32(lval & rval);
                } break;
                case BinaryOperator::Or: {
                    return ConstantExpr::createI32(lval | rval);
                } break;
                case BinaryOperator::Xor: {
                    return ConstantExpr::createI32(lval ^ rval);
                } break;
                case BinaryOperator::Shl: {
                    return ConstantExpr::createI32(lval << rval);
                } break;
                case BinaryOperator::Shr: {
                    return ConstantExpr::createI32(lval >> rval);
                } break;
                default: {
                    return nullptr;
                } break;
            }
        } break;
        case BinaryOperator::LAnd:
        case BinaryOperator::LOr: {
            UnaryExpr l(UnaryOperator::Not, lhs);
            UnaryExpr r(UnaryOperator::Not, rhs);
            auto lval = tryEvaluateCompileTimeUnaryExpr(&l)->tryIntoConstant();
            auto rval = tryEvaluateCompileTimeUnaryExpr(&r)->tryIntoConstant();
            if (!lval || !rval) { return nullptr; }
            switch (e->op) {
                case ast::BinaryOperator::LAnd: {
                    return ConstantExpr::createI32(!lval->i32 && !lval->i32);
                } break;
                case ast::BinaryOperator::LOr: {
                    return ConstantExpr::createI32(!lval->i32 || !lval->i32);
                } break;
                default: {
                    return nullptr;
                } break;
            }
        } break;
        case BinaryOperator::Comma:
        case BinaryOperator::Subscript: {
            assert(
                false
                && "CommaExpr and SubscriptExpr is unreachable in BinaryExpr");
            return nullptr;
        } break;
    }
}

bool ASTExprSimplifier::isFunctionCallCompileTimeEvaluable(
    ast::FunctionDecl* function, size_t maxStmtAllowed) {
    //! TODO: exclude non-const variable and extern function
    return false;
}

ConstantExpr* ASTExprSimplifier::tryEvaluateFunctionCall(CallExpr* call) {
    auto decl = call->callable->tryIntoDeclRef();
    if (!decl) { return nullptr; }
    auto fn = decl->source->tryIntoFunctionDecl();
    if (!fn || !fn->canBeConstExpr) { return nullptr; }
    //! TODO: execute AST evaluation machine
    return nullptr;
}

} // namespace slime::visitor
