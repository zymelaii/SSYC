#pragma once

#include "type.h"
#include "scope.h"

#include <slime/utils/list.h>
#include <slime/utils/cast.def>
#include <slime/utils/traits.h>
#include <stdint.h>
#include <string_view>

namespace slime::ast {

struct FunctionProtoType;
struct CompoundStmt;
struct ParamVarDecl;
struct VarDecl;
struct FunctionDecl;

using ParamVarDeclList = utils::ListTrait<ParamVarDecl *>;

enum class DeclID {
    Var,
    ParamVar,
    Function,
};

enum class NamedDeclSpecifier {
    Extern = 0b0001,
    Static = 0b0010,
    Inline = 0b0100,
    Const  = 0b1000,
};

struct DeclSpecifier : public utils::BuildTrait<DeclSpecifier> {
    DeclSpecifier()
        : type{nullptr}
        , specifiers{0} {}

    DeclSpecifier(const DeclSpecifier &specifier)
        : type{specifier.type}
        , specifiers{specifier.specifiers} {}

    DeclSpecifier *clone() {
        return new DeclSpecifier(*this);
    }

    DeclSpecifier &addSpecifier(NamedDeclSpecifier specifier) {
        specifiers |= static_cast<uint8_t>(specifier);
        return *this;
    }

    DeclSpecifier &removeSpecifier(NamedDeclSpecifier specifier) {
        specifiers &= ~static_cast<uint8_t>(specifier);
        return *this;
    }

    bool haveSpecifier(NamedDeclSpecifier specifier) const {
        const auto flag = static_cast<uint8_t>(specifier);
        return (specifiers & flag) == flag;
    }

    bool isExtern() const {
        return haveSpecifier(NamedDeclSpecifier::Extern);
    }

    bool isStatic() const {
        return haveSpecifier(NamedDeclSpecifier::Static);
    }

    bool isInline() const {
        return haveSpecifier(NamedDeclSpecifier::Inline);
    }

    bool isConst() const {
        return haveSpecifier(NamedDeclSpecifier::Const);
    }

    VarDecl *createVarDecl(std::string_view name);

    ParamVarDecl *createParamVarDecl(std::string_view name = "");

    FunctionDecl *createFunctionDecl(
        std::string_view name, ParamVarDeclList &params, CompoundStmt *body);

    Type   *type;
    uint8_t specifiers;
};

struct Decl {
    Decl(DeclID declId)
        : declId{declId} {}

    Decl *decay() {
        return static_cast<Decl *>(this);
    }

    RegisterCastDecl(declId, Var, Decl, DeclID);
    RegisterCastDecl(declId, ParamVar, Decl, DeclID);
    RegisterCastDecl(declId, Function, Decl, DeclID);

    DeclID declId;
};

struct NamedDecl : public Decl {
    NamedDecl(DeclID declId, std::string_view name, DeclSpecifier *specifier)
        : Decl(declId)
        , name{name}
        , specifier{specifier} {}

    Type *type() {
        return specifier->type;
    }

    Scope            scope;
    std::string_view name;
    DeclSpecifier   *specifier;
};

struct DeclaratorDecl : public NamedDecl {
    DeclaratorDecl(
        DeclID declId, std::string_view name, DeclSpecifier *specifier)
        : NamedDecl(declId, name, specifier) {}
};

struct VarLikeDecl : public DeclaratorDecl {
    VarLikeDecl(
        DeclID           declId,
        std::string_view name,
        DeclSpecifier   *specifier,
        Expr            *initValue)
        : DeclaratorDecl(declId, name, specifier)
        , initValue{initValue} {}

    Expr *initValue;
};

struct VarDecl final
    : public VarLikeDecl
    , public utils::BuildTrait<VarDecl> {
    VarDecl(std::string_view name, DeclSpecifier *specifier, Expr *initValue)
        : VarLikeDecl(DeclID::Var, name, specifier, initValue) {}

    VarDecl(std::string_view name, DeclSpecifier *specifier);
};

struct ParamVarDecl final
    : public VarLikeDecl
    , public utils::BuildTrait<ParamVarDecl> {
    ParamVarDecl(std::string_view name, DeclSpecifier *specifier);
    ParamVarDecl(DeclSpecifier *specifier);

    bool isNoEffectParam() {
        return name.empty();
    }
};

struct FunctionDecl final
    : public DeclaratorDecl
    , public ParamVarDeclList {
    FunctionDecl(
        std::string_view  name,
        Type             *returnType,
        ParamVarDeclList &params,
        CompoundStmt     *body)
        : DeclaratorDecl(DeclID::Function, name, DeclSpecifier::create())
        , ParamVarDeclList(std::move(params))
        , body{body}
        , canBeConstExpr{false} {
        TypeList list;
        extractTypeListFromParams(&list, params);
        specifier->type =
            FunctionProtoType::create(returnType, std::move(list));
    }

    FunctionDecl(
        std::string_view  name,
        DeclSpecifier    *specifier,
        ParamVarDeclList &params,
        CompoundStmt     *body)
        : DeclaratorDecl(DeclID::Function, name, specifier)
        , ParamVarDeclList(std::move(params))
        , body{body}
        , canBeConstExpr{false} {}

    static void extractTypeListFromParams(
        TypeList *typeListPtr, const ParamVarDeclList &params) {
        TypeList list;
        for (auto param : *const_cast<ParamVarDeclList *>(&params)) {
            list.insertToTail(param->type());
        }
        new (typeListPtr) TypeList(std::move(list));
    }

    static FunctionDecl *create(
        std::string_view  name,
        DeclSpecifier    *specifier, //<! contains return tyle only
        ParamVarDeclList &params,
        CompoundStmt     *body) {
        TypeList list;
        extractTypeListFromParams(&list, params);
        specifier = specifier->clone();
        specifier->type =
            FunctionProtoType::create(specifier->type, std::move(list));
        return new FunctionDecl(name, specifier, params, body);
    }

    FunctionProtoType *proto() {
        return type()->asFunctionProto();
    }

    CompoundStmt *body;
    bool          canBeConstExpr;
};

inline ParamVarDecl *DeclSpecifier::createParamVarDecl(std::string_view name) {
    return utils::BuildTrait<ParamVarDecl>::create(name, this);
}

inline FunctionDecl *DeclSpecifier::createFunctionDecl(
    std::string_view name, ParamVarDeclList &params, CompoundStmt *body) {
    //! FIXME: here assume returnType == specifier->type
    return FunctionDecl::create(name, this, params, body);
}

RegisterCastImpl(declId, Var, Decl, DeclID);
RegisterCastImpl(declId, ParamVar, Decl, DeclID);
RegisterCastImpl(declId, Function, Decl, DeclID);

} // namespace slime::ast
