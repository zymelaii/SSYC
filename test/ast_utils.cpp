#include <ssyc/ast/ast.h>
#include <ssyc/ast/utils/translate.h>
#include <ssyc/ast/utils/treeview.h>
#include <ssyc/ast/utils/ctor_helper.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace ssyc::ast::utils::ctor;
using UnaryOpType = ssyc::ast::UnaryOperatorExpr::UnaryOpType;
using BinaryOpType = ssyc::ast::BinaryOperatorExpr::BinaryOpType;
using ssyc::ast::utils::operator<<;
using ssyc::ast::utils::getNodeBrief;

template <typename T>
struct unsafe_wrapper_t {
    unsafe_wrapper_t()
        : data(T::emit()) {}

    ~unsafe_wrapper_t() {
        ssyc::ast::TreeWalkState state(data);

        auto node = state.next();
        while (node != nullptr) {
            delete node;
            node = state.next();
        }
    }

    decltype(T::emit()) data;
};

TEST(AstCtor, TypeCtor) {
    unsafe_wrapper_t<
        Type::Void
    >{};

    unsafe_wrapper_t<
        Type::Int
    >{};

    unsafe_wrapper_t<
        Type::Float
    >{};

    unsafe_wrapper_t<
        Type::Array<Type::Int, 1>
    >{};

    unsafe_wrapper_t<
        Type::Qualify<
            Type::Qualify<
                Type::Array<
                    Type::Int,
                    1,
                    2,
                    3,
                    4,
                    5
                    >>>
    >{};
}

TEST(AstCtor, StmtCtor) {
    /*!
     * source
     * ===
     * for (int i = 0; i < 1024; i = i + 1) {
     *     if (i % 19 == 7) break;
     * }
     */

    unsafe_wrapper_t<
        Stmt::For<
            Stmt::Decl<Decl::Var<Type::Int, Expr::Integer<0>>>,
            Expr::BinaryOp<
                BinaryOpType::Lt,
                Expr::SymRef<Decl::Var<Type::Int>>,
                Expr::Integer<1024>>,
            Expr::BinaryOp<
                BinaryOpType::Assign,
                Expr::SymRef<Decl::Var<Type::Int>>,
                Expr::BinaryOp<
                    BinaryOpType::Add,
                    Expr::SymRef<Decl::Var<Type::Int>>,
                    Expr::Integer<1>>>,
            Stmt::Compound<
                Stmt::IfElse<
                    Expr::BinaryOp<
                        BinaryOpType::Eq,
                        Expr::BinaryOp<
                            BinaryOpType::Mod,
                            Expr::SymRef<Decl::Var<Type::Int>>,
                            Expr::Integer<19>>,
                        Expr::Integer<7>>,
                    Stmt::Break>>>
    >{};
}

TEST(AstCtor, DeclCtor) {
    unsafe_wrapper_t<
        Decl::Function<
            Type::Function<Type::Int>,
            Stmt::Compound<
                Stmt::Return<Expr::Integer<3>>>>
    >{};
}

TEST(AstCtor, ProgramCtor) {
    unsafe_wrapper_t<Module<
        Decl::Var<Type::Int, Expr::Integer<3>>,
        Decl::Var<Type::Int, Expr::Integer<5>>,
        Decl::Function<
            Type::Function<Type::Int>,
            Stmt::Compound<Stmt::Return<Expr::BinaryOp<
                BinaryOpType::Add,
                Expr::SymRef<Decl::Var<Type::Int>>,
                Expr::SymRef<Decl::Var<Type::Int>>>>>>>
    >{};
}
