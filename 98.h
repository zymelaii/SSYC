#pragma once

#include "0.h"
#include "10.h"
#include "5.h"
#include "51.h"
#include <memory>

namespace slime::visitor {

class ASTToIRTranslator {
public:
    struct BooleanSimplifyResult {
        ir::Value *value;
        bool       changed           = false;
        bool       shouldReplace     = false;
        bool       shouldInsertFront = false;

        BooleanSimplifyResult(ir::Value *value)
            : value{value} {}
    };

    static ir::Type  *getCompatibleIRType(ast::Type *type);
    static ir::Value *getCompatibleIRValue(
        ast::Expr *value, ir::Type *desired, ir::Module *module = nullptr);
    static ir::Value *getMinimumBooleanExpression(
        BooleanSimplifyResult &in, ir::Module *module = nullptr);

    static ir::Module *translate(
        std::string_view name, const ast::TranslationUnit *unit);

protected:
    void translateVarDecl(ast::VarDecl *decl);
    void translateFunctionDecl(ast::FunctionDecl *decl);

    void        translateStmt(ir::BasicBlock *block, ast::Stmt *stmt);
    inline void translateDeclStmt(ir::BasicBlock *block, ast::DeclStmt *stmt);
    inline void translateCompoundStmt(
        ir::BasicBlock *block, ast::CompoundStmt *stmt);
    void translateIfStmt(ir::BasicBlock *block, ast::IfStmt *stmt);
    void translateDoStmt(ir::BasicBlock *block, ast::DoStmt *stmt);
    void translateWhileStmt(ir::BasicBlock *block, ast::WhileStmt *stmt);
    void translateForStmt(ir::BasicBlock *block, ast::ForStmt *stmt);
    void translateBreakStmt(ir::BasicBlock *block, ast::BreakStmt *stmt);
    void translateContinueStmt(ir::BasicBlock *block, ast::ContinueStmt *stmt);
    void translateReturnStmt(ir::BasicBlock *block, ast::ReturnStmt *stmt);

    ir::Value *translateExpr(ir::BasicBlock *block, ast::Expr *expr);
    ir::Value *translateDeclRefExpr(
        ir::BasicBlock *block, ast::DeclRefExpr *expr);
    ir::Value *translateConstantExpr(
        ir::BasicBlock *block, ast::ConstantExpr *expr);
    ir::Value *translateUnaryExpr(ir::BasicBlock *block, ast::UnaryExpr *expr);
    ir::Value *translateBinaryExpr(
        ir::BasicBlock *block, ast::BinaryExpr *expr);
    ir::Value *translateCommaExpr(ir::BasicBlock *block, ast::CommaExpr *expr);
    inline ir::Value *translateParenExpr(
        ir::BasicBlock *block, ast::ParenExpr *expr);
    ir::Value *translateCallExpr(ir::BasicBlock *block, ast::CallExpr *expr);
    ir::Value *translateSubscriptExpr(
        ir::BasicBlock *block, ast::SubscriptExpr *expr);

    void translateArrayInitAssign(ir::Value *address, ast::InitListExpr *data);
    ir::Value *bicastIntFP(ir::Value *value, ir::Type *expected);

protected:
    ASTToIRTranslator(std::string_view name)
        : module_(std::make_unique<ir::Module>(name.data())) {}

private:
    struct LoopDescription {
        ir::BasicBlock *branchCond;
        ir::BasicBlock *branchLoop;
        ir::BasicBlock *branchExit;
    };

    struct TranslateState {
        //! lval can both be accessed as address and value
        //! NOTE: the following 2 value updates in every expr translation,
        //! and the addressOfPrevExpr is set to nullptr if the expr do not
        //! return a lval
        //! NOTE: both valueOfPrevExpr and addressOfPrevExpr equal nullptr
        //! appears only when the expr is a void one
        //! NOTE: valueOfPrevExpr indicates the value of previous expression
        //! even it is actually an address value
        //! WANRING: never use it before an explicit expr-translation is called
        ir::Value *valueOfPrevExpr   = nullptr;
        ir::Value *addressOfPrevExpr = nullptr;
        //! the block where the current translation point is actually located
        //! NOTE: some translations may insert middle blocks that will break the
        //! original flow, and the currentBlock helps to locate the real
        //! position where the following instructions should be inserted
        //! NOTE: currentBlock is always valid
        ir::BasicBlock *currentBlock = nullptr;
        //! address map of all the variables
        std::map<ast::VarLikeDecl *, ir::Value *> variableAddressTable;
        //! map for loop description
        std::map<ast::Stmt *, LoopDescription> loopTable;
    };

    std::unique_ptr<ir::Module> module_;
    TranslateState              state_;
};

inline void ASTToIRTranslator::translateDeclStmt(
    ir::BasicBlock *block, ast::DeclStmt *stmt) {
    for (auto decl : *stmt) { translateVarDecl(decl); }
}

inline void ASTToIRTranslator::translateCompoundStmt(
    ir::BasicBlock *block, ast::CompoundStmt *stmt) {
    for (auto e : *stmt) { translateStmt(state_.currentBlock, e); }
}

inline ir::Value *ASTToIRTranslator::translateParenExpr(
    ir::BasicBlock *block, ast::ParenExpr *expr) {
    //! paren-expr derives the attribute of value, so forward it directly
    return translateExpr(block, expr->inner);
}

} // namespace slime::visitor
