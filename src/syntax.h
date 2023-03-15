#pragma once

#include <stdint.h>

namespace ssyc {

enum class SyntaxType : uint8_t {
    Program, //!< 程序单元
    Decl,    //!< 声明
    FuncDef, //!< 函数定义
    Block,   //!< 代码块

    //! TODO: more syntax type
};

} // namespace ssyc
