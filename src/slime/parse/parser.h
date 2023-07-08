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

class Parser {
public:
    Parser()
        : stringSet_{lexer_.strtable()} {
        //! the bottom is always alive for global symbols
        symbolTableStack_.push_back(new SymbolTable);
    }

    template <typename T>
    void reset(T& stream) {
        lexer_.reset(stream);
    }

    inline const Token& token() const;
    inline const Lexer& lexer() const;
    inline Lexer*       unbindLexer();

    bool        expect(TOKEN token, const char* msg = nullptr);
    const char* lookup(std::string_view s);

    void       addSymbol(NamedDecl* decl);
    NamedDecl* findSymbol(std::string_view name, DeclID declID);

    TranslationUnit* parse();

    void             parseGlobalDecl();
    DeclStmt*        parseDeclStmt();
    VarDecl*         parseVarDef();
    FunctionDecl*    parseFunction();
    ParamVarDeclList parseFunctionParams();
    Stmt*            parseStmt(bool standalone = false);
    IfStmt*          parseIfStmt();
    DoStmt*          parseDoStmt();
    WhileStmt*       parseWhileStmt();
    ForStmt*         parseForStmt();
    BreakStmt*       parseBreakStmt();
    ContinueStmt*    parseContinueStmt();
    ReturnStmt*      parseReturnStmt();
    CompoundStmt*    parseBlock();
    Expr*            parsePrimaryExpr();
    Expr*            parsePostfixExpr();
    Expr*            parseUnaryExpr();
    inline Expr*     parseBinaryExpr();
    Expr*            parseCommaExpr();
    InitListExpr*    parseInitListExpr();
    ExprList*        parseExprList();

protected:
    void          enterDecl();
    void          leaveDecl();
    FunctionDecl* enterFunction();
    void          leaveFunction();
    void          enterBlock();
    void          leaveBlock();

private:
    void addExternalFunction(
        const char* name, Type* returnType, ParamVarDeclList& params);

    void addPresetSymbols();

    Expr* parseBinaryExprWithPriority(int priority);

private:
    using SymbolTable = std::map<std::string_view, NamedDecl*>;

    struct ParseState {
        FunctionDecl*     cur_func              = nullptr;
        int               cur_depth             = 0;
        DeclID            decl_type             = DeclID::ParamVar;
        DeclSpecifier*    cur_specifs           = nullptr;
        ParamVarDeclList* cur_params            = nullptr;
        TranslationUnit*  tu                    = nullptr;
        LoopStmt*         cur_loop              = nullptr;
        bool              not_deepen_next_block = false;
        bool              ignore_next_funcdecl  = false;
    };

    Lexer                                  lexer_;
    ParseState                             state_;
    std::shared_ptr<std::set<const char*>> stringSet_;
    std::vector<SymbolTable*>              symbolTableStack_;
};

inline const Token& Parser::token() const {
    return lexer().this_token();
}

inline const Lexer& Parser::lexer() const {
    return lexer_;
}

inline Lexer* Parser::unbindLexer() {
    auto lexer = new Lexer(std::move(lexer_));
    new (&lexer_) Lexer;
    return lexer;
}

inline Expr* Parser::parseBinaryExpr() {
    return parseBinaryExprWithPriority(std::numeric_limits<int>::max());
}

} // namespace slime
