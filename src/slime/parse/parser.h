#pragma once

#include "../lex/lexer.h"
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
    Parser()
        : stringSet{lexer_.strtable()}
        , ps{} {
        //! the bottom is always alive for global symbols
        symbolTable.push_back(new SymbolTable);
    }

    template <typename T>
    void reset(T& stream) {
        lexer_.reset(stream);
    }

    const Lexer& lexer() const {
        return lexer_;
    }

    Lexer* move_lexer() {
        auto lexer = new Lexer(std::move(lexer_));
        new (&lexer_) Lexer;
        return lexer;
    }

    bool        expect(TOKEN token, const char* msg = nullptr);
    const char* lookupStringLiteral(std::string_view s);

protected:
    void          enterblock();
    void          leaveblock();
    void          enterdecl();
    void          leavedecl();
    FunctionDecl* enterfunc();
    void          leavefunc();

public:
    TranslationUnit* parse();
    void             addSymbol(NamedDecl* decl);

    void addExternalFunc(
        const char* name, Type* returnType, ParamVarDeclList& params);
    void presetFunction();

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

    Expr* primaryexpr();
    Expr* postfixexpr();
    Expr* unaryexpr();

    Expr* binexpr(int priority = std::numeric_limits<int>::max());
    Expr* commaexpr();

    ExprList* exprlist();

private:
    using SymbolTable = std::map<std::string_view, NamedDecl*>;

    Lexer                                  lexer_;
    ParseState                             ps;
    std::shared_ptr<std::set<const char*>> stringSet;
    std::vector<SymbolTable*>              symbolTable;
};

} // namespace slime
