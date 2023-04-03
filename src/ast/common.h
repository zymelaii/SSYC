#pragma once

namespace ssyc::ast {

//! 类型
struct Type {};

//! 声明
struct Decl {};

//! 表达式
struct Expr {};

//! 语句
struct Stmt {};

}; // namespace ssyc::ast

#include "type_decl.h"
#include "decl_decl.h"
#include "expr_decl.h"
#include "stmt_decl.h"
