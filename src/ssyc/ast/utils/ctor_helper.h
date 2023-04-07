#pragma once

#include "../typedef.h"
#include "../concept.h" // IWYU pragma: keep

namespace ssyc::ast::utils::ctor {

namespace Type {

namespace details {

template <typename T, size_t Dim>
struct ArrayAssocOnce {
    static inline auto emit() {
        auto e         = new ConstantArrayType;
        auto dim       = new IntegerLiteral;
        dim->value     = Dim;
        dim->type      = IntegerLiteral::IntegerType::u64;
        dim->literal   = *new std::string{std::to_string(Dim)};
        e->elementType = T::emit();
        e->length      = dim;
        return e;
    }
};

} // namespace details

template <typename T>
struct Qualify {
    static inline auto emit() {
        auto e  = new QualifiedType;
        e->type = T::emit();
        return e;
    }
};

struct Void {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Void;
        return e;
    }
};

struct Int {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Int;
        return e;
    }
};

struct Float {
    static inline auto emit() {
        auto e    = new BuiltinType;
        e->typeId = BuiltinType::Type::Float;
        return e;
    }
};

template <typename T, size_t Dim, size_t... Left>
struct Array {
    static inline auto emit() {
        if constexpr (sizeof...(Left) == 0) {
            return details::ArrayAssocOnce<T, Dim>::emit();
        } else {
            return Array<details::ArrayAssocOnce<T, Dim>, Left...>::emit();
        }
    }
};

template <typename R, typename... Params>
struct Function {
    static inline auto emit() {
        auto e        = new FunctionProtoType;
        e->retvalType = R::emit();
        e->params.clear();
        if constexpr (sizeof...(Params) > 0) {
            std::array types{static_cast<ast::Type *>(Params::emit())...};
            for (auto &type : types) {
                auto param        = new ParamVarDecl;
                param->paramType  = type;
                param->defaultVal = nullptr;
                e->params.push_back(param);
            }
        }
        return e;
    }
};

}; // namespace Type

namespace Decl {

template <typename... T>
struct Var;

template <typename Type>
struct Var<Type> {
    static inline auto emit() {
        auto e     = new VarDecl;
        e->varType = Type::emit();
        e->initVal = nullptr;
        return e;
    }
};

template <typename Type, typename Init>
struct Var<Type, Init> {
    static inline auto emit() {
        auto e     = Var<Type>::emit();
        e->initVal = Init::emit();
        return e;
    }
};

template <typename... T>
struct Param;

template <typename Type>
struct Param<Type> {
    static inline auto emit() {
        auto e        = new ParamVarDecl;
        e->paramType  = Type::emit();
        e->defaultVal = nullptr;
        return e;
    }
};

template <typename Type, typename Init>
struct Param<Type, Init> {
    static inline auto emit() {
        auto e        = Param<Type>::emit();
        e->defaultVal = Init::emit();
        return e;
    }
};

template <typename Proto, typename StmtBody>
struct Function {
    static inline auto emit() {
        auto e       = new FunctionDecl;
        e->protoType = Proto::emit();
        e->body      = StmtBody::emit();
        return e;
    }
};

} // namespace Decl

namespace Expr {

template <typename... E>
struct InitList {
    static inline auto emit() {
        auto       e = new InitListExpr;
        std::array exprs{E::emit()...};
        for (auto &expr : exprs) { e->elementList.push_back(expr); }
        return e;
    }
};

template <typename Func, typename... Args>
struct Call {
    static inline auto emit() {
        auto       e = new CallExpr;
        std::array params{Args::emit()...};
        e->func = Func::emit();
        for (auto &param : params) { e->params.push_back(param); }
        return e;
    }
};

template <typename E>
struct Paren {
    static inline auto emit() {
        auto e       = new ParenExpr;
        e->innerExpr = E::emit();
        return e;
    }
};

template <ast::UnaryOperatorExpr::UnaryOpType Op, typename Operand>
struct UnaryOp {
    static inline auto emit() {
        auto e     = new UnaryOperatorExpr;
        e->op      = Op;
        e->operand = Operand::emit();
    }
};

template <ast::BinaryOperatorExpr::BinaryOpType Op, typename Lhs, typename Rhs>
struct BinaryOp {
    static inline auto emit() {
        auto e = new BinaryOperatorExpr;
        e->op  = Op;
        e->lhs = Lhs::emit();
        e->rhs = Rhs::emit();
        return e;
    }
};

template <typename Ref>
struct SymRef {
    static inline auto emit() {
        //! FIXME: should support lookup here
        auto e = new DeclRef;
        e->ref = Ref::emit();
        return e;
    }
};

template <int32_t Value>
struct Integer {
    static inline auto emit() {
        auto e   = new IntegerLiteral;
        e->type  = IntegerLiteral::IntegerType::i32;
        e->value = Value;
        return e;
    }
};

} // namespace Expr

namespace Stmt {

namespace details {

template <ast_node T>
struct OrphanStmt {
    static inline auto emit() {
        return new T;
    }
};

} // namespace details

using Null = details::OrphanStmt<NullStmt>;

template <typename... Statement>
struct Compound {
    static inline auto emit() {
        using value_type = decltype(CompoundStmt::stmtList)::value_type;
        auto e           = new CompoundStmt;
        if constexpr (sizeof...(Statement) > 0) {
            std::array statements{value_type{Statement::emit()}...};
            for (auto &stmt : statements) {
                e->stmtList.emplace_back(std::move(stmt));
            }
        }
        return e;
    }
};

template <typename... VariableDecl>
    requires(sizeof...(VariableDecl) > 0)
struct Decl {
    static inline auto emit() {
        auto       e = new DeclStmt;
        std::array decls{VariableDecl::emit()...};
        for (auto &decl : decls) { e->varDeclList.push_back(decl); }
        return e;
    }
};

template <typename... T>
struct IfElse;

template <typename Cond, typename StmtOnTrue>
struct IfElse<Cond, StmtOnTrue> {
    static inline auto emit() {
        auto e    = new IfElseStmt;
        e->cond   = Cond::emit();
        e->onTrue = StmtOnTrue::emit();
        return e;
    }
};

template <typename Cond, typename StmtOnTrue, typename StmtOnFalse>
struct IfElse<Cond, StmtOnTrue, StmtOnFalse> {
    static inline auto emit() {
        auto e     = IfElse<Cond, StmtOnTrue>::emit();
        e->onFalse = StmtOnFalse::emit();
        return e;
    }
};

template <typename Cond, typename StmtBody>
struct While {
    static inline auto emit() {
        auto e  = new WhileStmt;
        e->cond = Cond::emit();
        e->body = StmtBody::emit();
        return e;
    }
};

template <typename Cond, typename StmtBody>
struct DoWhile {
    static inline auto emit() {
        auto e  = new DoWhileStmt;
        e->cond = Cond::emit();
        e->body = StmtBody::emit();
        return e;
    }
};

template <typename Init, typename Cond, typename Loop, typename StmtBody>
struct For {
    static inline auto emit() {
        auto e = new ForStmt;
        if constexpr (!std::is_same_v<Init, Null>) { e->init = Init::emit(); }
        if constexpr (!std::is_same_v<Cond, Null>) { e->cond = Cond::emit(); }
        if constexpr (!std::is_same_v<Loop, Null>) { e->loop = Cond::emit(); }
        e->body = StmtBody::emit();
        return e;
    }
};

using Continue = details::OrphanStmt<ContinueStmt>;

using Break = details::OrphanStmt<BreakStmt>;

template <typename... T>
struct Return;

template <>
struct Return<> {
    static inline auto emit() {
        return new ReturnStmt;
    }
};

template <typename RetVal>
struct Return<RetVal> {
    static inline auto emit() {
        auto e    = new ReturnStmt;
        e->retval = RetVal::emit();
        return e;
    }
};

} // namespace Stmt

template <typename... Unit>
struct Module {
    static inline auto emit() {
        using value_type = decltype(Program::unitList)::value_type;
        auto       e     = new Program;
        std::array units{value_type{Unit::emit()}...};
        for (auto &unit : units) { e->unitList.push_back(std::move(unit)); }
        return e;
    }
};

} // namespace ssyc::ast::utils::ctor
