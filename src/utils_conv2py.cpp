#include "utils_conv2py.h"
#include "ast.h"

#include <fmt/format.h>
#include <map>
#include <string_view>

using namespace ssyc::ast;

static void
    conv2py_resolve_TypeDecl(size_t, std::ostream &, const TypeDecl *, void *);
static void conv2py_resolve_InitializeList(
    size_t, std::ostream &, const InitializeList *, void *);
static void
    conv2py_resolve_Program(size_t, std::ostream &, const Program *, void *);
static void
    conv2py_resolve_FuncDef(size_t, std::ostream &, const FuncDef *, void *);
static void
    conv2py_resolve_Block(size_t, std::ostream &, const Block *, void *);
static void conv2py_resolve_Statement(
    size_t, std::ostream &, const Statement *, void *);
static void conv2py_resolve_DeclStatement(
    size_t, std::ostream &, const DeclStatement *, void *);
static void conv2py_resolve_NestedStatement(
    size_t, std::ostream &, const NestedStatement *, void *);
static void conv2py_resolve_ExprStatement(
    size_t, std::ostream &, const ExprStatement *, void *);
static void conv2py_resolve_IfElseStatement(
    size_t, std::ostream &, const IfElseStatement *, void *);
static void conv2py_resolve_WhileStatement(
    size_t, std::ostream &, const WhileStatement *, void *);
static void conv2py_resolve_BreakStatement(
    size_t, std::ostream &, const BreakStatement *, void *);
static void conv2py_resolve_ContinueStatement(
    size_t, std::ostream &, const ContinueStatement *, void *);
static void conv2py_resolve_ReturnStatement(
    size_t, std::ostream &, const ReturnStatement *, void *);
static void conv2py_resolve_Expr(size_t, std::ostream &, const Expr *, void *);
static void conv2py_resolve_UnaryExpr(
    size_t, std::ostream &, const UnaryExpr *, void *);
static void conv2py_resolve_BinaryExpr(
    size_t, std::ostream &, const BinaryExpr *, void *);
static void conv2py_resolve_FnCallExpr(
    size_t, std::ostream &, const FnCallExpr *, void *);
static void conv2py_resolve_ConstExprExpr(
    size_t, std::ostream &, const ConstExprExpr *, void *);
static void conv2py_resolve_OrphanExpr(
    size_t, std::ostream &, const OrphanExpr *, void *);

static void conv2py_resolve_TypeDecl(
    size_t indent, std::ostream &os, const TypeDecl *e, void *data) {
    static std::map<TypeDecl::Type, std::string_view> LOOKUP_TABLE{
        {TypeDecl::Type::Integer, "int"  },
        {TypeDecl::Type::Float,   "float"}
    };

    os << fmt::format("{:\t>{}}", "", indent);

    if (data != nullptr) {
        os << e->ident;
        if (e->optSubscriptList.has_value()) {
            const auto &list = e->optSubscriptList.value();
            for (const auto &e : list) {
                os << "[";
                conv2py_resolve_Expr(0, os, e, nullptr);
                os << "]";
            }
        }
    } else {
        //! TODO: 检查列表长度与 const 属性
        os << fmt::format(
            "{}: {}",
            e->ident,
            e->optSubscriptList.has_value() ? "list" : LOOKUP_TABLE[e->type]);
    }
}

static void conv2py_resolve_InitializeList(
    size_t indent, std::ostream &os, const InitializeList *e, void *data) {
    os << fmt::format("{:\t>{}}", "", indent);

    if (e->valueList.size() == 0) {
        os << "[]";
    } else {
        os << "[";
        conv2py_resolve_Expr(0, os, e->valueList[0], nullptr);
        for (int i = 1; i < e->valueList.size(); ++i) {
            os << ", ";
            conv2py_resolve_Expr(0, os, e->valueList[i], nullptr);
        }
        os << "]";
    }
}

static void conv2py_resolve_Program(
    size_t indent, std::ostream &os, const Program *e, void *data) {
    auto it0 = e->declFlows.begin();
    auto it1 = e->funcFlows.begin();

    while (true) {
        if (it0 == e->declFlows.end() && it1 == e->funcFlows.end()) {
            break;
        } else if (it0 == e->declFlows.end()) {
            auto e = (it1++)->second;
            conv2py_resolve_FuncDef(0, os, e, nullptr);
        } else if (it1 == e->funcFlows.end()) {
            auto e = (it0++)->second;
            conv2py_resolve_DeclStatement(0, os, e, nullptr);
        } else {
            const auto &[declIndex, decl] = *it0;
            const auto &[funcIndex, func] = *it1;
            if (declIndex < funcIndex) {
                ++it0;
                conv2py_resolve_DeclStatement(0, os, decl, nullptr);
            } else {
                ++it1;
                conv2py_resolve_FuncDef(0, os, func, nullptr);
            }
        }
    }

    os << "if __name__ == '__main__':\n\tprint(main())" << std::endl;
}

static void conv2py_resolve_FuncDef(
    size_t indent, std::ostream &os, const FuncDef *e, void *data) {
    static std::map<FuncDef::Type, std::string_view> LOOKUP_TABLE{
        {FuncDef::Type::Void,    "None" },
        {FuncDef::Type::Integer, "int"  },
        {FuncDef::Type::Float,   "float"}
    };

    os << fmt::format("{:\t>{}}", "", indent);

    os << fmt::format("def {}(", e->ident);
    if (e->params.size() > 0) {
        conv2py_resolve_TypeDecl(0, os, e->params[0], nullptr);
        for (int i = 1; i < e->params.size(); ++i) {
            os << ", ";
            conv2py_resolve_TypeDecl(0, os, e->params[i], nullptr);
        }
    }
    os << fmt::format(") -> {}:", LOOKUP_TABLE[e->retType]) << std::endl;

    if (e->body == nullptr) {
        os << fmt::format("{:\t>{}}pass", "", indent + 1) << std::endl;
    } else {
        conv2py_resolve_Block(indent + 1, os, e->body, nullptr);
    }
}

static void conv2py_resolve_Block(
    size_t indent, std::ostream &os, const Block *e, void *data) {
    for (const auto &statement : e->statementList) {
        conv2py_resolve_Statement(indent, os, statement, nullptr);
    }
}

static void conv2py_resolve_Statement(
    size_t indent, std::ostream &os, const Statement *e, void *data) {
    if (e->type == Statement::Type::Decl) {
        conv2py_resolve_DeclStatement(
            indent, os, static_cast<const DeclStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::Nested) {
        conv2py_resolve_NestedStatement(
            indent, os, static_cast<const NestedStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::Expr) {
        conv2py_resolve_ExprStatement(
            indent, os, static_cast<const ExprStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::IfElse) {
        conv2py_resolve_IfElseStatement(
            indent, os, static_cast<const IfElseStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::While) {
        conv2py_resolve_WhileStatement(
            indent, os, static_cast<const WhileStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::Break) {
        conv2py_resolve_BreakStatement(
            indent, os, static_cast<const BreakStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::Continue) {
        conv2py_resolve_ContinueStatement(
            indent, os, static_cast<const ContinueStatement *>(e), nullptr);
    } else if (e->type == Statement::Type::Return) {
        conv2py_resolve_ReturnStatement(
            indent, os, static_cast<const ReturnStatement *>(e), nullptr);
    }
}

static void conv2py_resolve_DeclStatement(
    size_t indent, std::ostream &os, const DeclStatement *e, void *data) {
    for (const auto &[decl, value] : e->declList) {
        os << fmt::format("{:\t>{}}", "", indent);
        conv2py_resolve_TypeDecl(0, os, decl, nullptr);
        os << " = ";
        if (value == nullptr) {
            if (decl->optSubscriptList.has_value()) {
                //! FIXME: 这里的初始化应该根据类型填充列表值
                os << "[]";
            } else if (decl->type == TypeDecl::Type::Integer) {
                os << "0";
            } else if (decl->type == TypeDecl::Type::Float) {
                os << "0.0";
            } else {
                std::unreachable();
            }
        } else {
            conv2py_resolve_Expr(0, os, value, nullptr);
        }
        os << std::endl;
    }
}

static void conv2py_resolve_NestedStatement(
    size_t indent, std::ostream &os, const NestedStatement *e, void *data) {
    os << fmt::format("{:\t>{}}if True:", "", indent) << std::endl;
    if (e->block == nullptr) {
        os << fmt::format("{:\t>{}}pass", "", indent + 1) << std::endl;
    } else {
        conv2py_resolve_Block(indent + 1, os, e->block, nullptr);
    }
}

static void conv2py_resolve_ExprStatement(
    size_t indent, std::ostream &os, const ExprStatement *e, void *data) {
    conv2py_resolve_Expr(indent, os, e->expr, nullptr);
    os << std::endl;
}

static void conv2py_resolve_IfElseStatement(
    size_t indent, std::ostream &os, const IfElseStatement *e, void *data) {
    os << fmt::format("{:\t>{}}if ", "", indent);
    conv2py_resolve_Expr(0, os, e->condition, nullptr);
    os << ":" << std::endl;

    if (e->trueRoute == nullptr) {
        os << fmt::format("{:\t>{}}pass", "", indent + 1) << std::endl;
    } else {
        conv2py_resolve_Statement(indent + 1, os, e->trueRoute, nullptr);
    }

    os << fmt::format("{:\t>{}}else:", "", indent) << std::endl;

    if (e->falseRoute == nullptr) {
        os << fmt::format("{:\t>{}}pass", "", indent + 1) << std::endl;
    } else {
        conv2py_resolve_Statement(indent + 1, os, e->falseRoute, nullptr);
    }
}

static void conv2py_resolve_WhileStatement(
    size_t indent, std::ostream &os, const WhileStatement *e, void *data) {
    os << fmt::format("{:\t>{}}if ", "", indent);
    conv2py_resolve_Expr(0, os, e->condition, nullptr);
    os << ":" << std::endl;

    if (e->body == nullptr) {
        os << fmt::format("{:\t>{}}pass", "", indent + 1) << std::endl;
    } else {
        conv2py_resolve_Statement(indent + 1, os, e->body, nullptr);
    }
}

static void conv2py_resolve_BreakStatement(
    size_t indent, std::ostream &os, const BreakStatement *e, void *data) {
    os << fmt::format("{:\t>{}}break", "", indent) << std::endl;
}

static void conv2py_resolve_ContinueStatement(
    size_t indent, std::ostream &os, const ContinueStatement *e, void *data) {
    os << fmt::format("{:\t>{}}continue", "", indent) << std::endl;
}

static void conv2py_resolve_ReturnStatement(
    size_t indent, std::ostream &os, const ReturnStatement *e, void *data) {
    os << fmt::format("{:\t>{}}return", "", indent);
    if (e->retval != nullptr) {
        os << " ";
        conv2py_resolve_Expr(0, os, e->retval, nullptr);
    }
    os << std::endl;
}

static void conv2py_resolve_Expr(
    size_t indent, std::ostream &os, const Expr *e, void *data) {
    if (e->type == Expr::Type::Unary) {
        conv2py_resolve_UnaryExpr(
            indent, os, static_cast<const UnaryExpr *>(e), nullptr);
    } else if (e->type == Expr::Type::Binary) {
        conv2py_resolve_BinaryExpr(
            indent, os, static_cast<const BinaryExpr *>(e), nullptr);
    } else if (e->type == Expr::Type::FnCall) {
        conv2py_resolve_FnCallExpr(
            indent, os, static_cast<const FnCallExpr *>(e), nullptr);
    } else if (e->type == Expr::Type::ConstExpr) {
        conv2py_resolve_ConstExprExpr(
            indent, os, static_cast<const ConstExprExpr *>(e), nullptr);
    } else if (e->type == Expr::Type::Orphan) {
        conv2py_resolve_OrphanExpr(
            indent, os, static_cast<const OrphanExpr *>(e), nullptr);
    }
}

static void conv2py_resolve_UnaryExpr(
    size_t indent, std::ostream &os, const UnaryExpr *e, void *data) {
    static std::map<UnaryExpr::Type, std::string_view> LOOKUP_TABLE{
        {UnaryExpr::Type::LNOT, "not"},
        {UnaryExpr::Type::POS,  "+"  },
        {UnaryExpr::Type::NEG,  "-"  },
    };
    os << fmt::format("{:\t>{}}{}", "", indent, LOOKUP_TABLE[e->op]);
    if (e->op == UnaryExpr::Type::LNOT) { os << " "; }
    os << "(";
    conv2py_resolve_Expr(0, os, e->operand, nullptr);
    os << ")";
}

static void conv2py_resolve_BinaryExpr(
    size_t indent, std::ostream &os, const BinaryExpr *e, void *data) {
    static std::map<BinaryExpr::Type, std::string_view> LOOKUP_TABLE{
        {BinaryExpr::Type::ASSIGN,    "="  },
        {BinaryExpr::Type::ADD,       "+"  },
        {BinaryExpr::Type::SUB,       "-"  },
        {BinaryExpr::Type::MUL,       "*"  },
        {BinaryExpr::Type::DIV,       "/"  },
        {BinaryExpr::Type::MOD,       "%"  },
        {BinaryExpr::Type::LT,        "<"  },
        {BinaryExpr::Type::LE,        "<=" },
        {BinaryExpr::Type::GT,        ">"  },
        {BinaryExpr::Type::GE,        ">=" },
        {BinaryExpr::Type::EQ,        "==" },
        {BinaryExpr::Type::NE,        "!=" },
        {BinaryExpr::Type::LAND,      "and"},
        {BinaryExpr::Type::LOR,       "or" },
        {BinaryExpr::Type::SUBSCRIPT, "["  },
    };

    os << fmt::format("{:\t>{}}", "", indent);

    os << "(";
    conv2py_resolve_Expr(0, os, e->lhs, nullptr);
    os << ")";

    os << fmt::format(" {} ", LOOKUP_TABLE[e->op]);

    os << "(";
    conv2py_resolve_Expr(0, os, e->rhs, nullptr);
    os << ")";

    if (e->op == BinaryExpr::Type::SUBSCRIPT) { os << "]"; }
}

static void conv2py_resolve_FnCallExpr(
    size_t indent, std::ostream &os, const FnCallExpr *e, void *data) {
    os << fmt::format("{:\t>{}}{}(", "", indent, e->func->ident);
    if (e->params.size() > 0) {
        conv2py_resolve_Expr(0, os, e->params[0], nullptr);
        for (int i = 1; i < e->params.size(); ++i) {
            os << ", ";
            conv2py_resolve_Expr(0, os, e->params[i], nullptr);
        }
    }
    os << ")";
}

static void conv2py_resolve_ConstExprExpr(
    size_t indent, std::ostream &os, const ConstExprExpr *e, void *data) {
    os << fmt::format("{:\t>{}}", "", indent);
    if (std::holds_alternative<int32_t>(e->value)) {
        os << std::get<int32_t>(e->value);
    } else if (std::holds_alternative<float>(e->value)) {
        os << std::get<float>(e->value);
    } else {
        std::unreachable();
    }
}

static void conv2py_resolve_OrphanExpr(
    size_t indent, std::ostream &os, const OrphanExpr *e, void *data) {
    os << fmt::format("{:\t>{}}", "", indent);
    if (std::holds_alternative<TypeDecl *>(e->ref)) {
        conv2py_resolve_TypeDecl(
            0, os, std::get<TypeDecl *>(e->ref), (void *)(uintptr_t)1);
    } else if (std::holds_alternative<InitializeList *>(e->ref)) {
        conv2py_resolve_InitializeList(
            0, os, std::get<InitializeList *>(e->ref), nullptr);
    } else {
        std::unreachable();
    }
}

void conv2py(std::ostream &os, const ssyc::ast::Program *program) {
	os << R"(
from sys import stdin
def getch() -> str:
	return stdin.read(1)
def getint() -> int:
	return int(input())
def putint(x: int) -> None:
	print(x, end='')
def putch(x: int) -> None:
	print(chr(x), end='')
)";
    conv2py_resolve_Program(0, os, program, nullptr);
}
