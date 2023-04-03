#pragma once

#include <string>
#include <stdint.h>

namespace ssyc::ast {

enum class NodeBaseType : uint8_t {
    Type = 0,
    Decl,
    Expr,
    Stmt,
    Unknown,
};

struct NodeBrief {
    inline NodeBrief()
        : type(NodeBaseType::Unknown)
        , name{}
        , hint{} {}

    NodeBaseType type;
    std::string  name;
    std::string  hint;
};

struct IBrief {
    virtual void acquire(NodeBrief &brief) const = 0;
};

struct ITreeWalker {

};

#ifndef SSYC_IMPL_AST_INTERFACE
#define SSYC_IMPL_AST_INTERFACE \
 inline virtual void acquire(NodeBrief &brief) const override; //!< IBrief
#endif

}; // namespace ssyc::ast
