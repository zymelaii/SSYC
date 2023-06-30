#pragma once

#include "../ast/ast.h"
#include "../ast/type.h"
#include "../ast/expr.h"
#include "../ir/minimal.h"
#include "../utils/list.h"

#include <map>

namespace slime::visitor {

using namespace slime::ast;
using namespace slime::ir;

using TopLevelIRObjectList = slime::utils::ListTrait<GlobalObject*>;

class ASTToIRVisitor : public TopLevelIRObjectList {
public:
    static ir::Type*     getIRTypeFromAstType(ast::Type* type);
    static ir::Constant* evaluateCompileTimeAstExpr(ast::Expr* expr);
    static ir::Value*    makeBooleanCondition(ir::Value* condition);

    void visit(TranslationUnit* e) {
        for (auto decl : *e) {
            switch (decl->declId) {
                case DeclID::Var: {
                    insertToTail(static_cast<GlobalVariable*>(
                        visit(nullptr, decl->asVarDecl())));
                } break;
                case DeclID::Function: {
                    insertToTail(visit(decl->asFunctionDecl()));
                } break;
                default: {
                    assert(false && "unexpected ast node type");
                } break;
            }
        }
    }

protected:
    Value* visit(BasicBlock* block, VarDecl* e) {
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
            //! NOTE: always let var-decl be at the front
            block->parent->blocks.front()->insertToHead(alloca);
            if (e->initValue != nullptr) {
                auto instr = new StoreInst(
                    evaluateCompileTimeAstExpr(e->initValue), alloca);
                block->insertToTail(instr);
            }
            return alloca;
        }
    }

    Function* visit(FunctionDecl* e) {
        std::vector<ir::Type*>  paramTypes;
        std::vector<Parameter*> params;
        for (auto param : *e) {
            auto type = getIRTypeFromAstType(param->type());
            paramTypes.push_back(type);
            params.push_back(new Parameter(type, nullptr, param->name));
        }
        auto proto = ir::Type::getFunctionType(
            getIRTypeFromAstType(e->proto()->returnType), paramTypes);
        auto function   = new Function(proto, e->name);
        int  paramIndex = 0;
        for (auto& e : params) {
            e->parent = function;
            e->index  = paramIndex++;
        }
        auto entry = new BasicBlock(function);
        function->blocks.push_back(entry);
        visit(function, entry, e->body);
        loopMap_.clear();
        return function;
    }

    BasicBlock* visit(Function* fn, BasicBlock* block, Stmt* e) {
        switch (e->stmtId) {
            case StmtID::Null: {
                //! NOTE: nothing to do
            } break;
            case StmtID::Decl: {
                visit(fn, block, e->asDeclStmt());
            } break;
            case StmtID::Expr: {
                visit(fn, block, e->asExprStmt());
            } break;
            case StmtID::Compound: {
                block = visit(fn, block, e->asCompoundStmt());
            } break;
            case StmtID::If: {
                block = visit(fn, block, e->asIfStmt());
            } break;
            case StmtID::Do: {
                block = visit(fn, block, e->asDoStmt());
            } break;
            case StmtID::While: {
                block = visit(fn, block, e->asWhileStmt());
            } break;
            case StmtID::Break: {
                visit(fn, block, e->asBreakStmt());
            } break;
            case StmtID::Continue: {
                visit(fn, block, e->asContinueStmt());
            } break;
            case StmtID::Return: {
                visit(fn, block, e->asReturnStmt());
            } break;
        }
        return block;
    }

    void visit(Function* fn, BasicBlock* block, DeclStmt* e) {
        for (auto decl : *e) { visit(block, decl); }
    }

    Value* visit(Function* fn, BasicBlock* block, ExprStmt* e) {
        return visit(block, e->unwrap());
    }

    BasicBlock* visit(Function* fn, BasicBlock* block, CompoundStmt* e) {
        for (auto stmt : *e) { block = visit(fn, block, stmt); }
        return block;
    }

    BasicBlock* visit(Function* fn, BasicBlock* block, IfStmt* e) {
        auto condition   = visit(fn, block, e->condition->asExprStmt());
        auto i1condition = makeBooleanCondition(condition);
        if (i1condition != condition) { block->insertToTail(i1condition); }
        auto branchExit = new BasicBlock(fn);
        auto branchIf   = new BasicBlock(fn);
        fn->blocks.push_back(branchIf);
        visit(fn, branchIf, e->branchIf);
        branchIf->insertToTail(new BranchInst(branchExit));
        if (e->branchElse != nullptr) {
            auto branchElse = new BasicBlock(fn);
            fn->blocks.push_back(branchElse);
            visit(fn, branchElse, e->branchElse);
            branchElse->insertToTail(new BranchInst(branchExit));
            block->insertToTail(
                new BranchInst(i1condition, branchIf, branchElse));
        } else {
            block->insertToTail(
                new BranchInst(i1condition, branchIf, branchExit));
        }
        fn->blocks.push_back(branchExit);
        return branchExit;
    }

    BasicBlock* visit(Function* fn, BasicBlock* block, DoStmt* e) {
        return createLoop(
            e, block, e->condition->asExprStmt()->unwrap(), e->loopBody, true);
    }

    BasicBlock* visit(Function* fn, BasicBlock* block, WhileStmt* e) {
        return createLoop(
            e, block, e->condition->asExprStmt()->unwrap(), e->loopBody, false);
    }

    void visit(Function* fn, BasicBlock* block, BreakStmt* e) {
        assert(e->parent != nullptr && "break not in a loop");
        auto& desc = loopMap_[e->parent];
        block->insertToTail(new BranchInst(desc.branchExit));
    }

    void visit(Function* fn, BasicBlock* block, ContinueStmt* e) {
        assert(e->parent != nullptr && "continue not in a loop");
        auto& desc = loopMap_[e->parent];
        block->insertToTail(new BranchInst(desc.branchCond));
    }

    void visit(Function* fn, BasicBlock* block, ReturnStmt* e) {
        block->insertToTail(new ReturnInst(visit(fn, block, e->returnValue)));
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
        auto         type  = e->operand->valueType->asBuiltin();
        Value*       value = visit(block, e->operand);
        Instruction* instr = nullptr;
        switch (e->op) {
            case UnaryOperator::Paren:
            case UnaryOperator::Pos: {
                return value;
            } break;
            case UnaryOperator::Neg: {
                assert(!type->isVoid());
                if (type->isInt()) {
                    instr = new BinaryOperatorInst(
                        InstructionID::Sub, new ConstantInt(0), value);
                } else if (type->isFloat()) {
                    instr = new BinaryOperatorInst(
                        InstructionID::FSub, new ConstantFloat(0.f), value);
                }
                assert(false && "invalid void type");
            } break;
            case UnaryOperator::Not: {
                assert(!type->isVoid());
                if (type->isInt()) {
                    instr = new ICmpInst(
                        ComparePredicationType::EQ, value, new ConstantInt(0));
                } else if (type->isFloat()) {
                    instr = new ICmpInst(
                        ComparePredicationType::EQ,
                        value,
                        new ConstantFloat(0.f));
                }
                assert(false && "invalid void type");
            } break;
            case UnaryOperator::Inv: {
                assert(type->isInt());
                instr = new BinaryOperatorInst(
                    InstructionID::Xor, value, new ConstantInt(-1));
            } break;
        }
        assert(instr != nullptr);
        block->insertToTail(instr);
        return instr;
    }

    Value* visit(BasicBlock* block, BinaryExpr* e) {
        auto         lhs   = visit(block, e->lhs);
        auto         rhs   = visit(block, e->rhs);
        auto         type  = e->implicitValueType()->asBuiltin();
        Instruction* instr = nullptr;
        switch (e->op) {
            case BinaryOperator::Assign: {
                //! FIXME: add conversion between i32 and f32
                auto store = new StoreInst(rhs, lhs);
                block->insertToTail(store);
                instr = new LoadInst(rhs->type, lhs);
            } break;
            case BinaryOperator::Add: {
                instr = new BinaryOperatorInst(InstructionID::Add, lhs, rhs);
            } break;
            case BinaryOperator::Sub: {
                instr = new BinaryOperatorInst(InstructionID::Sub, lhs, rhs);
            } break;
            case BinaryOperator::Mul: {
                instr = new BinaryOperatorInst(InstructionID::Mul, lhs, rhs);
            } break;
            case BinaryOperator::Div: {
                instr = new BinaryOperatorInst(InstructionID::SDiv, lhs, rhs);
            } break;
            case BinaryOperator::Mod: {
                instr = new BinaryOperatorInst(InstructionID::SRem, lhs, rhs);
            } break;
            case BinaryOperator::And: {
                instr = new BinaryOperatorInst(InstructionID::And, lhs, rhs);
            } break;
            case BinaryOperator::Or: {
                instr = new BinaryOperatorInst(InstructionID::Or, lhs, rhs);
            } break;
            case BinaryOperator::Xor: {
                instr = new BinaryOperatorInst(InstructionID::Xor, lhs, rhs);
            } break;
            case BinaryOperator::LAnd: {
                //! TODO: add cmp and and
            } break;
            case BinaryOperator::LOr: {
                //! TODO: add cmp and or
            } break;
            case BinaryOperator::LT: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::SLT, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::OLT, lhs, rhs);
                }
            } break;
            case BinaryOperator::LE: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::SLE, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::OLE, lhs, rhs);
                }
            } break;
            case BinaryOperator::GT: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::SGT, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::OGT, lhs, rhs);
                }
            } break;
            case BinaryOperator::GE: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::SGE, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::OGE, lhs, rhs);
                }
            } break;
            case BinaryOperator::EQ: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::EQ, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::OEQ, lhs, rhs);
                }
            } break;
            case BinaryOperator::NE: {
                if (type->isInt()) {
                    instr = new ICmpInst(ComparePredicationType::NE, lhs, rhs);
                } else if (type->isFloat()) {
                    instr = new FCmpInst(ComparePredicationType::ONE, lhs, rhs);
                }
            } break;
            case BinaryOperator::Shl: {
                assert(type->isInt());
                instr = new BinaryOperatorInst(InstructionID::Shl, lhs, rhs);
            } break;
            case BinaryOperator::Shr: {
                assert(type->isInt());
                instr = new BinaryOperatorInst(InstructionID::AShr, lhs, rhs);
            } break;
            case BinaryOperator::Comma: {
                assert(false && "comma expr always appears as CommaExpr");
            } break;
            case BinaryOperator::Subscript: {
                assert(false && "comma expr always appears as SubscriptExpr");
            } break;
        }
        assert(instr != nullptr);
        block->insertToTail(instr);
        return instr;
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
        auto fn = e->asDeclRef()->source->asFunctionDecl()->name;
        for (auto value : *this) {
            if (value->type->id == ir::TypeID::Function && value->name == fn) {
                std::vector<Value*> argList;
                auto                arg = e->argList.head();
                while (arg != nullptr) {
                    argList.push_back(visit(block, arg->value()));
                    arg = arg->next();
                }
                auto call =
                    new CallInst(static_cast<Function*>(value), argList);
                block->insertToTail(call);
                return call;
            }
        }
        return nullptr;
    }

    Value* visit(BasicBlock* block, SubscriptExpr* e) {
        auto value =
            new GetElementPtrInst(visit(block, e->lhs), visit(block, e->rhs));
        block->insertToTail(value);
        return value;
    }

private:
    BasicBlock* createLoop(
        void*       hint,
        BasicBlock* entry,
        Expr*       condition,
        Stmt*       body,
        bool        isLoopBodyFirst = false);

private:
    struct LoopDescription {
        BasicBlock* branchCond;
        BasicBlock* branchLoop;
        BasicBlock* branchExit;
    };

    std::map<void*, LoopDescription>   loopMap_;
    std::map<std::string_view, Value*> symbolTable_;
};

} // namespace slime::visitor
