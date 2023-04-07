/*!
 * \brief IR CodeGen 接口定义
 *
 * IR CodeGen 约定了从 AST 节点生成 IR 的接口
 * 对于任意 AST 节点集合，应该为每一个节点类型
 * 实现 CodeGen 接口，该接口将被 IR Builder
 * 规范处理
 */

#pragma once

#include "../ast/concept.h"
#include "ssa.h"
#include "builder.h"

namespace ssyc::ir {

using ::ssyc::ast::ast_node;

template <ast_node AstNodeType>
struct ICodeGen {
    virtual Value* codegen(Builder& builder) = 0;
};

} // namespace ssyc::ir
