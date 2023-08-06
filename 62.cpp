#include "63.h"
#include "64.h"
#include "61.h"

#include "96.h"
#include <stddef.h>
#include <sstream>
#include <map>
#include <array>
#include <stack>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string_view>

namespace slime {

using visitor::ASTExprSimplifier;

static BinaryOperator lookupBinaryOperator(const Token &token) {
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

bool Parser::expect(TOKEN token, const char *msg) {
    if (this->token() == token) {
        lexer_.next();
        return true;
    }
    //! TODO: prettify display message
    if (msg != nullptr) { fputs(msg, stderr); }
    //! TODO: raise an error
    return false;
}

const char *Parser::lookup(std::string_view s) {
    if (s.empty()) { return nullptr; }
    auto result = s.data();
    if (!stringSet_->count(result)) {
        result = strdup(result);
        assert(result != nullptr);
        auto [_, ok] = stringSet_->insert(result);
        assert(ok);
    }
    return result;
}

void Parser::addSymbol(NamedDecl *decl) {
    assert(state_.cur_depth + 1 == symbolTableStack_.size());
    if (decl->name.empty()) { return; }
    decl->scope.depth = state_.cur_depth;
    if (!decl->scope.isGlobal()) {
        assert(state_.cur_func != nullptr);
        decl->scope.scope = lookup(state_.cur_func->name);
    }
    symbolTableStack_.back()->insert_or_assign(lookup(decl->name), decl);
}

NamedDecl *Parser::findSymbol(std::string_view name, DeclID declId) {
    assert(symbolTableStack_.size() >= 1);
    assert(declId != DeclID::ParamVar);
    switch (declId) {
        case DeclID::Var:
        case DeclID::ParamVar: {
            auto       it  = symbolTableStack_.rbegin();
            const auto end = symbolTableStack_.rend();
            while (it != end) {
                if (auto result = (*it)->find(name); result != (*it)->end()) {
                    auto &[_, decl] = *result;
                    //! reduce param-var to normal var
                    auto type = decl->declId == DeclID::ParamVar ? DeclID::Var
                                                                 : decl->declId;
                    if (type == declId) {
                        state_.symref_set.insert(decl);
                        return decl;
                    }
                }
                ++it;
            }
        } break;
        case DeclID::Function: {
            auto &symbols = symbolTableStack_.front();
            if (auto it = symbols->find(name); it != symbols->end()) {
                auto &[_, decl] = *it;
                if (decl->tryIntoFunctionDecl()) {
                    state_.symref_set.insert(decl);
                    return decl;
                }
            }
        } break;
    }
    return nullptr;
}

TranslationUnit *Parser::parse() {
    state_.tu = TranslationUnit::create();
    addPresetSymbols();
    if (lexer_.isDiscard(token())) { lexer_.next(); }
    while (!token().isEOF()) { parseGlobalDecl(); }
    dropUnusedExternalSymbols();
    return state_.tu;
}

void Parser::parseGlobalDecl() {
    assert(state_.tu != nullptr);
    assert(symbolTableStack_.size() == 1);
    enterDecl();
    while (!token().isEOF()) {
        //! parse global variable
        if (lexer_.lookahead() != TOKEN::TK_LPAREN) {
            Diagnosis::assertTrue(
                !state_.cur_specifs->type->asBuiltin()->isVoid(),
                "variable has incomplete type 'void'");
            state_.tu->insertToTail(parseVarDef());
            state_.decl_type = DeclID::Var;
            if (token() == TOKEN::TK_SEMICOLON) {
                break;
            } else {
                expect(TOKEN::TK_COMMA, "expect ';' at the end of declaration");
                continue;
            }
        }
        //! parse function
        auto fn = parseFunction();
        if (!state_.ignore_next_funcdecl) { state_.tu->insertToTail(fn); }
        state_.ignore_next_funcdecl = false;
        state_.decl_type            = DeclID::Function;
        break;
    }
    leaveDecl();
}

DeclStmt *Parser::parseDeclStmt() {
    auto stmt = DeclStmt::create();
    enterDecl();
    state_.decl_type = DeclID::Var;
    while (!token().isEOF()) {
        Diagnosis::assertTrue(
            lexer_.lookahead() != TOKEN::TK_LPAREN,
            "nested function definition is not allowed");
        Diagnosis::assertTrue(
            !state_.cur_specifs->type->asBuiltin()->isVoid(),
            "variable has incomplete type 'void'");
        stmt->insertToTail(parseVarDef());
        if (token() == TOKEN::TK_SEMICOLON) {
            break;
        } else {
            expect(TOKEN::TK_COMMA, "expect ';' at the end of declaration");
        }
    }
    leaveDecl();
    return stmt;
}

VarDecl *Parser::parseVarDef() {
    Expr       *init      = NoInitExpr::get();
    const char *name      = lookup(token());
    ArrayType  *arrayType = nullptr;
    VarDecl    *decl      = nullptr;
    auto        spec      = state_.cur_specifs;
    expect(TOKEN::TK_IDENT, "expect identifier");
    //! aggregate as array type if possible
    if (token() == TOKEN::TK_LBRACKET) {
        assert(state_.cur_specifs->type->tryIntoBuiltin());
        arrayType = ArrayType::create(state_.cur_specifs->type);
        while (token() == TOKEN::TK_LBRACKET) {
            lexer_.next();
            arrayType->insertToTail(parseBinaryExpr());
            expect(TOKEN::TK_RBRACKET, "expect ']'");
        }
        spec       = spec->clone();
        spec->type = arrayType;
    }
    //! handle initialize
    if (token() == TOKEN::TK_ASS) {
        lexer_.next();
        init = token() == TOKEN::TK_LBRACE
                 ? ASTExprSimplifier::regulateInitListForArray(
                     arrayType, parseInitListExpr())
                 : parseBinaryExpr();
    }
    //! initialize global array with zeros explicitly
    if (!init && spec->type->isArrayLike() && state_.cur_depth == 0) {
        init = InitListExpr::create();
    }
    //! create var decl and update symbol table
    Diagnosis::assertTrue(
        symbolTableStack_.back()->count(name) == 0, "redefination of variable");
    decl = VarDecl::create(name, spec, init);
    addSymbol(decl);
    return decl;
}

FunctionDecl *Parser::parseFunction() {
    auto fn = enterFunction();
    if (token() == TOKEN::TK_SEMICOLON) {
        //! declare without defination
        lexer_.next();
    } else {
        fn->body = parseBlock();
    }
    leaveFunction();
    return fn;
}

ParamVarDeclList Parser::parseFunctionParams() {
    assert(token() == TOKEN::TK_LPAREN);
    lexer_.next();
    auto params = new ParamVarDeclList;
    auto spec   = DeclSpecifier::create();
    while (token() != TOKEN::TK_RPAREN) {
        const char *name = nullptr;
        bool        skip = false;
        //! get leading type declarator
        switch (token()) {
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
            case TOKEN::TK_CHAR: {
                spec->type = BuiltinType::getCharType();
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
        if (token() == TOKEN::TK_IDENT) {
            name = lookup(token());
            lexer_.next();
        }
        //! aggregate as incomplete array type if possible
        if (token() == TOKEN::TK_LBRACKET) {
            lexer_.next();
            if (token() != TOKEN::TK_RBRACKET) {
                auto decayedLength = parseCommaExpr();
                Diagnosis::expectAlwaysFalse("type decays to incomplete array");
            }
            expect(TOKEN::TK_RBRACKET, "expect ']'");
            auto type = IncompleteArrayType::create(spec->type->asBuiltin());
            while (token() == TOKEN::TK_LBRACKET) {
                lexer_.next();
                type->insertToTail(parseCommaExpr());
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
            //! NOTE: scope of param var will be set later (in enterFunction)
            params->insertToTail(param);
            //! check terminator of parameters
            bool hasMore = token() == TOKEN::TK_COMMA;
            if (hasMore) { lexer_.next(); }
            Diagnosis::assertTrue(
                !hasMore || hasMore && token() != TOKEN::TK_RPAREN,
                "expect parameter declarator");
            Diagnosis::assertTrue(
                hasMore || !hasMore && token() == TOKEN::TK_RPAREN,
                "expect ')");
        }
        //! avoid endless loop
        if (token() == TOKEN::TK_EOF) { break; }
    }
    expect(TOKEN::TK_RPAREN, "expect ')'");
    state_.cur_params = params;
    return *state_.cur_params;
}

Stmt *Parser::parseStmt(bool standalone) {
    Stmt *stmt = nullptr;
    switch (token()) {
        case TOKEN::TK_SEMICOLON: {
            lexer_.next();
            stmt = NullStmt::get();
        } break;
        case TOKEN::TK_IF: {
            stmt = parseIfStmt();
        } break;
        case TOKEN::TK_DO: {
            stmt = parseDoStmt();
        } break;
        case TOKEN::TK_WHILE: {
            stmt = parseWhileStmt();
        } break;
        case TOKEN::TK_FOR: {
            stmt = parseForStmt();
        } break;
        case TOKEN::TK_BREAK: {
            stmt = parseBreakStmt();
        } break;
        case TOKEN::TK_CONTINUE: {
            stmt = parseContinueStmt();
        } break;
        case TOKEN::TK_RETURN: {
            stmt = parseReturnStmt();
        } break;
        case TOKEN::TK_LBRACE: {
            stmt = parseBlock();
        } break;
        case TOKEN::TK_CONST:
        case TOKEN::TK_INLINE:
        case TOKEN::TK_STATIC:
        case TOKEN::TK_EXTERN:
        case TOKEN::TK_VOID:
        case TOKEN::TK_INT:
        case TOKEN::TK_CHAR:
        case TOKEN::TK_FLOAT: {
            if (standalone) { enterBlock(); }
            stmt = parseDeclStmt();
            if (standalone) { leaveBlock(); }
        } break;
        default: {
            stmt = ExprStmt::from(parseCommaExpr());
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        } break;
    }
    return stmt;
}

IfStmt *Parser::parseIfStmt() {
    assert(token() == TOKEN::TK_IF);
    auto stmt = IfStmt::create();
    lexer_.next();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'if'");
    stmt->condition = parseCommaExpr();
    Diagnosis::assertConditionalExpression(
        stmt->condition->asExprStmt()->unwrap());
    expect(TOKEN::TK_RPAREN, "expect ')'");
    stmt->branchIf = parseStmt(true);
    if (token() == TOKEN::TK_ELSE) {
        lexer_.next();
        stmt->branchElse = parseStmt(true);
    }
    return stmt;
}

DoStmt *Parser::parseDoStmt() {
    assert(token() == TOKEN::TK_DO);
    auto stmt = DoStmt::create();
    lexer_.next();
    auto upper_loop = state_.cur_loop;
    state_.cur_loop = stmt;
    stmt->loopBody  = parseStmt(true);
    expect(TOKEN::TK_WHILE, "expect 'while' in do statement");
    expect(TOKEN::TK_LPAREN, "expect '(' after 'while'");
    auto expr = parseCommaExpr();
    Diagnosis::assertConditionalExpression(expr);
    stmt->condition = expr;
    expect(TOKEN::TK_RPAREN, "expect ')'");
    expect(TOKEN::TK_SEMICOLON, "expect ';'");
    state_.cur_loop = upper_loop;
    return stmt;
}

WhileStmt *Parser::parseWhileStmt() {
    assert(token() == TOKEN::TK_WHILE);
    auto stmt = WhileStmt::create();
    lexer_.next();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'while'");
    stmt->condition = parseCommaExpr();
    Diagnosis::assertConditionalExpression(
        stmt->condition->asExprStmt()->unwrap());
    expect(TOKEN::TK_RPAREN, "expect ')'");
    auto upper_loop = state_.cur_loop;
    state_.cur_loop = stmt;
    stmt->loopBody  = parseStmt(true);
    state_.cur_loop = upper_loop;
    return stmt;
}

ForStmt *Parser::parseForStmt() {
    assert(token() == TOKEN::TK_FOR);
    auto stmt                = ForStmt::create();
    bool shouldRunLeaveBlock = true;
    lexer_.next();
    //! NOTE: ForStmt::init can be DeclStmt, and its depth differs from the
    //! current scope but is at the same level of the loop body
    enterBlock();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'for'");
    stmt->init = parseStmt();
    if (token() != TOKEN::TK_SEMICOLON) { stmt->condition = parseCommaExpr(); }
    expect(TOKEN::TK_SEMICOLON, "expect ';'");
    if (token() != TOKEN::TK_RPAREN) { stmt->increment = parseCommaExpr(); }
    expect(TOKEN::TK_RPAREN, "expect ')'");
    //! if follows with a CompoundStmt, do not deepen the depth
    if (token() == TOKEN::TK_LBRACE) {
        state_.not_deepen_next_block = true;
        shouldRunLeaveBlock          = false;
    }
    auto upper_loop = state_.cur_loop;
    state_.cur_loop = stmt;
    stmt->loopBody  = parseStmt();
    state_.cur_loop = upper_loop;
    if (shouldRunLeaveBlock) { leaveBlock(); }
    Diagnosis::assertWellFormedForStatement(stmt);
    return stmt;
}

BreakStmt *Parser::parseBreakStmt() {
    assert(token() == TOKEN::TK_BREAK);
    lexer_.next();
    auto stmt = BreakStmt::create(state_.cur_loop);
    Diagnosis::assertWellFormedBreakStatement(stmt);
    expect(TOKEN::TK_SEMICOLON, "expect ';' after break statement");
    return stmt;
}

ContinueStmt *Parser::parseContinueStmt() {
    assert(token() == TOKEN::TK_CONTINUE);
    lexer_.next();
    auto stmt = ContinueStmt::create(state_.cur_loop);
    Diagnosis::assertWellFormedContinueStatement(stmt);
    expect(TOKEN::TK_SEMICOLON, "expect ';' after continue statement");
    return stmt;
}

ReturnStmt *Parser::parseReturnStmt() {
    assert(token() == TOKEN::TK_RETURN);
    assert(state_.cur_func != nullptr);
    auto stmt = !lexer_.next().isSemicolon()
                  ? ReturnStmt::create(parseCommaExpr())
                  : ReturnStmt::create();
    Diagnosis::assertWellFormedReturnStatement(
        stmt, state_.cur_func->type()->asFunctionProto());
    expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
    return stmt;
}

CompoundStmt *Parser::parseBlock() {
    enterBlock();
    //! add function params to symbol table
    if (state_.cur_params != nullptr) {
        auto &symbols = symbolTableStack_.back();
        for (auto &param : *state_.cur_params) {
            symbols->insert_or_assign(param->name, param);
        }
        state_.cur_params = nullptr;
    }
    assert(token() == TOKEN::TK_LBRACE);
    lexer_.next();
    auto stmt = CompoundStmt::create();
    while (token() != TOKEN::TK_RBRACE) { stmt->insertToTail(parseStmt()); }
    lexer_.next();
    leaveBlock();
    return stmt;
}

Expr *Parser::parsePrimaryExpr() {
    Expr *expr = nullptr;
    switch (token()) {
        case TOKEN::TK_IDENT: {
            auto ident = findSymbol(
                token(),
                lexer_.lookahead() == TOKEN::TK_LPAREN ? DeclID::Function
                                                       : DeclID::Var);
            Diagnosis::assertTrue(
                ident != nullptr, "use of undeclared identifier");
            expr = DeclRefExpr::create(ident);
            if (auto token = lexer_.lookahead(); token == TOKEN::TK_LBRACKET) {
                //! is sequential value
                Diagnosis::assertSubscriptableValue(expr);
            } else if (token == TOKEN::TK_LPAREN) {
                //! is function
                Diagnosis::assertCallable(expr);
            }
            lexer_.next();
        } break;
        case TOKEN::TK_INTVAL: {
            expr = ConstantExpr::createI32(
                std::stoi(std::string{token()}, nullptr, 0));
            lexer_.next();
        } break;
        case TOKEN::TK_FLTVAL: {
            expr = ConstantExpr::createF32(atof(token()));
            lexer_.next();
        } break;
        case TOKEN::TK_STRING: {
            expr = ConstantExpr::createString(token().detail);
            lexer_.next();
        } break;
        case TOKEN::TK_LPAREN: {
            lexer_.next();
            expr = ParenExpr::create(parseCommaExpr());
            expect(TOKEN::TK_RPAREN, "expect ')' after expression");
        } break;
        default: {
            Diagnosis::assertAlwaysFalse("unexpected expression");
        } break;
    }
    return expr->intoSimplified();
}

Expr *Parser::parsePostfixExpr() {
    auto expr = parsePrimaryExpr();
    while (true) {
        if (token() == TOKEN::TK_LBRACKET) {
            //! subscript sequential value
            while (token() == TOKEN::TK_LBRACKET) {
                lexer_.next();
                Diagnosis::assertSubscriptableValue(expr);
                expr = SubscriptExpr::create(expr, parseCommaExpr());
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            }
            continue;
        }
        if (token() == TOKEN::TK_LPAREN) {
            //! function call
            Diagnosis::assertCallable(expr);
            lexer_.next();
            expr = token() != TOKEN::TK_RPAREN
                     ? CallExpr::create(expr, *parseExprList())
                     : CallExpr::create(expr);
            expect(TOKEN::TK_RPAREN, "expect ')'");
            Diagnosis::assertWellFormedFunctionCall(
                expr->asCall()->callable->valueType->asFunctionProto(),
                &expr->asCall()->argList);
            continue;
        }
        break;
    }
    return expr->intoSimplified();
}

Expr *Parser::parseUnaryExpr() {
    Expr *expr    = nullptr;
    auto  op      = UnaryOperator::Unreachable;
    auto  isUnary = true;
    switch (token()) {
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
    if (isUnary) { lexer_.next(); }
    return isUnary ? UnaryExpr::create(op, parseUnaryExpr())->intoSimplified()
                   : parsePostfixExpr();
}

Expr *foldExprList(BinaryOperator op, Expr **list, int first, int last) {
    assert(first <= last);
    if (first == last) {
        return list[first];
    } else if (first + 1 == last) {
        return BinaryExpr::create(op, list[first], list[last]);
    } else {
        int m = (first + last) / 2;
        return BinaryExpr::create(
            op,
            foldExprList(op, list, first, m - 1),
            foldExprList(op, list, m, last));
    }
}

Expr *Parser::parseBinaryExpr() {
    auto first = parseUnaryExpr();
    auto tok   = token();

    Expr *expr = nullptr;

    if (auto isUnary =
            tok.isSemicolon() || tok.isComma() || tok.isRightBracket()) {
        expr = first;
    } else {
        std::vector<Expr *>                        operands{first};
        std::vector<BinaryOperator>                ops;
        std::stack<Expr *>                         stack;
        std::stack<std::pair<int, BinaryOperator>> opStack;
        do {
            ops.emplace_back(lookupBinaryOperator(tok));
            lexer_.next();
            operands.emplace_back(parseUnaryExpr());
            tok = token();
            isUnary =
                tok.isSemicolon() || tok.isComma() || tok.isRightBracket();
        } while (!isUnary);
        stack.push(first);
        for (int i = 0; i < ops.size(); ++i) {
            const auto &op   = ops[i];
            const auto  desc = lookupOperatorPriority(op);

            //! FIXME: consider priority of left-assoc op is new ops are
            //! introduced
            if (!desc.assoc) { //<! left-assoc
                stack.push(operands[i + 1]);
                opStack.push({i, op});
                continue;
            }

            //! combine swappable ops sequence without side effects
            if (op == BinaryOperator::Add || op == BinaryOperator::Mul) {
                //! get op range longest continuous sequence
                auto seqFirst = i;
                auto seqLast  = seqFirst + 1;
                while (seqLast < ops.size()) {
                    if (ops[seqLast] != op) { break; }
                    ++seqLast;
                }
                --seqLast;
                if (seqFirst > 0) {
                    const auto &prevOp = ops[seqFirst - 1];
                    if (prevOp != BinaryOperator::Unreachable && prevOp != op) {
                        const auto prevDesc = lookupOperatorPriority(prevOp);
                        if (prevDesc.priority <= desc.priority) { ++seqFirst; }
                    }
                }
                if (seqLast + 1 < ops.size()) {
                    const auto nextDesc =
                        lookupOperatorPriority(ops[seqLast + 1]);
                    if (nextDesc.priority <= desc.priority) { --seqLast; }
                }
                if (i == seqFirst && seqFirst < seqLast) {
                    stack.top() = foldExprList(
                        op, operands.data(), seqFirst, seqLast + 1);
                    i = seqLast;
                    continue;
                }
            }

            //! default rpn solution
            while (!opStack.empty()) {
                const auto [index, lastOp] = opStack.top();
                const auto lastDesc        = lookupOperatorPriority(lastOp);
                if (lastDesc.assoc && lastDesc.priority <= desc.priority) {
                    auto rhs = stack.top();
                    stack.pop();
                    stack.top() = BinaryExpr::create(lastOp, stack.top(), rhs);
                    ops[index]  = index == 0 ? BinaryOperator::Unreachable
                                             : ops[index - 1]; //<! update ops
                    opStack.pop();
                } else {
                    break;
                }
            }
            stack.push(operands[i + 1]);
            opStack.push({i, op});
        }
        while (!opStack.empty()) {
            auto [index, op] = opStack.top();
            opStack.pop();
            auto rhs = stack.top();
            stack.pop();
            stack.top() = BinaryExpr::create(op, stack.top(), rhs);
        }
        assert(stack.size() == 1);
        expr = stack.top()->intoSimplified();
    }

    return expr;
}

Expr *Parser::parseCommaExpr() {
    auto expr = parseBinaryExpr();
    Diagnosis::assertWellFormedCommaExpression(expr);
    if (token() == TOKEN::TK_COMMA) {
        auto first = expr;
        expr       = CommaExpr::create(*parseExprList());
        expr->asComma()->insertToHead(first);
    }
    return expr->intoSimplified();
}

InitListExpr *Parser::parseInitListExpr() {
    assert(token() == TOKEN::TK_LBRACE);
    lexer_.next();
    auto list    = InitListExpr::create();
    bool hasMore = true;
    while (token() != TOKEN::TK_RBRACE) {
        list->insertToTail(
            token() == TOKEN::TK_LBRACE ? parseInitListExpr()
                                        : parseBinaryExpr());
        hasMore = false;
        //! this allows trailing-comma as well
        if (token() == TOKEN::TK_COMMA) {
            hasMore = true;
            lexer_.next();
        }
    }
    lexer_.next();
    return list;
}

ExprList *Parser::parseExprList() {
    auto args = new ExprList;
    args->insertToTail(parseBinaryExpr());
    while (token().isComma()) {
        lexer_.next();
        args->insertToTail(parseBinaryExpr());
    }
    return args;
}

void Parser::enterDecl() {
    //! get leading declarator
    auto spec = DeclSpecifier::create();
    bool done = false;
    while (!done) {
        switch (token().id) {
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
            case TOKEN::TK_CHAR: {
                spec->type = BuiltinType::getCharType();
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
    state_.cur_specifs = spec;
}

void Parser::leaveDecl() {
    //! clear decl environment
    //! NOTE: state_.cur_specifs != nullptr is not always true,
    /// if the current decl is a function then the defination
    /// will probably eat the cur_specifs
    assert(state_.decl_type != DeclID::ParamVar);
    if (state_.decl_type == DeclID::Var) {
        expect(TOKEN::TK_SEMICOLON, "expect ';'");
    }
    state_.cur_specifs = nullptr;
}

FunctionDecl *Parser::enterFunction() {
    //! parse function proto
    assert(symbolTableStack_.size() == 1);
    FunctionDecl *fn   = nullptr;
    const char   *name = lookup(token());
    expect(TOKEN::TK_IDENT, "expect unqualified-id");
    Diagnosis::assertTrue(token() == TOKEN::TK_LPAREN, "expect '('");
    auto params = std::move(parseFunctionParams());
    fn = FunctionDecl::create(name, state_.cur_specifs, params, nullptr);
    bool          haveBody = token() == TOKEN::TK_LBRACE;
    bool          skip     = false;
    FunctionDecl *rewrite  = nullptr;
    auto          symbols  = symbolTableStack_.back();
    if (auto it = symbols->find(name); it != symbols->end()) {
        auto [_, decl] = *it;
        bool duplicate = !fn->proto()->equals(decl->type());
        if (!duplicate) {
            auto func = decl->asFunctionDecl();
            if (func->body && haveBody) {
                duplicate = true;
            } else if (!haveBody) {
                state_.ignore_next_funcdecl = skip = true;
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
        auto node = state_.tu->head();
        assert(node != nullptr);
        while (!done) {
            if (auto target = node->value()->tryIntoFunctionDecl();
                target == rewrite) {
                node->removeFromList();
                done = true;
            }
            if (node == state_.tu->tail()) { break; }
            node = node->next();
        }
    }
    state_.cur_func = fn;
    if (haveBody) {
        enterBlock();
        state_.not_deepen_next_block = true;
    }
    for (auto param : *fn) { addSymbol(param); }
    return state_.cur_func;
}

void Parser::leaveFunction() {
    assert(state_.cur_func != nullptr);
    //! only allow external function to be undefined
    auto fn = state_.cur_func;
    Diagnosis::assertTrue(
        fn->body != nullptr || fn->specifier->isExtern(),
        "missing defination of local function");
    if (fn->body != nullptr) {
        fn->specifier->removeSpecifier(NamedDeclSpecifier::Extern);
    }
    //! reset current function
    state_.cur_func = nullptr;
}

void Parser::enterBlock() {
    if (!state_.not_deepen_next_block) {
        ++state_.cur_depth;
        symbolTableStack_.push_back(new SymbolTable);
    }
    state_.not_deepen_next_block = false;
}

void Parser::leaveBlock() {
    --state_.cur_depth;
    auto table = symbolTableStack_.back();
    symbolTableStack_.pop_back();
    delete table;
    assert(state_.cur_depth >= 0);
    assert(symbolTableStack_.size() >= 1);
}

void Parser::addExternalFunction(
    const char *name, Type *returnType, ParamVarDeclList &params) {
    auto spec = DeclSpecifier::create();
    spec->addSpecifier(NamedDeclSpecifier::Extern);
    spec->type = returnType;
    auto decl  = FunctionDecl::create(name, spec, params, nullptr);
    addSymbol(decl);
    state_.tu->insertToTail(decl);
}

void Parser::addPresetSymbols() {
    ParamVarDeclList params{};

    auto specInt         = DeclSpecifier::create();
    auto specChar        = DeclSpecifier::create();
    auto specFloat       = DeclSpecifier::create();
    specInt->type        = BuiltinType::getIntType();
    specChar->type       = BuiltinType::getCharType();
    specFloat->type      = BuiltinType::getFloatType();
    auto specIntArray    = specInt->clone();
    auto specCharArray   = specChar->clone();
    auto specFloatArray  = specFloat->clone();
    specIntArray->type   = IncompleteArrayType::create(specIntArray->type);
    specCharArray->type  = IncompleteArrayType::create(specCharArray->type);
    specFloatArray->type = IncompleteArrayType::create(specFloatArray->type);
    auto paramInt        = ParamVarDecl::create(specInt);
    auto paramIntArray   = ParamVarDecl::create(specIntArray);
    auto paramCharArray  = ParamVarDecl::create(specCharArray);
    auto paramFloat      = ParamVarDecl::create(specFloat);
    auto paramFloatArray = ParamVarDecl::create(specFloatArray);

    //! void starttime()
    addExternalFunction("starttime", BuiltinType::getVoidType(), params);

    //! void stoptime()
    addExternalFunction("stoptime", BuiltinType::getVoidType(), params);

    //! int getint()
    addExternalFunction("getint", BuiltinType::getIntType(), params);

    //! int getch()
    addExternalFunction("getch", BuiltinType::getIntType(), params);

    //! float getfloat()
    addExternalFunction("getfloat", BuiltinType::getFloatType(), params);

    //! void putint(int)
    auto pInt = params.insertToTail(paramInt);
    addExternalFunction("putint", BuiltinType::getVoidType(), params);

    //! void putch(int)
    addExternalFunction("putch", BuiltinType::getVoidType(), params);

    //! void _sysy_starttime(int)
    addExternalFunction("_sysy_starttime", BuiltinType::getVoidType(), params);

    //! void _sysy_stoptime(int)
    addExternalFunction("_sysy_stoptime", BuiltinType::getVoidType(), params);

    //! void putarray(int, int[])
    auto pIntArray = params.insertToTail(paramIntArray);
    addExternalFunction("putarray", BuiltinType::getVoidType(), params);

    //! void putfarray(int, float[])
    pIntArray->removeFromList();
    auto pFloatArray = params.insertToTail(paramFloatArray);
    addExternalFunction("putfarray", BuiltinType::getVoidType(), params);

    //! int getfarray(float[])
    pInt->removeFromList();
    addExternalFunction("getfarray", BuiltinType::getIntType(), params);

    //! int getarray(int[])
    pFloatArray->removeFromList();
    pIntArray->insertToTail(params);
    addExternalFunction("getarray", BuiltinType::getIntType(), params);

    //! void putfloat(float)
    pIntArray->removeFromList();
    auto pFloat = params.insertToTail(paramFloat);
    addExternalFunction("putfloat", BuiltinType::getVoidType(), params);

    //! FIXME: handle var-args
    //! void putf(char[], ...)
    pFloat->removeFromList();
    params.insertToHead(paramCharArray);
    addExternalFunction("putf", BuiltinType::getVoidType(), params);

    //! void __slime_starttime(const char*, int);
    params.insertToTail(paramInt);
    addExternalFunction(
        "__slime_starttime", BuiltinType::getVoidType(), params);

    //! void __slime_stoptime(const char*, int);
    addExternalFunction("__slime_stoptime", BuiltinType::getVoidType(), params);
}

void Parser::dropUnusedExternalSymbols() {
    auto       it  = state_.tu->node_begin();
    const auto end = state_.tu->node_end();
    while (it != end) {
        auto &node = *it++;
        auto  decl = static_cast<NamedDecl *>(node.value());
        if (state_.symref_set.count(decl) == 0 && decl->specifier->isExtern()) {
            node.removeFromList();
        }
    }
}

Expr *Parser::parseBinaryExprWithPriority(int priority) {
    auto        expr    = parseUnaryExpr();
    const auto &t       = token();
    bool        isUnary = t.isSemicolon() || t.isComma() || t.isRightBracket();
    if (!isUnary) {
        auto op     = lookupBinaryOperator(token());
        auto opdesc = lookupOperatorPriority(op);
        Diagnosis::assertNoAssignToConstQualifiedValue(expr, op);
        while (opdesc.priority < priority
               || (opdesc.priority == priority && !opdesc.assoc)) {
            lexer_.next();
            auto rhs = parseBinaryExprWithPriority(opdesc.priority);
            if (op == BinaryOperator::Assign) {
                Diagnosis::assertTrue(
                    Diagnosis::checkTypeConvertible(
                        rhs->valueType, expr->valueType),
                    "assign with incompatible type");
            }
            expr = BinaryExpr::create(op, expr, rhs);
            if (auto token = this->token(); token.isSemicolon()
                                            || token.isComma()
                                            || token.isRightBracket()) {
                break;
            } else {
                op     = lookupBinaryOperator(token);
                opdesc = lookupOperatorPriority(op);
            }
        }
        expr = expr->intoSimplified();
    }
    return expr;
}

} // namespace slime