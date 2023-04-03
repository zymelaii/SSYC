#pragma once

#include "interface.h" // IWYU pragma: export

namespace ssyc::ast {

/*!
 * \brief base ast node type
 */

struct AbstractAstNode; //!< 抽象语法节点
struct Type;            //!< 类型
struct Decl;            //!< 声明
struct Expr;            //!< 表达式
struct Stmt;            //!< 语句
struct Program;         //!< 程序模块

/*!
 * \brief Type node type
 */

struct QualifiedType;     //!< 修饰符限定类型
struct BuiltinType;       //!< 内置类型
struct ArrayType;         //!< 数组类型
struct ConstantArrayType; //!< 常量数组类型
struct VariableArrayType; //!< 边长数组类型
struct FunctionProtoType; //!< 函数原型

/*!
 * \brief Decl node type
 */

struct VarDecl;      //!< 变量声明
struct ParamVarDecl; //!< 函数形参声明
struct FunctionDecl; //!< 函数声明

/*!
 * \brief Expr node type
 */

struct InitListExpr;       //!< 初始化列表
struct CallExpr;           //!< 函数调用
struct ParenExpr;          //!< 括号表达式
struct UnaryOperatorExpr;  //!< 一元表达式
struct BinaryOperatorExpr; //!< 二元表达式
struct DeclRef;            //!< 符号引用
struct Literal;            //!< 常量
struct IntegerLiteral;     //!< 整形常量
struct FloatingLiteral;    //!< 浮点常量
struct StringLiteral;      //!< 字符串常量

/*!
 * \brief Stmt node type
 */

struct NullStmt;     //!< 空语句
struct CompoundStmt; //!< 组合语句
struct DeclStmt;     //!< 变量声明语句
struct IfElseStmt;   //!< if-else 语句
struct WhileStmt;    //!< while 语句
struct DoWhileStmt;  //!< do-while 语句
struct ForStmt;      //!< for 语句
struct ContinueStmt; //!< continue 语句
struct BreakStmt;    //!< break 语句
struct ReturnStmt;   //!< return 语句

} // namespace ssyc::ast

#include "type/base.h" // IWYU pragma: export
