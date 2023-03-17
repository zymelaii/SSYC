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

    AstNode *require();
    void     deleteLater(AstNode *node);

    enum class ContextFlag : uint8_t {
        SyntaxAnalysisDone = 0, //!< 完成语法分析
    };
    std::bitset<std::numeric_limits<uint8_t>::max()> flags;

    inline bool testOrSetFlag(const ContextFlag flag) {
        const auto p = static_cast<uint8_t>(flag);
        if (flags.test(p)) { return false; }
        flags.set(p);
        return true;
    }

    std::set<AstNode *> freelist;

    AstNode *program;
    AstNode *cursor;
};

} // namespace ssyc
