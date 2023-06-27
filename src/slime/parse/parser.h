#pragma once

#include "../lex/lex.h"
#include "../ast/ast.h"
#include "../ast/stmt.h"
#include "../ast/expr.h"

#include <set>
#include <map>
#include <queue>
#include <string_view>

namespace slime {

using namespace ast;

using SymbolTable     = std::map<std::string_view , NamedDecl*>;
using SymbolTableList = slime::utils::ListTrait<SymbolTable *>;

struct ParseState {
    FunctionDecl*
        cur_func; //<! index in gsym of current parsing function(-1 if not in a
                  // function)
    int              cur_depth;
    DeclSpecifier    cur_specifs;
    TranslationUnit* tu;
    WhileStmt*       cur_loop;
};

class Parser {
public:
    LexState               ls;
    ParseState             ps;
    std::set<const char*>& sharedStringSet;
    SymbolTableList        symbolTable;

    Parser()
        : sharedStringSet{ls.sharedStringSet()}
        , ps{NULL, 0, {}, NULL, NULL} {}

    void        next();
    bool        expect(TOKEN token, const char* msg = nullptr);
    const char* lookupStringLiteral(std::string_view s);

protected:
    void          enterblock();
    void          leaveblock();
    void          enterdecl();
    void          leavedecl(DeclID tag);
    FunctionDecl* enterfunc(); // add func symbol to g_sym and return its index
    void          leavefunc();

public:
    TranslationUnit* parse();
    void             global_decl();
    DeclStmt*        decl();
    VarDecl*         vardef();
    InitListExpr*    initlist();
    FunctionDecl*    func();
    ParamVarDeclList funcargs();
    Stmt*            statement();
    IfStmt*          ifstat();
    WhileStmt*       whilestat();
    BreakStmt*       breakstat();
    ContinueStmt*    continuestat();
    ReturnStmt*      returnstat();
    CompoundStmt*    block();

    BinaryOperator binastop(TOKEN token);

    NamedDecl* findSymbol(std::string_view name, DeclID declID);
    // search a symbol in g_sym and return
    // its index(-1 if failed)

    //! search a local symbol in a block and return its index in l_sym.
    //! pblock will point to that block if it is not NULL

    Expr* primaryexpr();
    Expr* postfixexpr();
    Expr* unaryexpr();

    Expr* binexpr(int priority);
    Expr* expr();

    ExprList* exprlist();

    // 输出AST（后序遍历）
};

} // namespace slime
