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
    std::cerr << Type::Void::emit();
    std::cerr << Type::Int::emit();
    std::cerr << Type::Float::emit();

    std::cerr << Type::Array<Type::Int, 1>::emit();

    std::cerr << Type::Qualify<
        Type::Qualify<Type::Array<Type::Int, 1, 2, 3, 4, 5, 6>>>::emit();

    std::cerr << (Type::Array<Type::Int, 1>::emit());
}
