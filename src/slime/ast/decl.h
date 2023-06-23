#pragma once

#include "../utils/list.h"
#include "type.h"
#include "scope.h"

#include <stdint.h>
#include <string_view>

namespace slime::ast {

struct FunctionProtoType;
struct CompoundStmt;
struct ParamVarDecl;
struct VarDecl;
struct FunctionDecl;

using ParamVarDeclList = slime::utils::ListTrait<ParamVarDecl *>;

enum class DeclID {
    Var,
    ParamVar,
    Function,
};

enum class NamedDeclSpecifier {
    Extern = 0b001,
    Static = 0b010,
    Inline = 0b100,
};

struct DeclSpecifier {
    DeclSpecifier()
        : type{nullptr}
        , specifiers{0} {}

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

    VarDecl *createVarDecl(std::string_view name);

    inline FunctionDecl *createFunctionDecl(
        std::string_view name, ParamVarDeclList &params, CompoundStmt *body);

    Type   *type;
    uint8_t specifiers;
};

struct Decl {
    Decl(DeclID declId)
        : declId{declId} {}

    VarDecl *asVarDecl() {
        assert(declId == DeclID::Var);
        return reinterpret_cast<VarDecl *>(this);
    }

    ParamVarDecl *asParamVarDecl() {
        assert(declId == DeclID::ParamVar);
        return reinterpret_cast<ParamVarDecl *>(this);
    }

    FunctionDecl *asFunctionDecl() {
        assert(declId == DeclID::Function);
        return reinterpret_cast<FunctionDecl *>(this);
    }

    VarDecl *tryIntoVarDecl() {
        return declId == DeclID::Var ? asVarDecl() : nullptr;
    }

    ParamVarDecl *tryIntoParamVarDecl() {
        return declId == DeclID::ParamVar ? asParamVarDecl() : nullptr;
    }

    FunctionDecl *tryIntoFunctionDecl() {
        return declId == DeclID::Function ? asFunctionDecl() : nullptr;
    }

    DeclID declId;
};

struct NamedDecl : public Decl {
    NamedDecl(DeclID declId, std::string_view name)
        : Decl(declId)
        , name{name}
        , scope() {}

    Scope            scope;
    std::string_view name;
};

struct DeclaratorDecl : public NamedDecl {
    DeclaratorDecl(DeclID declId, std::string_view name)
        : NamedDecl(declId, name) {}
};

struct VarDecl : public DeclaratorDecl {
    VarDecl(DeclID declId, std::string_view name, Type *type, Expr *initValue)
        : DeclaratorDecl(declId, name)
        , type{type}
        , initValue{initValue} {}

    VarDecl(std::string_view name, Type *type, Expr *initValue)
        : DeclaratorDecl(DeclID::Var, name)
        , type{type}
        , initValue{initValue} {}

    static VarDecl *create(std::string_view name, Type *type, Expr *initValue) {
        return new VarDecl(name, type, initValue);
    }

    Type *type;
    Expr *initValue;
};

struct ParamVarDecl : public VarDecl {
    ParamVarDecl(std::string_view name, Type *type);
    ParamVarDecl(Type *type);

    static ParamVarDecl *create(std::string_view name, Type *type) {
        return new ParamVarDecl(name, type);
    }

    static ParamVarDecl *create(Type *type) {
        return new ParamVarDecl(type);
    }

    bool isNoEffectParam() {
        return name.empty();
    }
};

struct FunctionDecl
    : public DeclaratorDecl
    , public ParamVarDeclList {
    FunctionDecl(
        std::string_view  name,
        Type             *returnType,
        ParamVarDeclList &params,
        CompoundStmt     *body)
        : DeclaratorDecl(DeclID::Function, name)
        , ParamVarDeclList(std::move(params))
        , proto{nullptr}
        , body{body} {
        TypeList list;
        for (auto param = params.head(); param != nullptr;
             param      = param->next()) {
            list.insertToTail(param->value()->type);
        }
        proto = FunctionProtoType::create(returnType, std::move(list));
    }

    static FunctionDecl *create(
        std::string_view  name,
        Type             *returnType,
        ParamVarDeclList &params,
        CompoundStmt     *body) {
        return new FunctionDecl(name, returnType, params, body);
    }

    FunctionProtoType *proto;
    CompoundStmt      *body;
};

inline FunctionDecl *DeclSpecifier::createFunctionDecl(
    std::string_view name, ParamVarDeclList &params, CompoundStmt *body) {
    return FunctionDecl::create(name, type, params, body);
}

} // namespace slime::ast
