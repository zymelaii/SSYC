#pragma once

#include "58.h"
#include "0.h"
#include "10.h"
#include "5.h"
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
    void reset(T& stream, std::string_view source) {
        lexer_.reset(stream, source);
    }

    inline const Token&         token() const;
    inline const Lexer&         lexer() const;
    [[nodiscard]] inline Lexer* unbindLexer();

    bool        expect(TOKEN token, const char* msg = nullptr);
    const char* lookup(std::string_view s);

    void                     addSymbol(NamedDecl* decl);
    [[nodiscard]] NamedDecl* findSymbol(std::string_view name, DeclID declID);

    [[nodiscard]] TranslationUnit* parse();

    void                           parseGlobalDecl();
    [[nodiscard]] DeclStmt*        parseDeclStmt();
    [[nodiscard]] VarDecl*         parseVarDef();
    [[nodiscard]] FunctionDecl*    parseFunction();
    [[nodiscard]] ParamVarDeclList parseFunctionParams();
    [[nodiscard]] Stmt*            parseStmt(bool standalone = false);
    [[nodiscard]] IfStmt*          parseIfStmt();
    [[nodiscard]] DoStmt*          parseDoStmt();
    [[nodiscard]] WhileStmt*       parseWhileStmt();
    [[nodiscard]] ForStmt*         parseForStmt();
    [[nodiscard]] BreakStmt*       parseBreakStmt();
    [[nodiscard]] ContinueStmt*    parseContinueStmt();
    [[nodiscard]] ReturnStmt*      parseReturnStmt();
    [[nodiscard]] CompoundStmt*    parseBlock();
    [[nodiscard]] Expr*            parsePrimaryExpr();
    [[nodiscard]] Expr*            parsePostfixExpr();
    [[nodiscard]] Expr*            parseUnaryExpr();
    [[nodiscard]] Expr*            parseBinaryExpr();
    [[nodiscard]] Expr*            parseCommaExpr();
    [[nodiscard]] InitListExpr*    parseInitListExpr();
    [[nodiscard]] ExprList*        parseExprList();

protected:
    void                        enterDecl();
    void                        leaveDecl();
    [[nodiscard]] FunctionDecl* enterFunction();
    void                        leaveFunction();
    void                        enterBlock();
    void                        leaveBlock();

private:
    void addExternalFunction(
        const char* name, Type* returnType, ParamVarDeclList& params);

    void addPresetSymbols();

    void dropUnusedExternalSymbols();

    [[nodiscard]] Expr* parseBinaryExprWithPriority(int priority);

private:
    using SymbolTable = std::map<std::string_view, NamedDecl*>;

    struct ParseState {
        FunctionDecl*        cur_func              = nullptr;
        int                  cur_depth             = 0;
        DeclID               decl_type             = DeclID::ParamVar;
        DeclSpecifier*       cur_specifs           = nullptr;
        ParamVarDeclList*    cur_params            = nullptr;
        TranslationUnit*     tu                    = nullptr;
        LoopStmt*            cur_loop              = nullptr;
        bool                 not_deepen_next_block = false;
        bool                 ignore_next_funcdecl  = false;
        std::set<NamedDecl*> symref_set;
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

} // namespace slime
