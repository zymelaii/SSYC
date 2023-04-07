#include "../typedef.h"

#include <string_view>

namespace ssyc::ast {

using std::operator""sv;

void AbstractAstNode::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Unknown;
}

void Program::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Unknown;
    brief.name = "Module"sv;
}

void Type::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.hint = "(anonymous)"sv;
}

void Decl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.hint = "(anonymous)"sv;
}

void Expr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.hint = "(anonymous)"sv;
}

void Stmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.hint = "(anonymous)"sv;
}

void QualifiedType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "Qualified type"sv;
    brief.hint = "const"sv;
}

void BuiltinType::acquire(NodeBrief &brief) const {
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

void ArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "Array type"sv;
}

void ConstantArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "ConstantArray type"sv;
    //! TODO: add hint: array length
}

void VariableArrayType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "VariableArray type"sv;
    //! TODO: add hint: array length
}

void FunctionProtoType::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Type;
    brief.name = "FunctionProto type"sv;
}

void VarDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "Var"sv;
    brief.hint = ident;
}

void ParamVarDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "ParamVar"sv;
    brief.hint = ident.empty() ? "(anonymous)" : ident;
}

void FunctionDecl::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Decl;
    brief.name = "Function"sv;
    brief.hint = ident;
}

void InitListExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "InitList"sv;
}

void CallExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Call"sv;
}

void ParenExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Paren"sv;
}

void UnaryOperatorExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "UnaryOperator"sv;
    switch (op) {
        case UnaryOpType::Pos: {
            brief.hint = "'+'"sv;
        } break;
        case UnaryOpType::Neg: {
            brief.hint = "'-'"sv;
        } break;
        case UnaryOpType::LNot: {
            brief.hint = "'!'"sv;
        } break;
        case UnaryOpType::Inv: {
            brief.hint = "'~'"sv;
        } break;
    }
}

void BinaryOperatorExpr::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "BinaryOperator"sv;
    switch (op) {
        case BinaryOpType::Add: {
            brief.hint = "'+'"sv;
        } break;
        case BinaryOpType::Sub: {
            brief.hint = "'-'"sv;
        } break;
        case BinaryOpType::Mul: {
            brief.hint = "'*'"sv;
        } break;
        case BinaryOpType::Div: {
            brief.hint = "'/'"sv;
        } break;
        case BinaryOpType::Mod: {
            brief.hint = "'%'"sv;
        } break;
        case BinaryOpType::Lsh: {
            brief.hint = "'<<'"sv;
        } break;
        case BinaryOpType::Rsh: {
            brief.hint = "'>>'"sv;
        } break;
        case BinaryOpType::And: {
            brief.hint = "'&'"sv;
        } break;
        case BinaryOpType::Or: {
            brief.hint = "'|'"sv;
        } break;
        case BinaryOpType::Xor: {
            brief.hint = "'^'"sv;
        } break;
        case BinaryOpType::Eq: {
            brief.hint = "'=='"sv;
        } break;
        case BinaryOpType::NE: {
            brief.hint = "'!='"sv;
        } break;
        case BinaryOpType::Gt: {
            brief.hint = "'>'"sv;
        } break;
        case BinaryOpType::Lt: {
            brief.hint = "'<'"sv;
        } break;
        case BinaryOpType::GE: {
            brief.hint = "'>='"sv;
        } break;
        case BinaryOpType::LE: {
            brief.hint = "'<='"sv;
        } break;
        case BinaryOpType::LAnd: {
            brief.hint = "'&&'"sv;
        } break;
        case BinaryOpType::LOr: {
            brief.hint = "'||'"sv;
        } break;
        case BinaryOpType::Comma: {
            brief.hint = "','"sv;
        } break;
    }
}

void DeclRef::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "DeclRef"sv;
    brief.hint = ref->ident;
}

void Literal::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "Literal"sv;
}

void IntegerLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "IntegerLiteral"sv;
    brief.hint = literal;
}

void FloatingLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "FloatingLiteral"sv;
    brief.hint = literal;
}

void StringLiteral::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Expr;
    brief.name = "StringLiteral"sv;
}

void NullStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Null"sv;
}

void CompoundStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Compound"sv;
}

void DeclStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Decl"sv;
}

void IfElseStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "If"sv;
}

void WhileStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "While"sv;
}

void DoWhileStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Do"sv;
}

void ForStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "For"sv;
}

void ContinueStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Continue"sv;
}

void BreakStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Break"sv;
}

void ReturnStmt::acquire(NodeBrief &brief) const {
    brief.type = NodeBaseType::Stmt;
    brief.name = "Return"sv;
}

} // namespace ssyc::ast
