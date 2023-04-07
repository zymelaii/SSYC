#pragma once

#include "../type_declare.h"
#include "base.h"

#include <vector>
#include <string_view>
#include <variant>
#include <stdint.h>

namespace ssyc::ast {

struct InitListExpr : public Expr {
    SSYC_IMPL_AST_INTERFACE

    //! NOTE: InitListExpr 只能出现在变量定义的初始化阶段或强制数组类型转换中
    //! NOTE: InitListExpr 本身不要求元素类型一致，但其允许出现的情形要求元素
    //! 值必须能够无损地转换到目标数组类型的元素类型
    std::vector<Expr*> elementList;
};

struct CallExpr : public Expr {
    SSYC_IMPL_AST_INTERFACE

    //! NOTE: 调用时 func 必须已经声明，解析完成时 func 必须完成定义
    //! FIXME: 函数调用表达式本质上应该是对函数指针的调用
    FunctionDecl* func;

    //! NOTE: 实参数量必须与 func 形参数目一致，除非 func 支持可变参数
    //! NOTE: 实参必须与对应形参类型兼容
    std::vector<Expr*> params;
};

struct ParenExpr : public Expr {
    SSYC_IMPL_AST_INTERFACE

    Expr* innerExpr;
};

struct UnaryOperatorExpr : public Expr {
    SSYC_IMPL_AST_INTERFACE

    enum class UnaryOpType {
        //! TODO: to be completed
        Pos,
        Neg,
        LNot,
        Inv,
    };

    UnaryOpType op;

    Expr* operand;
};

struct BinaryOperatorExpr : public Expr {
    SSYC_IMPL_AST_INTERFACE

    enum class BinaryOpType {
        //! TODO: to be completed
        Assign,
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        Lsh,
        Rsh,
        And,
        Or,
        Xor,
        Eq,
        NE,
        Gt,
        Lt,
        GE,
        LE,
        LAnd,
        LOr,
        Comma,
    };

    BinaryOpType op;

    Expr* lhs;
    Expr* rhs;
};

struct DeclRef : public Expr {
    SSYC_IMPL_AST_INTERFACE

    VarDecl* ref;
};

struct Literal : public Expr {
    SSYC_IMPL_AST_INTERFACE

    std::string_view literal;
};

struct IntegerLiteral : public Literal {
    SSYC_IMPL_AST_INTERFACE

    enum class IntegerType {
        i8,
        i16,
        i32,
        i64,
        u8,
        u16,
        u32,
        u64,
    };

    IntegerType type;

    std::variant<
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        uint8_t,
        uint16_t,
        uint32_t,
        uint64_t>
        value;
};

struct FloatingLiteral : public Literal {
    SSYC_IMPL_AST_INTERFACE

    enum class FloatType {
        f32,
        f64,
    };

    FloatType type;

    std::variant<float, double> value;
};

struct StringLiteral : public Literal {
    SSYC_IMPL_AST_INTERFACE

    //! NOTE: StringLiteral 允许包含 '\0' 字符，必须额外指定字符串长度
    const char* raw;
    size_t      length;
};

} // namespace ssyc::ast
