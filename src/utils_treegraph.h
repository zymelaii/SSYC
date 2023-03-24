#pragma once

/*!
 * \file utils_treegraph.h
 * \brief 树形图生成组件
 * 这玩意是用来渲染 SSYC 生成的 AST 的，但是无所谓，我懒得写了。
 */

#include "ast_decl.h"

#include <vector>
#include <string>

namespace ssyc::utils {
namespace graph {

struct AbstractTreeNode {
    enum class NodeType {
        Sequence,
        Unary,
        Binary,
    };

    virtual ~AbstractTreeNode();

    virtual NodeType type() const = 0;

    AbstractTreeNode *parent;
};

struct SequenceTreeNode;
struct UnaryTreeNode;
struct BinaryTreeNode;

struct SequenceTreeNode : public AbstractTreeNode {
    virtual ~SequenceTreeNode();

    inline virtual NodeType type() const override {
        return NodeType::Sequence;
    }

    std::vector<UnaryTreeNode *> list;
};

struct UnaryTreeNode : public AbstractTreeNode {
    virtual ~UnaryTreeNode();

    inline virtual NodeType type() const override {
        return NodeType::Unary;
    }

    std::string       hint;
    AbstractTreeNode *child;
};

struct BinaryTreeNode : public AbstractTreeNode {
    virtual ~BinaryTreeNode();

    inline virtual NodeType type() const override {
        return NodeType::Binary;
    }

    std::string       hint;
    AbstractTreeNode *lhs;
    AbstractTreeNode *rhs;
};

} // namespace graph

graph::UnaryTreeNode *buildGraphTree(ssyc::ast::Program *program);

} // namespace ssyc::utils
