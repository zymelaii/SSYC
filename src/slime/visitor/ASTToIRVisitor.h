#pragma once

#include "../ast/ast.h"
#include "../ast/type.h"
#include "../ast/expr.h"
#include "../ir/minimal.h"
#include "../utils/list.h"

namespace slime::visitor {

using namespace slime::ast;
using namespace slime::ir;

using TopLevelIRObjectList = slime::utils::ListTrait<GlobalObject*>;

class ASTToIRVisitor : public TopLevelIRObjectList {
public:
    static ir::Type*     getIRTypeFromAstType(ast::Type* type);
    static ir::Constant* evaluateCompileTimeAstExpr(ast::Expr* expr);

    void visit(TranslationUnit* e) {
        auto node = e->head();
        while (node != nullptr) {
            auto decl = node->value();
            switch (decl->declId) {
                case DeclID::Var: {
                    insertToTail(
                        static_cast<GlobalVariable*>(visit(decl->asVarDecl())));
                } break;
                case DeclID::Function: {
                    insertToTail(visit(decl->asFunctionDecl()));
                } break;
                default: {
                    assert(false && "unexpected ast node type");
                } break;
            }
            node = node->next();
        }
    }

protected:
    Value* visit(VarDecl* e) {
        auto type = getIRTypeFromAstType(e->type());
        if (e->scope.depth == 0) {
            assert(!e->name.empty());
            //! TODO: make init-list
            return new GlobalVariable(
                type,
                e->name,
                e->specifier->isConst(),
                evaluateCompileTimeAstExpr(e->initValue));
        } else {
            auto alloca = new AllocaInst(type);
            if (e->initValue != nullptr) {
                return new StoreInst(
                    evaluateCompileTimeAstExpr(e->initValue), alloca);
            }
            return alloca;
        }
    }

    Function* visit(FunctionDecl* e) {
        std::vector<ir::Type*>  paramTypes;
        std::vector<Parameter*> params;
        auto                    param = e->head();
        while (param != nullptr) {
            auto type = getIRTypeFromAstType(param->value()->type());
            paramTypes.push_back(type);
            params.push_back(
                new Parameter(type, nullptr, param->value()->name));
            param = param->next();
        }
        auto proto = ir::Type::getFunctionType(
            getIRTypeFromAstType(e->proto()->returnType), paramTypes);
        auto function   = new Function(proto, e->name);
        int  paramIndex = 0;
        for (auto& e : params) {
            e->parent = function;
            e->index  = paramIndex++;
        }
        //! TODO: build BasicBlocks
        return function;
    }

    Value* visit(BasicBlock* block, Expr* e) {
        switch (e->exprId) {
            case ExprID::DeclRef: {
                return visit(block, e->asDeclRef());
            } break;
            case ExprID::Constant: {
                return visit(block, e->asConstant());
            } break;
            case ExprID::Unary: {
                return visit(block, e->asUnary());
            } break;
            case ExprID::Binary: {
                return visit(block, e->asBinary());
            } break;
            case ExprID::Comma: {
                return visit(block, e->asComma());
            } break;
            case ExprID::Paren: {
                return visit(block, e->asParen());
            } break;
            case ExprID::Stmt: {
                assert(false && "unsupported statement expression");
            } break;
            case ExprID::Call: {
                return visit(block, e->asCall());
            } break;
            case ExprID::Subscript: {
                return visit(block, e->asSubscript());
            } break;
            case ExprID::InitList: {
                assert(false && "single init-list expression cannot exist");
            } break;
            case ExprID::NoInit: {
                assert(false && "single no-init expression cannot exist");
            } break;
        }
        return nullptr;
    }

    Value* visit(BasicBlock* block, DeclRefExpr* e) {
        //! TODO: decide to use ptr or value
        return nullptr;
    }

    Value* visit(BasicBlock* block, ConstantExpr* e) {
        if (e->type == ConstantType::i32) {
            return new ConstantInt(e->i32);
        } else if (e->type == ConstantType::f32) {
            return new ConstantFloat(e->f32);
        }
        assert(false && "unexpected error");
        return nullptr;
    }

    Value* visit(BasicBlock* block, UnaryExpr* e) {
        auto   type  = e->operand->valueType->asBuiltin();
        Value* value = visit(block, e->operand);
        switch (e->op) {
            case UnaryOperator::Paren:
            case UnaryOperator::Pos: {
                return value;
            } break;
            case UnaryOperator::Neg: {
                assert(!type->isVoid());
                if (type->isInt()) {
                    return new BinaryOperatorInst(
                        InstructionID::Sub, new ConstantInt(0), value);
                } else if (type->isFloat()) {
                    return new BinaryOperatorInst(
                        InstructionID::FSub, new ConstantFloat(0.f), value);
                }
                assert(false && "invalid void type");
            } break;
            case UnaryOperator::Not: {
                assert(!type->isVoid());
                if (type->isInt()) {
                    return new ICmpInst(
                        ComparePredicationType::EQ, value, new ConstantInt(0));
                } else if (type->isFloat()) {
                    return new ICmpInst(
                        ComparePredicationType::EQ,
                        value,
                        new ConstantFloat(0.f));
                }
                assert(false && "invalid void type");
            } break;
            case UnaryOperator::Inv: {
                assert(type->isInt());
                return new BinaryOperatorInst(
                    InstructionID::Xor, value, new ConstantInt(-1));
            } break;
        }
        return nullptr;
    }

    Value* visit(BasicBlock* block, BinaryExpr* e) {
        auto lhs  = visit(block, e->lhs);
        auto rhs  = visit(block, e->rhs);
        auto type = e->implicitValueType()->asBuiltin();
        switch (e->op) {
            case BinaryOperator::Assign: {
                //! FIXME: add conversion between i32 and f32
                auto store = new StoreInst(rhs, lhs);
                //! TODO: return load inst
            } break;
            case BinaryOperator::Add: {
                return new BinaryOperatorInst(InstructionID::Add, lhs, rhs);
            } break;
            case BinaryOperator::Sub: {
                return new BinaryOperatorInst(InstructionID::Sub, lhs, rhs);
            } break;
            case BinaryOperator::Mul: {
                return new BinaryOperatorInst(InstructionID::Mul, lhs, rhs);
            } break;
            case BinaryOperator::Div: {
                return new BinaryOperatorInst(InstructionID::SDiv, lhs, rhs);
            } break;
            case BinaryOperator::Mod: {
                return new BinaryOperatorInst(InstructionID::SRem, lhs, rhs);
            } break;
            case BinaryOperator::And: {
                return new BinaryOperatorInst(InstructionID::And, lhs, rhs);
            } break;
            case BinaryOperator::Or: {
                return new BinaryOperatorInst(InstructionID::Or, lhs, rhs);
            } break;
            case BinaryOperator::Xor: {
                return new BinaryOperatorInst(InstructionID::Xor, lhs, rhs);
            } break;
            case BinaryOperator::LAnd: {
                //! TODO: add cmp and and
            } break;
            case BinaryOperator::LOr: {
                //! TODO: add cmp and or
            } break;
            case BinaryOperator::LT: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::SLT, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::OLT, lhs, rhs);
                }
            } break;
            case BinaryOperator::LE: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::SLE, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::OLE, lhs, rhs);
                }
            } break;
            case BinaryOperator::GT: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::SGT, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::OGT, lhs, rhs);
                }
            } break;
            case BinaryOperator::GE: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::SGE, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::OGE, lhs, rhs);
                }
            } break;
            case BinaryOperator::EQ: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::EQ, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::OEQ, lhs, rhs);
                }
            } break;
            case BinaryOperator::NE: {
                if (type->isInt()) {
                    return new ICmpInst(ComparePredicationType::NE, lhs, rhs);
                } else if (type->isFloat()) {
                    return new FCmpInst(ComparePredicationType::ONE, lhs, rhs);
                }
            } break;
            case BinaryOperator::Shl: {
                assert(type->isInt());
                return new BinaryOperatorInst(InstructionID::Shl, lhs, rhs);
            } break;
            case BinaryOperator::Shr: {
                assert(type->isInt());
                return new BinaryOperatorInst(InstructionID::AShr, lhs, rhs);
            } break;
            case BinaryOperator::Comma: {
                assert(false && "comma expr always appears as CommaExpr");
            } break;
            case BinaryOperator::Subscript: {
                assert(false && "comma expr always appears as SubscriptExpr");
            } break;
        }
        return nullptr;
    }

    Value* visit(BasicBlock* block, CommaExpr* e) {
        auto node = e->head();
        while (node != e->tail()) {
            if (!node->value()->isNoEffectExpr()) {
                visit(block, node->value());
            }
            node = node->next();
        }
        return visit(block, e->tail()->value());
    }

    Value* visit(BasicBlock* block, ParenExpr* e) {
        return visit(block, e->inner);
    }

    Value* visit(BasicBlock* block, CallExpr* e) {
        auto fn   = e->asDeclRef()->source->asFunctionDecl()->name;
        auto node = head();
        while (node != nullptr) {
            auto v = node->value();
            if (v->type->id == ir::TypeID::Function && v->name == fn) {
                std::vector<Value*> argList;
                auto                arg = e->argList.head();
                while (arg != nullptr) {
                    argList.push_back(visit(block, arg->value()));
                    arg = arg->next();
                }
                return new CallInst(static_cast<Function*>(v), argList);
            }
            node = node->next();
        }
        return nullptr;
    }

    Value* visit(BasicBlock* block, SubscriptExpr* e) {
        return new GetElementPtrInst(
            visit(block, e->lhs), visit(block, e->rhs));
    }
};

} // namespace slime::visitor
