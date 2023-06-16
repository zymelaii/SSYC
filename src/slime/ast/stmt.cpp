#include "stmt.h"
#include "expr.h"

namespace slime::ast {

Type* Stmt::implicitValueType() {
    switch (stmtId) {
        case StmtID::Null:
        case StmtID::Decl:
        case StmtID::If:
        case StmtID::Do:
        case StmtID::While:
        case StmtID::Break:
        case StmtID::Continue:
        case StmtID::Return: {
            return BuiltinType::getVoidType();
        } break;
        case StmtID::Expr: {
            return asExprStmt()->unwrap()->valueType;
        } break;
        case StmtID::Compound: {
            auto stmt = asCompoundStmt();
            if (stmt->size() == 0) {
                return NoneType::get();
            } else {
                return stmt->tail()->value()->implicitValueType();
            }
        } break;
    }
}

} // namespace slime::ast
