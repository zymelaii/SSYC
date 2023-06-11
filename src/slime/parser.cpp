#include "parser.h"

#include <assert.h>

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

//! 初始化声明环境
void Parser::enterdecl() {
    if (ls.token.id == TOKEN::TK_CONST) {
        //! TODO: 标记声明环境为 const
        next();
    }
    //! TODO: 初始化操作
}

//! 结束声明并清理声明环境
void Parser::leavedecl() {
    assert(ls.token.id == TOKEN::TK_SEMICOLON);
    next();
    //! TODO: 清理操作
}

//! 获取函数原型并初始化定义
void Parser::enterfunc() {
    switch (ls.token.id) {
        case TOKEN::TK_VOID: {
            //! TODO: 标记返回值类型
            next();
        } break;
        case TOKEN::TK_INT: {
            //! TODO: 标记返回值类型
            next();
        } break;
        case TOKEN::TK_FLOAT: {
            //! TODO: 标记返回值类型
            next();
        } break;
        default: {
            //! TODO: 处理错误：无法处理的返回值类型
        } break;
    }
    if (ls.token.id != TOKEN::TK_IDENT) {
        //! TODO: 处理错误：函数名缺失
    }
    //! TODO: 检查并处理函数名
    next();
    if (ls.token.id != TOKEN::TK_LPAREN) {
        //! TODO: 处理错误：丢失参数列表
    }
    funcargs();
    //! TODO: 解析函数原型
}

//! 结束函数定义
void Parser::leavefunc() {
    //! TODO: 检查并处理函数体
}

//! 处理嵌套层数并初始化块环境
void Parser::enterblock() {}

//! 清理块环境
void Parser::leaveblock() {}

/*!
 * decl ->
 *      [ 'const' ] type vardef { ',' vardef } ';'
 */
void Parser::decl() {
    enterdecl();
    bool err = false;
    switch (ls.token.id) {
        case TOKEN::TK_INT: {
            //! TODO: 设置声明类型
            next();
        } break;
        case TOKEN::TK_FLOAT: {
            //! TODO: 设置声明类型
            next();
        } break;
        default: {
            //! TODO: 处理报错
            err = true;
        } break;
    }
    if (!err) {
        while (true) {
            vardef();
            if (ls.token.id == TOKEN::TK_SEMICOLON) {
                break;
            } else if (ls.token.id == TOKEN::TK_COMMA) {
                next();
            } else {
                //! TODO: 处理报错
            }
        }
    }
    leavedecl();
}

/*!
 * vardef ->
 *      ident { '[' expr ']' }
 *      ident '='
 */
void Parser::vardef() {
    if (ls.token.id != TOKEN::TK_IDENT) {
        //! TODO: 处理错误
        return;
    }
    //! TODO: 检查变量同名
    //! TODO: 处理变量名
    next();
    if (ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON) {
        return;
    }
    while (ls.token.id == TOKEN::TK_LBRACKET) { //<! 数组长度声明
        next();
        //! NOTE: 目前不支持数组长度推断
        expr();
        //! TODO: 获取并处理数组长度
        if (ls.token.id == TOKEN::TK_RBRACKET) {
            next();
        } else {
            //! TODO: 处理错误：括号未闭合
        }
    }
    if (ls.token.id == TOKEN::TK_ASS) {
        next();
        if (ls.token.id == TOKEN::TK_LBRACE) {
            initlist();
        } else {
            expr();
        }
        //! TODO: 获取并处理初始化赋值
    }
    if (!(ls.token.id == TOKEN::TK_COMMA
          || ls.token.id == TOKEN::TK_SEMICOLON)) {
        //! TODO: 处理错误：丢失的变量定义终止符
    }
}

/*!
 * init-list ->
 *      '{' '}' |
 *      '{' (expr | init-list) { ',' (expr | init-list) } [ ',' ] '}'
 */
void Parser::initlist() {
    //! NOTE: 初始化列表必须有类型约束，由 ParseState 提供
    assert(ls.token.id == TOKEN::TK_LBRACE);
    next();
    bool has_more = true; //<! 是否允许存在下一个值
    while (ls.token.id != TOKEN::TK_RBRACE) {
        if (!has_more) {
            //! TODO: 处理错误
        }
        //! TODO: 处理可能出现的错误
        if (ls.token.id == TOKEN::TK_LBRACE) {
            initlist();
        } else {
            expr();
        }
        has_more = false;
        if (ls.token.id == TOKEN::TK_COMMA) {
            //! NOTE: 允许 trailing-comma
            has_more = true;
            next();
        }
    }
    next();
    //! TODO: 处理并存储初始化列表的值
}

void Parser::func() {
    enterfunc();
    //! NOTE: 暂时不允许只声明不定义
    block();
    //! TODO: 处理函数体
    leavefunc();
}

void Parser::funcargs() {
    assert(ls.token.id == TOKEN::TK_LPAREN);
    next();
    while (ls.token.id != TOKEN::TK_RPAREN) {
        //! FIXME: 可能死循环
        switch (ls.token.id) {
            case TOKEN::TK_INT: {
                //! TODO: 标记参数基本类型
                next();
            } break;
            case TOKEN::TK_FLOAT: {
                //! TODO: 标记参数基本类型
                next();
            } break;
            default: {
                //! TODO: 处理错误：未知的参数类型
            } break;
        }
        if (ls.token.id != TOKEN::TK_IDENT) {
            //! TODO: 处理错误：缺少参数名
        }
        //! TODO: 处理参数名
        next();
        if (ls.token.id == TOKEN::TK_LBRACKET) {
            //! 处理数组参数类型
            next();
            if (ls.token.id != TOKEN::TK_RBRACKET) {
                //! TODO: 处理错误：数组作为参数第一个下标必须为空
            }
            next();
            while (ls.token.id == TOKEN::TK_LBRACKET) {
                expr();
                //! NOTE: 处理并存储长度值
                if (ls.token.id != TOKEN::TK_RBRACKET) {
                    //! TODO: 处理错误：数组长度声明括号未闭合
                }
                next();
            }
        }
        //! TODO: 完成参数类型并写入函数原型
        if (ls.token.id == TOKEN::TK_COMMA) {
            if (ls.lookahead() == TOKEN::TK_RPAREN) {
                //! TODO: 处理错误：参数列表不允许有 trailing-comma
            }
            next();
        } else {
            //! TODO: 处理错误：非法的终止符
        }
    }
    next();
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
void Parser::statement() {
    switch (ls.token.id) {
        case TOKEN::TK_SEMICOLON: {
            next();
        } break;
        case TOKEN::TK_IF: {
            ifstat();
        } break;
        case TOKEN::TK_WHILE: {
            whilestat();
        } break;
        case TOKEN::TK_BREAK: {
            breakstat();
        } break;
        case TOKEN::TK_CONTINUE: {
            continuestat();
        } break;
        case TOKEN::TK_RETURN: {
            returnstat();
        } break;
        case TOKEN::TK_LBRACE: {
            block();
        } break;
        default: {
            expr();
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        } break;
    }
}

void Parser::ifstat() {
    assert(ls.token.id == TOKEN::TK_IF);
    next();
    //! TODO: 提升嵌套层次
    expect(TOKEN::TK_LPAREN, "expect '(' after 'if'");
    expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_RPAREN, "expect ')'");
    statement();
    if (ls.token.id == TOKEN::TK_ELSE) {
        next();
        statement();
    }
}

void Parser::whilestat() {
    assert(ls.token.id == TOKEN::TK_WHILE);
    next();
    //! TODO: 提升嵌套层次
    expect(TOKEN::TK_WHILE, "expect '(' after 'while'");
    expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_WHILE, "expect ')'");
    statement();
}

void Parser::breakstat() {
    assert(ls.token.id == TOKEN::TK_BREAK);
    next();
    //! TODO: 外层环境检查
    expect(TOKEN::TK_SEMICOLON, "expect ';' after break statement");
}

void Parser::continuestat() {
    assert(ls.token.id == TOKEN::TK_CONTINUE);
    next();
    //! TODO: 外层环境检查
    expect(TOKEN::TK_SEMICOLON, "expect ';' after continue statement");
}

void Parser::returnstat() {
    assert(ls.token.id == TOKEN::TK_RETURN);
    next();
    if (ls.token.id != TOKEN::TK_SEMICOLON) { expr(); }
    //! TODO: 返回值检验
    expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
}

void Parser::block() {
    enterblock();
    assert(ls.token.id == TOKEN::TK_LBRACE);
    next();
    while (ls.token.id != TOKEN::TK_RBRACE) {
        switch (ls.token.id) {
            case TOKEN::TK_INT:
            case TOKEN::TK_FLOAT:
            case TOKEN::TK_CONST: {
                decl();
            } break;
            default: {
                statement();
            } break;
        }
        //! FIXME: 有概率陷入死循环
    }
    next();
    leaveblock();
}

/*!
 * expr-list ->
 *      expr { ',' expr-list }
 */
void Parser::exprlist() {
    expr();
    while (ls.token.id == TOKEN::TK_COMMA) {
        next();
        expr();
    }
}

/*!
 * primary-expr ->
 *      ident |
 *      integer-constant |
 *      floating-constant |
 *      string-literal |
 *      '(' expr ')'
 */
void Parser::primaryexpr() {
    switch (ls.token.id) {
        case TOKEN::TK_IDENT: {
            next();
        } break;
        case TOKEN::TK_INTVAL: {
            next();
        } break;
        case TOKEN::TK_FLTVAL: {
            next();
        } break;
        case TOKEN::TK_STRING: {
            //! NOTE: #feature(string)
            next();
        } break;
        case TOKEN::TK_LPAREN: {
            next();
            expr();
            expect(TOKEN::TK_RPAREN, "expect ')' after expression");
        } break;
        default: {
            //! FIXME: 错误处理
        } break;
    }
}

/*!
 * postfix-expr ->
 *      primary-expr |
 *      postfix-expr '[' expr ']' |
 *      postfix-expr '(' [ expr-list ] ')'
 */
void Parser::postfixexpr() {
    primaryexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_LBRACKET: { //<! array index
                expr();
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            } break;
            case TOKEN::TK_LPAREN: { //<! function call
                exprlist();
                expect(TOKEN::TK_RBRACKET, "expect ')'");
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * unary-expr ->
 *      postfix-expr |
 *      '+' unary-expr |
 *      '-' unary-expr |
 *      '~' unary-expr |
 *      '!' unary-expr
 */
void Parser::unaryexpr() {
    switch (ls.token.id) {
        case TOKEN::TK_ADD:
        case TOKEN::TK_SUB:
        case TOKEN::TK_INV:
        case TOKEN::TK_NOT: {
            next();
            unaryexpr();
        } break;
        default: {
            postfixexpr();
        } break;
    }
}

/*!
 * mul-expr ->
 *      unary-expr |
 *      mul-expr '*' unary-expr |
 *      mul-expr '/' unary-expr |
 *      mul-expr '%' unary-expr
 */
void Parser::mulexpr() {
    unaryexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_MUL:
            case TOKEN::TK_DIV:
            case TOKEN::TK_MOD: {
                next();
                unaryexpr();
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * add-expr ->
 *      mul-expr |
 *      add-expr '+' mul-expr |
 *      add-expr '-' mul-expr
 */
void Parser::addexpr() {
    mulexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_ADD:
            case TOKEN::TK_SUB: {
                next();
                mulexpr();
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * shift-expr ->
 *      add-expr |
 *      shift-expr '<<' add-expr |
 *      shift-expr '>>' add-expr
 */
void Parser::shiftexpr() {
    addexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_SHL:
            case TOKEN::TK_SHR: {
                next();
                addexpr();
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * rel-expr ->
 *      shift-expr |
 *      rel-expr '<' shift-expr |
 *      rel-expr '>' shift-expr |
 *      rel-expr '<=' shift-expr |
 *      rel-expr '>=' shift-expr
 */
void Parser::relexpr() {
    shiftexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_LT:
            case TOKEN::TK_GT:
            case TOKEN::TK_LE:
            case TOKEN::TK_GE: {
                next();
                shiftexpr();
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * eq-expr ->
 *      rel-expr |
 *      eq-expr '==' rel-expr |
 *      eq-expr '!=' rel-expr
 */
void Parser::eqexpr() {
    relexpr();
    bool ok = false;
    while (!ok) {
        switch (ls.token.id) {
            case TOKEN::TK_EQ:
            case TOKEN::TK_NE:
            case TOKEN::TK_LE: {
                next();
                relexpr();
            } break;
            default: {
                ok = true;
            } break;
        }
    }
}

/*!
 * and-expr ->
 *      eq-expr |
 *      and-expr '&' eq-expr
 */
void Parser::andexpr() {
    eqexpr();
    while (ls.token.id == TOKEN::TK_AND) {
        next();
        eqexpr();
    }
}

/*!
 * xor-expr ->
 *      and-expr |
 *      xor-expr '^' and-expr
 */
void Parser::xorexpr() {
    andexpr();
    while (ls.token.id == TOKEN::TK_XOR) {
        next();
        andexpr();
    }
}

/*!
 * or-expr ->
 *      xor-expr |
 *      or-expr '|' xor-expr
 */
void Parser::orexpr() {
    xorexpr();
    while (ls.token.id == TOKEN::TK_OR) {
        next();
        xorexpr();
    }
}

/*!
 * land-expr ->
 *      or-expr |
 *      land-expr '&&' or-expr
 */
void Parser::landexpr() {
    orexpr();
    while (ls.token.id == TOKEN::TK_LAND) {
        next();
        orexpr();
    }
}

/*!
 * lor-expr ->
 *      land-expr |
 *      lor-expr '||' land-expr
 */
void Parser::lorexpr() {
    landexpr();
    while (ls.token.id == TOKEN::TK_LOR) {
        next();
        landexpr();
    }
}

/*!
 * cond-expr ->
 *      lor-expr
 */
void Parser::condexpr() {
    lorexpr();
}

/*!
 * assign-expr ->
 *      cond-expr |
 *      unary-expr '=' assign-expr
 */
void Parser::assignexpr() {
    condexpr();
    if (ls.token.id == TOKEN::TK_ASS) {
        next();
        assignexpr();
    }
}

/*!
 * expr ->
 *      assign-expr |
 *      expr ',' assign-expr
 */
void Parser::expr() {
    assignexpr();
    while (ls.token.id == TOKEN::TK_COMMA) {
        next();
        assignexpr();
    }
}
