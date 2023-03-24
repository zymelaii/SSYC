#include "utils_treegraph.h"
#include "ast.h"

#include <set>
#include <utility>
#include <queue>
#include <stack>
#include <assert.h>

using namespace ssyc::ast;

namespace ssyc::utils {
namespace graph {

AbstractTreeNode::~AbstractTreeNode() {
    parent = nullptr;
}

SequenceTreeNode::~SequenceTreeNode() {
    for (auto &e : list) {
        assert(e != nullptr);
        delete e;
    }
    list.clear();
}

UnaryTreeNode::~UnaryTreeNode() {
    if (child != nullptr) {
        delete child;
        child = nullptr;
    }
}

BinaryTreeNode::~BinaryTreeNode() {
    if (lhs != nullptr) {
        delete lhs;
        lhs = nullptr;
    }

    if (rhs != nullptr) {
        delete rhs;
        rhs = nullptr;
    }
}

} // namespace graph

using namespace graph;

UnaryTreeNode *buildGraphTree(Program *program) {
    auto root  = new UnaryTreeNode;
    root->hint = "program";

    //! NOTE: 摆烂了，不想写了。

    std::stack<std::tuple<int, ssyc::ast::Type, ProgramUnit *>> stackIn;
    std::stack<std::tuple<int, ssyc::ast::Type, ProgramUnit *>> stackOut;
    std::set<ProgramUnit *>                                     outSet;

    stackIn.emplace(0, Type::Program, program);

    while (!stackIn.empty()) {
        auto &[hint, type, unit] = stackIn.top();

        switch (type) {
            case Type::TypeDecl: {
                stackIn.pop();
            } break;
            case Type::InitializeList: {
                stackIn.pop();
            } break;
            case Type::Program: {
                auto e    = static_cast<Program *>(unit);
                auto decl = std::find_if(
                    e->declFlows.begin(),
                    e->declFlows.end(),
                    [index = hint](auto e) {
                        return e.first == index;
                    });
                auto func = std::find_if(
                    e->funcFlows.begin(),
                    e->funcFlows.end(),
                    [index = hint](auto e) {
                        return e.first == index;
                    });
                if (decl != e->declFlows.end()) {
                    stackIn.emplace(0, Type::DeclStatement, decl->second);
                } else if (func != e->funcFlows.end()) {
                    stackIn.emplace(0, Type::FuncDef, decl->second);
                } else {
                    std::unreachable();
                }
                if (++hint == e->declFlows.size() + e->funcFlows.size()) {
                    stackIn.pop();
                }
            } break;
            case Type::FuncDef: {
                stackIn.emplace(0, Type::Block, unit);
                stackIn.pop();
            } break;
            case Type::Block: {
                auto e = static_cast<Block *>(unit);
                switch (e->statementList[hint]->type) {
                    case Statement::Type::Decl: {
                        stackIn.emplace(
                            0, Type::DeclStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::Nested: {
                        stackIn.emplace(
                            0, Type::NestedStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::Expr: {
                        stackIn.emplace(
                            0, Type::ExprStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::IfElse: {
                        stackIn.emplace(
                            0, Type::IfElseStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::While: {
                        stackIn.emplace(
                            0, Type::WhileStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::Break: {
                        stackIn.emplace(
                            0, Type::BreakStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::Continue: {
                        stackIn.emplace(
                            0, Type::ContinueStatement, e->statementList[hint]);
                    } break;
                    case Statement::Type::Return: {
                        stackIn.emplace(
                            0, Type::ReturnStatement, e->statementList[hint]);
                    } break;
                }
                if (++hint == e->statementList.size()) { stackIn.pop(); }
            } break;
            case Type::Statement: {
                std::unreachable();
            } break;
            case Type::DeclStatement: {
            } break;
            case Type::NestedStatement: {
            } break;
            case Type::ExprStatement: {
            } break;
            case Type::IfElseStatement: {
            } break;
            case Type::WhileStatement: {
            } break;
            case Type::BreakStatement: {
            } break;
            case Type::ContinueStatement: {
            } break;
            case Type::ReturnStatement: {
            } break;
            case Type::Expr: {
            } break;
            case Type::UnaryExpr: {
            } break;
            case Type::BinaryExpr: {
            } break;
            case Type::FnCallExpr: {
            } break;
            case Type::ConstExprExpr: {
            } break;
            case Type::OrphanExpr: {
            } break;
            default: {
                std::unreachable();
            } break;
        }

        if (!outSet.contains(unit)) {
            stackOut.emplace(type, unit);
            outSet.insert(unit);
        }
    }

    return root;
}

} // namespace ssyc::utils