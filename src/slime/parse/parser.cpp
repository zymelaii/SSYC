#include "parser.h"
#include "ast.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace slime
{

    static symtable g_sym = {.symbols = NULL, .sym_num = 0};

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

    ASTNode *Parser::global_parse()
    {
        return decl();
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
    void Parser::leavedecl(int tag)
    {
        if(tag == S_VARIABLE)
        {
            assert(ls.token.id == TOKEN::TK_SEMICOLON || ls.token.id == TOKEN::TK_COMMA);
            next();
        }
        //! TODO: 清理操作
    }

    // 标记函数类型，处理函数参数列表
    int Parser::enterfunc(int type)
    {
        int ret = 0;
        assert(ps.cur_block == NULL);   //暂不支持局部函数声明
        if (ls.token.id != TOKEN::TK_IDENT)
        {
            fprintf(stderr, "Missing function name in enterfunc()!\n");
            exit(-1);
            //! TODO: 处理错误：函数名缺失
        }
        //! TODO: 检查并处理函数名
        if(find_globalsym(ls.token.detail.data()) != -1){
            fprintf(stderr, "Duplicate defined symbol:%s in enterfunc()!\n", ls.token.detail.data());
        }

        ret = add_globalsym(type, S_FUNCTION);
        next();
        if (ls.token.id != TOKEN::TK_LPAREN)
        {
            //! TODO: 处理错误：丢失参数列表
            fprintf(stderr, "Missing argument list of function %s!\n", g_sym.symbols[g_sym.sym_num-1].name);
        }
        g_sym.symbols[ret].content.funcparams = funcargs();
        ps.cur_funcparam = g_sym.symbols[ret].content.funcparams;
        //! TODO: 解析函数原型
        return ret;
    }

    //! 结束函数定义
    void Parser::leavefunc()
    {
        ps.cur_func = -1;
        ps.cur_funcparam = NULL;
        //! TODO: 检查并处理函数体
    }

    //! 处理嵌套层数并初始化块环境
    void Parser::enterblock()
    {
        blockinfo *b = (blockinfo *)malloc(sizeof(blockinfo));
        if(ps.cur_block && ps.cur_block->head == NULL)
            ps.cur_block->head = b;
        else if(ps.cur_block)
        {
            blockinfo *p = ps.cur_block->head;
            while(p->next) p = p->next;
            p->next = b;
        }
        else
            b->head = NULL;

        b->next      = NULL;
        b->prev_head = ps.cur_block;
        b->l_sym.symbols = NULL;
        b->l_sym.sym_num = 0;
        ps.cur_block = b;
    }

    //! 清理块环境
    void Parser::leaveblock() 
    {
        ps.cur_block = ps.cur_block->prev_head;
    }

    /*!
     * decl ->
     *      [ 'const' ] type vardef { ',' vardef } ';'
     */
    ASTNode *Parser::decl()
    {
        enterdecl();
        ASTNode *left = NULL, *right = NULL;
        ASTVal32 val = {.intvalue = 0};
        bool err = false;
        int type, tag;

        switch (ls.token.id)
        {
        case TOKEN::TK_VOID:
        {
            type = TYPE_VOID;
            next();
        }
        case TOKEN::TK_INT:
        {
            type = TYPE_INT;
            next();
        }
        break;
        case TOKEN::TK_FLOAT:
        {
            type = TYPE_FLOAT;
            next();
        }
        break;
        default:
        {
            //! TODO: 处理报错
            fprintf(stderr, "Unknown declare type:%s in decl()!\n", ls.token.detail.data());
            err = true;
        }
        break;
        }
        if (!err)
        {
            while (ls.token.id != TOKEN::TK_EOF)
            {
                if(ls.lookahead() == TOKEN::TK_LPAREN)
                {
                    tag  = S_FUNCTION;
                    right = func(type);
                    break;
                }
                else{
                    if(type == TYPE_VOID)
                    {
                        fprintf(stderr, "Variable %s declared void.\n", ls.token.detail.data());
                        exit(-1);
                    }
                    tag  = S_VARIABLE;
                    left = vardef(type);
                    if(!right)
                        right = left;
                    else if(left)
                        right = mkastnode(A_STMT, left, NULL, right, val);
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
        }
        leavedecl(tag);
        return right;
    }

    /*!
     * vardef ->
     *      ident { '[' expr ']' }
     *      ident '='
     */
    ASTNode *Parser::vardef(int type)
    {
        ASTNode *root = NULL, *left = NULL;
        ASTVal32 val = {.intvalue = 0};

        if (ls.token.id != TOKEN::TK_IDENT)
        {
            //! TODO: 处理错误
            fprintf(stderr, "No TK_IDENT found in vardef()!\n");
            exit(-1);
            return NULL;
        }

        if(ps.cur_block == NULL)
            val.symindex = add_globalsym(type ,S_VARIABLE);
        else
            val.symindex = add_localsym(type, S_VARIABLE);

        next();
        if (ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON)
        {
            return NULL;
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
            left = mkastleaf(A_IDENT, val);   
            next();
            if (ls.token.id == TOKEN::TK_LBRACE)
            {
                initlist();
            }
            else
            {
                val.intvalue = 0;
                root = expr();
                root = mkastnode(A_ASSIGN, left, NULL, root, val);
            }
            //! TODO: 获取并处理初始化赋值
        }
        if (!(ls.token.id == TOKEN::TK_COMMA || ls.token.id == TOKEN::TK_SEMICOLON))
        {
            fprintf(stderr, "Missing ';' in vardef()!\n");
            //! TODO: 处理错误：丢失的变量定义终止符
        }

        return root;
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

    ASTNode *Parser::func(int type)
    {
        ASTNode *tree;
        ASTVal32 val = {.intvalue = 0};
        val.symindex = enterfunc(type);
        ps.cur_func = val.symindex;
        //! NOTE: 暂时不允许只声明不定义
        tree = block();
        if(!tree){
            fprintf(stderr, "Missing statements in function!\n");
            return NULL;
        }
        tree = mkastunary(A_FUNCTION, tree, val);
        //! TODO: 处理函数体
        leavefunc();
        return tree;
    }

    paramtable *Parser::funcargs()
    {
        assert(ls.token.id == TOKEN::TK_LPAREN);
        next();
        paramtable *params = (paramtable *)malloc(sizeof(paramtable));
        params->params = NULL;
        params->param_num = 0;
        int type = -1;
        while (ls.token.id != TOKEN::TK_RPAREN)
        {
            //! FIXME: 可能死循环
            
            params->params = (paraminfo *)realloc(params->params, (params->param_num + 1) * sizeof(paraminfo));
            switch (ls.token.id)
            {
            case TOKEN::TK_VOID:
            {
                fprintf(stderr, "Void type is invalid in function parameters!\n");
                exit(-1);
            }
            case TOKEN::TK_INT:
            {
                type = TYPE_INT;
                next();
            }
            break;
            case TOKEN::TK_FLOAT:
            {
                type = TYPE_FLOAT;
                next();
            }
            break;
            default:
            {
                fprintf(stderr, "Unknown parameter type: %s", ls.token.detail.data());
                exit(-1);
            }
            break;
            }
            if (ls.token.id != TOKEN::TK_IDENT)
            {
                fprintf(stderr, "Missing parameter's name in funcargs()!\n");
                exit(-1);
            }
            params->params[params->param_num].name  = strdup(ls.token.detail.data());
            params->params[params->param_num].stype = S_VARIABLE;
            params->params[params->param_num].type  = type;
            params->param_num++; 
            next();

            //!TODO: 支持数组作为参数传入
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
                    fprintf(stderr, "Unexpected comma in funcargs()!\n");
                    exit(-1);
                }
                
                next();
            }
            else if(ls.token.id != TOKEN::TK_RPAREN)
            {
                fprintf(stderr, "Invalid terminator in function parameters' definition!\n");
                exit(-1);
            }
        }
        next();
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
    ASTNode *Parser::statement()
    {
        ASTNode *tree = NULL;
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
            //!TODO: if statement
        }
        break;
        case TOKEN::TK_WHILE:
        {
            whilestat();
            //!TODO: while statement
        }
        break;
        case TOKEN::TK_BREAK:
        {
            breakstat();
            //!TODO: break statement;
        }
        break;
        case TOKEN::TK_CONTINUE:
        {
            continuestat();
            //!TODO: continue statement;
        }
        break;
        case TOKEN::TK_RETURN:
        {
            tree = returnstat();
            //!TODO: return statement;
        }
        break;
        case TOKEN::TK_LBRACE:
        {
            tree = block();
        }
        break;
        case TOKEN::TK_CONST:
        case TOKEN::TK_INT:
        case TOKEN::TK_FLOAT:
            tree = decl();
            break;
        default:
        {
            tree = expr();
            expect(TOKEN::TK_SEMICOLON, "expect ';' after expression");
        }
        break;
        }

        return tree;
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

    ASTNode *Parser::returnstat()
    {
        assert(ls.token.id == TOKEN::TK_RETURN);
        assert(ps.cur_func != -1);
        ASTNode *tree = NULL;
        ASTVal32 val = {.intvalue = 0};
        next();
        if (ls.token.id != TOKEN::TK_SEMICOLON)
        {
            tree = expr();
            tree = mkastunary(A_RETURN, tree, val);
        }
        
        //! TODO: 返回值检验
        if(!tree->left && g_sym.symbols[ps.cur_func].type != TYPE_VOID){
            fprintf(stderr, "error: Missing return value in non-void function %s!\n", g_sym.symbols[ps.cur_func].name);
            exit(-1);
        }
        expect(TOKEN::TK_SEMICOLON, "expect ';' after return statement");
        return tree;
    }

    ASTNode *Parser::block()
    {
        enterblock();
        ASTNode *s1, *s2 = NULL;
        ASTVal32 val = {.intvalue = 0};

        if(ps.cur_funcparam)        //add parameter to lsymtable
        {
            paramtable *p = ps.cur_funcparam;
            for(int i=0;i<p->param_num;i++)
            {
                p->params[i].lsym_index = add_localsym(
                    p->params[i].type,
                    p->params[i].stype,
                    p->params[i].name,
                    ps.cur_block
                );
            }
        }
        assert(ls.token.id == TOKEN::TK_LBRACE);
        next();
        while (ls.token.id != TOKEN::TK_RBRACE)
        {
            s1 = statement();
            if(s1){
                if(!s2)
                    s2 = s1;
                else
                    s2 = mkastnode(A_STMT, s2, NULL, s1, val);
            }
        }
        next();
        s2 = mkastunary(A_BLOCK, s2, val);
        s2->block = ps.cur_block;
        leaveblock();
        return s2;
    }

    int Parser::add_globalsym(int type, int stype)
    {
        assert(ls.token.id == TOKEN::TK_IDENT);
        if(find_globalsym(ls.token.detail.data()) != -1){
            fprintf(
                stderr,
                "Duplicate definition of symbol %s          ---- add_globalsym()!\n",
                ls.token.detail.data());
            return -1;
        }
        g_sym.symbols = (syminfo *)realloc(g_sym.symbols, sizeof(syminfo) * (g_sym.sym_num+1));
        g_sym.symbols[g_sym.sym_num].name = strdup(ls.token.detail.data());
        g_sym.symbols[g_sym.sym_num].type = type;
        g_sym.symbols[g_sym.sym_num].stype = stype;

        return g_sym.sym_num++;
    }

    int Parser::find_globalsym(const char *name)
    {
        for (int i = 0; i < g_sym.sym_num; i++)
        {
            if (!strcmp(ls.token.detail.data(), g_sym.symbols[i].name))
            {
                return i;
            }
        }

        return -1;
    }

    int Parser::find_localsym(const char *name, blockinfo **pblock)
    {
        blockinfo *b = ps.cur_block;
        while(b)
        {
            symtable *lsyms = &b->l_sym;
            for(int i=0;i<lsyms->sym_num;i++)
            {
                if(!strcmp(name, lsyms->symbols[i].name))
                {
                    if(pblock != NULL)
                        *pblock = b;
                    return i;
                }
            }
            b = b->prev_head;
        }
        //not found in any block
        
        if(pblock) *pblock = NULL;
        return find_globalsym(name);
    }

    int Parser::add_localsym(int type, int stype)
    {
        assert(ls.token.id == TOKEN::TK_IDENT);
        blockinfo *b = ps.cur_block, **pblock;
        pblock = (blockinfo **)malloc(sizeof(blockinfo *));
        symtable *lsyms = &b->l_sym;
        if(find_localsym(ls.token.detail.data(), pblock) != -1)
        {
            if(*pblock && *pblock == b)
            {
                fprintf(
                    stderr,
                    "Duplicate definition of symbol %s          ---- add_localsym()!\n",
                    ls.token.detail.data());
                free(pblock);
                exit(-1);
            }
        }

        lsyms->symbols = (syminfo *)realloc(lsyms->symbols, sizeof(syminfo) * (lsyms->sym_num + 1));
        lsyms->symbols[lsyms->sym_num].name  = strdup(ls.token.detail.data());
        lsyms->symbols[lsyms->sym_num].type  = type;
        lsyms->symbols[lsyms->sym_num].stype = stype;
        
        free(pblock);
        return lsyms->sym_num++;
    }

    int Parser::add_localsym(int type, int stype, char *name, blockinfo *block)
    {
        symtable *lsyms = &block->l_sym;
        for(int i=0;i<block->l_sym.sym_num;i++)
        {
            if(strcmp(name, lsyms->symbols[i].name))
            {
                fprintf(
                    stderr,
                    "Duplicate definition of symbol %s          ---- add_localsym()!\n",
                    name);
                exit(-1);
            }
        }

        lsyms->symbols = (syminfo *)realloc(lsyms->symbols, sizeof(syminfo) * (lsyms->sym_num + 1));
        lsyms->symbols[lsyms->sym_num].name  = strdup(name);
        lsyms->symbols[lsyms->sym_num].type  = type;
        lsyms->symbols[lsyms->sym_num].stype = stype;
        
        return lsyms->sym_num++;
    }

    /*!
     * expr-list ->
     *      expr { ',' expr-list }
     */
    ASTNode *Parser::exprlist(int funcindex)
    {
        paraminfo *params = g_sym.symbols[funcindex].content.funcparams->params;
        int param_num = g_sym.symbols[funcindex].content.funcparams->param_num, num = 0;
        ASTVal32 val = {.intvalue = 0};
        ASTNode *root, *left, *right;

        right = assignexpr();
        assert(right->op == A_INTLIT || right->op == A_FLTLIT || right->op == A_FUNCCALL || right->op == A_IDENT);
        val.symindex = params[num].lsym_index;
        left  = mkastleaf(A_IDENT, val);
        root  = mkastnode(A_ASSIGN, left, NULL, right, val);
        num++;

        while (ls.token.id == TOKEN::TK_COMMA)
        {
            next();
            right = assignexpr();
            val.symindex = num;
            left  = mkastleaf(A_IDENT, val);
            root  = mkastnode(A_ASSIGN, left, NULL, right, val);
            num++;
        }

        if(num < param_num)
        {
            fprintf(stderr, "Too few arguments when calling function %s. Expected %d, got %d.\n",
                g_sym.symbols[funcindex].name, param_num, num
            );
            exit(-1);
        }
        else if(num > param_num){
            fprintf(stderr, "Too many arguments when calling function %s. Expected %d, got %d.\n",
                g_sym.symbols[funcindex].name, param_num, num
            );
            exit(-1);
        }

        return root;
    }

    /*!
     * primary-expr ->
     *      ident |
     *      integer-constant |
     *      floating-constant |
     *      string-literal |
     *      '(' expr ')'
     */
    ASTNode *Parser::primaryexpr()
    {
        ASTNode *n = NULL;
        ASTVal32 val;

        switch (ls.token.id)
        {
            case TOKEN::TK_IDENT:
            {
                blockinfo **pblock = (blockinfo **)malloc(sizeof(blockinfo *));
                if(!ps.cur_block)
                    val.symindex = find_globalsym(ls.token.detail.data());
                else
                    val.symindex = find_localsym(ls.token.detail.data(), pblock);
                if(val.symindex == -1){
                    fprintf(stderr, "undefined symbol %s in primaryexpr()!\n", ls.token.detail.data());
                    exit(-1);
                }
                n = mkastleaf(A_IDENT, val);
                n->block = *pblock;
                free(pblock);
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
                fprintf(
                    stderr,
                    "The preceding properties will be done later! (ref. string)\n");
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
    ASTNode *Parser::postfixexpr()
    {
        ASTNode *left, *right = NULL;
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
                if(left->op != A_IDENT)
                {
                    fprintf(stderr, "Missing identifier when calling a function!\n");
                    exit(-1);
                }
                else if(left->block != NULL)
                {
                    fprintf(stderr, "Definition of a local function is not supported!\n");
                    exit(-1);
                }
                
                assert(g_sym.symbols[left->val.symindex].stype == S_FUNCTION);
                if(ls.lookahead() != TOKEN::TK_RPAREN)
                {
                    next();
                    right = exprlist(left->val.symindex);
                }
                left = mkastnode(A_FUNCCALL, left, NULL, right, val);
                expect(TOKEN::TK_RPAREN, "expect ')'");
            }
            break;
            default:
            {
                ok = true;
            }
            break;
            }
        }

        //! TODO: support array index, function call
        return left;
    }

    /*!
     * unary-expr ->
     *      postfix-expr |
     *      '+' unary-expr |
     *      '-' unary-expr |
     *      '~' unary-expr |
     *      '!' unary-expr
     */
    ASTNode *Parser::unaryexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        int nodetype = 0;

        left = NULL;

        switch (ls.token.id)
        {
        case TOKEN::TK_ADD:
            nodetype = A_PLUS;
            goto handle;
        case TOKEN::TK_SUB:
            nodetype = A_MINUS;
            goto handle;
        case TOKEN::TK_INV:
        case TOKEN::TK_NOT:
        {
            nodetype = tok2ast(ls.token.id);
        handle:
            next();
            left = unaryexpr();
            left = mkastunary(nodetype, left, val);
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
    ASTNode *Parser::mulexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        TOKEN tokentype;

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
                tokentype = ls.token.id;
                next();
                right = unaryexpr();
                left = mkastnode(tok2ast(tokentype), left, NULL, right, val);
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
    ASTNode *Parser::addexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        TOKEN tokentype;

        left = mulexpr();
        bool ok = false;
        while (!ok)
        {
            switch (ls.token.id)
            {
            case TOKEN::TK_ADD: 
            case TOKEN::TK_SUB: 
            {
                tokentype = ls.token.id;
                next();
                right = mulexpr();
                left = mkastnode(tok2ast(tokentype), left, NULL, right, val);
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
    ASTNode *Parser::shiftexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        TOKEN tokentype;

        left = addexpr();
        bool ok = false;
        while (!ok)
        {
            switch (ls.token.id)
            {
            case TOKEN::TK_SHL:
            case TOKEN::TK_SHR:
            {
                tokentype = ls.token.id;
                next();
                right = addexpr();
                left = mkastnode(tok2ast(tokentype), left, NULL, right, val);
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
    ASTNode *Parser::relexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        TOKEN tokentype ;

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
                tokentype = ls.token.id;
                next();
                right = shiftexpr();
                left = mkastnode(tok2ast(tokentype), left, NULL, right, val);
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
    ASTNode *Parser::eqexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};
        TOKEN tokentype;

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
                tokentype = ls.token.id;
                next();
                right = relexpr();
                left = mkastnode(tok2ast(tokentype), left, NULL, right, val);
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
    ASTNode *Parser::andexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};

        left = eqexpr();
        while (ls.token.id == TOKEN::TK_AND)
        {
            next();
            right = eqexpr();
            mkastnode(tok2ast(TOKEN::TK_AND), left, NULL, right, val);
        }

        return left;
    }

    /*!
     * xor-expr ->
     *      and-expr |
     *      xor-expr '^' and-expr
     */
    ASTNode *Parser::xorexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};

        left = andexpr();
        while (ls.token.id == TOKEN::TK_XOR)
        {
            next();
            right = andexpr();
            mkastnode(tok2ast(TOKEN::TK_XOR), left, NULL, right, val);
        }

        return left;
    }

    /*!
     * or-expr ->
     *      xor-expr |
     *      or-expr '|' xor-expr
     */
    ASTNode *Parser::orexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};

        left = xorexpr();
        while (ls.token.id == TOKEN::TK_OR)
        {
            next();
            right = xorexpr();
            mkastnode(tok2ast(TOKEN::TK_OR), left, NULL, right, val);
        }

        return left;
    }

    /*!
     * land-expr ->
     *      or-expr |
     *      land-expr '&&' or-expr
     */
    ASTNode *Parser::landexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};

        left = orexpr();
        while (ls.token.id == TOKEN::TK_LAND)
        {
            next();
            right = orexpr();
            mkastnode(tok2ast(TOKEN::TK_LAND), left, NULL, right, val);
        }

        return left;
    }

    /*!
     * lor-expr ->
     *      land-expr |
     *      lor-expr '||' land-expr
     */
    ASTNode *Parser::lorexpr()
    {
        ASTNode *left, *right;
        ASTVal32 val = {.intvalue = 0};

        left = landexpr();
        while (ls.token.id == TOKEN::TK_LOR)
        {
            next();
            right = landexpr();
            mkastnode(tok2ast(TOKEN::TK_LOR), left, NULL, right, val);
        }

        return left;
    }

    /*!
     * cond-expr ->
     *      lor-expr
     */
    ASTNode *Parser::condexpr()
    {
        return lorexpr();
    }

    /*!
     * assign-expr ->
     *      cond-expr |
     *      unary-expr '=' assign-expr
     */
    ASTNode *Parser::assignexpr()
    {
        ASTNode *left, *right, *root;
        ASTVal32 v = {.intvalue = 0};
        left = condexpr();

        if (ls.token.id == TOKEN::TK_ASS)
        {
            if(left->op != A_IDENT)
            {
                fprintf(stderr, "Missing identifier on the left of assignment!\n");
                exit(-1);
            }
            next();
            right = assignexpr();
            left = mkastnode(A_ASSIGN, left, NULL, right, v);
        }

        return left;
    }

    /*!
     * expr ->
     *      assign-expr |
     *      expr ',' assign-expr
     */
    ASTNode *Parser::expr()
    {
        ASTNode *root;
        root = assignexpr();
        if(!root){
            fprintf(stderr, "Missing expression before comma!\n");
            exit(-1);
        }
        while (ls.token.id == TOKEN::TK_COMMA)
        {
            next();
            root = assignexpr();
        }

        return root;
    }

    void Parser::inorder(ASTNode *n) {
        if (!n) return;
        if (n->left) inorder(n->left);
        printf("%s", ast2str(n->op));
        if (n->op == A_INTLIT)
            printf(": %d", n->val.intvalue);
        else if (n->op == A_FLTLIT)
            printf(": %f", n->val.fltvalue);
        else if (n->op == A_IDENT)
            displaySymInfo(n->val.symindex, n->block);
        printf("\n");
        if (n->mid) inorder(n->mid);
        if (n->right) inorder(n->right);
    }

    void Parser::traverseAST(ASTNode *root)
    {
        inorder(root);
    }


    void Parser::displaySymInfo(int index, blockinfo *block)
    {
        if(!block)
        {
            if(g_sym.symbols[index].stype == S_FUNCTION)
                printf("type: function ");
            else
                printf("type: variable ");
            printf("name: %s\n", g_sym.symbols[index].name);    
        }
        else{
            if(block->l_sym.symbols[index].stype == S_FUNCTION)
                printf(": type: function ");
            else
                printf(": type: variable ");
            printf("name: %s", block->l_sym.symbols[index].name);    
        }
    }

} // namespace slime
