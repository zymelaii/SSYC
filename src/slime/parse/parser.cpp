#include "parser.h"
#include "diagnosis.h"
#include "../visitor/ASTExprSimplifier.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <map>
#include <array>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string_view>

namespace slime {

using visitor::ASTExprSimplifier;

struct OperatorPriority {
    constexpr OperatorPriority(int priority, bool assoc)
        : priority{priority}
        , assoc{assoc} {}

    //! 优先级
    int priority;

    //! 结合性
    //! false: 右结合
    //! true: 左结合
    bool assoc;
};

static constexpr size_t TOTAL_OPERATORS = 25;
static constexpr std::array<OperatorPriority, TOTAL_OPERATORS> PRIORITIES{
    //! UnaryOperator (except Paren)
    OperatorPriority(1, false),
    OperatorPriority(1, false),
    OperatorPriority(1, false),
    OperatorPriority(1, false),
    //! Subscript '[]'
    OperatorPriority(0, true),
    //! BinaryOperator
    //! Mul Div Mod
    OperatorPriority(2, true),
    OperatorPriority(2, true),
    OperatorPriority(2, true),
    //! Add Sub
    OperatorPriority(3, true),
    OperatorPriority(3, true),
    //! Shl Shr
    OperatorPriority(4, true),
    OperatorPriority(4, true),
    //! Comparaison Operator
    OperatorPriority(5, true),
    OperatorPriority(5, true),
    OperatorPriority(5, true),
    OperatorPriority(5, true),
    //! EQ NE
    OperatorPriority(6, true),
    OperatorPriority(6, true),
    //! And
    OperatorPriority(7, true),
    //! Xor
    OperatorPriority(8, true),
    //! Or
    OperatorPriority(9, true),
    //! Logic And
    OperatorPriority(10, true),
    //! Logic Or
    OperatorPriority(11, true),
    //! Assign
    OperatorPriority(12, false),
    //! Comma
    OperatorPriority(13, true),
};

inline OperatorPriority lookupOperatorPriority(UnaryOperator op) {
    assert(op != UnaryOperator::Paren);
    return PRIORITIES[static_cast<int>(op)];
}

inline OperatorPriority lookupOperatorPriority(BinaryOperator op) {
    return PRIORITIES
        [static_cast<int>(op) + static_cast<int>(UnaryOperator::Paren)];
}

bool Parser::expect(TOKEN token, const char *msg) {
    if (lexer_.this_token() == token) {
        lexer_.next();
        return true;
    }
    //! TODO: prettify display message
    if (msg != nullptr) { fprintf(stderr, "%s", msg); }
    //! TODO: raise an error
    return false;
}

void Parser::addSymbol(NamedDecl *decl) {
    assert(ps.cur_depth + 1 == symbolTable.size());
    if (decl->name.empty()) { return; }
    decl->scope.depth = ps.cur_depth;
    if (!decl->scope.isGlobal()) {
        assert(ps.cur_func != nullptr);
        decl->scope.scope = lookupStringLiteral(ps.cur_func->name);
    }
    symbolTable.back()->insert_or_assign(lookupStringLiteral(decl->name), decl);
}

void Parser::addExternalFunc(
    const char *name, Type *returnType, ParamVarDeclList &params) {
    auto spec = DeclSpecifier::create();
    spec->addSpecifier(NamedDeclSpecifier::Extern);
    spec->type = returnType;
    addSymbol(FunctionDecl::create(name, spec, params, nullptr));
}

void Parser::presetFunction() {
    ParamVarDeclList params{};

    auto specInt         = DeclSpecifier::create();
    auto specFloat       = DeclSpecifier::create();
    auto specIntArray    = specInt->clone();
    auto specFloatArray  = specFloat->clone();
    specInt->type        = BuiltinType::getIntType();
    specFloat->type      = BuiltinType::getFloatType();
    specIntArray->type   = IncompleteArrayType::create(specIntArray->type);
    specFloatArray->type = IncompleteArrayType::create(specFloatArray->type);
    auto paramInt        = ParamVarDecl::create(specInt);
    auto paramIntArray   = ParamVarDecl::create(specIntArray);
    auto paramFloat      = ParamVarDecl::create(specFloat);
    auto paramFloatArray = ParamVarDecl::create(specFloatArray);

    //! int getint()
    addExternalFunc("getint", BuiltinType::getIntType(), params);

    //! int getch()
    addExternalFunc("getch", BuiltinType::getIntType(), params);

    //! float getfloat()
    addExternalFunc("getfloat", BuiltinType::getFloatType(), params);

    //! void putint(int)
    auto pInt = params.insertToTail(paramInt);
    addExternalFunc("putint", BuiltinType::getVoidType(), params);

    //! void putch(int)
    addExternalFunc("putch", BuiltinType::getVoidType(), params);

    //! void putarray(int, int[])
    auto pIntArray = params.insertToTail(paramIntArray);
    addExternalFunc("putarray", BuiltinType::getVoidType(), params);

    //! void putfarray(int, float[])
    pIntArray->removeFromList();
    auto pFloatArray = params.insertToTail(paramFloatArray);
    addExternalFunc("putfarray", BuiltinType::getVoidType(), params);

    //! int getfarray(float[])
    pInt->removeFromList();
    addExternalFunc("getfarray", BuiltinType::getIntType(), params);

    //! int getarray(int[])
    pFloatArray->removeFromList();
    pIntArray->insertToTail(params);
    addExternalFunc("getarray", BuiltinType::getIntType(), params);

    //! void putfloat(float)
    pIntArray->removeFromList();
    auto pFloat = params.insertToTail(paramFloat);
    addExternalFunc("putfloat", BuiltinType::getVoidType(), params);

    //! TODO: void putf(char[], ...)
}

const char *Parser::lookupStringLiteral(std::string_view s) {
    if (s.empty()) { return nullptr; }
    auto result = s.data();
    if (!stringSet->count(result)) {
        result = strdup(result);
        assert(result != nullptr);
        auto [_, ok] = stringSet->insert(result);
        assert(ok);
    }
    return result;
}

NamedDecl *Parser::findSymbol(std::string_view name, DeclID declId) {
    assert(symbolTable.size() >= 1);
    assert(declId != DeclID::ParamVar);
    switch (declId) {
        case DeclID::Var:
        case DeclID::ParamVar: {
            auto       it  = symbolTable.rbegin();
            const auto end = symbolTable.rend();
            while (it != end) {
                if (auto result = (*it)->find(name); result != (*it)->end()) {
                    auto &[_, decl] = *result;
                    //! reduce param var to normal var
                    auto type = decl->declId == DeclID::ParamVar ? DeclID::Var
                                                                 : decl->declId;
                    if (type == declId) { return decl; }
                }
                ++it;
            }
        } break;
        case DeclID::Function: {
            auto &symbols = symbolTable.front();
            if (auto it = symbols->find(name); it != symbols->end()) {
                auto &[_, decl] = *it;
                if (decl->tryIntoFunctionDecl()) { return decl; }
            }
        } break;
    }
    return nullptr;
}

BinaryOperator Parser::binastop(TOKEN token) {
    switch (token) {
        case TOKEN::TK_ASS: {
            return BinaryOperator::Assign;
        } break;
        case TOKEN::TK_AND: {
            return BinaryOperator::And;
        } break;
        case TOKEN::TK_OR: {
            return BinaryOperator::Or;
        } break;
        case TOKEN::TK_XOR: {
            return BinaryOperator::Xor;
        } break;
        case TOKEN::TK_LAND: {
            return BinaryOperator::LAnd;
        } break;
        case TOKEN::TK_LOR: {
            return BinaryOperator::LOr;
        } break;
        case TOKEN::TK_EQ: {
            return BinaryOperator::EQ;
        } break;
        case TOKEN::TK_NE: {
            return BinaryOperator::NE;
        } break;
        case TOKEN::TK_LT: {
            return BinaryOperator::LT;
        } break;
        case TOKEN::TK_LE: {
            return BinaryOperator::LE;
        } break;
        case TOKEN::TK_GT: {
            return BinaryOperator::GT;
        } break;
        case TOKEN::TK_GE: {
            return BinaryOperator::GE;
        } break;
        case TOKEN::TK_SHL: {
            return BinaryOperator::Shl;
        } break;
        case TOKEN::TK_SHR: {
            return BinaryOperator::Shr;
        } break;
        case TOKEN::TK_ADD: {
            return BinaryOperator::Add;
        } break;
        case TOKEN::TK_SUB: {
            return BinaryOperator::Sub;
        } break;
        case TOKEN::TK_MUL: {
            return BinaryOperator::Mul;
        } break;
        case TOKEN::TK_DIV: {
            return BinaryOperator::Div;
        } break;
        case TOKEN::TK_MOD: {
            return BinaryOperator::Mod;
        } break;
        default: {
            Diagnosis::assertAlwaysFalse("unknown binary operator");
        } break;
    }
}

TranslationUnit *Parser::parse() {
    ps.tu = new TranslationUnit;
    presetFunction();
    if (lexer_.isDiscard(lexer_.this_token())) { lexer_.next(); }
    while (!lexer_.this_token().isEOF()) { global_decl(); }
    return ps.tu;
}

void Parser::enterdecl() {
    //! get leading declarator
    auto spec = DeclSpecifier::create();
    bool done = false;
    while (!done) {
        switch (lexer_.this_token().id) {
            case TOKEN::TK_CONST: {
                spec->addSpecifier(NamedDeclSpecifier::Const);
            } break;
            case TOKEN::TK_EXTERN: {
                spec->addSpecifier(NamedDeclSpecifier::Extern);
            } break;
            case TOKEN::TK_STATIC: {
                spec->addSpecifier(NamedDeclSpecifier::Static);
            } break;
            case TOKEN::TK_INLINE: {
                spec->addSpecifier(NamedDeclSpecifier::Inline);
            } break;
            case TOKEN::TK_VOID: {
                spec->type = BuiltinType::getVoidType();
                done       = true;
            } break;
            case TOKEN::TK_INT: {
                spec->type = BuiltinType::getIntType();
                done       = true;
            } break;
            case TOKEN::TK_FLOAT: {
                spec->type = BuiltinType::getFloatType();
                done       = true;
            } break;
            case TOKEN::TK_IDENT: {
                Diagnosis::assertAlwaysFalse("unknown type name");
            } break;
            default: {
                Diagnosis::assertAlwaysFalse("expect unqualified-id");
            } break;
        }
        lexer_.next();
    }
    ps.cur_specifs = spec;
}

void Parser::leavedecl() {
    //! clear decl environment
    //! NOTE: ps.cur_specifs != nullptr is not always true,
    /// if the current decl is a function then the defination
    /// will probably eat the cur_specifs
    assert(ps.decl_type != DeclID::ParamVar);
    if (ps.decl_type == DeclID::Var) {
        expect(TOKEN::TK_SEMICOLON, "expect ';'");
    }
    ps.cur_specifs = nullptr;
}

FunctionDecl *Parser::enterfunc() {
    //! parse function proto
    assert(symbolTable.size() == 1);
    FunctionDecl *fn   = nullptr;
    const char   *name = lookupStringLiteral(lexer_.this_token().detail);
    expect(TOKEN::TK_IDENT, "expect unqualified-id");
    Diagnosis::assertTrue(
        lexer_.this_token() == TOKEN::TK_LPAREN, "expect '('");
    auto params = std::move(funcargs());
    fn          = FunctionDecl::create(name, ps.cur_specifs, params, nullptr);
    bool          haveBody = lexer_.this_token() == TOKEN::TK_LBRACE;
    bool          skip     = false;
    FunctionDecl *rewrite  = nullptr;
    auto          symbols  = symbolTable.back();
    if (auto it = symbols->find(name); it != symbols->end()) {
        auto [_, decl] = *it;
        bool duplicate = !fn->proto()->equals(decl->type());
        if (!duplicate) {
            auto func = decl->asFunctionDecl();
            if (func->body && haveBody) {
                duplicate = true;
            } else if (!haveBody) {
                ps.ignore_next_funcdecl = skip = true;
            } else {
                rewrite    = func;
                it->second = fn;
            }
        }
        Diagnosis::assertTrue(!duplicate, "duplicate symbol");
    }
    if (!skip && !rewrite) { addSymbol(fn); }
    if (rewrite != nullptr) {
        bool done = false;
        auto node = ps.tu->head();
        assert(node != nullptr);
        while (!done) {
            if (auto target = node->value()->tryIntoFunctionDecl();
                target == rewrite) {
                node->removeFromList();
                done = true;
            }
            if (node == ps.tu->tail()) { break; }
            node = node->next();
        }
    }
    ps.cur_func = fn;
    if (haveBody) {
        enterblock();
        ps.next_block_as_fn = true;
    }
    for (auto param : *fn) { addSymbol(param); }
    return ps.cur_func;
}

void Parser::leavefunc() {
    assert(ps.cur_func != nullptr);
    //! only allow external function to be undefined
    auto fn = ps.cur_func;
    Diagnosis::assertTrue(
        fn->body != nullptr || fn->specifier->isExtern(),
        "missing defination of local function");
    if (fn->body != nullptr) {
        fn->specifier->removeSpecifier(NamedDeclSpecifier::Extern);
    }
    //! reset current function
    ps.cur_func = nullptr;
}

void Parser::enterblock() {
    if (!ps.next_block_as_fn) {
        ++ps.cur_depth;
        symbolTable.push_back(new SymbolTable);
    }
    ps.next_block_as_fn = false;
}

void Parser::leaveblock() {
    --ps.cur_depth;
    symbolTable.pop_back();
    assert(ps.cur_depth >= 0);
    assert(symbolTable.size() >= 1);
}

void Parser::global_decl() {
    assert(ps.tu != nullptr);
    assert(symbolTable.size() == 1);
    enterdecl();
    while (!lexer_.this_token().isEOF()) {
        //! parse global variable
        if (lexer_.lookahead() != TOKEN::TK_LPAREN) {
            Diagnosis::assertTrue(
                !ps.cur_specifs->type->asBuiltin()->isVoid(),
                "variable has incomplete type 'void'");
            ps.tu->insertToTail(vardef());
            ps.decl_type = DeclID::Var;
            if (lexer_.this_token() == TOKEN::TK_SEMICOLON) {
                break;
            } else {
                expect(TOKEN::TK_COMMA, "expect ';' at the end of declaration");
                continue;
            }
        }
        //! parse function
        auto fn = func();
        if (!ps.ignore_next_funcdecl) { ps.tu->insertToTail(fn); }
        ps.ignore_next_funcdecl = false;
        ps.decl_type            = DeclID::Function;
        break;
    }
    leavedecl();
}

/*!
 * decl ->
 *      [ 'const' ] type vardef { ',' vardef } ';'
 */
DeclStmt *Parser::decl() {
    auto stmt = new DeclStmt;
    enterdecl();
    ps.decl_type = DeclID::Var;
    while (!lexer_.this_token().isEOF()) {
        Diagnosis::assertTrue(
            lexer_.lookahead() != TOKEN::TK_LPAREN,
            "nested function definition is not allowed");
        Diagnosis::assertTrue(
            !ps.cur_specifs->type->asBuiltin()->isVoid(),
            "variable has incomplete type 'void'");
        stmt->insertToTail(vardef());
        if (lexer_.this_token() == TOKEN::TK_SEMICOLON) {
            break;
        } else {
            expect(TOKEN::TK_COMMA, "expect ';' at the end of declaration");
        }
    }
    leavedecl();
    return stmt;
}

/*!
 * vardef ->
 *      ident { '[' expr ']' }
 *      ident '='
 */
VarDecl *Parser::vardef() {
    Expr       *init      = NoInitExpr::get();
    const char *name      = lookupStringLiteral(lexer_.this_token().detail);
    ArrayType  *arrayType = nullptr;
    VarDecl    *decl      = nullptr;
    auto        spec      = ps.cur_specifs;
    expect(TOKEN::TK_IDENT, "expect identifier");
    //! aggregate as array type if possible
    if (lexer_.this_token() == TOKEN::TK_LBRACKET) {
        assert(ps.cur_specifs->type->tryIntoBuiltin());
        arrayType = ArrayType::create(ps.cur_specifs->type);
        while (lexer_.this_token() == TOKEN::TK_LBRACKET) {
            lexer_.next();
            arrayType->insertToTail(binexpr());
            expect(TOKEN::TK_RBRACKET, "expect ']'");
        }
        spec       = spec->clone();
        spec->type = arrayType;
    }
    //! handle initialize
    if (lexer_.this_token() == TOKEN::TK_ASS) {
        lexer_.next();
        init = lexer_.this_token() == TOKEN::TK_LBRACE
                 ? ASTExprSimplifier::regulateInitListForArray(
                     arrayType, initlist())
                 : binexpr();
    }
    //! initialize global array with zeros explicitly
    if (spec->type->isArrayLike() && ps.cur_depth == 0) {
        init = InitListExpr::create();
    }
    //! create var decl and update symbol table
    decl = VarDecl::create(name, spec, init);
    addSymbol(decl);
    return decl;
}

/*!
 * init-list ->
 *      '{' '}' |
 *      '{' (expr | init-list) { ',' (expr | init-list) } [ ',' ] '}'
 */
InitListExpr *Parser::initlist() {
    assert(lexer_.this_token() == TOKEN::TK_LBRACE);
    lexer_.next();
    auto list    = InitListExpr::create();
    bool hasMore = true;
    while (lexer_.this_token() != TOKEN::TK_RBRACE) {
        list->insertToTail(
            lexer_.this_token() == TOKEN::TK_LBRACE ? initlist() : binexpr());
        hasMore = false;
        //! this allows trailing-comma as well
        if (lexer_.this_token() == TOKEN::TK_COMMA) {
            hasMore = true;
            lexer_.next();
        }
    }
    lexer_.next();
    return list;
}

FunctionDecl *Parser::func() {
    auto fn = enterfunc();
    if (lexer_.this_token() == TOKEN::TK_SEMICOLON) {
        //! declare without defination
        lexer_.next();
    } else {
        fn->body = block();
    }
    leavefunc();
    return fn;
}

ParamVarDeclList Parser::funcargs() {
    assert(lexer_.this_token() == TOKEN::TK_LPAREN);
    lexer_.next();
    auto params = new ParamVarDeclList;
    auto spec   = DeclSpecifier::create();
    while (lexer_.this_token() != TOKEN::TK_RPAREN) {
        const char *name = nullptr;
        bool        skip = false;
        //! get leading type declarator
        switch (lexer_.this_token().id) {
            case TOKEN::TK_VOID: {
                if (lexer_.lookahead() == TOKEN::TK_RPAREN) {
                    lexer_.next();
                    skip = true;
                    break;
                }
                Diagnosis::assertAlwaysFalse(
                    "argument can not have 'void' type");
            } break;
            case TOKEN::TK_INT: {
                spec->type = BuiltinType::getIntType();
                lexer_.next();
            } break;
            case TOKEN::TK_FLOAT: {
                spec->type = BuiltinType::getFloatType();
                lexer_.next();
            } break;
            default: {
                Diagnosis::assertAlwaysFalse("unknown type name");
            } break;
        }
        //! collect name of parameter var
        //! NOTE: parameter var may be anonymous
        if (lexer_.this_token() == TOKEN::TK_IDENT) {
            name = lookupStringLiteral(lexer_.this_token().detail);
            lexer_.next();
        }
        //! aggregate as incomplete array type if possible
        if (lexer_.this_token() == TOKEN::TK_LBRACKET) {
            lexer_.next();
            if (lexer_.this_token() != TOKEN::TK_RBRACKET) {
                auto decayedLength = commaexpr();
                Diagnosis::expectAlwaysFalse("type decays to incomplete array");
            }
            expect(TOKEN::TK_RBRACKET, "expect ']'");
            auto type = IncompleteArrayType::create(spec->type->asBuiltin());
            while (lexer_.this_token() == TOKEN::TK_LBRACKET) {
                lexer_.next();
                type->insertToTail(commaexpr());
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            }
            Diagnosis::assertWellFormedArrayType(type);
            spec->type = type;
        }
        if (!skip) {
            //! collect new parameter
            auto param = name != nullptr
                           ? ParamVarDecl::create(name, spec->clone())
                           : ParamVarDecl::create(spec->clone());
            //! NOTE: scope of param var will be set later (in enterfunc)
            params->insertToTail(param);
            //! check terminator of parameters
            bool hasMore = lexer_.this_token() == TOKEN::TK_COMMA;
            if (hasMore) { lexer_.next(); }
            Diagnosis::assertTrue(
                !hasMore || hasMore && lexer_.this_token() != TOKEN::TK_RPAREN,
                "expect parameter declarator");
            Diagnosis::assertTrue(
                hasMore || !hasMore && lexer_.this_token() == TOKEN::TK_RPAREN,
                "expect ')");
        }
        //! avoid endless loop
        if (lexer_.this_token() == TOKEN::TK_EOF) { break; }
    }
    expect(TOKEN::TK_RPAREN, "expect ')'");
    ps.cur_params = params;
    return *ps.cur_params;
}

/*!
 * statement ->
 *      ';' |
 *      'if' '(' cond-expr ')' statement [ 'else' statement ] |
 *      'while' '(' cond-expr ')' statement |
 *      'break' ';' |
 *      'continue' ';' |
 *      'return' [ expr ] ';' |
 *      block |
 *      expr ';'
 */
Stmt *Parser::statement() {
    Stmt *stmt = nullptr;
    switch (lexer_.this_token().id) {
        case TOKEN::TK_SEMICOLON: {
            lexer_.next();
            stmt = NullStmt::get();
        } break;
        case TOKEN::TK_IF: {
            stmt = ifstat();
        } break;
        case TOKEN::TK_WHILE: {
            stmt = whilestat();
        } break;
        case TOKEN::TK_BREAK: {
            stmt = breakstat();
        } break;
        case TOKEN::TK_CONTINUE: {
            stmt = continuestat();
        } break;
        case TOKEN::TK_RETURN: {
            stmt = returnstat();
        } break;
        case TOKEN::TK_LBRACE: {
            stmt = block();
        } break;
        case TOKEN::TK_CONST:
        case TOKEN::TK_INLINE:
        case TOKEN::TK_STATIC:
        case TOKEN::TK_EXTERN:
        case TOKEN::TK_VOID:
        case TOKEN::TK_INT:
        case TOKEN::TK_FLOAT: {
            stmt = decl();
        } break;
        default: {
            stmt = ExprStmt::from(commaexpr());
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        } break;
    }
    return stmt;
}

IfStmt *Parser::ifstat() {
    assert(lexer_.this_token() == TOKEN::TK_IF);
    auto stmt = new IfStmt;
    lexer_.next();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'if'");
    stmt->condition = commaexpr();
    Diagnosis::assertConditionalExpression(
        stmt->condition->asExprStmt()->unwrap());
    expect(TOKEN::TK_RPAREN, "expect ')'");
    stmt->branchIf = statement();
    if (lexer_.this_token() == TOKEN::TK_ELSE) {
        lexer_.next();
        stmt->branchElse = statement();
    } else {
        stmt->branchElse = NullStmt::get();
    }
    return stmt;
}

WhileStmt *Parser::whilestat() {
    assert(lexer_.this_token() == TOKEN::TK_WHILE);
    auto stmt = new WhileStmt;
    lexer_.next();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'while'");
    stmt->condition = commaexpr();
    Diagnosis::assertConditionalExpression(
        stmt->condition->asExprStmt()->unwrap());
    expect(TOKEN::TK_RPAREN, "expect ')'");
    auto upper_loop = ps.cur_loop;
    ps.cur_loop     = stmt;
    stmt->loopBody  = statement();
    ps.cur_loop     = upper_loop;
    return stmt;
}

BreakStmt *Parser::breakstat() {
    assert(lexer_.this_token() == TOKEN::TK_BREAK);
    lexer_.next();
    auto stmt    = new BreakStmt;
    stmt->parent = ps.cur_loop;
    Diagnosis::assertWellFormedBreakStatement(stmt);
    expect(TOKEN::TK_SEMICOLON, "expect ';' after break statement");
    return stmt;
}

ContinueStmt *Parser::continuestat() {
    assert(lexer_.this_token() == TOKEN::TK_CONTINUE);
    lexer_.next();
    auto stmt    = new ContinueStmt;
    stmt->parent = ps.cur_loop;
    Diagnosis::assertWellFormedContinueStatement(stmt);
    expect(TOKEN::TK_SEMICOLON, "expect ';' after continue statement");
    return stmt;
}

ReturnStmt *Parser::returnstat() {
    assert(lexer_.this_token() == TOKEN::TK_RETURN);
    assert(ps.cur_func != nullptr);
    auto stmt         = new ReturnStmt;
    stmt->returnValue = lexer_.next() != TOKEN::TK_SEMICOLON
                          ? ExprStmt::from(commaexpr())->decay()
                          : NullStmt::get()->decay();
    Diagnosis::assertWellFormedReturnStatement(
        stmt, ps.cur_func->type()->asFunctionProto());
    expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
    return stmt;
}

CompoundStmt *Parser::block() {
    enterblock();
    //! add function params to symbol table
    if (ps.cur_params != nullptr) {
        auto &symbols = symbolTable.back();
        for (auto &param : *ps.cur_params) {
            symbols->insert_or_assign(param->name, param);
        }
        ps.cur_params = nullptr;
    }
    assert(lexer_.this_token() == TOKEN::TK_LBRACE);
    lexer_.next();
    auto stmt = new CompoundStmt;
    while (lexer_.this_token() != TOKEN::TK_RBRACE) {
        stmt->insertToTail(statement());
    }
    lexer_.next();
    leaveblock();
    return stmt;
}

/*!
 * expr-list ->
 *      expr { ',' expr-list }
 */
ExprList *Parser::exprlist() {
    auto args = new ExprList;
    args->insertToTail(binexpr());
    while (lexer_.this_token() == TOKEN::TK_COMMA) {
        lexer_.next();
        args->insertToTail(binexpr());
    }
    return args;
}

/*!
 * primary-expr ->
 *      ident |
 *      integer-constant |
 *      floating-constant |
 *      string-literal |
 *      '(' expr ')'
 */
Expr *Parser::primaryexpr() {
    Expr *expr = nullptr;
    switch (lexer_.this_token().id) {
        case TOKEN::TK_IDENT: {
            auto ident = findSymbol(
                lexer_.this_token().detail,
                lexer_.lookahead() == TOKEN::TK_LPAREN ? DeclID::Function
                                                       : DeclID::Var);
            Diagnosis::assertTrue(
                ident != nullptr, "use of undeclared identifier");
            auto ref = new DeclRefExpr;
            ref->setSource(ident);
            if (auto token = lexer_.lookahead(); token == TOKEN::TK_LBRACKET) {
                //! is sequential value
                Diagnosis::assertSubscriptableValue(ref);
            } else if (token == TOKEN::TK_LPAREN) {
                //! is function
                Diagnosis::assertCallable(ref);
            }
            expr = ref;
            lexer_.next();
        } break;
        case TOKEN::TK_INTVAL: {
            expr = ConstantExpr::createI32(
                atoi(lexer_.this_token().detail.data()));
            lexer_.next();
        } break;
        case TOKEN::TK_FLTVAL: {
            expr = ConstantExpr::createF32(
                atof(lexer_.this_token().detail.data()));
            lexer_.next();
        } break;
        case TOKEN::TK_STRING: {
            //! TODO: #featrue(string)
            Diagnosis::assertAlwaysFalse("string literal is not supported yet");
        } break;
        case TOKEN::TK_LPAREN: {
            lexer_.next();
            expr = ParenExpr::create(commaexpr());
            expect(TOKEN::TK_RPAREN, "expect ')' after expression");
        } break;
        default: {
            Diagnosis::assertAlwaysFalse("unexpected expression");
        } break;
    }
    return ASTExprSimplifier::trySimplify(expr);
}

/*!
 * postfix-expr ->
 *      primary-expr |
 *      postfix-expr '[' expr ']' |
 *      postfix-expr '(' [ expr-list ] ')'
 */
Expr *Parser::postfixexpr() {
    auto expr = primaryexpr();
    while (true) {
        if (lexer_.this_token() == TOKEN::TK_LBRACKET) {
            //! subscript sequential value
            while (lexer_.this_token() == TOKEN::TK_LBRACKET) {
                lexer_.next();
                Diagnosis::assertSubscriptableValue(expr);
                expr = SubscriptExpr::create(expr, commaexpr());
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            }
            continue;
        }
        if (lexer_.this_token() == TOKEN::TK_LPAREN) {
            //! function call
            Diagnosis::assertCallable(expr);
            lexer_.next();
            expr = lexer_.this_token() != TOKEN::TK_RPAREN
                     ? CallExpr::create(expr, *exprlist())
                     : CallExpr::create(expr);
            expect(TOKEN::TK_RPAREN, "expect ')'");
            Diagnosis::assertWellFormedFunctionCall(
                expr->asCall()->callable->valueType->asFunctionProto(),
                &expr->asCall()->argList);
            continue;
        }
        break;
    }
    return ASTExprSimplifier::trySimplify(expr);
}

/*!
 * unary-expr ->
 *      postfix-expr |
 *      '+' unary-expr |
 *      '-' unary-expr |
 *      '~' unary-expr |
 *      '!' unary-expr
 */
Expr *Parser::unaryexpr() {
    Expr *expr    = nullptr;
    auto  op      = UnaryOperator::Paren;
    auto  isUnary = true;
    switch (lexer_.this_token().id) {
        case TOKEN::TK_ADD: {
            op = UnaryOperator::Pos;
        } break;
        case TOKEN::TK_SUB: {
            op = UnaryOperator::Neg;
        } break;
        case TOKEN::TK_INV: {
            op = UnaryOperator::Inv;
        } break;
        case TOKEN::TK_NOT: {
            op = UnaryOperator::Not;
        } break;
        default: {
            isUnary = false;
        } break;
    }
    if (isUnary) {
        lexer_.next();
        expr = new UnaryExpr(op, unaryexpr());
    } else {
        expr = postfixexpr();
    }
    return ASTExprSimplifier::trySimplify(expr);
}

inline bool isTerminateTokenForStmtOrExpr(const Token &token) {
    return token.isSemicolon() || token.isComma() || token.isRightBracket();
}

Expr *Parser::binexpr(int priority) {
    Expr *left  = unaryexpr();
    Expr *right = nullptr;
    if (!isTerminateTokenForStmtOrExpr(lexer_.this_token())) {
        auto op     = binastop(lexer_.this_token().id);
        auto opdesc = lookupOperatorPriority(op);
        Diagnosis::assertNoAssignToConstQualifiedValue(left, op);
        while (opdesc.priority < priority
               || (opdesc.priority == priority && !opdesc.assoc)) {
            lexer_.next();
            right = binexpr(opdesc.priority);
            left  = BinaryExpr::create(op, left, right);
            if (isTerminateTokenForStmtOrExpr(lexer_.this_token())) { break; }
            op     = binastop(lexer_.this_token().id);
            opdesc = lookupOperatorPriority(op);
            //! TODO: check assign type
        }
    }
    return ASTExprSimplifier::trySimplify(left);
}

/*!
 * expr ->
 *      assign-expr |
 *      expr ',' assign-expr
 */
Expr *Parser::commaexpr() {
    auto expr = binexpr();
    Diagnosis::assertWellFormedCommaExpression(expr);
    if (lexer_.this_token() == TOKEN::TK_COMMA) {
        auto first = expr;
        expr       = new CommaExpr(*exprlist());
        expr->asComma()->insertToHead(first);
    }
    return ASTExprSimplifier::trySimplify(expr);
}

} // namespace slime
