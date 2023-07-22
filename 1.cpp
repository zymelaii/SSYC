#include "2.h"
#include "5.h"

namespace slime::ast {

VarDecl::VarDecl(std::string_view name, DeclSpecifier *specifier)
    : VarDecl(name, specifier, NoInitExpr::get()) {}

ParamVarDecl::ParamVarDecl(std::string_view name, DeclSpecifier *specifier)
    : VarLikeDecl(DeclID::ParamVar, name, specifier, NoInitExpr::get()) {}

ParamVarDecl::ParamVarDecl(DeclSpecifier *specifier)
    : ParamVarDecl("", specifier) {}

VarDecl *DeclSpecifier::createVarDecl(std::string_view name) {
    return VarDecl::create(name, this, NoInitExpr::get());
}

} // namespace slime::ast