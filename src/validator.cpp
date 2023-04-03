#include "validator.h"

namespace ssyc::ast {

template <>
bool validate(TypeDecl *e, ParserContext *context) {
    if (e->optSubscriptList.has_value()) {
        const auto &list = e->optSubscriptList.value();
        for (const auto &e : list) {
            //! FIXME: 除了 FnCall 的其它表达式也可能构成常量值
            //! NOTE: 上面的情况推荐在语法分析阶段直接合并
            if (e->type != Expr::Type::ConstExpr) { return false; }
        }
    }
    return true;
}

template <>
bool validate(DeclStatement *e, ParserContext *context) {
    //! TODO: 赋值类型匹配与隐式转换
    for (const auto &[decl, expr] : e->declList) {
        //! 常量必须在定义时初始化
        if (decl->constant && expr == nullptr) { return false; }
        //! 数组定义必须使用元素列表赋值
        if (decl->optSubscriptList.has_value()) {
            if (expr->type != Expr::Type::Orphan) { return false; }
            auto orphan = static_cast<OrphanExpr *>(expr)->ref;
            if (std::holds_alternative<InitializeList *>(orphan)) {
                return false;
            }
        }
    }
    return true;
}

template <>
bool validate(FnCallExpr *e, ParserContext *context) {
    //! TODO: 类型匹配
    return true;
}

template <>
bool validate(Statement *e, ParserContext *context) {
    switch (e->type) {
        case Statement::Type::Decl: {
            return validate(static_cast<DeclStatement *>(e), context);
        } break;
        case Statement::Type::Compound: {
            return validate(static_cast<CompoundStatement *>(e), context);
        } break;
        case Statement::Type::Expr: {
            return validate(static_cast<ExprStatement *>(e), context);
        } break;
        case Statement::Type::IfElse: {
            return validate(static_cast<IfElseStatement *>(e), context);
        } break;
        case Statement::Type::While: {
            return validate(static_cast<WhileStatement *>(e), context);
        } break;
        case Statement::Type::Break: {
            return validate(static_cast<BreakStatement *>(e), context);
        } break;
        case Statement::Type::Continue: {
            return validate(static_cast<ContinueStatement *>(e), context);
        } break;
        case Statement::Type::Return: {
            return validate(static_cast<ReturnStatement *>(e), context);
        } break;
    }
}

template <>
bool validate(Expr *e, ParserContext *context) {
    switch (e->type) {
        case Expr::Type::Unary: {
            return validate(static_cast<UnaryExpr *>(e), context);
        } break;
        case Expr::Type::Binary: {
            return validate(static_cast<BinaryExpr *>(e), context);
        } break;
        case Expr::Type::FnCall: {
            return validate(static_cast<FnCallExpr *>(e), context);
        } break;
        case Expr::Type::ConstExpr: {
            return validate(static_cast<ConstExprExpr *>(e), context);
        } break;
        case Expr::Type::Orphan: {
            return validate(static_cast<OrphanExpr *>(e), context);
        } break;
    }
}

template <>
bool validate(Program *e, ParserContext *context) {
    //! NOTE: 重复符号问题应该在语法解析阶段解决
    //! 程序必须包含返回值为 int，参数列表为空的 main 函数
    auto resp = std::find_if(
        e->funcFlows.begin(), e->funcFlows.end(), [](const auto &e) {
            const auto &[index, func] = e;
            //! FIXME: FuncDef::ident 之后会被替换为符号哈希
            return func->ident == "main"
                && func->retType == FuncDef::Type::Integer
                && func->params.size() == 0;
        });
    if (resp == e->funcFlows.end()) { return false; }
    for (auto &[id, decl] : e->declFlows) {
        if (!validate(decl, context)) { return false; }
    }
    for (auto &[id, func] : e->funcFlows) {
        if (!validate(func, context)) { return false; }
    }
    return true;
}

} // namespace ssyc::ast
