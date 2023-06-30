#include "ASTToIRVisitor.h"

namespace slime::visitor {

ir::Type* ASTToIRVisitor::getIRTypeFromAstType(ast::Type* type) {
    switch (type->typeId) {
        case ast::TypeID::None:
        case ast::TypeID::Unresolved:
        case ast::TypeID::FunctionProto: {
            assert(false && "unsupported type conversion");
        } break;
        case ast::TypeID::Builtin: {
            auto e = type->asBuiltin();
            switch (e->type) {
                case BuiltinTypeID::Int: {
                    return ir::Type::getIntegerType();
                } break;
                case BuiltinTypeID::Float: {
                    return ir::Type::getFloatType();
                } break;
                case BuiltinTypeID::Void: {
                    return ir::Type::getVoidType();
                } break;
            }
        } break;
        case ast::TypeID::Array: {
            auto e = type->asArray();
            auto t = getIRTypeFromAstType(e->type);
            for (auto value : *e) {
                //! FIXME: check validity
                t = ir::Type::getArrayType(t, value->asConstant()->i32);
            }
            return t;
        } break;
        case ast::TypeID::IncompleteArray: {
            auto e = type->asIncompleteArray();
            auto t = getIRTypeFromAstType(e->type);
            for (auto value : *e) {
                //! FIXME: check validity
                t = ir::Type::getArrayType(t, value->asConstant()->i32);
            }
            return ir::Type::getPointerType(t);
        } break;
    }
    return nullptr;
}

static ir::Constant* evaluateCompileTimeBinaryExpr(ast::BinaryExpr* expr) {
    auto    lhs        = ASTToIRVisitor::evaluateCompileTimeAstExpr(expr->lhs);
    auto    rhs        = ASTToIRVisitor::evaluateCompileTimeAstExpr(expr->rhs);
    bool    lhsIsFloat = lhs->type->id == ir::TypeID::Float;
    bool    rhsIsFloat = rhs->type->id == ir::TypeID::Float;
    int32_t ilhs       = lhsIsFloat ? 0 : static_cast<ConstantInt*>(lhs)->value;
    int32_t irhs       = rhsIsFloat ? 0 : static_cast<ConstantInt*>(rhs)->value;
    float   flhs = lhsIsFloat ? static_cast<ConstantFloat*>(lhs)->value : 0.f;
    float   frhs = rhsIsFloat ? static_cast<ConstantFloat*>(rhs)->value : 0.f;
    if (!lhsIsFloat) { flhs = ilhs * 1.0; }
    if (!rhsIsFloat) { frhs = irhs * 1.0; }
    switch (expr->op) {
        case ast::BinaryOperator::Assign:
        case ast::BinaryOperator::Comma:
        case ast::BinaryOperator::Subscript: {
            return nullptr;
        } break;
        case ast::BinaryOperator::Add: {
            if (lhsIsFloat || rhsIsFloat) {
                return new ConstantFloat(flhs + frhs);
            } else {
                return new ConstantInt(ilhs + irhs);
            }
        } break;
        case ast::BinaryOperator::Sub: {
            if (lhsIsFloat || rhsIsFloat) {
                return new ConstantFloat(flhs - frhs);
            } else {
                return new ConstantInt(ilhs - irhs);
            }
        } break;
        case ast::BinaryOperator::Mul: {
            if (lhsIsFloat || rhsIsFloat) {
                return new ConstantFloat(flhs * frhs);
            } else {
                return new ConstantInt(ilhs * irhs);
            }
        } break;
        case ast::BinaryOperator::Div: {
            if (lhsIsFloat || rhsIsFloat) {
                return new ConstantFloat(flhs / frhs);
            } else {
                return new ConstantInt(ilhs / irhs);
            }
        } break;
        case ast::BinaryOperator::Mod: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs % irhs);
            } else {
                return nullptr;
            }
        } break;
        case ast::BinaryOperator::And: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs & irhs);
            } else {
                return nullptr;
            }
        } break;
        case ast::BinaryOperator::Or: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs | irhs);
            } else {
                return nullptr;
            }
        } break;
        case ast::BinaryOperator::Xor: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs ^ irhs);
            } else {
                return nullptr;
            }
        } break;
        case ast::BinaryOperator::LAnd: {
            auto lhs = lhsIsFloat ? !!flhs : !!ilhs;
            auto rhs = rhsIsFloat ? !!frhs : !!irhs;
            return new ConstantInt(lhs && rhs);
        } break;
        case ast::BinaryOperator::LOr: {
            auto lhs = lhsIsFloat ? !!flhs : !!ilhs;
            auto rhs = rhsIsFloat ? !!frhs : !!irhs;
            return new ConstantInt(lhs || rhs);
        } break;
        case ast::BinaryOperator::LT: {
            return new ConstantInt(
                (lhsIsFloat || rhsIsFloat) ? flhs < frhs : ilhs < irhs);
        } break;
        case ast::BinaryOperator::LE: {
            return new ConstantInt(
                (lhsIsFloat || rhsIsFloat) ? flhs <= frhs : ilhs <= irhs);
        } break;
        case ast::BinaryOperator::GT: {
            return new ConstantInt(
                (lhsIsFloat || rhsIsFloat) ? flhs > frhs : ilhs > irhs);
        } break;
        case ast::BinaryOperator::GE: {
            return new ConstantInt(
                (lhsIsFloat || rhsIsFloat) ? flhs >= frhs : ilhs >= irhs);
        } break;
        case ast::BinaryOperator::EQ: {
            return new ConstantInt(flhs == frhs);
        } break;
        case ast::BinaryOperator::NE: {
            return new ConstantInt(flhs != frhs);
        } break;
        case ast::BinaryOperator::Shl: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs << irhs);
            } else {
                return nullptr;
            }
        } break;
        case ast::BinaryOperator::Shr: {
            if (!lhsIsFloat && !rhsIsFloat) {
                return new ConstantInt(ilhs >> irhs);
            } else {
                return nullptr;
            }
        } break;
    }
}

ir::Constant* ASTToIRVisitor::evaluateCompileTimeAstExpr(ast::Expr* expr) {
    switch (expr->exprId) {
        case ast::ExprID::DeclRef: {
            auto source = expr->asDeclRef()->source;
            if (!source->specifier->isConst()) { return nullptr; }
            return evaluateCompileTimeAstExpr(source->asVarDecl()->initValue);
        } break;
        case ast::ExprID::Constant: {
            auto e = expr->asConstant();
            if (e->type == ConstantType::i32) {
                return new ConstantInt(e->i32);
            } else if (e->type == ConstantType::f32) {
                return new ConstantFloat(e->f32);
            }
            return nullptr;
        } break;
        case ast::ExprID::Unary: {
            auto e     = expr->asUnary();
            auto value = evaluateCompileTimeAstExpr(e->operand);
            switch (e->op) {
                case ast::UnaryOperator::Pos:
                case ast::UnaryOperator::Paren: {
                    return value;
                } break;
                case ast::UnaryOperator::Neg: {
                    if (value->type->id == ir::TypeID::Integer) {
                        auto e   = static_cast<ConstantInt*>(value);
                        e->value = -e->value;
                    } else if (value->type->id == ir::TypeID::Float) {
                        auto e   = static_cast<ConstantFloat*>(value);
                        e->value = -e->value;
                    }
                    return value;
                } break;
                case ast::UnaryOperator::Not: {
                    bool isTrue = false;
                    if (value->type->id == ir::TypeID::Integer) {
                        auto e = static_cast<ConstantInt*>(value);
                        isTrue = e->value == 0;
                    } else if (value->type->id == ir::TypeID::Float) {
                        auto e = static_cast<ConstantFloat*>(value);
                        isTrue = e->value == 0.f;
                    }
                    return new ConstantInt(isTrue);
                } break;
                case ast::UnaryOperator::Inv: {
                    assert(value->type->id == ir::TypeID::Integer);
                    auto e   = static_cast<ConstantInt*>(value);
                    e->value = ~e->value;
                    return value;
                } break;
            }
        } break;
        case ast::ExprID::Binary: {
            return evaluateCompileTimeBinaryExpr(expr->asBinary());
        } break;
        case ast::ExprID::Comma: {
            return evaluateCompileTimeAstExpr(expr->asComma()->tail()->value());
        } break;
        case ast::ExprID::Paren: {
            return evaluateCompileTimeAstExpr(expr->asParen()->inner);
        } break;
        case ast::ExprID::Stmt: {
            assert(false && "StmtExpr is not supported");
            return nullptr;
        } break;
        case ast::ExprID::Call: {
            assert(false && "function call is not compile-time");
            return nullptr;
        } break;
        case ast::ExprID::Subscript: {
            assert(false && "array-subscript is not compile-time");
            return nullptr;
        } break;
        case ast::ExprID::InitList: {
            assert(false && "init-list constant requires qualifed type");
            return nullptr;
        } break;
        case ast::ExprID::NoInit: {
            return nullptr;
        } break;
    }
}

ir::Value* ASTToIRVisitor::makeBooleanCondition(ir::Value* condition) {
    if (condition->type->id == ir::TypeID::Integer) {
        condition = new ICmpInst(
            ComparePredicationType::NE, condition, new ConstantInt(0));
    } else if (condition->type->id == ir::TypeID::Float) {
        condition = new FCmpInst(
            ComparePredicationType::UNE, condition, new ConstantFloat(0.f));
    }
    return condition;
}

BasicBlock* ASTToIRVisitor::createLoop(
    void*       hint,
    BasicBlock* entry,
    Expr*       condition,
    Stmt*       body,
    bool        isLoopBodyFirst) {
    auto            function = entry->parent;
    LoopDescription desc;
    desc.branchLoop = new BasicBlock(function);
    desc.branchCond = new BasicBlock(function);
    desc.branchExit = new BasicBlock(function);
    function->blocks.push_back(desc.branchLoop);
    function->blocks.push_back(desc.branchCond);
    function->blocks.push_back(desc.branchExit);
    if (isLoopBodyFirst) {
        entry->insertToTail(new BranchInst(desc.branchLoop));
    } else {
        entry->insertToTail(new BranchInst(desc.branchCond));
    }
    auto block = visit(function, desc.branchLoop, body);
    block->insertToTail(new BranchInst(desc.branchCond));
    auto cond   = visit(function, desc.branchCond, condition->asExprStmt());
    auto i1cond = makeBooleanCondition(cond);
    if (i1cond != cond) { desc.branchCond->insertToTail(i1cond); }
    desc.branchCond->insertToTail(
        new BranchInst(i1cond, desc.branchLoop, desc.branchExit));
    loopMap_.insert_or_assign(hint, desc);
    return desc.branchExit;
}

} // namespace slime::visitor
