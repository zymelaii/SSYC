//#include "../common.h"
#include "../typedef.h"

#include <string_view>

namespace ssyc::ast {

using std::operator""sv;

inline void AbstractAstNode::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Unknown;
}

inline void Type::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.hint = "(anonymous)"sv;
}

inline void Decl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.hint = "(anonymous)"sv;
}

inline void Expr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.hint = "(anonymous)"sv;
}

inline void Stmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.hint = "(anonymous)"sv;
}

inline void QualifiedType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "Qualified type"sv;
    brief.hint = "const"sv;
}

inline void BuiltinType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "Builtin type"sv;

    switch (typeId) {
        case Type::Void: {
            brief.hint = "void"sv;
        } break;
        case Type::Int: {
            brief.hint = "int"sv;
        } break;
        case Type::Float: {
            brief.hint = "float"sv;
        } break;
    }
}

inline void ArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "Array type"sv;
}

inline void ConstantArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "ConstantArray type"sv;
    //! TODO: add hint: array length
}

inline void VariableArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "VariableArray type"sv;
    //! TODO: add hint: array length
}

inline void FunctionProtoType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "FunctionProto type"sv;
}

inline void VarDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "Var"sv;
    brief.hint = ident;
}

inline void ParamVarDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "ParamVar"sv;
    brief.hint = ident.empty() ? "(anonymous)" : ident;
}

inline void FunctionDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "Function"sv;
    brief.hint = ident;
}

inline void InitListExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "InitList"sv;
}

inline void CallExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Call"sv;
}

inline void ParenExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Paren"sv;
}

inline void UnaryOperatorExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "UnaryOperator"sv;
    //! TODO: add hint: operator
}

inline void BinaryOperatorExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "BinaryOperator"sv;
    //! TODO: add hint: operator
}

inline void DeclRef::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "DeclRef"sv;
    brief.hint = ref->ident;
}

inline void Literal::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Literal"sv;
}

inline void IntegerLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "IntegerLiteral"sv;
    brief.hint = literal;
}

inline void FloatingLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "FloatingLiteral"sv;
    brief.hint = literal;
}

inline void StringLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "StringLiteral"sv;
}

inline void NullStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Null"sv;
}

inline void CompoundStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Compound"sv;
}

inline void DeclStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Decl"sv;
}

inline void IfElseStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "If"sv;
}

inline void WhileStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "While"sv;
}

inline void DoWhileStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Do"sv;
}

inline void ForStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "For"sv;
}

inline void ContinueStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Continue"sv;
}

inline void BreakStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Break"sv;
}

inline void ReturnStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Return"sv;
}

inline void Program::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Unknown;
    brief.name = "Module"sv;
}

} // namespace ssyc::ast
