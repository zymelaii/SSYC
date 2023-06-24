#include "decl.h"
#include "expr.h"

namespace slime::ast {

ParamVarDecl::ParamVarDecl(std::string_view name, DeclSpecifier *specifier)
    : VarDecl(DeclID::ParamVar, name, specifier, NoInitExpr::get()) {}

ParamVarDecl::ParamVarDecl(DeclSpecifier *specifier)
    : VarDecl(DeclID::ParamVar, "", specifier, NoInitExpr::get()) {}

VarDecl *DeclSpecifier::createVarDecl(std::string_view name) {
    return VarDecl::create(name, this, NoInitExpr::get());
}

} // namespace slime::ast
