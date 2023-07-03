#pragma once

#include "../lex/lex.h"
#include "../ast/ast.h"
#include "../ast/stmt.h"
#include "../ast/expr.h"

#include <set>
#include <map>
#include <vector>
#include <limits>
#include <string_view>

namespace slime {

using namespace ast;

struct ParseState {
    FunctionDecl*     cur_func             = nullptr;
    int               cur_depth            = 0;
    DeclID            decl_type            = DeclID::ParamVar;
    DeclSpecifier*    cur_specifs          = nullptr;
    ParamVarDeclList* cur_params           = nullptr;
    TranslationUnit*  tu                   = nullptr;
    WhileStmt*        cur_loop             = nullptr;
    bool              next_block_as_fn     = false;
    bool              ignore_next_funcdecl = false;
};

class Parser {
public:
    using SymbolTable = std::map<std::string_view, NamedDecl*>;

    LexState                  ls;
    ParseState                ps;
    std::set<const char*>&    sharedStringSet;
    std::vector<SymbolTable*> symbolTable;

    Parser()
        : sharedStringSet{ls.sharedStringSet()}
        , ps{} {
        //! the bottom is always alive for global symbols
        symbolTable.push_back(new SymbolTable);
    }

    void        next();
    bool        expect(TOKEN token, const char* msg = nullptr);
    const char* lookupStringLiteral(std::string_view s);

protected:
    void          enterblock();
    void          leaveblock();
    void          enterdecl();
    void          leavedecl();
    FunctionDecl* enterfunc(); // add func symbol to g_sym and return its index
    void          leavefunc();

public:
    TranslationUnit* parse();
    void             addSymbol(NamedDecl* decl);
    void             addExternalFunc(
                    const char*       name,
                    Type*             returnType,
                    ParamVarDeclList& params); // preset some external functions
    void             presetFunction();
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

    Expr* binexpr(int priority = std::numeric_limits<int>::max());
    Expr* commaexpr();

    ExprList* exprlist();

    // 输出AST（后序遍历）
};

} // namespace slime
