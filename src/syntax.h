#pragma once

#include <stdint.h>

namespace ssyc {

enum class SyntaxType : uint8_t {
    Program, //!< 程序单元
    Decl,    //!< 声明
    FuncDef, //!< 函数定义
    Block,   //!< 代码块
    Expr,    //!< 表达式
    Assign,  //!< 赋值语句
    IfElse,  //!< if-else 语句块
    While,   //!< while 语句块
    Return,  //!< return 语句
    //! TODO: more syntax type
};

} // namespace ssyc
