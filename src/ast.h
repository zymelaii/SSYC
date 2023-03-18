#pragma once

#include <stdint.h>
#include <concepts>
#include <string>
#include <variant>
#include <optional>

namespace ssyc::ast {

struct ProgramUnit {
    virtual ~ProgramUnit()         = default;
    virtual std::string id() const = 0;
};

struct TypeDecl;
struct InitializeList;

struct Program;
struct FuncDef;
struct Block;

struct Statement;
struct DeclStatement;
struct NestStatement;
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

struct TypeDecl final : public ProgramUnit {
    std::string id() const override {
        return "type-decl";
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
    std::string id() const override {
        return "initialize-list";
    }

    std::vector<Expr *> valueList; //!< 数值列表
};

struct Expr : public ProgramUnit {
    virtual std::string id() const       = 0;
    virtual bool        writable() const = 0;

    enum class Type : uint8_t {
        Unary,     //!< 一元表达式
        Binary,    //!< 二元表达式
        FnCall,    //!< 函数调用
        ConstExpr, //!< 编译期取值
        Orphan,    //!< 孤立值
                //!< NOTE: Orphan 是初始化列表与非 ConstExpr 的变量值
    };

    Type type; //!< 表达式类型
};

struct UnaryExpr final : public Expr {
    std::string id() const override {
        return "unary-expr";
    }

    inline bool writable() const override {
        return false;
    }

    UnaryExpr() {
        Expr::type = Expr::Type::Unary;
    }

    enum class Type : uint8_t {
        POS,  //!< 取正
        NEG,  //!< 取负
        LNOT, //!< 逻辑非
    };

    Type  op;      //!< 操作符
    Expr *operand; //!< 操作数
};

struct BinaryExpr final : public Expr {
    std::string id() const override {
        return "binary-expr";
    }

    inline bool writable() const override {
        //! FIXME: 多维数组索引的左操作数实际是不一定可写的
        return lhs->writable() && (op == Type::ASSIGN || op == Type::SUBSCRIPT);
    }

    BinaryExpr() {
        Expr::type = Expr::Type::Binary;
    }

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
    std::string id() const override {
        return "func-call";
    }

    inline bool writable() const override {
        return false;
    }

    FnCallExpr() {
        Expr::type = Expr::Type::FnCall;
    }

    FuncDef            *func;   //!< 函数目标
    std::vector<Expr *> params; //!< 参数列表
};

struct ConstExprExpr final : public Expr {
    std::string id() const override {
        return "constexpr";
    }

    inline bool writable() const override {
        return false;
    }

    ConstExprExpr() {
        Expr::type = Expr::Type::ConstExpr;
    }

    enum class Type : uint8_t {
        Integer,
        Float,
    };

    Type                                         type;  //!< 类型
    std::variant<std::monostate, int32_t, float> value; //!< 数值
};

struct OrphanExpr final : public Expr {
    std::string id() const override {
        return "orphan-expr";
    }

    inline bool writable() const override {
        return std::holds_alternative<TypeDecl *>(ref)
            && !std::get<TypeDecl *>(ref)->constant;
    }

    OrphanExpr() {
        Expr::type = Expr::Type::Orphan;
    }

    using OrphanType =
        std::variant<std::monostate, TypeDecl *, InitializeList *>;
    OrphanType ref; //!< 目标引用
};

struct Statement : public ProgramUnit {
    virtual std::string id() const = 0;

    enum class Type : uint8_t {
        Decl,
        Nested,
        Expr,
        IfElse,
        While,
        Break,
        Continue,
        Return,
    };

    Type type; //!< 语句类型
};

struct DeclStatement final : public Statement {
    std::string id() const override {
        return "decl-statement";
    }

    DeclStatement() {
        Statement::type = Statement::Type::Decl;
    }

    std::vector<std::pair<TypeDecl *, Expr *>> declList; //!< 声明列表
};

struct NestedStatement final : public Statement {
    std::string id() const override {
        return "nested-statement";
    }

    NestedStatement() {
        Statement::type = Statement::Type::Expr;
    }

    Block *block;
};

struct ExprStatement final : public Statement {
    std::string id() const override {
        return "expr-statement";
    }

    ExprStatement() {
        Statement::type = Statement::Type::Expr;
    }

    Expr *expr;
};

struct IfElseStatement final : public Statement {
    std::string id() const override {
        return "if-else-statement";
    }

    IfElseStatement() {
        Statement::type = Statement::Type::IfElse;
    }

    Expr      *condition;
    Statement *trueRoute;
    Statement *falseRoute;
};

struct WhileStatement final : public Statement {
    std::string id() const override {
        return "while-statement";
    }

    WhileStatement() {
        Statement::type = Statement::Type::While;
    }

    Expr      *condition;
    Statement *body;
};

struct BreakStatement final : public Statement {
    std::string id() const override {
        return "break-statement";
    }

    BreakStatement() {
        Statement::type = Statement::Type::Break;
    }
};

struct ContinueStatement final : public Statement {
    std::string id() const override {
        return "continue-statement";
    }

    ContinueStatement() {
        Statement::type = Statement::Type::Continue;
    }
};

struct ReturnStatement final : public Statement {
    std::string id() const override {
        return "return-statement";
    }

    ReturnStatement() {
        Statement::type = Statement::Type::Return;
    }

    Expr *retval;
};

struct Block final : public ProgramUnit {
    std::string id() const override {
        return "block";
    }

    std::vector<Statement *> statementList; //!< 语句列表
};

struct FuncDef final : public ProgramUnit {
    std::string id() const override {
        return "func-def";
    }

    enum class Type : uint8_t {
        Void,
        Integer,
        Float,
    };

    Type                    retType; //!< 返回值类型
    std::string             ident;   //!< 函数名
    std::vector<TypeDecl *> params;  //!< 参数列表
    Block                  *body;    //!< 函数体
                 //!< NOTE: 为 nullptr 时表示函数体为空
};

struct Program final : public ProgramUnit {
    std::string id() const override {
        return "program";
    }

    void append(auto unit)
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
