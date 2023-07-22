#include "10.h"
#include "5.h"

namespace slime::ast {

Type* Stmt::implicitValueType() {
    switch (stmtId) {
        case StmtID::Null:
        case StmtID::Decl:
        case StmtID::If:
        case StmtID::Do:
        case StmtID::While:
        case StmtID::For:
        case StmtID::Break:
        case StmtID::Continue: {
            return BuiltinType::getVoidType();
        } break;
        case StmtID::Return: {
            return asReturnStmt()->returnValue->implicitValueType();
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