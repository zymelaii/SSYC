#pragma once

#include "0.h"
#include <iostream>
#include <iomanip>
#include <stddef.h>
#include <assert.h>

namespace slime::visitor {

class ASTDumpVisitor {
public:
    ASTDumpVisitor(const ASTDumpVisitor&)            = delete;
    ASTDumpVisitor& operator=(const ASTDumpVisitor&) = delete;

    static ASTDumpVisitor* createWithOstream(std::ostream* os) {
        return new ASTDumpVisitor(os);
    }

    void setIndent(size_t w) {
        assert(w > 0);
        indent_ = w;
    }

    void visit(ast::TranslationUnit* e);
    void visit(ast::VarDecl* e);
    void visit(ast::ParamVarDecl* e);
    void visit(ast::FunctionDecl* e);
    void visit(ast::Stmt* e);
    void visit(ast::Expr* e);
    void visit(ast::UnaryExpr* e);
    void visit(ast::BinaryExpr* e);
    void visit(ast::Type* e);

protected:
    ASTDumpVisitor(std::ostream* os)
        : os_{os}
        , depth_{0} {
        setIndent(2);
    }

    std::ostream& os() {
        assert(depth_ <= 1024 && "AST is too deep");
        *os_ << std::setw(depth_ * indent_) << "";
        return *os_;
    }

    struct DepthGuard {
        DepthGuard(int& value)
            : value_{value} {
            ++value_;
        }

        ~DepthGuard() {
            --value_;
        }

        int& value_;
    };

private:
    std::ostream* os_;
    size_t        indent_;
    int           depth_;
};

} // namespace slime::visitor