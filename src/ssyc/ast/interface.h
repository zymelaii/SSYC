#pragma once

#include "common.h"
#include "type_declare.h"

#include <string>
#include <list>
#include <stack>
#include <stdint.h>

namespace ssyc::ast {

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

class TreeWalkState {
public:
    struct node_type {
        node_type *prev;
        node_type *next;

        const AbstractAstNode *node;
    };

    struct walkproc_t {
        int number;
        int depth;
    };

    TreeWalkState(const AbstractAstNode *root);
    ~TreeWalkState();

    void push(const AbstractAstNode *node);

    const AbstractAstNode *next();

    const int depth() const;

private:
    int        complete() const;
    node_type *acquireNewNode();

private:
    node_type *walkCursor_; //!< 遍历游标指针
    node_type *pushCursor_; //!< 插入游标指针

    mutable size_t pushNumber_;
    int            currentDepth_;

    std::stack<node_type *> freeList_;
    std::stack<walkproc_t>  walkProc_;
};

struct ITreeWalkBuilder {
    virtual void flattenInsert(TreeWalkState &state) const = 0;
};

#ifndef SSYC_IMPL_AST_INTERFACE
#define SSYC_IMPL_AST_INTERFACE                         \
 virtual void acquire(NodeBrief &brief) const override; \
 virtual void flattenInsert(TreeWalkState &state) const override;
#endif

}; // namespace ssyc::ast
