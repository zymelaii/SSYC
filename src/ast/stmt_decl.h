#pragma once

#include "common.h" // IWYU pragma: export

namespace ssyc::ast {

//! 空语句
struct NullStmt;

//! 组合语句
struct CompoundStmt;

//! 变量声明语句
struct DeclStmt;

//! if-else 语句
struct IfElseStmt;

//! while 语句
struct WhileStmt;

//! do-while 语句
struct DoWhileStmt;

//! for 语句
struct ForStmt;

//! continue 语句
struct ContinueStmt;

//! break 语句
struct BreakStmt;

//! return 语句
struct ReturnStmt;

} // namespace ssyc::ast
