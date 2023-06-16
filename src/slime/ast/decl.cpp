#include "decl.h"
#include "expr.h"

namespace slime::ast {

ParamVarDecl::ParamVarDecl(std::string_view name, Type *type)
    : VarDecl(DeclID::ParamVar, name, type, NoInitExpr::get()) {}

ParamVarDecl::ParamVarDecl(Type *type)
    : VarDecl(DeclID::ParamVar, "", type, NoInitExpr::get()) {}

} // namespace slime::ast
