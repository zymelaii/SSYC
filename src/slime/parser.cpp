#include "parser.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

void Parser::next()
{
    do
    {
        ls.next();
    } while (ls.token.id == TOKEN::TK_COMMENT || ls.token.id == TOKEN::TK_MLCOMMENT);
}

bool Parser::expect(TOKEN token, const char *msg)
{
    if (ls.token.id == token)
    {
        ls.next();
        return true;
    }
    //! TODO: prettify display message
    if (msg != nullptr)
    {
        fprintf(stderr, "%s", msg);
    }
    //! TODO: raise an error
    return false;
}

//! 初始化声明环境
void Parser::enterdecl()
{
    if (ls.token.id == TOKEN::TK_CONST)
    {
        //! TODO: 标记声明环境为 const
        next();
    }
    //! TODO: 初始化操作
}

//! 结束声明并清理声明环境
void Parser::leavedecl()
{
    assert(ls.token.id == TOKEN::TK_SEMICOLON);
    next();
    //! TODO: 清理操作
}

//! 获取函数原型并初始化定义
void Parser::enterfunc()
{
    switch (ls.token.id)
    {
        case TOKEN::TK_VOID:
        {
            //! TODO: 标记返回值类型
            next();
        }
        break;
        case TOKEN::TK_INT:
        {
            //! TODO: 标记返回值类型
            next();
        }
        break;
        case TOKEN::TK_FLOAT:
        {
            //! TODO: 标记返回值类型
            next();
        }
        break;
        default:
        {
            //! TODO: 处理错误：无法处理的返回值类型
        }
        break;
    }
    if (ls.token.id != TOKEN::TK_IDENT)
    {
        //! TODO: 处理错误：函数名缺失
    }
    //! TODO: 检查并处理函数名
    next();
    if (ls.token.id != TOKEN::TK_LPAREN)
    {
        //! TODO: 处理错误：丢失参数列表
    }
    funcargs();
    //! TODO: 解析函数原型
}

//! 结束函数定义
void Parser::leavefunc()
{
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
void Parser::decl()
{
    enterdecl();
    bool err = false;
    switch (ls.token.id)
    {
    case TOKEN::TK_INT:
    {
        //! TODO: 设置声明类型
        next();
    }
    break;
    case TOKEN::TK_FLOAT:
    {
        //! TODO: 设置声明类型
        next();
    }
    break;
    default:
    {
        //! TODO: 处理报错
        err = true;
    }
    break;
    }
    if (!err)
    {
        while (true)
        {
            vardef();
            if (ls.token.id == TOKEN::TK_SEMICOLON)
            {
                break;
            }
            else if (ls.token.id == TOKEN::TK_COMMA)
            {
                next();
            }
            else
            {
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
void Parser::vardef()
{
    struct ASTNode *root;
    if (ls.token.id != TOKEN::TK_IDENT)
    {
        //! TODO: 处理错误
        fprintf(stderr, "No TK_IDENT found in vardef()!\n");
        return;
    }
    //! TODO: 检查变量同名
    //! TODO: 处理变量名
    next();
    if (ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON)
    {
        return;
    }
    while (ls.token.id == TOKEN::TK_LBRACKET)
    { //<! 数组长度声明
        next();
        //! NOTE: 目前不支持数组长度推断
        expr();
        //! TODO: 获取并处理数组长度
        if (ls.token.id == TOKEN::TK_RBRACKET)
        {
            next();
        }
        else
        {
            //! TODO: 处理错误：括号未闭合
        }
    }
    if (ls.token.id == TOKEN::TK_ASS)
    {
        next();
        if (ls.token.id == TOKEN::TK_LBRACE)
        {
            initlist();
        }
        else
        {
            expr();
        }
        //! TODO: 获取并处理初始化赋值
    }
    if (!(ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON))
    {
        //! TODO: 处理错误：丢失的变量定义终止符
    }
}

/*!
 * init-list ->
 *      '{' '}' |
 *      '{' (expr | init-list) { ',' (expr | init-list) } [ ',' ] '}'
 */
void Parser::initlist()
{
    //! NOTE: 初始化列表必须有类型约束，由 ParseState 提供
    assert(ls.token.id == TOKEN::TK_LBRACE);
    next();
    bool has_more = true; //<! 是否允许存在下一个值
    while (ls.token.id != TOKEN::TK_RBRACE)
    {
        if (!has_more)
        {
            //! TODO: 处理错误
        }
        //! TODO: 处理可能出现的错误
        if (ls.token.id == TOKEN::TK_LBRACE)
        {
            initlist();
        }
        else
        {
            expr();
        }
        has_more = false;
        if (ls.token.id == TOKEN::TK_COMMA)
        {
            //! NOTE: 允许 trailing-comma
            has_more = true;
            next();
        }
    }
    next();
    //! TODO: 处理并存储初始化列表的值
}

void Parser::func()
{
    enterfunc();
    //! NOTE: 暂时不允许只声明不定义
    block();
    //! TODO: 处理函数体
    leavefunc();
}

void Parser::funcargs()
{
    assert(ls.token.id == TOKEN::TK_LPAREN);
    next();
    while (ls.token.id != TOKEN::TK_RPAREN)
    {
        //! FIXME: 可能死循环
        switch (ls.token.id)
        {
            case TOKEN::TK_INT:
            {
                //! TODO: 标记参数基本类型
                next();
            }
            break;
            case TOKEN::TK_FLOAT:
            {
                //! TODO: 标记参数基本类型
                next();
            }
            break;
            default:
            {
                //! TODO: 处理错误：未知的参数类型
            }
            break;
        }
        if (ls.token.id != TOKEN::TK_IDENT)
        {
            //! TODO: 处理错误：缺少参数名
        }
        //! TODO: 处理参数名
        next();
        if (ls.token.id == TOKEN::TK_LBRACKET)
        {
            //! 处理数组参数类型
            next();
            if (ls.token.id != TOKEN::TK_RBRACKET)
            {
                //! TODO: 处理错误：数组作为参数第一个下标必须为空
            }
            next();
            while (ls.token.id == TOKEN::TK_LBRACKET)
            {
                expr();
                //! NOTE: 处理并存储长度值
                if (ls.token.id != TOKEN::TK_RBRACKET)
                {
                    //! TODO: 处理错误：数组长度声明括号未闭合
                }
                next();
            }
        }
        //! TODO: 完成参数类型并写入函数原型
        if (ls.token.id == TOKEN::TK_COMMA)
        {
            if (ls.lookahead() == TOKEN::TK_RPAREN)
            {
                //! TODO: 处理错误：参数列表不允许有 trailing-comma
            }
            next();
        }
        else
        {
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
void Parser::statement()
{
    switch (ls.token.id)
    {
        case TOKEN::TK_SEMICOLON:
        {
            next();
        }
        break;
        case TOKEN::TK_IF:
        {
            ifstat();
        }
        break;
        case TOKEN::TK_WHILE:
        {
            whilestat();
        }
        break;
        case TOKEN::TK_BREAK:
        {
            breakstat();
        }
        break;
        case TOKEN::TK_CONTINUE:
        {
            continuestat();
        }
        break;
        case TOKEN::TK_RETURN:
        {
            returnstat();
        }
        break;
        case TOKEN::TK_LBRACE:
        {
            block();
        }
        break;
        default:
        {
            expr();
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        }
        break;
    }
}

void Parser::ifstat()
{
    assert(ls.token.id == TOKEN::TK_IF);
    next();
    //! TODO: 提升嵌套层次
    expect(TOKEN::TK_LPAREN, "expect '(' after 'if'");
    expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_RPAREN, "expect ')'");
    statement();
    if (ls.token.id == TOKEN::TK_ELSE)
    {
        next();
        statement();
    }
}

void Parser::whilestat()
{
    assert(ls.token.id == TOKEN::TK_WHILE);
    next();
    //! TODO: 提升嵌套层次
    expect(TOKEN::TK_WHILE, "expect '(' after 'while'");
    expr();
    //! TODO: 检查 expr 是否为条件表达式
    expect(TOKEN::TK_WHILE, "expect ')'");
    statement();
}

void Parser::breakstat()
{
    assert(ls.token.id == TOKEN::TK_BREAK);
    next();
    //! TODO: 外层环境检查
    expect(TOKEN::TK_SEMICOLON, "expect ';' after break statement");
}

void Parser::continuestat()
{
    assert(ls.token.id == TOKEN::TK_CONTINUE);
    next();
    //! TODO: 外层环境检查
    expect(TOKEN::TK_SEMICOLON, "expect ';' after continue statement");
}

void Parser::returnstat()
{
    assert(ls.token.id == TOKEN::TK_RETURN);
    next();
    if (ls.token.id != TOKEN::TK_SEMICOLON)
    {
        expr();
    }
    //! TODO: 返回值检验
    expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
}

void Parser::block()
{
    enterblock();
    assert(ls.token.id == TOKEN::TK_LBRACE);
    next();
    while (ls.token.id != TOKEN::TK_RBRACE)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_INT:
        case TOKEN::TK_FLOAT:
        case TOKEN::TK_CONST:
        {
            decl();
        }
        break;
        default:
        {
            statement();
        }
        break;
        }
        //! FIXME: 有概率陷入死循环
    }
    next();
    leaveblock();
}

int add_globalsym(LexState &ls, int type)
{
    for(int i=0;i<gsym.sym_num;i++)
    {
        if(!strcmp(ls.token.detail.data(), gsym.symbols[i].name)){
            fprintf(stderr, "Duplicate definition of symbol %s!\n", gsym.symbols[i].name);
            return -1;
        }
    }

    gsym.symbols[gsym.sym_num].name = strdup(ls.token.detail.data());
    gsym.symbols[gsym.sym_num].type = type;

    return gsym.sym_num++;
}

/*!
 * expr-list ->
 *      expr { ',' expr-list }
 */
void Parser::exprlist()
{
    expr();
    while (ls.token.id == TOKEN::TK_COMMA)
    {
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
struct ASTNode* Parser::primaryexpr()
{
    struct ASTNode *n;
    ASTVal32 val;

    switch (ls.token.id)
    {
        case TOKEN::TK_IDENT:
        {
            add_globalsym(ls);    
            //! TODO: 支持局部变量
            next();
        }
        break;
        case TOKEN::TK_INTVAL:
        {
            val.intvalue = atoi(ls.token.detail.data());
            n = mkastleaf(A_INTLIT, val);
            next();
        }
        break;
        case TOKEN::TK_FLTVAL:
        {
            val.fltvalue = atof(ls.token.detail.data());
            n = mkastleaf(A_FLTLIT, val);
            next();
        }
        break;
        case TOKEN::TK_STRING:
        {
            //! NOTE: #feature(string)
            fprintf(stderr, "前面的特性以后再来做吧!(指string)\n");
            next();
        }
        break;
        case TOKEN::TK_LPAREN:
        {
            next();
            expr();
            expect(TOKEN::TK_RPAREN, "expect ')' after expression");
        }
        break;
        default:
        {
            //! FIXME: 错误处理
        }
        break;
    }

    return n;
}

/*!
 * postfix-expr ->
 *      primary-expr |
 *      postfix-expr '[' expr ']' |
 *      postfix-expr '(' [ expr-list ] ')'
 */
struct ASTNode* Parser::postfixexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = primaryexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
            case TOKEN::TK_LBRACKET:
            { //<! array index
                expr();
                expect(TOKEN::TK_RBRACKET, "expect ']'");
            }
            break;
            case TOKEN::TK_LPAREN:
            { //<! function call
                exprlist();
                expect(TOKEN::TK_RBRACKET, "expect ')'");
            }
            break;
            default:
            {
                ok = true;
            }
            break;
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
struct ASTNode* Parser::unaryexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    int nodetype = 0;

    left = NULL;

    switch (ls.token.id)
    {
        case TOKEN::TK_ADD:
            nodetype = A_PLUS; goto handle;
        case TOKEN::TK_SUB:
            nodetype = A_MINUS; goto handle;
        case TOKEN::TK_INV:
        case TOKEN::TK_NOT:
        {
            nodetype = tok2ast(ls.token.id);
    handle:
            next();
            right = unaryexpr();
            left = mkastnode(nodetype, NULL, right, val);
        }
        break;
        default:
        {
            left = postfixexpr();
        }
        break;
    }

    return left;
}

/*!
 * mul-expr ->
 *      unary-expr |
 *      mul-expr '*' unary-expr |
 *      mul-expr '/' unary-expr |
 *      mul-expr '%' unary-expr
 */
struct ASTNode* Parser::mulexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    TOKEN tokentype = ls.token.id;
    
    left = unaryexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_MUL:
        case TOKEN::TK_DIV:
        case TOKEN::TK_MOD:
        {
            next();
            right = unaryexpr();
            left = mkastnode(tok2ast(tokentype), left, right, val);
        }
        break;
        default:
        {
            ok = true;
        } 
        break;
        }
    }
    return left;
}

/*! 
 * add-expr ->
 *      mul-expr |
 *      add-expr '+' mul-expr |
 *      add-expr '-' mul-expr
 */
struct ASTNode* Parser::addexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    TOKEN tokentype = ls.token.id;

    left = mulexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_ADD:
        case TOKEN::TK_SUB:
        {
            next();
            right = mulexpr();
            left = mkastnode(tok2ast(tokentype), left, right, val);
        }
        break;
        default:
        {
            ok = true;
        }
        break;
        }
    }

    return left;
}

/*!
 * shift-expr ->
 *      add-expr |
 *      shift-expr '<<' add-expr |
 *      shift-expr '>>' add-expr
 */
struct ASTNode* Parser::shiftexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    TOKEN tokentype = ls.token.id;

    left = addexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_SHL:
        case TOKEN::TK_SHR:
        {
            next();
            right = addexpr();
            left = mkastnode(tok2ast(tokentype), left, right, val);
        }
        break;
        default:
        {
            ok = true;
        }
        break;
        }
    }
    return left;
}

/*!
 * rel-expr ->
 *      shift-expr |
 *      rel-expr '<' shift-expr |
 *      rel-expr '>' shift-expr |
 *      rel-expr '<=' shift-expr |
 *      rel-expr '>=' shift-expr
 */
struct ASTNode* Parser::relexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    TOKEN tokentype = ls.token.id;

    left = shiftexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_LT:
        case TOKEN::TK_GT:
        case TOKEN::TK_LE:
        case TOKEN::TK_GE:
        {
            next();
            right = shiftexpr();
            left = mkastnode(tok2ast(tokentype), left, right, val);
        }
        break;
        default:
        {
            ok = true;
        }
        break;
        }
    }
    return left;
}

/*!
 * eq-expr ->
 *      rel-expr |
 *      eq-expr '==' rel-expr |
 *      eq-expr '!=' rel-expr
 */
struct ASTNode* Parser::eqexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};
    TOKEN tokentype = ls.token.id;

    left = relexpr();
    bool ok = false;
    while (!ok)
    {
        switch (ls.token.id)
        {
        case TOKEN::TK_EQ:
        case TOKEN::TK_NE:
        case TOKEN::TK_LE:
        {
            next();
            right = relexpr();
            left = mkastnode(tok2ast(tokentype), left, right, val);
        }
        break;
        default:
        {
            ok = true;
        }
        break;
        }
    }

    return left;
}

/*!
 * and-expr ->
 *      eq-expr |
 *      and-expr '&' eq-expr
 */
struct ASTNode* Parser::andexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = eqexpr();
    while (ls.token.id == TOKEN::TK_AND)
    {
        next();
        right = eqexpr();
        mkastnode(tok2ast(TOKEN::TK_AND), left, right, val);
    }

    return left;
}

/*!
 * xor-expr ->
 *      and-expr |
 *      xor-expr '^' and-expr
 */
struct ASTNode* Parser::xorexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = andexpr();
    while (ls.token.id == TOKEN::TK_XOR)
    {
        next();
        right = andexpr();
        mkastnode(tok2ast(TOKEN::TK_XOR), left, right, val);
    }

    return left;
}

/*!
 * or-expr ->
 *      xor-expr |
 *      or-expr '|' xor-expr
 */
struct ASTNode* Parser::orexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = xorexpr();
    while (ls.token.id == TOKEN::TK_OR)
    {
        next();
        right = xorexpr();
        mkastnode(tok2ast(TOKEN::TK_OR), left, right, val);
    }

    return left;
}

/*!
 * land-expr ->
 *      or-expr |
 *      land-expr '&&' or-expr
 */
struct ASTNode* Parser::landexpr()
{
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = orexpr();
    while (ls.token.id == TOKEN::TK_LAND)
    {
        next();
        right = orexpr();
        mkastnode(tok2ast(TOKEN::TK_LAND), left, right, val);
    }

    return left;
}

/*!
 * lor-expr ->
 *      land-expr |
 *      lor-expr '||' land-expr
 */
struct ASTNode* Parser::lorexpr() {
    struct ASTNode *left, *right;
    ASTVal32 val = {.intvalue = 0};

    left = landexpr();
    while (ls.token.id == TOKEN::TK_LOR)
    {
        next();
        right = landexpr();
        mkastnode(tok2ast(TOKEN::TK_LOR), left, right, val);
    }

    return left;
}

/*!
 * cond-expr ->
 *      lor-expr
 */
struct ASTNode* Parser::condexpr()
{
    return lorexpr();
}

/*!
 * assign-expr ->
 *      cond-expr |
 *      unary-expr '=' assign-expr
 */
struct ASTNode* Parser::assignexpr()
{
    struct ASTNode *left, *right, *root;
    left = condexpr();
    if (ls.token.id == TOKEN::TK_ASS)
    {
        next();
        right = assignexpr();
    }

}

/*!
 * expr ->
 *      assign-expr |
 *      expr ',' assign-expr
 */
struct ASTNode* Parser::expr()
{
    assignexpr();
    while (ls.token.id == TOKEN::TK_COMMA)
    {
        next();
        assignexpr();
    }
}
