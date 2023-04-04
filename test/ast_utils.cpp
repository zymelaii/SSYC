#include <ssyc/ast/ast.h>
#include <ssyc/ast/utils/translate.h>
#include <ssyc/ast/utils/treeview.h>
#include <ssyc/ast/utils/ctor_helper.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace ssyc::ast::utils::ctor;
using ssyc::ast::utils::getNodeBrief;

TEST(AstTranslate, getNodeBrief) {}

TEST(AstCtor, TypeCtor) {
    Type::Void::emit();
    Type::Int::emit();
    Type::Float::emit();

    Type::Array<Type::Int, 1>::emit();

    Type::Qualify<
        Type::Qualify<Type::Array<Type::Int, 1, 2, 3, 4, 5, 6>>>::emit();

    std::cerr << getNodeBrief(
        Type::Qualify<
            Type::Qualify<Type::Array<Type::Int, 1, 2, 3, 4, 5, 6>>>::emit())
              << std::endl;

    ssyc::ast::utils::treeview(Type::Array<Type::Int, 1>::emit());
}
