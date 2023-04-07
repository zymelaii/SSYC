#include <ssyc/ast/ast.h>
#include <ssyc/ast/utils/translate.h>
#include <ssyc/ast/utils/treeview.h>
#include <ssyc/ast/utils/ctor_helper.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace ssyc::ast::utils::ctor;
using ssyc::ast::utils::operator<<;
using ssyc::ast::utils::getNodeBrief;

TEST(AstTranslate, getNodeBrief) {}

TEST(AstCtor, TypeCtor) {
    std::cerr << Type::Void::emit() << Type::Int::emit() << Type::Float::emit();

    std::cerr << Type::Array<Type::Int, 1>::emit();

    std::cerr << Type::Qualify<
        Type::Qualify<Type::Array<Type::Int, 1, 2, 3, 4, 5, 6>>>::emit();

    std::cerr << Type::Array<Type::Int, 1>::emit();

    std::cerr << Type::Array<Type::Int, 1>::emit();
}

TEST(AstCtor, StmtCtor) {
    std::cerr << Stmt::For<
        Stmt::Null,
        Stmt::Null,
        Stmt::Null,
        Stmt::Compound<Stmt::Break, Stmt::Return<>>>::emit();
}

TEST(AstCtor, DeclCtor) {
    std::cerr << Decl::Function<
        Type::Function<Type::Int>,
        Stmt::Compound<Stmt::Return<Expr::Integer<3>>>>::emit();
}

TEST(AstCtor, ProgramCtor) {
    auto program = Module<
        Decl::Var<Type::Int, Expr::Integer<3>>,
        Decl::Var<Type::Int, Expr::Integer<5>>,
        Decl::Function<
            Type::Function<Type::Int>,
            Stmt::Compound<Stmt::Return<Expr::BinaryOp<
                ssyc::ast::BinaryOperatorExpr::BinaryOpType::Add,
                Expr::SymRef<Decl::Var<Type::Int>>,
                Expr::SymRef<Decl::Var<Type::Int>>>>>>>::emit();

    std::cerr << program << std::endl;
}
