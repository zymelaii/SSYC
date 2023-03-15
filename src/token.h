#pragma once

#include <stdint.h>

namespace ssyc {

enum class TokenType : uint8_t {
    TT_IDENT,       //!< 标识符
    TT_COMMENT,     //!< 注释
    TT_CONST_INT,   //!< 整型常量
    TT_CONST_FLOAT, //!< 浮点常量
    TT_T_VOID,      //!< void 类型
    TT_T_INT,       //!< int 类型
    TT_T_FLOAT,     //!< float 类型
    TT_CONST,       //!< const 修饰符
    TT_IF,          //!< if 关键字
    TT_ELSE,        //!< else 关键字
    TT_WHILE,       //!< while 关键字
    TT_BREAK,       //!< break 关键字
    TT_CONTINUE,    //!< continue 关键字
    TT_RETURN,      //!< return 关键字
    TT_OP_ASS,      //!< 赋值操作符
    TT_OP_POS,      //!< 取正操作符
    TT_OP_NEG,      //!< 取负操作符
    TT_OP_ADD,      //!< 加法操作符
    TT_OP_SUB,      //!< 减法操作符
    TT_OP_MUL,      //!< 乘法操作符
    TT_OP_DIV,      //!< 除法操作符
    TT_OP_MOD,      //!< 取模操作符
    TT_OP_LT,       //!< 小于比较符
    TT_OP_GT,       //!< 大于比较符
    TT_OP_LE,       //!< 小于等于比较符
    TT_OP_GE,       //!< 大于等于比较符
    TT_OP_EQ,       //!< 等于比较符
    TT_OP_NE,       //!< 不等于比较符
    TT_OP_LNOT,     //!< 逻辑非操作符
    TT_OP_LAND,     //!< 逻辑与操作符
    TT_OP_LOR,      //!< 逻辑或操作符
    TT_LPAREN,      //!< 左小括号
    TT_RPAREN,      //!< 右小括号
    TT_LBRACKET,    //!< 左中括号
    TT_RBRACKET,    //!< 右中括号
    TT_LBRACE,      //!< 左花括号
    TT_RBRACE,      //!< 右花括号
    TT_COMMA,       //!< 逗号
    TT_SEMICOLON,   //!< 分号
};

} // namespace ssyc
