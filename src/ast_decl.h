#pragma once

#include <stdint.h>
#include <type_traits>
#include <concepts>

namespace ssyc::ast {

enum class Type : uint32_t {
    TypeDecl,
    InitializeList,

    Program,
    FuncDef,
    Block,

    Statement,
    DeclStatement,
    NestedStatement,
    ExprStatement,
    IfElseStatement,
    WhileStatement,
    BreakStatement,
    ContinueStatement,
    ReturnStatement,

    Expr,
    UnaryExpr,
    BinaryExpr,
    FnCallExpr,
    ConstExprExpr,
    OrphanExpr,

    ProgramUnit, //!< to be the last for size indication
};

struct ProgramUnit {
    virtual ~ProgramUnit() = default;

    constexpr ProgramUnit()
        : type(Type::Program) {}

    const Type type;
};

template <typename T>
concept AstNodeType =
    requires (const T *e) {
        requires std::derived_from<T, ProgramUnit>;
        requires std::is_same_v<
                     std::remove_cvref_t<std::decay_t<T>>,
                     ProgramUnit>
                     || requires {
                            {
                                std::remove_cvref_t<std::decay_t<T>>::id()
                                } -> std::same_as<Type>;
                        };
    };

struct TypeDecl;
struct InitializeList;

struct Program;
struct FuncDef;
struct Block;

struct Statement;
struct DeclStatement;
struct NestedStatement;
struct ExprStatement;
struct IfElseStatement;
struct WhileStatement;
struct BreakStatement;
struct ContinueStatement;
struct ReturnStatement;

struct Expr;
struct UnaryExpr;
struct BinaryExpr;
struct FnCallExpr;
struct ConstExprExpr;
struct OrphanExpr;

} // namespace ssyc::ast
