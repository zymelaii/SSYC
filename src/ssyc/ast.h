#pragma once

#include "ast_decl.h"
#include "ast_utils.h" // IWYU pragma: export

#include <stdint.h>
#include <concepts>
#include <string>
#include <variant>
#include <optional>
#include <utility>

namespace ssyc::ast {

struct TypeDecl final : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::TypeDecl;
    }

    enum class Type : uint8_t {
        Integer,
        Float,
    };

    //! 指针型类型
    bool pointerLike() const {
        return !optSubscriptList.has_value()
            && optSubscriptList.value().size() > 0
            && optSubscriptList.value()[0] == nullptr;
    }

    Type                               type;             //!< 基本类型
    bool                               constant;         //!< const 类型
    std::string                        ident;            //!< 标识符
    std::optional<std::vector<Expr *>> optSubscriptList; //!< 数组下标列表
};

struct InitializeList final : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::InitializeList;
    }

    std::vector<Expr *> valueList; //!< 数值列表
};

struct Expr : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::Expr;
    }

    virtual bool writable() const = 0;

    enum class Type : uint8_t {
        Unary,     //!< 一元表达式
        Binary,    //!< 二元表达式
        FnCall,    //!< 函数调用
        ConstExpr, //!< 编译期取值
        Orphan,    //!< 孤立值
                //!< NOTE: Orphan 是初始化列表与非 ConstExpr 的变量值
    };

    const Type type; //!< 表达式类型

protected:
    inline Expr(Type exprType)
        : type(exprType) {}
};

struct UnaryExpr final : public Expr {
    static constexpr ast::Type id() {
        return ast::Type::UnaryExpr;
    }

    inline bool writable() const override {
        return false;
    }

    inline UnaryExpr()
        : Expr(Expr::Type::Unary)
        , operand(nullptr) {}

    enum class Type : uint8_t {
        POS,  //!< 取正
        NEG,  //!< 取负
        LNOT, //!< 逻辑非
    };

    Type  op;      //!< 操作符
    Expr *operand; //!< 操作数
};

struct BinaryExpr final : public Expr {
    static constexpr ast::Type id() {
        return ast::Type::BinaryExpr;
    }

    inline bool writable() const override {
        //! FIXME: 多维数组索引的左操作数实际是不一定可写的
        return lhs->writable() && (op == Type::ASSIGN || op == Type::SUBSCRIPT);
    }

    inline BinaryExpr()
        : Expr(Expr::Type::Binary)
        , lhs(nullptr)
        , rhs(nullptr) {}

    enum class Type : uint8_t {
        ASSIGN,    //!< 赋值
        ADD,       //!< 加法
        SUB,       //!< 减法
        MUL,       //!< 乘法
        DIV,       //!< 除法
        MOD,       //!< 取模
        LT,        //!< 小于
        LE,        //!< 小于等于
        GT,        //!< 大于
        GE,        //!< 大于等于
        EQ,        //!< 等于
        NE,        //!< 不等于
        LAND,      //!< 逻辑与
        LOR,       //!< 逻辑或
        SUBSCRIPT, //!< 数组索引
    };

    Type  op;  //!< 操作符
    Expr *lhs; //!< 左操作数
    Expr *rhs; //!< 右操作数
};

struct FnCallExpr final : public Expr {
    static constexpr ast::Type id() {
        return ast::Type::FnCallExpr;
    }

    inline bool writable() const override {
        return false;
    }

    inline FnCallExpr()
        : Expr(Expr::Type::FnCall)
        , func(nullptr) {}

    FuncDef            *func;   //!< 函数目标
    std::vector<Expr *> params; //!< 参数列表
};

struct ConstExprExpr final : public Expr {
    static constexpr ast::Type id() {
        return ast::Type::ConstExprExpr;
    }

    inline bool writable() const override {
        return false;
    }

    inline ConstExprExpr()
        : Expr(Expr::Type::ConstExpr) {}

    enum class Type : uint8_t {
        Integer,
        Float,
    };

    Type                                         type;  //!< 类型
    std::variant<std::monostate, int32_t, float> value; //!< 数值
};

struct OrphanExpr final : public Expr {
    static constexpr ast::Type id() {
        return ast::Type::OrphanExpr;
    }

    inline bool writable() const override {
        return std::holds_alternative<TypeDecl *>(ref)
            && !std::get<TypeDecl *>(ref)->constant;
    }

    inline OrphanExpr()
        : Expr(Expr::Type::Orphan) {}

    using OrphanType =
        std::variant<std::monostate, TypeDecl *, InitializeList *>;
    OrphanType ref; //!< 目标引用
};

struct Statement : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::Statement;
    }

    enum class Type : uint8_t {
        Decl,
        Compound,
        Expr,
        IfElse,
        While,
        Break,
        Continue,
        Return,
    };

    const Type type; //!< 语句类型

protected:
    inline Statement(Type statementType)
        : type(statementType) {}
};

struct DeclStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::DeclStatement;
    }

    inline DeclStatement()
        : Statement(Statement::Type::Decl) {}

    std::vector<std::pair<TypeDecl *, Expr *>> declList; //!< 声明列表
};

struct CompoundStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::CompoundStatement;
    }

    inline CompoundStatement()
        : Statement(Statement::Type::Compound)
        , block(nullptr) {}

    Block *block;
};

struct ExprStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::ExprStatement;
    }

    inline ExprStatement()
        : Statement(Statement::Type::Expr)
        , expr(nullptr) {}

    Expr *expr;
};

struct IfElseStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::IfElseStatement;
    }

    inline IfElseStatement()
        : Statement(Statement::Type::IfElse)
        , condition(nullptr)
        , trueRoute(nullptr)
        , falseRoute(nullptr) {}

    Expr      *condition;
    Statement *trueRoute;
    Statement *falseRoute;
};

struct WhileStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::WhileStatement;
    }

    inline WhileStatement()
        : Statement(Statement::Type::While)
        , condition(nullptr)
        , body(nullptr) {}

    Expr      *condition;
    Statement *body;
};

struct BreakStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::BreakStatement;
    }

    inline BreakStatement()
        : Statement(Statement::Type::Break) {}
};

struct ContinueStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::ContinueStatement;
    }

    inline ContinueStatement()
        : Statement(Statement::Type::Continue) {}
};

struct ReturnStatement final : public Statement {
    static constexpr ast::Type id() {
        return ast::Type::ReturnStatement;
    }

    inline ReturnStatement()
        : Statement(Statement::Type::Return)
        , retval(nullptr) {}

    Expr *retval;
};

struct Block final : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::Block;
    }

    std::vector<Statement *> statementList; //!< 语句列表
};

struct FuncDef final : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::FuncDef;
    }

    enum class Type : uint8_t {
        Void,
        Integer,
        Float,
    };

    inline FuncDef()
        : body(nullptr) {}

    Type                    retType; //!< 返回值类型
    std::string             ident;   //!< 函数名
    std::vector<TypeDecl *> params;  //!< 参数列表
    Block                  *body;    //!< 函数体
                 //!< NOTE: 为 nullptr 时表示函数体为空
};

struct Program final : public ProgramUnit {
    static constexpr ast::Type id() {
        return ast::Type::Program;
    }

    inline void append(auto unit)
        requires(
            std::is_same_v<std::remove_cvref_t<decltype(*unit)>, DeclStatement>
            || std::is_same_v<std::remove_cvref_t<decltype(*unit)>, FuncDef>)
    {
        using T                   = std::remove_cvref_t<decltype(*unit)>;
        constexpr auto isDeclUnit = std::is_same_v<T, DeclStatement>;
        const auto     nextId     = declFlows.size() + funcFlows.size();
        if constexpr (isDeclUnit) {
            declFlows.push_back({nextId, unit});
        } else {
            funcFlows.push_back({nextId, unit});
        }
    }

    std::vector<std::pair<size_t, DeclStatement *>> declFlows; //!< 声明语句流
    std::vector<std::pair<size_t, FuncDef *>> funcFlows; //!< 函数定义流
};

} // namespace ssyc::ast
