#include "76.h"

#include "2.h"
#include "10.h"
#include "5.h"

namespace slime::visitor {

using namespace ast;

void ASTDumpVisitor::visit(TranslationUnit* e) {
    for (auto decl : *e) {
        switch (decl->declId) {
            case DeclID::Var: {
                visit(decl->asVarDecl());
            } break;
            case DeclID::Function: {
                visit(decl->asFunctionDecl());
            } break;
            default: {
                assert(false && "unexpected ParamVarDecl at the top level");
            } break;
        }
    }
}

void ASTDumpVisitor::visit(VarDecl* e) {
    os() << "Var: " << e->name << std::endl;
    DepthGuard guard(depth_);
    visit(e->type());
    assert(e->initValue != nullptr);
    visit(e->initValue);
}

void ASTDumpVisitor::visit(ParamVarDecl* e) {
    if (e->name.empty()) {
        os() << "ParamVar" << std::endl;
    } else {
        os() << "ParamVar: " << e->name << std::endl;
    }
    visit(e->asParamVarDecl()->type());
}

void ASTDumpVisitor::visit(FunctionDecl* e) {
    os() << "Function: " << e->name << std::endl;
    DepthGuard guard(depth_);
    visit(e->proto());
    if (e->body != nullptr && !e->specifier->isExtern()) { visit(e->body); }
}

void ASTDumpVisitor::visit(Stmt* e) {
    switch (e->stmtId) {
        case StmtID::Null: {
            os() << "NullStmt" << std::endl;
        } break;
        case StmtID::Decl: {
            os() << "DeclStmt" << std::endl;
            DepthGuard guard(depth_);
            for (auto decl : *e->asDeclStmt()) { visit(decl); }
        } break;
        case StmtID::Expr: {
            visit(e->asExprStmt()->unwrap());
        } break;
        case StmtID::Compound: {
            os() << "CompoundStmt" << std::endl;
            DepthGuard guard(depth_);
            for (auto stmt : *e->asCompoundStmt()) { visit(stmt); }
        } break;
        case StmtID::If: {
            os() << "IfStmt" << std::endl;
            auto       stmt = e->asIfStmt();
            DepthGuard guard(depth_);
            visit(stmt->condition);
            visit(stmt->branchIf);
            if (stmt->branchElse != nullptr
                && stmt->branchElse->stmtId != StmtID::Null) {
                visit(stmt->branchElse);
            }
        } break;
        case StmtID::Do: {
            os() << "DoStmt" << std::endl;
            auto       stmt = e->asDoStmt();
            DepthGuard guard(depth_);
            visit(stmt->condition);
            visit(stmt->loopBody);
        } break;
        case StmtID::While: {
            os() << "WhileStmt" << std::endl;
            auto       stmt = e->asWhileStmt();
            DepthGuard guard(depth_);
            visit(stmt->condition);
            visit(stmt->loopBody);
        } break;
        case StmtID::For: {
            os() << "ForStmt" << std::endl;
            auto       stmt = e->asForStmt();
            DepthGuard guard(depth_);
            visit(stmt->init);
            visit(stmt->condition);
            visit(stmt->increment);
            visit(stmt->loopBody);
        } break;
        case StmtID::Break: {
            os() << "BreakStmt" << std::endl;
        } break;
        case StmtID::Continue: {
            os() << "ContinueStmt" << std::endl;
        } break;
        case StmtID::Return: {
            os() << "ReturnStmt" << std::endl;
            auto stmt = e->asReturnStmt();
            assert(stmt->returnValue != nullptr);
            if (stmt->returnValue->stmtId != StmtID::Null) {
                DepthGuard guard(depth_);
                visit(stmt->returnValue);
            }
        } break;
    }
}

void ASTDumpVisitor::visit(Expr* e) {
    switch (e->exprId) {
        case ExprID::DeclRef: {
            os() << "DeclRef: " << e->asDeclRef()->source->name << std::endl;
        } break;
        case ExprID::Constant: {
            auto c = e->asConstant();
            switch (c->type) {
                case ConstantType::i32: {
                    os() << "IntegerLiteral: " << c->i32 << std::endl;
                } break;
                case ConstantType::f32: {
                    os() << "FloatingLiteral: " << c->f32 << std::endl;
                } break;
            }
        } break;
        case ExprID::Unary: {
            visit(e->asUnary());
        } break;
        case ExprID::Binary: {
            visit(e->asBinary());
        } break;
        case ExprID::Comma: {
            os() << "CommaExpr" << std::endl;
            DepthGuard guard(depth_);
            for (auto expr : *e->asComma()) { visit(expr); }
        } break;
        case ExprID::Paren: {
            os() << "ParenExpr" << std::endl;
            DepthGuard guard(depth_);
            visit(e->asParen()->inner);
        } break;
        case ExprID::Stmt: {
            assert(false && "unsupported StmtExpr");
        } break;
        case ExprID::Call: {
            os() << "CallExpr" << std::endl;
            DepthGuard guard(depth_);
            visit(e->asCall()->callable);
            for (auto arg : e->asCall()->argList) { visit(arg); }
        } break;
        case ExprID::Subscript: {
            os() << "SubscriptExpr" << std::endl;
            DepthGuard guard(depth_);
            visit(e->asSubscript()->lhs);
            visit(e->asSubscript()->rhs);
        } break;
        case ExprID::InitList: {
            os() << "InitList" << std::endl;
            DepthGuard guard(depth_);
            for (auto expr : *e->asInitList()) { visit(expr); }
        } break;
        case ExprID::NoInit: {
            os() << "NoInitExpr" << std::endl;
        } break;
    }
}

void ASTDumpVisitor::visit(UnaryExpr* e) {
    static const char* sops[5]{"+", "-", "!", "~", "()"};
    assert(e->op != UnaryOperator::Paren);
    os() << "UnaryOperator: " << sops[static_cast<int>(e->op)] << std::endl;
    DepthGuard guard(depth_);
    visit(e->operand);
}

void ASTDumpVisitor::visit(BinaryExpr* e) {
    static const char* sops[21]{
        "=", "+",  "-", "*",  "/",  "%",  "&",  "|",  "^", "&&", "||",
        "<", "<=", ">", ">=", "==", "!=", "<<", ">>", ",", "[]",
    };
    assert(e->op != BinaryOperator::Subscript);
    assert(e->op != BinaryOperator::Comma);
    os() << "BinaryOperator: " << sops[static_cast<int>(e->op)] << std::endl;
    DepthGuard guard(depth_);
    visit(e->lhs);
    visit(e->rhs);
}

void ASTDumpVisitor::visit(Type* e) {
    static const char* stypes[3]{"int", "float", "void"};
    switch (e->typeId) {
        case TypeID::None: {
            os() << "NoneType" << std::endl;
        } break;
        case TypeID::Unresolved: {
            os() << "UnresolvedType" << std::endl;
        } break;
        case TypeID::Builtin: {
            os() << "BuiltinType: "
                 << stypes[static_cast<int>(e->asBuiltin()->type)] << std::endl;
        } break;
        case TypeID::Array: {
            os() << "ArrayType: "
                 << stypes[static_cast<int>(
                        e->asArray()->type->asBuiltin()->type)]
                 << std::endl;
            DepthGuard guard(depth_);
            for (auto dim : *e->asArray()) { visit(dim); }
        } break;
        case TypeID::IncompleteArray: {
            os() << "IncompleteArrayType: "
                 << stypes[static_cast<int>(
                        e->asIncompleteArray()->type->asBuiltin()->type)]
                 << std::endl;
            DepthGuard guard(depth_);
            for (auto dim : *e->asIncompleteArray()) { visit(dim); }
        } break;
        case TypeID::FunctionProto: {
            os() << "FunctionProtoType" << std::endl;
            DepthGuard guard(depth_);
            auto       fn = e->asFunctionProto();
            visit(fn->returnType);
            for (auto param : *fn) { visit(param); }
        } break;
    }
}

} // namespace slime::visitor