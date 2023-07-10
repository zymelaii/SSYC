#include "ASTToIRTranslator.h"

#include <slime/ir/user.h>
#include <slime/ir/instruction.h>
#include <slime/pass/ValueNumbering.h>
#include <slime/pass/DeadCodeElimination.h>
#include <assert.h>

namespace slime::visitor {

using namespace ir;
using namespace ast;

ir::Type *ASTToIRTranslator::getCompatibleIRType(ast::Type *type) {
    switch (type->typeId) {
        case ast::TypeID::None:
        case ast::TypeID::Unresolved: {
            return nullptr;
        } break;
        case ast::TypeID::Builtin: {
            auto buitin = type->asBuiltin();
            switch (buitin->type) {
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
            auto array = type->asArray();
            auto type  = getCompatibleIRType(array->type);
            auto it    = array->rbegin();
            auto end   = array->rend();
            while (it != end) {
                type =
                    ir::Type::createArrayType(type, (*it)->asConstant()->i32);
                ++it;
            }
            return type;
        } break;
        case ast::TypeID::IncompleteArray: {
            auto array = type->asIncompleteArray();
            auto type  = getCompatibleIRType(array->type);
            auto it    = array->rbegin();
            auto end   = array->rend();
            while (it != end) {
                type =
                    ir::Type::createArrayType(type, (*it)->asConstant()->i32);
                ++it;
            }
            return ir::Type::createPointerType(type);
        } break;
        case ast::TypeID::FunctionProto: {
            auto proto = type->asFunctionProto();
            auto type  = getCompatibleIRType(proto->returnType);
            std::vector<ir::Type *> params;
            for (auto param : *proto) {
                params.push_back(getCompatibleIRType(param));
            }
            return ir::Type::createFunctionType(type, params);
        } break;
    }
}

Value *ASTToIRTranslator::getCompatibleIRValue(
    Expr *value, ir::Type *desired, Module *module) {
    //! NOTE: assume the value is already regulated
    if (value->tryIntoNoInit() && desired != nullptr) {
        switch (desired->kind()) {
            case TypeKind::Integer: {
                return !module ? ConstantData::createI32(0)
                               : module->createI32(0);
            } break;
            case TypeKind::Float: {
                return !module ? ConstantData::createF32(0.f)
                               : module->createF32(0.f);
            } break;
            case TypeKind::Array: {
                return ConstantArray::create(desired->asArrayType());
            } break;
            default: {
            } break;
        }
    }
    if (auto v = value->tryIntoConstant();
        v && (!desired || desired->isBuiltinType())) {
        if (!desired) {
            switch (v->type) {
                case ConstantType::i32: {
                    return !module ? ConstantData::createI32(v->i32)
                                   : module->createI32(v->i32);
                } break;
                case ConstantType::f32: {
                    return !module ? ConstantData::createF32(v->f32)
                                   : module->createF32(v->f32);
                } break;
            }
        } else if (desired->isInteger()) {
            auto value = v->type == ConstantType::i32
                           ? v->i32
                           : static_cast<int32_t>(v->f32);
            return !module ? ConstantData::createI32(value)
                           : module->createI32(value);
        } else {
            auto value = v->type == ConstantType::f32
                           ? v->f32
                           : static_cast<float>(v->i32);
            return !module ? ConstantData::createF32(value)
                           : module->createF32(value);
        }
    }
    if (auto v = value->tryIntoInitList(); v && desired != nullptr) {
        auto  value       = ConstantArray::create(desired->asArrayType());
        auto &array       = *value;
        auto  type        = desired->tryIntoArrayType();
        auto  elementType = type->elementType();
        int   index       = 0;
        for (auto e : *v) {
            auto element   = getCompatibleIRValue(e, elementType, module);
            array[index++] = element->asConstantData();
        }
        return value;
    }
    return nullptr;
}

Value *ASTToIRTranslator::getMinimumBooleanExpression(
    BooleanSimplifyResult &in, Module *module) {
    auto v               = in.value;
    auto type            = v->type();
    in.changed           = false;
    in.shouldReplace     = false;
    in.shouldInsertFront = false;
    assert(v != nullptr);
    if (type->isBoolean()) { return v; }
    if (v->isImmediate()) {
        auto imm = v->asConstantData();
        if (imm->type()->isInteger()) {
            auto i32         = static_cast<ConstantInt *>(imm)->value;
            in.changed       = true;
            in.shouldReplace = true;
            return ConstantData::getBoolean(i32 != 0);
        } else {
            assert(imm->type()->isFloat());
            auto f32         = static_cast<ConstantFloat *>(imm)->value;
            in.changed       = true;
            in.shouldReplace = true;
            return ConstantData::getBoolean(f32 != 0.f);
        }
    }
    if (v->isGlobal() || v->isReadOnly()) {
        //! global objects always represent as its address
        in.changed       = true;
        in.shouldReplace = true;
        return ConstantData::getBoolean(true);
    }
    if (v->isParameter() || v->isInstruction()) {
        Value *value = nullptr;
        if (type->isInteger()) {
            auto rhs =
                !module ? ConstantData::createI32(0) : module->createI32(0);
            value = ICmpInst::createNE(v, rhs);
        } else if (type->isFloat()) {
            auto rhs =
                !module ? ConstantData::createF32(0.f) : module->createF32(0.f);
            value = FCmpInst::createUNE(v, rhs);
        }
        if (value != nullptr) {
            in.changed           = true;
            in.shouldReplace     = true;
            in.shouldInsertFront = true;
        }
        return value;
    }
    return nullptr;
}

Module *ASTToIRTranslator::translate(
    std::string_view name, const TranslationUnit *unit) {
    ASTToIRTranslator translator(name);
    for (auto e : *unit) {
        switch (e->declId) {
            case DeclID::Var: {
                translator.translateVarDecl(e->asVarDecl());
            } break;
            case DeclID::Function: {
                translator.translateFunctionDecl(e->asFunctionDecl());
            } break;
            default: {
                assert(false && "illegal top-level declaration");
            } break;
        }
    }
    auto module = translator.module_.release();
    pass::DeadCodeEliminationPass{}.run(module);
    pass::DeadCodeEliminationPass{}.run(module);
    pass::ValueNumberingPass{}.run(module);
    return module;
}

void ASTToIRTranslator::translateVarDecl(VarDecl *decl) {
    auto &addressTable = state_.variableAddressTable;

    assert(decl != nullptr);
    assert(decl->initValue != nullptr);
    auto type = getCompatibleIRType(decl->type());
    assert(type != nullptr);

    //! translate into local variable
    if (decl->scope.depth > 0) {
        assert(state_.currentBlock != nullptr);
        auto address = Instruction::createAlloca(type);
        address->insertToTail(state_.currentBlock);

        if (!decl->initValue->tryIntoNoInit()) {
            if (auto list = decl->initValue->tryIntoInitList()) {
                assert(type->isArray());
                size_t nbytes = 4;
                auto   t      = type;
                while (t->isArray()) {
                    nbytes *= t->asArrayType()->size();
                    t      = t->tryGetElementType();
                }
                module_->createMemset(address, 0, nbytes)
                    ->insertToTail(state_.currentBlock);
                translateArrayInitAssign(address, list);
                //! TODO: handle init-list value-by-value copy assign
            } else {
                assert(type->isBuiltinType());
                auto value =
                    translateExpr(state_.currentBlock, decl->initValue);
                assert(value->type()->isBuiltinType());
                if (type->isFloat() && value->type()->isInteger()) {
                    auto inst = Instruction::createSIToFP(value);
                    inst->insertToTail(state_.currentBlock);
                    value = inst->unwrap();
                } else if (type->isInteger() && value->type()->isFloat()) {
                    auto inst = Instruction::createFPToSI(value);
                    inst->insertToTail(state_.currentBlock);
                    value = inst->unwrap();
                }
                assert(value->type()->equals(type));
                Instruction::createStore(address->unwrap(), value)
                    ->insertToTail(state_.currentBlock);
            }
        }

        addressTable[decl]       = address->unwrap();
        state_.addressOfPrevExpr = nullptr;
        state_.valueOfPrevExpr   = nullptr;
        return;
    }

    //! translate into global variable
    auto value    = getCompatibleIRValue(decl->initValue, type, module_.get());
    auto constant = decl->specifier->isConst();
    auto var      = GlobalVariable::create(decl->name, type, constant);
    assert(value != nullptr);
    assert(value->tryIntoConstantData());
    var->setInitData(value->asConstantData());

    auto accepted = module_->acceptGlobalVariable(var);
    assert(accepted); //<! assume that the input AST is regulated
    if (!accepted) {
        delete var;
    } else {
        addressTable[decl] = var;
    }
}

void ASTToIRTranslator::translateFunctionDecl(FunctionDecl *decl) {
    assert(decl != nullptr);
    auto  proto        = getCompatibleIRType(decl->proto());
    auto  fn           = Function::create(decl->name, proto->asFunctionType());
    auto &addressTable = state_.variableAddressTable;

    if (decl->body != nullptr) {
        auto entryBlock = BasicBlock::create(fn);
        entryBlock->insertOrMoveToHead();
        state_.currentBlock      = entryBlock;
        state_.valueOfPrevExpr   = nullptr;
        state_.addressOfPrevExpr = nullptr;

        int  index = 0;
        auto it    = decl->begin();
        for (auto param : *decl) {
            auto type = fn->proto()->paramTypeAt(index);
            fn->setParam(param->name, index);
            if (!param->name.empty()) {
                VarLikeDecl *ptr     = *it;
                auto         address = Instruction::createAlloca(type);
                address->insertToTail(entryBlock);
                auto store = Instruction::createStore(
                    address, const_cast<Parameter *>(fn->paramAt(index)));
                store->insertToTail(entryBlock);
                addressTable[ptr] = address->unwrap();
            }
            ++index;
            ++it;
        }

        translateCompoundStmt(entryBlock, decl->body);
    }

    auto accepted = module_->acceptFunction(fn);
    assert(accepted); //<! assume that the input AST is regulated
    if (!accepted) { delete fn; }

    state_.currentBlock      = nullptr;
    state_.valueOfPrevExpr   = nullptr;
    state_.addressOfPrevExpr = nullptr;
}

void ASTToIRTranslator::translateStmt(BasicBlock *block, Stmt *stmt) {
    switch (stmt->stmtId) {
        case ast::StmtID::Null: {
        } break;
        case ast::StmtID::Decl: {
            translateDeclStmt(block, stmt->asDeclStmt());
        } break;
        case ast::StmtID::Expr: {
            translateExpr(block, stmt->asExprStmt()->unwrap());
        } break;
        case ast::StmtID::Compound: {
            translateCompoundStmt(block, stmt->asCompoundStmt());
        } break;
        case ast::StmtID::If: {
            translateIfStmt(block, stmt->asIfStmt());
        } break;
        case ast::StmtID::Do: {
            translateDoStmt(block, stmt->asDoStmt());
        } break;
        case ast::StmtID::While: {
            translateWhileStmt(block, stmt->asWhileStmt());
        } break;
        case ast::StmtID::For: {
            translateForStmt(block, stmt->asForStmt());
        } break;
        case ast::StmtID::Break: {
            translateBreakStmt(block, stmt->asBreakStmt());
        } break;
        case ast::StmtID::Continue: {
            translateContinueStmt(block, stmt->asContinueStmt());
        } break;
        case ast::StmtID::Return: {
            translateReturnStmt(block, stmt->asReturnStmt());
        } break;
    }
}

void ASTToIRTranslator::translateIfStmt(BasicBlock *block, IfStmt *stmt) {
    assert(stmt->condition->tryIntoExprStmt());
    auto e         = stmt->condition->asExprStmt()->unwrap();
    auto condition = translateExpr(block, e);
    block          = state_.currentBlock;
    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        assert(condition->isInstruction());
        condition->asInstruction()->insertToTail(block);
    }

    auto fn         = block->parent();
    auto branchIf   = BasicBlock::create(fn);
    auto branchExit = BasicBlock::create(fn);
    branchIf->insertOrMoveAfter(block);
    branchExit->insertOrMoveAfter(branchIf);

    if (stmt->hasBranchElse()) {
        auto branchElse = BasicBlock::create(fn);
        branchElse->insertOrMoveAfter(branchIf);
        Instruction::createBr(condition, branchIf, branchElse)
            ->insertToTail(block);
        block->resetBranch(condition, branchIf, branchElse);
        state_.currentBlock = branchElse;
        translateStmt(branchElse, stmt->branchElse);
        branchElse = state_.currentBlock;
        Instruction::createBr(branchExit)->insertToTail(branchElse);
        branchElse->resetBranch(branchExit);
    } else {
        Instruction::createBr(condition, branchIf, branchExit)
            ->insertToTail(block);
        block->resetBranch(condition, branchIf, branchExit);
    }

    state_.currentBlock = branchIf;
    translateStmt(branchIf, stmt->branchIf);
    branchIf = state_.currentBlock;
    Instruction::createBr(branchExit)->insertToTail(branchIf);
    branchIf->resetBranch(branchExit);

    state_.currentBlock = branchExit;
}

void ASTToIRTranslator::translateDoStmt(BasicBlock *block, DoStmt *stmt) {
    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    desc.branchLoop->insertOrMoveAfter(block);
    desc.branchCond->insertOrMoveAfter(desc.branchLoop);
    desc.branchExit->insertOrMoveAfter(desc.branchCond);

    //! update loop table before body translation
    state_.loopTable[stmt] = desc;

    Instruction::createBr(desc.branchLoop)->insertToTail(block);
    block->resetBranch(desc.branchLoop);

    state_.currentBlock = desc.branchLoop;
    translateStmt(desc.branchLoop, stmt->loopBody);
    auto loopEndBlock = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(loopEndBlock);
    loopEndBlock->resetBranch(desc.branchCond);

    state_.currentBlock = desc.branchCond;
    assert(stmt->condition->tryIntoExprStmt());
    auto e            = stmt->condition->asExprStmt()->unwrap();
    auto condition    = translateExpr(desc.branchCond, e);
    auto condEndBlock = state_.currentBlock;

    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        assert(condition->isInstruction());
        condition->asInstruction()->insertToTail(condEndBlock);
    }

    Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
        ->insertToTail(condEndBlock);
    condEndBlock->resetBranch(condition, desc.branchLoop, desc.branchExit);

    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateWhileStmt(BasicBlock *block, WhileStmt *stmt) {
    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    desc.branchCond->insertOrMoveAfter(block);
    desc.branchLoop->insertOrMoveAfter(desc.branchCond);
    desc.branchExit->insertOrMoveAfter(desc.branchLoop);

    //! update loop table before body translation
    state_.loopTable[stmt] = desc;

    Instruction::createBr(desc.branchCond)->insertToTail(block);
    block->resetBranch(desc.branchCond);

    state_.currentBlock = desc.branchCond;
    assert(stmt->condition->tryIntoExprStmt());
    auto e            = stmt->condition->asExprStmt()->unwrap();
    auto condition    = translateExpr(desc.branchCond, e);
    auto condEndBlock = state_.currentBlock;

    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        assert(condition->isInstruction());
        condition->asInstruction()->insertToTail(condEndBlock);
    }

    Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
        ->insertToTail(condEndBlock);
    condEndBlock->resetBranch(condition, desc.branchLoop, desc.branchExit);

    state_.currentBlock = desc.branchLoop;
    translateStmt(desc.branchLoop, stmt->loopBody);
    auto loopEndBlock = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(loopEndBlock);
    loopEndBlock->resetBranch(desc.branchCond);

    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateForStmt(BasicBlock *block, ForStmt *stmt) {
    translateStmt(block, stmt->init);
    block = state_.currentBlock;

    bool isEndlessLoop = stmt->condition->tryIntoNullStmt() != nullptr;

    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchCond->insertOrMoveAfter(block);
    desc.branchLoop->insertOrMoveAfter(desc.branchCond);
    desc.branchExit->insertOrMoveAfter(desc.branchLoop);

    //! update loop table before body translation
    state_.loopTable[stmt] = desc;

    Instruction::createBr(desc.branchCond)->insertToTail(block);
    block->resetBranch(desc.branchCond);

    if (!isEndlessLoop) {
        state_.currentBlock = desc.branchCond;
        assert(stmt->condition->tryIntoExprStmt());
        auto e            = stmt->condition->asExprStmt()->unwrap();
        auto condition    = translateExpr(desc.branchCond, e);
        auto condEndBlock = state_.currentBlock;

        BooleanSimplifyResult in(condition);
        condition = getMinimumBooleanExpression(in, module_.get());
        assert(condition != nullptr);
        if (in.shouldInsertFront) {
            assert(condition->isInstruction());
            condition->asInstruction()->insertToTail(condEndBlock);
        }

        Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
            ->insertToTail(condEndBlock);
        condEndBlock->resetBranch(condition, desc.branchLoop, desc.branchExit);
    } else {
        Instruction::createBr(desc.branchLoop)->insertToTail(desc.branchCond);
        desc.branchCond->resetBranch(desc.branchLoop);
    }

    state_.currentBlock = desc.branchLoop;
    translateStmt(desc.branchLoop, stmt->loopBody);
    auto loopEndBlock = state_.currentBlock;
    translateStmt(loopEndBlock, stmt->increment);
    loopEndBlock = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(loopEndBlock);
    loopEndBlock->resetBranch(desc.branchCond);

    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateBreakStmt(BasicBlock *block, BreakStmt *stmt) {
    assert(state_.loopTable.count(stmt->parent) == 1);
    auto &desc = state_.loopTable[stmt->parent];
    Instruction::createBr(desc.branchExit)->insertToTail(block);
    block->resetBranch(desc.branchExit);
    auto unreachable = BasicBlock::create(block->parent());
    unreachable->insertOrMoveAfter(block);
    state_.currentBlock = unreachable;
}

void ASTToIRTranslator::translateContinueStmt(
    BasicBlock *block, ContinueStmt *stmt) {
    assert(state_.loopTable.count(stmt->parent) == 1);
    auto &desc = state_.loopTable[stmt->parent];
    Instruction::createBr(desc.branchCond)->insertToTail(block);
    block->resetBranch(desc.branchCond);
    auto unreachable = BasicBlock::create(block->parent());
    unreachable->insertOrMoveAfter(block);
    state_.currentBlock = unreachable;
}

void ASTToIRTranslator::translateReturnStmt(
    BasicBlock *block, ReturnStmt *stmt) {
    Instruction *inst = nullptr;
    if (auto expr = stmt->returnValue->tryIntoExprStmt()) {
        auto value = translateExpr(block, expr->unwrap());
        block      = state_.currentBlock;
        inst       = Instruction::createRet(value);
    } else if (stmt->returnValue->tryIntoNullStmt()) {
        inst = Instruction::createRet();
    }
    assert(inst != nullptr);
    inst->insertToTail(block);
    state_.currentBlock = BasicBlock::create(block->parent());
    state_.currentBlock->insertOrMoveAfter(block);
    state_.addressOfPrevExpr = nullptr;
    state_.valueOfPrevExpr   = nullptr;
}

Value *ASTToIRTranslator::translateExpr(BasicBlock *block, Expr *expr) {
    switch (expr->exprId) {
        case ExprID::DeclRef: {
            return translateDeclRefExpr(block, expr->asDeclRef());
        } break;
        case ExprID::Constant: {
            return translateConstantExpr(block, expr->asConstant());
        } break;
        case ExprID::Unary: {
            return translateUnaryExpr(block, expr->asUnary());
        } break;
        case ExprID::Binary: {
            return translateBinaryExpr(block, expr->asBinary());
        } break;
        case ExprID::Comma: {
            return translateCommaExpr(block, expr->asComma());
        } break;
        case ExprID::Paren: {
            return translateParenExpr(block, expr->asParen());
        } break;
        case ExprID::Call: {
            return translateCallExpr(block, expr->asCall());
        } break;
        case ExprID::Subscript: {
            return translateSubscriptExpr(block, expr->asSubscript());
        } break;
        default: {
        } break;
    }
    assert(false && "unreachable branch");
    state_.addressOfPrevExpr = nullptr;
    state_.valueOfPrevExpr   = nullptr;
    return nullptr;
}

Value *ASTToIRTranslator::translateDeclRefExpr(
    BasicBlock *block, DeclRefExpr *expr) {
    state_.addressOfPrevExpr = nullptr;
    auto src                 = expr->source;
    if (auto decl = src->tryIntoFunctionDecl()) {
        auto func = decl->name == block->parent()->name()
                      ? block->parent() //<! recursive call
                      : module_->lookupFunction(decl->name);
        assert(func != nullptr);
        state_.valueOfPrevExpr = func;
        return state_.valueOfPrevExpr;
    }
    //! decl-ref is either a var-decl or a named param-var
    assert(!src->tryIntoParamVarDecl() || !src->name.empty());
    auto decl = static_cast<VarLikeDecl *>(src);
    //! address of a var is constant and is definately stored in the
    //! variableAddressTable
    assert(state_.variableAddressTable.count(decl) == 1);
    state_.addressOfPrevExpr = state_.variableAddressTable.at(decl);
    auto inst = Instruction::createLoad(state_.addressOfPrevExpr);
    inst->insertToTail(block);
    state_.valueOfPrevExpr = inst->unwrap();
    return state_.valueOfPrevExpr;
}

Value *ASTToIRTranslator::translateConstantExpr(
    BasicBlock *block, ConstantExpr *expr) {
    state_.addressOfPrevExpr = nullptr;
    switch (expr->type) {
        case ast::ConstantType::i32: {
            state_.valueOfPrevExpr = module_->createI32(expr->i32);
        } break;
        case ast::ConstantType::f32: {
            state_.valueOfPrevExpr = module_->createF32(expr->f32);
        } break;
    }
    return state_.valueOfPrevExpr;
}

Value *ASTToIRTranslator::translateUnaryExpr(
    BasicBlock *block, UnaryExpr *expr) {
    assert(expr->op != UnaryOperator::Unreachable);
    auto operand = translateExpr(block, expr->operand);
    assert(operand != nullptr);
    assert(operand == state_.valueOfPrevExpr);

    //! synchronzie block location
    block = state_.currentBlock;
    //! unary expr is no more a lval
    state_.addressOfPrevExpr = nullptr;

    if (expr->op == UnaryOperator::Pos) { return operand; }

    if (expr->op == UnaryOperator::Inv) {
        assert(operand->type()->isInteger());
        auto inst = Instruction::createXor(operand, module_->createI32(-1));
        inst->insertToTail(block);
        state_.valueOfPrevExpr = inst->unwrap();
        return state_.valueOfPrevExpr;
    }

    if (operand->type()->isInteger()) {
        Instruction *inst = nullptr;
        if (expr->op == UnaryOperator::Not) {
            inst = ICmpInst::createNE(operand, module_->createI32(0));
        } else if (expr->op == UnaryOperator::Neg) {
            inst = Instruction::createSub(module_->createI32(0), operand);
        }
        assert(inst != nullptr);
        inst->insertToTail(block);
        state_.valueOfPrevExpr = inst->unwrap();
        return state_.valueOfPrevExpr;
    }

    if (operand->type()->isFloat()) {
        Instruction *inst = nullptr;
        if (expr->op == UnaryOperator::Not) {
            inst = FCmpInst::createUNE(operand, module_->createF32(0.f));
        } else if (expr->op == UnaryOperator::Neg) {
            inst = Instruction::createFNeg(operand);
        }
        assert(inst != nullptr);
        inst->insertToTail(block);
        state_.valueOfPrevExpr = inst->unwrap();
        return state_.valueOfPrevExpr;
    }

    assert(false && "translateUnaryExpr: unreachable branch");
    state_.valueOfPrevExpr   = nullptr;
    state_.addressOfPrevExpr = nullptr;
    return nullptr;
}

Value *ASTToIRTranslator::translateBinaryExpr(
    BasicBlock *block, BinaryExpr *expr) {
    auto fn  = block->parent();
    auto lhs = translateExpr(block, expr->lhs);
    block    = state_.currentBlock;
    assert(!lhs->isInstruction() || lhs->asInstruction()->parent() == block);
    auto addressOfLhs = state_.addressOfPrevExpr;

    //! implement '&&' short-circuit logic
    //! ---
    //! entry:
    //!     %lhs = ...
    //!     %1 = %lhs as i1
    //!     br i1 %1, label %next, label %exit
    //! next:
    //!     %rhs = ...
    //!     %2 = %rhs as i1
    //!     br label %exit
    //! exit:
    //!     %result = phi i1 [ %1, %entry ], [ %2, %next ]
    //! ===

    //! implement '||' short-circuit logic
    //! ---
    //! entry:
    //!     %lhs = ...
    //!     %1 = %lhs as i1
    //!     br i1 %1, label %exit, label %next
    //! next:
    //!     %rhs = ...
    //!     %2 = %rhs as i1
    //!     br label %exit
    //! exit:
    //!     %result = phi i1 [ %1, %entry ], [ %2, %next ]
    //! ===

    const bool isShortCircutLogic =
        expr->op == BinaryOperator::LAnd || expr->op == BinaryOperator::LOr;
    if (isShortCircutLogic) {
        BooleanSimplifyResult inLhs(lhs);
        auto lhs = getMinimumBooleanExpression(inLhs, module_.get());
        assert(lhs != nullptr);
        if (inLhs.shouldInsertFront) {
            assert(lhs->isInstruction());
            lhs->asInstruction()->insertToTail(block);
        }

        auto nextBlock = BasicBlock::create(fn);
        auto exitBlock = BasicBlock::create(fn);
        nextBlock->insertOrMoveAfter(block);
        assert(nextBlock->isInserted());
        exitBlock->insertOrMoveAfter(nextBlock);
        assert(exitBlock->isInserted());

        if (expr->op == BinaryOperator::LAnd) {
            Instruction::createBr(lhs, nextBlock, exitBlock)
                ->insertToTail(block);
            block->resetBranch(lhs, nextBlock, exitBlock);
        } else if (expr->op == BinaryOperator::LOr) {
            Instruction::createBr(lhs, exitBlock, nextBlock)
                ->insertToTail(block);
            block->resetBranch(lhs, exitBlock, nextBlock);
        } else {
            assert(false && "translateBinaryExpr: unreachable branch");
            state_.valueOfPrevExpr   = nullptr;
            state_.addressOfPrevExpr = nullptr;
            return nullptr;
        }

        state_.currentBlock = nextBlock;
        auto rhs            = translateExpr(nextBlock, expr->rhs);
        nextBlock           = state_.currentBlock;
        assert(
            !rhs->isInstruction()
            || rhs->asInstruction()->parent() == nextBlock);

        BooleanSimplifyResult inRhs(rhs);
        rhs = getMinimumBooleanExpression(inRhs, module_.get());
        assert(lhs != nullptr);
        if (inRhs.shouldInsertFront) {
            assert(rhs->isInstruction());
            rhs->asInstruction()->insertToTail(nextBlock);
        }
        Instruction::createBr(exitBlock)->insertToTail(nextBlock);

        auto phi = Instruction::createPhi(lhs->type());
        phi->addIncomingValue(lhs, block);
        phi->addIncomingValue(rhs, nextBlock);
        phi->insertToTail(exitBlock);

        //! value of phi-inst is definately not a lval
        state_.addressOfPrevExpr = nullptr;
        state_.valueOfPrevExpr   = phi->unwrap();
        state_.currentBlock      = exitBlock;
        return state_.valueOfPrevExpr;
    }

    auto rhs = translateExpr(block, expr->rhs);
    block    = state_.currentBlock;
    assert(!rhs->isInstruction() || rhs->asInstruction()->parent() == block);

    assert(lhs->type()->isBuiltinType() && rhs->type()->isBuiltinType());
    if (lhs->type()->isFloat() && rhs->type()->isInteger()) {
        auto inst = Instruction::createSIToFP(rhs);
        inst->insertToTail(block);
        rhs = inst->unwrap();
    }
    if (lhs->type()->isInteger() && rhs->type()->isFloat()) {
        if (expr->op == BinaryOperator::Assign) {
            auto inst = Instruction::createFPToSI(rhs);
            inst->insertToTail(block);
            rhs = inst->unwrap();
        } else {
            auto inst = Instruction::createSIToFP(lhs);
            inst->insertToTail(block);
            lhs = inst->unwrap();
        }
    }

    Instruction *inst = nullptr;
    switch (expr->op) {
        case BinaryOperator::Assign: {
            assert(addressOfLhs != nullptr);
            inst = Instruction::createStore(addressOfLhs, rhs);
            inst->insertToTail(block);
            inst = Instruction::createLoad(addressOfLhs);
            inst->insertToTail(block);
            //! assign-expr will forward lhs as lval
            state_.addressOfPrevExpr = addressOfLhs;
            state_.valueOfPrevExpr   = inst->unwrap();
            return state_.valueOfPrevExpr;
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
            assert(
                lhs->type()->isBuiltinType()
                && lhs->type()->equals(rhs->type()));
            if (lhs->type()->isInteger()) {
                switch (expr->op) {
                    case BinaryOperator::Add: {
                        inst = Instruction::createAdd(lhs, rhs);
                    } break;
                    case BinaryOperator::Sub: {
                        inst = Instruction::createSub(lhs, rhs);
                    } break;
                    case BinaryOperator::Mul: {
                        inst = Instruction::createMul(lhs, rhs);
                    } break;
                    case BinaryOperator::Div: {
                        inst = Instruction::createSDiv(lhs, rhs);
                    } break;
                    case BinaryOperator::LT: {
                        inst = ICmpInst::createSLT(lhs, rhs);
                    } break;
                    case BinaryOperator::LE: {
                        inst = ICmpInst::createSLE(lhs, rhs);
                    } break;
                    case BinaryOperator::GT: {
                        inst = ICmpInst::createSGT(lhs, rhs);
                    } break;
                    case BinaryOperator::GE: {
                        inst = ICmpInst::createSGE(lhs, rhs);
                    } break;
                    case BinaryOperator::EQ: {
                        inst = ICmpInst::createEQ(lhs, rhs);
                    } break;
                    case BinaryOperator::NE: {
                        inst = ICmpInst::createNE(lhs, rhs);
                    } break;
                    default: {
                    } break;
                }
            } else {
                switch (expr->op) {
                    case BinaryOperator::Add: {
                        inst = Instruction::createFAdd(lhs, rhs);
                    } break;
                    case BinaryOperator::Sub: {
                        inst = Instruction::createFSub(lhs, rhs);
                    } break;
                    case BinaryOperator::Mul: {
                        inst = Instruction::createFMul(lhs, rhs);
                    } break;
                    case BinaryOperator::Div: {
                        inst = Instruction::createFDiv(lhs, rhs);
                    } break;
                    case BinaryOperator::LT: {
                        inst = FCmpInst::createOLT(lhs, rhs);
                    } break;
                    case BinaryOperator::LE: {
                        inst = FCmpInst::createOLE(lhs, rhs);
                    } break;
                    case BinaryOperator::GT: {
                        inst = FCmpInst::createOGT(lhs, rhs);
                    } break;
                    case BinaryOperator::GE: {
                        inst = FCmpInst::createOGE(lhs, rhs);
                    } break;
                    case BinaryOperator::EQ: {
                        inst = FCmpInst::createOEQ(lhs, rhs);
                    } break;
                    case BinaryOperator::NE: {
                        inst = FCmpInst::createONE(lhs, rhs);
                    } break;
                    default: {
                    } break;
                }
            }
            if (!inst) { break; }
            inst->insertToTail(block);
            state_.addressOfPrevExpr = nullptr;
            state_.valueOfPrevExpr   = inst->unwrap();
            return state_.valueOfPrevExpr;
        } break;
        case BinaryOperator::Mod:
        case BinaryOperator::And:
        case BinaryOperator::Or:
        case BinaryOperator::Xor:
        case BinaryOperator::Shl:
        case BinaryOperator::Shr: {
            assert(lhs->type()->isInteger() && rhs->type()->isInteger());
            switch (expr->op) {
                case BinaryOperator::Mod: {
                    inst = Instruction::createSRem(lhs, rhs);
                } break;
                case BinaryOperator::And: {
                    inst = Instruction::createAnd(lhs, rhs);
                } break;
                case BinaryOperator::Or: {
                    inst = Instruction::createOr(lhs, rhs);
                } break;
                case BinaryOperator::Xor: {
                    inst = Instruction::createXor(lhs, rhs);
                } break;
                case BinaryOperator::Shl: {
                    inst = Instruction::createShl(lhs, rhs);
                } break;
                case BinaryOperator::Shr: {
                    inst = Instruction::createAShr(lhs, rhs);
                } break;
                default: {
                } break;
            }
            if (!inst) { break; }
            inst->insertToTail(block);
            state_.addressOfPrevExpr = nullptr;
            state_.valueOfPrevExpr   = inst->unwrap();
            return state_.valueOfPrevExpr;
        } break;
        default: {
        } break;
    }

    assert(false && "translateBinaryExpr: unreachable branch");
    state_.addressOfPrevExpr = nullptr;
    state_.valueOfPrevExpr   = nullptr;
    return nullptr;
}

Value *ASTToIRTranslator::translateCommaExpr(
    BasicBlock *block, CommaExpr *expr) {
    assert(expr->size() > 0);
    //! TODO: use Expr::isNoEffect to filter no-effect expr [optional]
    //! NOTE: last value cannot ignore
    Value *value = nullptr;
    for (auto e : *expr) { value = translateExpr(state_.currentBlock, e); }
    assert(value != nullptr);
    //! comma-expr derives the attribute as paren-expr, so simply forward
    //! valueOfPrevExpr and addressOfPrefExpr
    assert(value == state_.valueOfPrevExpr);
    return state_.valueOfPrevExpr;
}

Value *ASTToIRTranslator::translateCallExpr(BasicBlock *block, CallExpr *expr) {
    assert(expr->callable->tryIntoDeclRef());
    assert(expr->callable->asDeclRef()->source->tryIntoFunctionDecl());
    auto result = translateDeclRefExpr(block, expr->callable->asDeclRef());
    assert(block == state_.currentBlock);
    assert(result != nullptr);
    assert(result->isFunction());
    auto callee = result->asFunction();
    auto call   = CallInst::create(callee);
    assert(callee->totalParams() == expr->argList.size());
    assert(call->totalUse() == callee->totalParams() + 1);
    int index = 1;
    for (auto arg : expr->argList) {
        call->op()[index++] = translateExpr(state_.currentBlock, arg);
    }
    call->insertToTail(state_.currentBlock);
    state_.addressOfPrevExpr = nullptr;
    state_.valueOfPrevExpr   = call->unwrap();
    return state_.valueOfPrevExpr;
}

Value *ASTToIRTranslator::translateSubscriptExpr(
    BasicBlock *block, SubscriptExpr *expr) {
    auto         address  = translateExpr(state_.currentBlock, expr->lhs);
    Instruction *valuePtr = nullptr;
    if (state_.addressOfPrevExpr != nullptr) {
        address    = state_.addressOfPrevExpr;
        auto index = translateExpr(state_.currentBlock, expr->rhs);
        valuePtr   = GetElementPtrInst::create(address, index);
    } else {
        assert(false && "only accept lhs as symref");
    }
    valuePtr->insertToTail(state_.currentBlock);
    auto value = Instruction::createLoad(valuePtr->unwrap());
    value->insertToTail(state_.currentBlock);
    state_.addressOfPrevExpr = valuePtr->unwrap();
    state_.valueOfPrevExpr   = value->unwrap();
    return state_.valueOfPrevExpr;
}

void ASTToIRTranslator::translateArrayInitAssign(
    Value *address, InitListExpr *data) {
    //! assume that init-list is regulated
    int  index = 0;
    auto type  = address->type()->tryGetElementType()->tryGetElementType();
    if (type->isArray()) {
        for (auto e : *data) {
            auto list = e->tryIntoInitList();
            assert(list != nullptr);
            if (list->size() > 0) {
                auto inst = GetElementPtrInst::create(
                    address, module_->createI32(index));
                inst->insertToTail(state_.currentBlock);
                translateArrayInitAssign(inst->unwrap(), list);
            }
            ++index;
        }
    } else {
        for (auto e : *data) {
            auto inst =
                GetElementPtrInst::create(address, module_->createI32(index));
            inst->insertToTail(state_.currentBlock);
            auto value = translateExpr(state_.currentBlock, e);
            assert(value->type()->isBuiltinType() && type->isBuiltinType());
            if (!value->type()->equals(type)) {
                if (type->isInteger()) {
                    auto inst = Instruction::createFPToSI(value);
                    inst->insertToTail(state_.currentBlock);
                    value = inst->unwrap();
                } else {
                    auto inst = Instruction::createSIToFP(value);
                    inst->insertToTail(state_.currentBlock);
                    value = inst->unwrap();
                }
            }
            assert(value->type()->equals(type));
            Instruction::createStore(inst->unwrap(), value)
                ->insertToTail(state_.currentBlock);
            ++index;
        }
    }
}

} // namespace slime::visitor
