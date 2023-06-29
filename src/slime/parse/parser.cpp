#include "parser.h"

#include <map>
#include <array>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string_view>

namespace slime {

struct OperatorPriority {
    constexpr OperatorPriority(int priority, bool assoc)
        : priority{priority}
        , assoc{assoc} {}

    //! 优先级
    int priority;

    //! 结核性
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

    // Comma
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

void Parser::next() {
    do {
        ls.next();
    } while (ls.token.id == TOKEN::TK_COMMENT
             || ls.token.id == TOKEN::TK_MLCOMMENT);
}

bool Parser::expect(TOKEN token, const char *msg) {
    if (ls.token.id == token) {
        ls.next();
        return true;
    }
    //! TODO: prettify display message
    if (msg != nullptr) { fprintf(stderr, "%s", msg); }
    //! TODO: raise an error
    return false;
}

const char *Parser::lookupStringLiteral(std::string_view s) {
    const char *result = *sharedStringSet.find(s.data());
    if (result == nullptr) {
        result       = strdup(s.data());
        auto [_, ok] = sharedStringSet.insert(s.data());
        assert(ok);
    }
    return result;
}

TranslationUnit *Parser::parse() {
    ps.tu = new TranslationUnit();
    symbolTable.insertToHead(new SymbolTable);
    while (ls.token.id != TOKEN::TK_EOF) global_decl();
    return ps.tu;
}

//! 初始化声明环境
void Parser::enterdecl() {
    ps.cur_specifs = DeclSpecifier::create();
    if (ls.token.id == TOKEN::TK_CONST) {
        ps.cur_specifs->addSpecifier(NamedDeclSpecifier::Const);
        next();
    }

    switch (ls.token.id) {
        case TOKEN::TK_VOID: {
            ps.cur_specifs->type = BuiltinType::getVoidType();
            next();
        }
        case TOKEN::TK_INT: {
            ps.cur_specifs->type = BuiltinType::getIntType();
            next();
        } break;
        case TOKEN::TK_FLOAT: {
            ps.cur_specifs->type = BuiltinType::getFloatType();
            next();
        } break;
        default: {
            fprintf(
                stderr,
                "Unknown declare type:%s in global_decl()!\n",
                ls.token.detail.data());
            exit(-1);
        } break;
    }
    //! TODO: 初始化操作
}

//! 结束声明并清理声明环境
void Parser::leavedecl(DeclID tag) {
    if (tag == DeclID::Var) {
        assert(
            ls.token.id == TOKEN::TK_SEMICOLON
            || ls.token.id == TOKEN::TK_COMMA);
        next();
    }
    //! TODO: 清理操作
    ps.cur_specifs = NULL;
}

// 标记函数类型，处理函数参数列表
FunctionDecl *Parser::enterfunc() {
    FunctionDecl *ret;
    const char   *funcname = ls.token.detail.data();

    assert(ps.cur_depth == 0); // 暂不支持局部函数声明

    //! 处理错误：函数名缺失
    if (ls.token.id != TOKEN::TK_IDENT) {
        fprintf(stderr, "Missing function name in enterfunc()!\n");
        exit(-1);
    }
    //! 检查并处理函数名
    std::map<std::string_view, NamedDecl *> *gsyms =
        symbolTable.head()->value();
    if (gsyms->find(ls.token.detail.data()) != gsyms->end()) {
        fprintf(
            stderr,
            "Duplicate defined symbol:%s in enterfunc()!\n",
            ls.token.detail.data());
    }
    next();
    //! 处理错误：丢失参数列表
    if (ls.token.id != TOKEN::TK_LPAREN) {
        fprintf(stderr, "Missing argument list of function %s!\n", funcname);
    }

    ParamVarDeclList funcparams  = funcargs();
    bool             shouldClear = false;
    bool             err         = false;

    for (auto param : funcparams) {
        auto name         = param->name;
        auto is_type_void = param->type()->asBuiltin()->isVoid();
        if (is_type_void && name.empty()) {
            if (funcparams.size() == 1) {
                shouldClear = true;
                break;
            } else {
                fprintf(stderr, "Void type parameter is invalid.\n");
                exit(-1);
            }
        }
    }
    //! NOTE: function body will be added later.
    if (shouldClear) { funcparams.head()->removeFromList(); }
    //! FIXME: SIGSEV here
    ret = FunctionDecl::create(
        funcname, ps.cur_specifs->clone(), funcparams, NULL);
    gsyms->insert(std::pair<std::string_view, NamedDecl*>(lookupStringLiteral(funcname), ret));
    return ret;
}

//! 结束函数定义
void Parser::leavefunc() {
    ps.cur_func = NULL;
    //! TODO: 检查并处理函数体
}

//! 处理嵌套层数并初始化块环境
void Parser::enterblock() {
    ps.cur_depth++;
    symbolTable.insertToTail(new SymbolTable);
}

//! 清理块环境
void Parser::leaveblock() {
    symbolTable.tail()->removeFromList();
    ps.cur_depth--;
    assert(ps.cur_depth >= 0);
    assert(symbolTable.size() != 0);
}

void Parser::global_decl() {
    assert(ps.tu != NULL);
    assert(symbolTable.size() != 0);

    bool   err = false;
    DeclID tag;
    enterdecl();

    while (ls.token.id != TOKEN::TK_EOF) {
        if (ls.lookahead() == TOKEN::TK_LPAREN) {
            tag = DeclID::Function;
            ps.tu->insertToTail(func());
            break;
        } else {
            if (ps.cur_specifs->type->asBuiltin()->isVoid()) {
                fprintf(
                    stderr,
                    "Variable %s declared void.\n",
                    ls.token.detail.data());
                exit(-1);
            }
            tag = DeclID::Var;
            ps.tu->insertToTail(vardef());
            if (ls.token.id == TOKEN::TK_SEMICOLON) {
                next();
                break;
            } else if (ls.token.id == TOKEN::TK_COMMA) {
                next();
            } else {
                //! TODO: 处理报错
            }
        }
    }
}

/*!
 * decl ->
 *      [ 'const' ] type vardef { ',' vardef } ';'
 */
ast::DeclStmt *Parser::decl() {
    bool      err = false;
    DeclID    tag;
    DeclStmt *ret = new DeclStmt();
    enterdecl();

    while (ls.token.id != TOKEN::TK_EOF) {
        if (ls.lookahead() == TOKEN::TK_LPAREN) {
            fprintf(stderr, "Unsupport definition of local funtion.\n");
            exit(-1);
            break;
        } else {
            if (ps.cur_specifs->type->asBuiltin()->isVoid()) {
                fprintf(
                    stderr,
                    "Variable %s declared void.\n",
                    ls.token.detail.data());
                exit(-1);
            }
            tag = DeclID::Var;
            ret->insertToTail(vardef());
            if (ls.token.id == TOKEN::TK_SEMICOLON) {
                break;
            } else if (ls.token.id == TOKEN::TK_COMMA) {
                next();
            } else {
                //! TODO: 处理报错
            }
        }
    }

    leavedecl(tag);
    return ret;
}

/*!
 * vardef ->
 *      ident { '[' expr ']' }
 *      ident '='
 */
VarDecl *Parser::vardef() {
    if (ls.token.id != TOKEN::TK_IDENT) {
        //! TODO: 处理错误
        fprintf(stderr, "No TK_IDENT found in vardef()!\n");
        exit(-1);
        return NULL;
    }

    Expr       *initexpr = new NoInitExpr();
    const char *varname  = lookupStringLiteral(ls.token.detail.data());
    ArrayType  *arrType  = NULL;
    VarDecl    *ret      = NULL;
    next();
    // if (ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON) {
    //     return NULL;
    // }

    //! TODO: 支持数组
    if (ls.token.id == TOKEN::TK_LBRACKET) {
        arrType = ArrayType::create(
            BuiltinType::get(ps.cur_specifs->type->asBuiltin()->type));
        while (ls.token.id == TOKEN::TK_LBRACKET) { //<! 数组长度声明
            next();
            //! NOTE: 目前不支持数组长度推断
            arrType->insertToTail(expr());
            if (ls.token.id == TOKEN::TK_RBRACKET) {
                next();
            } else {
                fprintf(stderr, "Missing ']' when defining an array.\n");
                exit(-1);
            }
        }
    }

    if (ls.token.id == TOKEN::TK_ASS) {
        next();
        //! TODO: 支持数组
        if (ls.token.id == TOKEN::TK_LBRACE) {
            initexpr = initlist();
        } else {
            initexpr = expr();
        }
        //! TODO: 获取并处理初始化赋值
    }
    if (!(ls.token.id == TOKEN::TK_COMMA
          || ls.token.id == TOKEN::TK_SEMICOLON)) {
        fprintf(stderr, "Missing ';' in vardef()!\n");
        exit(-1);
    }

    if (arrType) {
        auto e              = ps.cur_specifs->type;
        ps.cur_specifs->type = arrType;
        ret = VarDecl::create(varname, ps.cur_specifs->clone(), initexpr);
        ps.cur_specifs->type = e;
    } else
        ret = VarDecl::create(varname, ps.cur_specifs->clone(), initexpr);

    if (ps.cur_depth) {
        assert(ps.cur_func != NULL);
        assert(ps.cur_depth == symbolTable.size() - 1);
        ret->scope.scope = lookupStringLiteral(ps.cur_func->name);
        ret->scope.depth = ps.cur_depth;
        std::map<std::string_view, NamedDecl *> *lsym =
            symbolTable.tail()->value();
        lsym->insert(std::pair<std::string_view, NamedDecl *>(varname, ret));
    } else {
        std::map<std::string_view, NamedDecl *> *gsym =
            symbolTable.head()->value();
        gsym->insert(std::pair<std::string_view, NamedDecl *>(varname, ret));
    }
    return ret;
}

/*!
 * init-list ->
 *      '{' '}' |
 *      '{' (expr | init-list) { ',' (expr | init-list) } [ ',' ] '}'
 */
InitListExpr *Parser::initlist() {
    //! NOTE: 初始化列表必须有类型约束，由 ParseState 提供
    assert(ls.token.id == TOKEN::TK_LBRACE);
    InitListExpr *ret = new InitListExpr();
    next();
    bool has_more = true; //<! 是否允许存在下一个值
    while (ls.token.id != TOKEN::TK_RBRACE) {
        //! TODO: 处理可能出现的错误
        //! TODO: 处理初始化的值过多的情况
        if (ls.token.id == TOKEN::TK_LBRACE) {
            ret->insertToTail(initlist());
        } else {
            ret->insertToTail(expr());
        }
        has_more = false;
        if (ls.token.id == TOKEN::TK_COMMA) {
            //! NOTE: 允许 trailing-comma
            has_more = true;
            next();
        }
    }
    next();
    return ret;
    //! TODO: 处理并存储初始化列表的值
}

FunctionDecl *Parser::func() {
    FunctionDecl *ret      = enterfunc();
    ps.cur_func            = ret;
    CompoundStmt *funcbody = block();
    //! NOTE: 暂时不允许只声明不定义
    ret->body = funcbody;
    leavefunc();
    return ret;
}

ParamVarDeclList Parser::funcargs() {
    assert(ls.token.id == TOKEN::TK_LPAREN);
    next();
    ParamVarDeclList params;
    DeclSpecifier   *specif = DeclSpecifier::create();
    while (ls.token.id != TOKEN::TK_RPAREN) {
        //! FIXME: 可能死循环
        const char *paramname = NULL;
        switch (ls.token.id) {
            case TOKEN::TK_VOID: {
                fprintf(
                    stderr, "Void type is invalid in function parameters!\n");
                exit(-1);
            }
            case TOKEN::TK_INT: {
                specif->type = BuiltinType::getIntType();
                next();
            } break;
            case TOKEN::TK_FLOAT: {
                specif->type = BuiltinType::getFloatType();
                next();
            } break;
            default: {
                fprintf(
                    stderr,
                    "Unknown parameter type: %s",
                    ls.token.detail.data());
                exit(-1);
            } break;
        }
        if (ls.token.id != TOKEN::TK_IDENT) {
            fprintf(
                stderr, "Warning: Missing parameter's name in funcargs()!\n");
        }

        if (ls.token.id == TOKEN::TK_LBRACKET) {
            //! 处理数组参数类型
            specif->type = IncompleteArrayType::create(
                BuiltinType::get(specif->type->asBuiltin()->type));
            next();
            if (ls.token.id != TOKEN::TK_RBRACKET) {
                //! 处理错误：数组作为参数第一个下标必须为空
                fprintf(
                    stderr,
                    "The first index of array must be empty as function "
                    "parameter.\n");
                exit(-1);
            }
            specif->type->asArray()->insertToTail(NoInitExpr::get());
            next();
            while (ls.token.id == TOKEN::TK_LBRACKET) {
                specif->type->asArray()->insertToTail(expr());
                //! NOTE: 处理并存储长度值
                if (ls.token.id != TOKEN::TK_RBRACKET) {
                    //! 处理错误：数组长度声明括号未闭合
                    fprintf(stderr, "Missing ']' in array parameter.\n");
                    exit(-1);
                }
                next();
            }
        }

        if (ls.token.id == TOKEN::TK_IDENT)
            paramname = lookupStringLiteral(ls.token.detail.data());
        auto new_param = ParamVarDecl::create(paramname, specif->clone());
        new_param->scope.depth = 1;
        new_param->scope.scope = paramname;
        params.insertToTail(new_param);
        next();

        //! 完成参数类型并写入函数原型
        if (ls.token.id == TOKEN::TK_COMMA) {
            if (ls.lookahead() == TOKEN::TK_RPAREN) {
                fprintf(stderr, "Unexpected comma in funcargs()!\n");
                exit(-1);
            }

        } else if (ls.token.id != TOKEN::TK_RPAREN) {
            fprintf(
                stderr,
                "Invalid terminator in function parameters' definition!\n");
            exit(-1);
        }
    }
    next();
    ps.cur_params = &params;
    return params;
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
    Stmt *ret;
    switch (ls.token.id) {
        case TOKEN::TK_SEMICOLON: {
            next();
        } break;
        case TOKEN::TK_IF: {
            ret = ifstat();
        } break;
        case TOKEN::TK_WHILE: {
            ret = whilestat();
        } break;
        case TOKEN::TK_BREAK: {
            ret = breakstat();
        } break;
        case TOKEN::TK_CONTINUE: {
            ret = continuestat();
        } break;
        case TOKEN::TK_RETURN: {
            ret = returnstat();
        } break;
        case TOKEN::TK_LBRACE: {
            ret = block();
        } break;
        case TOKEN::TK_CONST:
        case TOKEN::TK_INT:
        case TOKEN::TK_FLOAT:
            ret = decl();
            break;
        default: {
            ret = ExprStmt::from(expr());
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        } break;
    }
    return ret;
}

IfStmt *Parser::ifstat() {
    assert(ls.token.id == TOKEN::TK_IF);
    IfStmt *ret = new IfStmt();
    next();
    expect(TOKEN::TK_LPAREN, "expect '(' after 'if'");
    ret->condition = expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_RPAREN, "expect ')'");

    ret->branchIf = statement();
    if (ls.token.id == TOKEN::TK_ELSE) {
        next();
        ret->branchElse = statement();
    } else
        ret->branchElse = new NullStmt();
    return ret;
}

WhileStmt *Parser::whilestat() {
    assert(ls.token.id == TOKEN::TK_WHILE);
    WhileStmt *ret = new WhileStmt();
    next();
    //! TODO: 提升嵌套层次
    expect(TOKEN::TK_WHILE, "expect '(' after 'while'");
    ret->condition = expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_WHILE, "expect ')'");
    WhileStmt *upper_loop = ps.cur_loop;
    ps.cur_loop           = ret;
    ret->loopBody         = statement();
    ps.cur_loop           = upper_loop;
    return ret;
}

BreakStmt *Parser::breakstat() {
    assert(ls.token.id == TOKEN::TK_BREAK);
    next();
    if (!ps.cur_loop) {
        fprintf(stderr, "Invalid break statement out of a loop\n");
        exit(-1);
    }
    expect(TOKEN::TK_SEMICOLON, "expect ';' after break statement");
    return new BreakStmt();
}

ContinueStmt *Parser::continuestat() {
    assert(ls.token.id == TOKEN::TK_CONTINUE);
    next();
    //! TODO: 外层环境检查
    if (!ps.cur_loop) {
        fprintf(stderr, "Invalid break statement out of a loop\n");
        exit(-1);
    }
    expect(TOKEN::TK_SEMICOLON, "expect ';' after continue statement");
    return new ContinueStmt();
}

ReturnStmt *Parser::returnstat() {
    assert(ls.token.id == TOKEN::TK_RETURN);
    assert(ps.cur_func != NULL);
    next();
    ReturnStmt *ret = new ReturnStmt();
    if (ls.token.id != TOKEN::TK_SEMICOLON) {
        ret->returnValue = ExprStmt::from(expr());
    } else
        ret->returnValue = new NullStmt();

    //! TODO: 返回值检验
    auto builtin = ret->typeOfReturnValue()->tryIntoBuiltin();
    if (builtin == nullptr) {
        fprintf(stderr, "Invalid return type.");
        exit(-1);
    }
    if (ret->typeOfReturnValue()->asBuiltin()->isVoid()
        && !ps.cur_func->type()->asBuiltin()->isVoid()) {
        fprintf(
            stderr,
            "error: Missing return value in non-void function %s!\n",
            ps.cur_func->name.data());
        exit(-1);
    }
    expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
    return ret;
}

CompoundStmt *Parser::block() {
    enterblock();
    //! add function params to symboltable

    if (ps.cur_params != NULL) {
        auto lsyms = symbolTable.tail()->value();
        for (auto param : *ps.cur_params) {
            lsyms->insert(
                std::pair<std::string_view, NamedDecl *>(param->name, param));
        }
        ps.cur_params = NULL;
    }
    assert(ls.token.id == TOKEN::TK_LBRACE);
    next();
    CompoundStmt *ret = new CompoundStmt();
    while (ls.token.id != TOKEN::TK_RBRACE) { ret->insertToTail(statement()); }
    next();
    leaveblock();
    return ret;
}

NamedDecl *Parser::findSymbol(std::string_view name, DeclID declID) {
    if (declID == DeclID::Var || declID == DeclID::ParamVar) {
        for (auto it = symbolTable.rbegin(); it != symbolTable.rend(); ++it) {
            auto result = (*it)->find(name);
            if (result != (*it)->end()
                && result->second->declId != DeclID::Function) {
                return result->second;
            }
        }

    } else if (declID == DeclID::Function) {
        SymbolTable *gsyms = symbolTable.head()->value();
        auto         it    = gsyms->find(name);
        if (it != gsyms->end() && it->second->declId == DeclID::Function)
            return it->second;
    }
    return NULL;
}

BinaryOperator Parser::binastop(TOKEN token) {
    switch (ls.token.id) {
        case TOKEN::TK_ASS:
            return BinaryOperator::Assign;
        case TOKEN::TK_AND:
            return BinaryOperator::And;
        case TOKEN::TK_OR:
            return BinaryOperator::Or;
        case TOKEN::TK_XOR:
            return BinaryOperator::Xor;
        case TOKEN::TK_LAND:
            return BinaryOperator::LAnd;
        case TOKEN::TK_LOR:
            return BinaryOperator::LOr;
        case TOKEN::TK_EQ:
            return BinaryOperator::EQ;
        case TOKEN::TK_NE:
            return BinaryOperator::NE;
        case TOKEN::TK_LT:
            return BinaryOperator::LT;
        case TOKEN::TK_LE:
            return BinaryOperator::LE;
        case TOKEN::TK_GT:
            return BinaryOperator::GT;
        case TOKEN::TK_GE:
            return BinaryOperator::GE;
        case TOKEN::TK_SHL:
            return BinaryOperator::Shl;
        case TOKEN::TK_SHR:
            return BinaryOperator::Shr;
        case TOKEN::TK_ADD:
            return BinaryOperator::Add;
        case TOKEN::TK_SUB:
            return BinaryOperator::Sub;
        case TOKEN::TK_MUL:
            return BinaryOperator::Mul;
        case TOKEN::TK_DIV:
            return BinaryOperator::Div;
        case TOKEN::TK_MOD:
            return BinaryOperator::Mod;
        default:
            fprintf(stderr, "Unknown binary operator:%d\n", token);
            exit(-1);
    }
}

/*!
 * expr-list ->
 *      expr { ',' expr-list }
 */
ExprList *Parser::exprlist() {
    ExprList *args = new ExprList();
    args->insertToTail(binexpr(PRIORITIES.size()));
    while (ls.token.id == TOKEN::TK_COMMA) {
        next();
        args->insertToTail(binexpr(PRIORITIES.size()));
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
    Expr *ret;

    switch (ls.token.id) {
        case TOKEN::TK_IDENT: {
            NamedDecl *ident = findSymbol(ls.token.detail.data(), DeclID::Var);
            if (!ident) {
                ident = findSymbol(ls.token.detail.data(), DeclID::Function);
                if (!ident) {
                    fprintf(
                        stderr,
                        "undefined symbol %s in primaryexpr()!\n",
                        ls.token.detail.data());
                    exit(-1);
                }
            }
            switch (ls.lookahead()) {
                //! array
                case TOKEN::TK_LBRACKET: {
                    if (ident->declId == DeclID::Function) {
                        fprintf(stderr, "Can't use function as array.\n");
                        exit(-1);
                    }
                    auto e = new DeclRefExpr;
                    e->setSource(ident);
                    ret = e;
                } break;
                case TOKEN::TK_LPAREN: {
                    if (ident->declId != DeclID::Function) {
                        fprintf(
                            stderr,
                            "%s is not a function!\n",
                            ls.token.detail.data());
                        exit(-1);
                    }
                    auto e = new DeclRefExpr;
                    e->setSource(ident);
                    ret = e;
                } break;
                default: {
                    auto e = new DeclRefExpr;
                    e->setSource(ident);
                    ret = e;
                } break;
            }
            next();
        } break;
        case TOKEN::TK_INTVAL: {
            ret = ConstantExpr::createI32(atoi(ls.token.detail.data()));
            next();
        } break;
        case TOKEN::TK_FLTVAL: {
            ret = ConstantExpr::createF32(atof(ls.token.detail.data()));
            next();
        } break;
        case TOKEN::TK_STRING: {
            //! NOTE: #featrue(string)
            fprintf(
                stderr,
                "The preceding properties will be done later! (ref. string)\n");
            next();
            exit(-1);
        } break;
        case TOKEN::TK_LPAREN: {
            next();
            ret = new ParenExpr(expr());
            expect(TOKEN::TK_RPAREN, "expect ')' after expression");
        } break;
        default: {
            fprintf(
                stderr,
                "Unknown type of primary expression:%s\n",
                ls.token.detail.data());
            exit(-1);
        } break;
    }

    return ret;
}

/*!
 * postfix-expr ->
 *      primary-expr |
 *      postfix-expr '[' expr ']' |
 *      postfix-expr '(' [ expr-list ] ')'
 */
Expr *Parser::postfixexpr() {
    Expr *ret = primaryexpr();
    bool  ok  = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_LBRACKET: { //<! array index
                if (ret->valueType->typeId != TypeID::Array) {
                    fprintf(stderr, "Invalid array name.\n");
                    exit(-1);
                }
                ret = SubscriptExpr::create(ret, expr());
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            } break;
            case TOKEN::TK_LPAREN: { //<! function call
                if (ret->valueType->typeId != TypeID::FunctionProto) {
                    fprintf(stderr, "Invalid function name.\n");
                    exit(-1);
                }
                if (ls.lookahead() != TOKEN::TK_RPAREN) {
                    next();
                    ret = CallExpr::create(ret, *exprlist());
                } else
                    ret = CallExpr::create(ret);
                expect(TOKEN::TK_RPAREN, "expect ')'");
            } break;
            default: {
                ok = true;
            } break;
        }
    }

    //! TODO: support array index, function call
    return ret;
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
    Expr *ret = postfixexpr();
    // switch (ls.token.id) {
    //     case TOKEN::TK_ADD:
    //         ret = new UnaryExpr(UnaryOperator::Pos, ret);
    //         next();
    //         break;
    //     case TOKEN::TK_SUB:
    //         ret = new UnaryExpr(UnaryOperator::Neg, ret);
    //         next();
    //         break;
    //     case TOKEN::TK_INV:
    //         ret = new UnaryExpr(UnaryOperator::Inv, ret);
    //         next();
    //         break;
    //     case TOKEN::TK_NOT:
    //         ret = new UnaryExpr(UnaryOperator::Not, ret);
    //         next();
    //         break;
    //     default: {
    //         // postfixexpr();
    //     } break;
    // }

    return ret;
}

Expr *Parser::binexpr(int priority) {
    Expr *left, *right;
    left                         = unaryexpr();
    if(ls.token.id == TOKEN::TK_SEMICOLON || ls.token.id == TOKEN::TK_RPAREN)
        return left;
    BinaryOperator   op          = binastop(ls.token.id);
    OperatorPriority curPriority = lookupOperatorPriority(op);
    while (curPriority.priority < priority
           || (curPriority.priority == priority && !curPriority.assoc)) {
        
        next();
        right = binexpr(curPriority.priority);
        left  = new BinaryExpr(
            BinaryExpr::resolveType(op, left->valueType, right->valueType),
            op,
            left,
            right);
        if(ls.token.id == TOKEN::TK_SEMICOLON)
            return left;
        op          = binastop(ls.token.id);
        curPriority = lookupOperatorPriority(op);
        //! TODO: 赋值时的类型检查
    }

    return left;
}

/*!
 * expr ->
 *      assign-expr |
 *      expr ',' assign-expr
 */
Expr *Parser::expr() {
    Expr *ret = binexpr(PRIORITIES.size());
    if (!ret) {
        fprintf(stderr, "Missing expression before comma!\n");
        exit(-1);
    }
    if (ls.token.id == TOKEN::TK_COMMA) {
        Expr *tmp = ret;
        ret       = new CommaExpr(*exprlist());
        static_cast<CommaExpr *>(ret)->insertToHead(tmp);
    }

    return ret;
}

} // namespace slime
