#pragma once

#include "token.h"  // IWYU pragma: export
#include "syntax.h" // IWYU pragma: export
#include <concepts>
#include <stdint.h>
#include <string>
#include <variant>

namespace ssyc {

struct AstNode {
    AstNode();
    ~AstNode();

    struct ScopeID {
        uint32_t depth : 8;  //!< NOTE: 最多支持 256 级作用域
        uint32_t hash  : 24; //!< 当前作用域的哈希值
    } scope;

    union {
        uint32_t flags;

        struct {
            bool FLAGS_ConstEval;    //!< 编译期可求值
            bool FLAGS_Constant;     //!< 内容不可变
            bool FLAGS_EmptyBlock;   //!< 空代码块
            bool FLAGS_Arithmetical; //!< 可运算的
            bool FLAGS_ArrayBlock;   //!< 数组元素块
            bool FLAGS_Nested;       //!< 嵌套语法节点
        };
    };

    union {
        TokenType  token;
        SyntaxType syntax;
    };

    std::variant<std::monostate, std::string, int, float> value;

    AstNode *parent;

    AstNode *left, *right;
};

} // namespace ssyc
