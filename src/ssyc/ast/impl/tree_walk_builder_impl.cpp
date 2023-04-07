#include "../typedef.h"

#include <string_view>

namespace ssyc::ast {

void AbstractAstNode::flattenInsert(TreeWalkState &state) const {}

void Program::flattenInsert(TreeWalkState &state) const {
    for (const auto &e : unitList) {
        if (std::holds_alternative<VarDecl *>(e)) {
            state.push(std::get<VarDecl *>(e));
        } else {
            state.push(std::get<FunctionDecl *>(e));
        }
    }
}

void Type::flattenInsert(TreeWalkState &state) const {}

void Decl::flattenInsert(TreeWalkState &state) const {}

void Expr::flattenInsert(TreeWalkState &state) const {}

void Stmt::flattenInsert(TreeWalkState &state) const {}

void QualifiedType::flattenInsert(TreeWalkState &state) const {
    state.push(type);
}

void BuiltinType::flattenInsert(TreeWalkState &state) const {}

void ArrayType::flattenInsert(TreeWalkState &state) const {}

void ConstantArrayType::flattenInsert(TreeWalkState &state) const {
    state.push(elementType);
    state.push(length);
}

void VariableArrayType::flattenInsert(TreeWalkState &state) const {
    state.push(elementType);
    state.push(length);
}

void FunctionProtoType::flattenInsert(TreeWalkState &state) const {
    state.push(retvalType);
    for (const auto &e : params) { state.push(e); }
}

void VarDecl::flattenInsert(TreeWalkState &state) const {
    state.push(varType);
    state.push(initVal);
}

void ParamVarDecl::flattenInsert(TreeWalkState &state) const {
    state.push(paramType);
    if (defaultVal != nullptr) { state.push(defaultVal); }
}

void FunctionDecl::flattenInsert(TreeWalkState &state) const {
    state.push(protoType);
    if (body != nullptr) { state.push(body); }
}

void InitListExpr::flattenInsert(TreeWalkState &state) const {
    for (const auto &e : elementList) { state.push(e); }
}

void CallExpr::flattenInsert(TreeWalkState &state) const {
    state.push(func);
    for (const auto &e : params) { state.push(e); }
}

void ParenExpr::flattenInsert(TreeWalkState &state) const {
    state.push(innerExpr);
}

void UnaryOperatorExpr::flattenInsert(TreeWalkState &state) const {
    state.push(operand);
}

void BinaryOperatorExpr::flattenInsert(TreeWalkState &state) const {
    state.push(lhs);
    state.push(rhs);
}

void DeclRef::flattenInsert(TreeWalkState &state) const {}

void Literal::flattenInsert(TreeWalkState &state) const {}

void IntegerLiteral::flattenInsert(TreeWalkState &state) const {}

void FloatingLiteral::flattenInsert(TreeWalkState &state) const {}

void StringLiteral::flattenInsert(TreeWalkState &state) const {}

void NullStmt::flattenInsert(TreeWalkState &state) const {}

void CompoundStmt::flattenInsert(TreeWalkState &state) const {
    for (const auto &e : stmtList) {
        if (std::holds_alternative<Stmt *>(e)) {
            state.push(std::get<Stmt *>(e));
        } else {
            state.push(std::get<Expr *>(e));
        }
    }
}

void DeclStmt::flattenInsert(TreeWalkState &state) const {
    for (const auto &e : varDeclList) { state.push(e); }
}

void IfElseStmt::flattenInsert(TreeWalkState &state) const {
    state.push(cond);
    if (std::holds_alternative<Expr *>(onTrue)) {
        state.push(std::get<Expr *>(onTrue));
    } else {
        state.push(std::get<Stmt *>(onTrue));
    }
    if (std::holds_alternative<std::monostate>(onFalse)) {
    } else {
        if (std::holds_alternative<Expr *>(onFalse)) {
            state.push(std::get<Expr *>(onFalse));
        } else {
            state.push(std::get<Stmt *>(onFalse));
        }
    }
}

void WhileStmt::flattenInsert(TreeWalkState &state) const {
    state.push(cond);
    if (std::holds_alternative<Stmt *>(body)) {
        state.push(std::get<Stmt *>(body));
    } else {
        state.push(std::get<Expr *>(body));
    }
}

void DoWhileStmt::flattenInsert(TreeWalkState &state) const {
    state.push(cond);
    if (std::holds_alternative<Stmt *>(body)) {
        state.push(std::get<Stmt *>(body));
    } else {
        state.push(std::get<Expr *>(body));
    }
}

void ForStmt::flattenInsert(TreeWalkState &state) const {
    if (std::holds_alternative<Expr *>(init)) {
        state.push(std::get<Expr *>(init));
    } else if (std::holds_alternative<DeclStmt *>(init)) {
        state.push(std::get<DeclStmt *>(init));
    }

    if (std::holds_alternative<Expr *>(cond)) {
        state.push(std::get<Expr *>(cond));
    }

    if (std::holds_alternative<Expr *>(loop)) {
        state.push(std::get<Expr *>(loop));
    }

    if (std::holds_alternative<Expr *>(body)) {
        state.push(std::get<Expr *>(body));
    } else {
        state.push(std::get<Stmt *>(body));
    }
}

void ContinueStmt::flattenInsert(TreeWalkState &state) const {}

void BreakStmt::flattenInsert(TreeWalkState &state) const {}

void ReturnStmt::flattenInsert(TreeWalkState &state) const {
    if (retval.has_value()) { state.push(retval.value()); }
}

} // namespace ssyc::ast
