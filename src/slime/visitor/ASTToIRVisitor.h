#pragma once

#include "../ast/ast.h"
#include "../ast/expr.h"
#include "../ast/type.h"
#include "../ir/minimal.h"
#include "../utils/list.h"

#include <map>

namespace slime::visitor {

using TopLevelIRObjectList = slime::utils::ListTrait<ir::GlobalObject *>;

class ASTToIRVisitor : public TopLevelIRObjectList {
public:
    static ir::Type     *getIRTypeFromAstType(ast::Type *type);
    static ir::Constant *evaluateCompileTimeAstExpr(ast::Expr *expr);
    static ir::Value    *makeBooleanCondition(ir::Value *condition);

    void visit(ast::TranslationUnit *e);

protected:
    ir::Value      *visit(ir::BasicBlock *block, ast::VarDecl *e);
    ir::Function   *visit(ast::FunctionDecl *e);
    ir::BasicBlock *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::Stmt *e);
    inline void visit(
        ir::Function *fn, ir::BasicBlock *block, ast::DeclStmt *e);
    inline ir::Value *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::ExprStmt *e);
    inline ir::BasicBlock *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::CompoundStmt *e);
    ir::BasicBlock *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::IfStmt *e);
    inline ir::BasicBlock *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::DoStmt *e);
    inline ir::BasicBlock *visit(
        ir::Function *fn, ir::BasicBlock *block, ast::WhileStmt *e);
    void visit(ir::Function *fn, ir::BasicBlock *block, ast::BreakStmt *e);
    void visit(ir::Function *fn, ir::BasicBlock *block, ast::ContinueStmt *e);
    inline void visit(
        ir::Function *fn, ir::BasicBlock *block, ast::ReturnStmt *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::Expr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::DeclRefExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::ConstantExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::UnaryExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::BinaryExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::CommaExpr *e);
    inline ir::Value *visit(ir::BasicBlock *block, ast::ParenExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::CallExpr *e);
    ir::Value        *visit(ir::BasicBlock *block, ast::SubscriptExpr *e);

private:
    ir::BasicBlock *createLoop(
        ast::Stmt      *hint,
        ir::BasicBlock *entry,
        ast::Expr      *condition,
        ast::Stmt      *body,
        bool            isLoopBodyFirst = false);

private:
    struct LoopDescription {
        ir::BasicBlock *branchCond;
        ir::BasicBlock *branchLoop;
        ir::BasicBlock *branchExit;
    };

    std::map<ast::Stmt *, LoopDescription>    loopMap_;
    std::map<ast::DeclRefExpr *, ir::Value *> symbolTable_;
};

inline void ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::DeclStmt *e) {
    for (auto decl : *e) { visit(block, decl); }
}

inline ir::Value *ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::ExprStmt *e) {
    return visit(block, e->unwrap());
}

inline ir::BasicBlock *ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::CompoundStmt *e) {
    for (auto stmt : *e) { block = visit(fn, block, stmt); }
    return block;
}

inline ir::BasicBlock *ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::DoStmt *e) {
    return createLoop(
        e, block, e->condition->asExprStmt()->unwrap(), e->loopBody, true);
}

inline ir::BasicBlock *ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::WhileStmt *e) {
    return createLoop(
        e, block, e->condition->asExprStmt()->unwrap(), e->loopBody, false);
}

inline void ASTToIRVisitor::visit(
    ir::Function *fn, ir::BasicBlock *block, ast::ReturnStmt *e) {
    block->insertToTail(new ir::ReturnInst(visit(fn, block, e->returnValue)));
}

inline ir::Value *ASTToIRVisitor::visit(
    ir::BasicBlock *block, ast::ParenExpr *e) {
    return visit(block, e->inner);
}
} // namespace slime::visitor
