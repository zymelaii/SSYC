#pragma once

#include "ast.h"

#include <stdint.h>
#include <limits>
#include <bitset>
#include <set>

namespace ssyc {

struct ParserContext {
    ParserContext();
    ~ParserContext();

    enum class ContextFlag : uint8_t {
        SyntaxAnalysisDone = 0, //!< 完成语法分析
    };

    inline bool testAndSetFlag(const ContextFlag flag) {
        const auto p = static_cast<uint8_t>(flag);
        if (flags.test(p)) { return false; }
        flags.set(p);
        return true;
    }

    std::bitset<std::numeric_limits<uint8_t>::max()> flags;   //!< 标志位
    std::unique_ptr<ast::Program>                    program; //!< 源程序
};

} // namespace ssyc
