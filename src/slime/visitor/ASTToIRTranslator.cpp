#include "ASTToIRTranslator.h"
#include "../ir/user.h"
#include "../ir/instruction.h"

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
            for (auto n : *array) {
                type = ir::Type::createArrayType(type, n->asConstant()->i32);
            }
            return type;
        } break;
        case ast::TypeID::IncompleteArray: {
            auto array = type->asIncompleteArray();
            auto type  = getCompatibleIRType(array->type);
            for (auto n : *array) {
                type = ir::Type::createArrayType(type, n->asConstant()->i32);
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
    //! NOTE: assume the value is already regular
    if (value->tryIntoNoInit() && desired != nullptr) {
        switch (desired->kind()) {
            case TypeKind::Integer: {
                return !module ? ConstantData::createI32(0)
                               : module->createInt(0);
            } break;
            case TypeKind::Float: {
                return !module ? ConstantData::createF32(0.f)
                               : module->createFloat(0.f);
            } break;
            case TypeKind::Array: {
                return ConstantArray::create(desired->asArrayType());
            } break;
            default: {
            } break;
        }
    }
    if (auto v = value->tryIntoConstant()) {
        switch (v->type) {
            case ConstantType::i32: {
                if (!desired || desired->isInteger()) {
                    return !module ? ConstantData::createI32(v->i32)
                                   : module->createInt(v->i32);
                }
            } break;
            case ConstantType::f32: {
                if (!desired || desired->isFloat()) {
                    return !module ? ConstantData::createF32(v->f32)
                                   : module->createFloat(v->f32);
                }
            } break;
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
    in.changed           = false;
    in.shouldReplace     = false;
    in.shouldInsertFront = false;
    assert(v != nullptr);
    if (v->isCompareInst()) { return v; }
    if (v->isImmediate()) {
        auto imm = v->asConstantData();
        if (imm->type()->isInteger()) {
            auto i32 = static_cast<ConstantInt *>(imm);
            if (i32->value != 0 && i32->value != 1) {
                i32 =
                    !module ? ConstantData::createI32(1) : module->createInt(1);
                in.changed       = true;
                in.shouldReplace = true;
            }
            return i32;
        } else {
            assert(imm->type()->isFloat());
            auto f32    = static_cast<ConstantFloat *>(imm);
            auto result = !module ? ConstantData::createI32(f32->value == 0.f)
                                  : module->createInt(f32->value == 0.f);
            in.changed  = true;
            in.shouldReplace = true;
            return result;
        }
    }
    if (v->isInstruction()) {
        auto inst = v->asInstruction();
        switch (inst->id()) {
            case InstructionID::Alloca:
            case InstructionID::GetElementPtr:
            case InstructionID::Add:
            case InstructionID::Sub:
            case InstructionID::Mul:
            case InstructionID::UDiv:
            case InstructionID::SDiv:
            case InstructionID::URem:
            case InstructionID::SRem:
            case InstructionID::Shl:
            case InstructionID::LShr:
            case InstructionID::AShr:
            case InstructionID::And:
            case InstructionID::Or:
            case InstructionID::Xor:
            case InstructionID::FPToUI:
            case InstructionID::FPToSI: {
                auto value = ICmpInst::createNE(
                    inst->unwrap(),
                    !module ? ConstantData::createI32(0)
                            : module->createInt(0));
                in.changed           = true;
                in.shouldReplace     = true;
                in.shouldInsertFront = true;
                return value;
            } break;
            case InstructionID::FNeg:
            case InstructionID::FAdd:
            case InstructionID::FSub:
            case InstructionID::FMul:
            case InstructionID::FDiv:
            case InstructionID::FRem:
            case InstructionID::UIToFP:
            case InstructionID::SIToFP: {
                auto value = FCmpInst::createUNE(
                    inst->unwrap(),
                    !module ? ConstantData::createF32(0.f)
                            : module->createFloat(0.f));
                in.changed           = true;
                in.shouldReplace     = true;
                in.shouldInsertFront = true;
                return value;
            } break;
            case InstructionID::Phi:
            case InstructionID::Call:
            case InstructionID::Load: {
                if (v->type()->isInteger()) {
                    auto value = ICmpInst::createNE(
                        inst->unwrap(),
                        !module ? ConstantData::createI32(0)
                                : module->createInt(0));
                    in.changed           = true;
                    in.shouldReplace     = true;
                    in.shouldInsertFront = true;
                    return value;
                } else if (v->type()->isFloat()) {
                    auto value = FCmpInst::createUNE(
                        inst->unwrap(),
                        !module ? ConstantData::createF32(0.f)
                                : module->createFloat(0.f));
                    in.changed           = true;
                    in.shouldReplace     = true;
                    in.shouldInsertFront = true;
                    return value;
                } else {
                    return nullptr;
                }
            } break;
            default: {
                return nullptr;
            } break;
        }
    }
    if (v->isFunction() || v->isReadOnly()) {
        auto value =
            !module ? ConstantData::createI32(1) : module->createInt(1);
        in.changed       = true;
        in.shouldReplace = true;
        return value;
    }
    if (v->isGlobal() || v->isParameter()) {
        if (v->type()->isInteger()) {
            auto value = ICmpInst::createNE(
                v, !module ? ConstantData::createI32(0) : module->createInt(0));
            in.changed           = true;
            in.shouldReplace     = true;
            in.shouldInsertFront = true;
            return value;
        } else if (v->type()->isFloat()) {
            auto value = FCmpInst::createUNE(
                v,
                !module ? ConstantData::createF32(0.f)
                        : module->createFloat(0.f));
            in.changed           = true;
            in.shouldReplace     = true;
            in.shouldInsertFront = true;
            return value;
        } else {
            return nullptr;
        }
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
    return translator.module_.release();
}

void ASTToIRTranslator::translateVarDecl(VarDecl *decl) {
    assert(decl != nullptr);
    auto type  = getCompatibleIRType(decl->type());
    auto value = getCompatibleIRValue(decl->initValue, type);
    //! translate into local variable
    if (decl->scope.depth > 0) {
        assert(state_.currentBlock != nullptr);
        auto address = Instruction::createAlloca(type);
        address->insertToTail(state_.currentBlock);
        if (decl->initValue != nullptr) {
            if (!value) {
                value = translateExpr(state_.currentBlock, decl->initValue);
            }
            assert(value != nullptr);
            auto inst = Instruction::createStore(address->unwrap(), value);
            inst->insertToTail(state_.currentBlock);
        }
        state_.variableAddressTable.insert_or_assign(decl, address->unwrap());
        return;
    }
    //! translate into global variable
    auto var =
        GlobalVariable::create(decl->name, type, decl->specifier->isConst());
    assert(decl->initValue != nullptr);
    assert(value != nullptr);
    var->setInitData(value->asConstantData());
    auto accepted = module_->acceptGlobalVariable(var);
    if (!accepted) {
        delete var;
    } else {
        state_.variableAddressTable.insert_or_assign(decl, var);
    }
}

void ASTToIRTranslator::translateFunctionDecl(FunctionDecl *decl) {
    assert(decl != nullptr);
    auto proto = getCompatibleIRType(decl->proto());
    auto fn    = Function::create(decl->name, proto->asFunctionType());
    fn->insertToHead(BasicBlock::create(fn));
    int  index = 0;
    auto it    = decl->begin();
    for (auto param : *decl) {
        auto type = fn->proto()->paramTypeAt(index);
        fn->setParam(param->name, index++);
        if (!param->name.empty()) {
            auto address = Instruction::createAlloca(type);
            address->insertToTail(fn->front());
            state_.variableAddressTable.insert_or_assign(
                *it, address->unwrap());
        }
        ++it;
    }
    if (decl->body != nullptr) {
        translateCompoundStmt(fn->front(), decl->body);
    }
    auto accepted = module_->acceptFunction(fn);
    if (!accepted) { delete fn; }
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

void ASTToIRTranslator::translateDeclStmt(BasicBlock *block, DeclStmt *stmt) {
    state_.currentBlock = block;
    for (auto decl : *stmt) { translateVarDecl(decl); }
}

void ASTToIRTranslator::translateCompoundStmt(
    BasicBlock *block, CompoundStmt *stmt) {
    for (auto e : *stmt) {
        translateStmt(block, e);
        if (state_.currentBlock != nullptr && state_.currentBlock != block) {
            block = state_.currentBlock;
        }
    }
}

void ASTToIRTranslator::translateIfStmt(BasicBlock *block, IfStmt *stmt) {
    auto condition =
        translateExpr(block, stmt->condition->asExprStmt()->unwrap());
    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        condition->asInstruction()->insertToTail(block);
    }
    auto fn         = block->parent();
    auto branchExit = BasicBlock::create(fn);
    auto branchIf   = BasicBlock::create(fn);
    fn->insertToTail(branchIf);
    translateStmt(branchIf, stmt->branchIf);
    Instruction::createBr(branchExit)->insertToTail(branchIf);
    if (stmt->branchElse != nullptr) {
        auto branchElse = BasicBlock::create(fn);
        fn->insertToTail(branchElse);
        translateStmt(branchElse, stmt->branchElse);
        Instruction::createBr(branchExit)->insertToTail(branchElse);
        Instruction::createBr(condition, branchIf, branchElse)
            ->insertToTail(block);
    } else {
        Instruction::createBr(condition, branchIf, branchExit)
            ->insertToTail(block);
    }
    fn->insertToTail(branchExit);
    state_.currentBlock = branchExit;
    //! FIXME: fix broken cfg
}

void ASTToIRTranslator::translateDoStmt(BasicBlock *block, DoStmt *stmt) {
    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    fn->insertToTail(desc.branchCond);
    fn->insertToTail(desc.branchExit);
    fn->insertToTail(desc.branchLoop);

    Instruction::createBr(desc.branchLoop)->insertToTail(block);

    translateStmt(desc.branchLoop, stmt->loopBody);
    block = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(block);

    auto condition =
        translateExpr(desc.branchCond, stmt->condition->asExprStmt()->unwrap());
    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        condition->asInstruction()->insertToTail(desc.branchCond);
    }
    Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
        ->insertToTail(desc.branchCond);
    state_.loopTable.insert_or_assign(stmt, desc);
    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateWhileStmt(BasicBlock *block, WhileStmt *stmt) {
    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    fn->insertToTail(desc.branchCond);
    fn->insertToTail(desc.branchExit);
    fn->insertToTail(desc.branchLoop);

    Instruction::createBr(desc.branchCond)->insertToTail(block);

    translateStmt(desc.branchLoop, stmt->loopBody);
    block = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(block);

    auto condition =
        translateExpr(desc.branchCond, stmt->condition->asExprStmt()->unwrap());
    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        condition->asInstruction()->insertToTail(desc.branchCond);
    }
    Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
        ->insertToTail(desc.branchCond);
    state_.loopTable.insert_or_assign(stmt, desc);
    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateForStmt(BasicBlock *block, ForStmt *stmt) {
    translateStmt(block, stmt->init);

    LoopDescription desc;
    auto            fn = block->parent();
    desc.branchCond    = BasicBlock::create(fn);
    desc.branchExit    = BasicBlock::create(fn);
    desc.branchLoop    = BasicBlock::create(fn);
    fn->insertToTail(desc.branchCond);
    fn->insertToTail(desc.branchExit);
    fn->insertToTail(desc.branchLoop);

    Instruction::createBr(desc.branchCond)->insertToTail(block);

    translateStmt(desc.branchLoop, stmt->loopBody);
    translateStmt(desc.branchLoop, stmt->increment);
    block = state_.currentBlock;
    Instruction::createBr(desc.branchCond)->insertToTail(block);

    auto condition =
        translateExpr(desc.branchCond, stmt->condition->asExprStmt()->unwrap());
    BooleanSimplifyResult in(condition);
    condition = getMinimumBooleanExpression(in, module_.get());
    assert(condition != nullptr);
    if (in.shouldInsertFront) {
        condition->asInstruction()->insertToTail(desc.branchCond);
    }
    Instruction::createBr(condition, desc.branchLoop, desc.branchExit)
        ->insertToTail(desc.branchCond);
    state_.loopTable.insert_or_assign(stmt, desc);
    state_.currentBlock = desc.branchExit;
}

void ASTToIRTranslator::translateBreakStmt(BasicBlock *block, BreakStmt *stmt) {
    auto &desc = state_.loopTable[stmt->parent];
    auto  inst = Instruction::createBr(desc.branchExit);
    inst->insertToTail(block);
    //! FIXME: fix the broken cfg
}

void ASTToIRTranslator::translateContinueStmt(
    BasicBlock *block, ContinueStmt *stmt) {
    auto &desc = state_.loopTable[stmt->parent];
    auto  inst = Instruction::createBr(desc.branchCond);
    inst->insertToTail(block);
    //! FIXME: fix the broken cfg
}

void ASTToIRTranslator::translateReturnStmt(
    BasicBlock *block, ReturnStmt *stmt) {
    auto inst = stmt->tryIntoNullStmt()
                  ? Instruction::createRet()
                  : Instruction::createRet(translateExpr(
                      block, stmt->returnValue->asExprStmt()->unwrap()));
    inst->insertToTail(block);
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
        case ExprID::Stmt:
        case ExprID::InitList:
        case ExprID::NoInit: {
            assert(false && "unreachable branch");
            return nullptr;
        } break;
    }
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
            state_.valueOfPrevExpr = module_->createInt(expr->i32);
        } break;
        case ast::ConstantType::f32: {
            state_.valueOfPrevExpr = module_->createFloat(expr->f32);
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
        auto inst = Instruction::createXor(operand, module_->createInt(-1));
        inst->insertToTail(block);
        state_.valueOfPrevExpr = inst->unwrap();
        return state_.valueOfPrevExpr;
    }

    if (operand->type()->isInteger()) {
        Instruction *inst = nullptr;
        if (expr->op == UnaryOperator::Not) {
            inst = ICmpInst::createNE(operand, module_->createInt(0));
        } else {
            assert(expr->op == UnaryOperator::Neg);
            inst = Instruction::createSub(module_->createInt(0), operand);
        }
        inst->insertToTail(block);
        state_.valueOfPrevExpr = inst->unwrap();
        return state_.valueOfPrevExpr;
    }

    if (operand->type()->isFloat()) {
        Instruction *inst = nullptr;
        if (expr->op == UnaryOperator::Not) {
            inst = FCmpInst::createUNE(operand, module_->createFloat(0.f));
        } else {
            assert(expr->op == UnaryOperator::Neg);
            inst = Instruction::createFNeg(operand);
        }
        inst->insertToTail(block);
        return inst->unwrap();
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

    //! maybe used if lhs is a decl-ref
    auto address = state_.addressOfLastDeclRef;

    //! TODO: implement '&&' short-circuit logic
    if (expr->op == BinaryOperator::LAnd) {
        BooleanSimplifyResult in(lhs);
        auto condition = getMinimumBooleanExpression(in, module_.get());
        if (in.shouldInsertFront) {
            condition->asInstruction()->insertToTail(block);
        }
        auto branchNext = BasicBlock::create(fn);
        auto branchExit = BasicBlock::create(fn);
        fn->insertToTail(branchNext);
        fn->insertToTail(branchExit);
        Instruction::createBr(condition, branchNext, branchExit)
            ->insertToTail(block);
        auto rhs = translateExpr(branchNext, expr->rhs);
        Instruction::createBr(branchExit)->insertToTail(branchNext);
        auto phi = Instruction::createPhi(ir::Type::getIntegerType());
        phi->addIncomingValue(condition, block);
        phi->addIncomingValue(rhs, branchNext);
        phi->insertToTail(branchExit);
        return phi->unwrap();
    }

    //! TODO: implement '||' short-circuit logic
    if (expr->op == BinaryOperator::LOr) {
        BooleanSimplifyResult in(lhs);
        auto condition = getMinimumBooleanExpression(in, module_.get());
        if (in.shouldInsertFront) {
            condition->asInstruction()->insertToTail(block);
        }
        auto branchNext  = BasicBlock::create(fn);
        auto branchEntry = BasicBlock::create(fn);
        fn->insertToTail(branchNext);
        fn->insertToTail(branchEntry);
        Instruction::createBr(condition, branchEntry, branchNext)
            ->insertToTail(block);
        auto rhs = translateExpr(branchNext, expr->rhs);
        Instruction::createBr(branchEntry)->insertToTail(branchNext);
        auto phi = Instruction::createPhi(ir::Type::getIntegerType());
        phi->addIncomingValue(condition, block);
        phi->addIncomingValue(rhs, branchNext);
        phi->insertToTail(branchEntry);
        return phi->unwrap();
    }

    auto rhs = translateExpr(block, expr->rhs);
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
            assert(address != nullptr);
            inst = Instruction::createStore(
                expr->lhs->tryIntoDeclRef() ? address : lhs, rhs);
            inst->insertToTail(block);
            inst = Instruction::createLoad(address);
            inst->insertToTail(block);
            return inst->unwrap();
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
                        assert(false && "unreachable branch");
                        return nullptr;
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
                        assert(false && "unreachable branch");
                        return nullptr;
                    } break;
                }
            }
            inst->insertToTail(block);
            return inst->unwrap();
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
                    assert(false && "unreachable branch");
                    return nullptr;
                } break;
            }
            inst->insertToTail(block);
            return inst->unwrap();
        } break;
        default: {
            assert(false && "unreachable branch");
            return nullptr;
        } break;
    }
}

Value *ASTToIRTranslator::translateCommaExpr(
    BasicBlock *block, CommaExpr *expr) {
    assert(expr->size() > 0);
    Value *value = nullptr;
    auto   last  = expr->tail()->value();
    for (auto e : *expr) {
        if (!e->isNoEffectExpr() || e == last) {
            value = translateExpr(block, e);
        }
    }
    assert(value != nullptr);
    return value;
}

Value *ASTToIRTranslator::translateParenExpr(
    BasicBlock *block, ParenExpr *expr) {
    return translateExpr(block, expr->inner);
}

Value *ASTToIRTranslator::translateCallExpr(BasicBlock *block, CallExpr *expr) {
    auto ref = expr->callable->tryIntoDeclRef();
    assert(ref != nullptr);
    assert(ref->source->tryIntoFunctionDecl());
    auto result = translateDeclRefExpr(block, ref);
    assert(result != nullptr);
    auto callee = result->asFunction();
    assert(callee != nullptr);
    auto call = CallInst::create(callee);
    assert(callee->totalParams() == expr->argList.size());
    assert(call->totalUse() == callee->totalParams() + 1);
    int index = 1;
    for (auto arg : expr->argList) {
        call->op()[index++] = translateExpr(block, arg);
    }
    call->insertToTail(block);
    return call;
}

Value *ASTToIRTranslator::translateSubscriptExpr(
    BasicBlock *block, SubscriptExpr *expr) {
    auto address = translateExpr(block, expr->lhs);
    auto index   = translateExpr(block, expr->rhs);
    auto value   = GetElementPtrInst::create(address, index);
    value->insertToTail(block);
    return value;
}

} // namespace slime::visitor

//} // namespace slime::visitor
