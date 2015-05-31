#include <stdio.h>
#include <ctype.h>
#include <string>
#include <stdlib.h>
#include <stdarg.h>
#include <map>

#include "ast.h"

#define _ACODE(t) #t,
const char * const AstCodeNames[] = { AST_CODES };
#undef _ACODE


#define TERMS \
    TERM(_EOF,"<end of file>")\
    TERM(IDENT,"identifier")\
    TERM(PLUS,"+")\
    TERM(MINUS,"-")\
    TERM(MUL,"*")\
    TERM(DIV,"/")\
    TERM(LPAR,"(")\
    TERM(RPAR,")")\
    TERM(EQ,"==")\
    TERM(NE,"!=")\
    TERM(LT,"<")\
    TERM(GT,">")\
    TERM(LBRACE,"{")\
    TERM(RBRACE,"}")\
    TERM(NUMBER,"number") \
    TERM(SEMI,";") \
    TERM(COMMA,",") \
    TERM(ASSIGN,"=") \
    TERM(IF,"if") \
    TERM(ELSE,"else") \
    TERM(WHILE,"while") \
    TERM(RETURN,"return") \
    TERM(FN,"fn")


#define TERM(t,_) t,
enum Term { TERMS };
#undef TERM

#define TERM(t,_)  #t,
static const char * s_termName[] = { TERMS };
#undef TERM

#define TERM(_,t)  #t,
static const char * s_termUI[] = { TERMS };
#undef TERM

static int s_startLine, s_startCol;
static int s_line, s_col;
static int s_nextCh;
static std::string s_ident;
static long s_number;

static Term s_term = _EOF;

static std::map<std::string,Term> s_kw;

static int nextChar ()
{
    int c = getchar();
    if (c == '\n') {
        ++s_line;
        s_col = 0;
    }
    else
        ++s_col;
    return c;
}

void initScanner ()
{
    s_kw["return"] = RETURN;
    s_kw["if"] = IF;
    s_kw["else"] = ELSE;
    s_kw["while"] = WHILE;
    s_kw["fn"] = FN;
    s_line = 1; s_col = 0;
    s_nextCh = nextChar();
}

static void saveStart ()
{
    s_startLine = s_line;
    s_startCol = s_col;
}

static void error ( const char * msg, ... )
{
    va_list  ap;
    va_start(ap, msg);
    fprintf( stderr, "Error line %d col %d:", s_startLine, s_startCol );
    vfprintf( stderr, msg, ap );
    fputc( '\n', stderr );
    exit( 1 );
}

void runtimeError ( const char * msg, ... )
{
    va_list  ap;
    va_start(ap, msg);
    fprintf( stderr, "Runtime error:" );
    vfprintf( stderr, msg, ap );
    fputc( '\n', stderr );
    exit( 1 );
}

Term getNextTerm ()
{
    for(;;) {
        saveStart();
        if (isalpha(s_nextCh) || s_nextCh == '_') {
            s_ident.clear();
            do {
                s_ident.push_back( (char)s_nextCh );
                s_nextCh = nextChar();
            } while (isalpha(s_nextCh) || isdigit(s_nextCh) || s_nextCh == '_');
            auto it = s_kw.find(s_ident);
            if (it == s_kw.end())
                return s_term = IDENT;
            else
                return s_term = it->second;
        }
        else if (s_nextCh == '+') {
            s_nextCh = nextChar();
            return s_term = PLUS;
        }
        else if (s_nextCh == '-') {
            s_nextCh = nextChar();
            return s_term = MINUS;
        }
        else if (s_nextCh == '*') {
            s_nextCh = nextChar();
            return s_term = MUL;
        }
        else if (s_nextCh == '/') {
            s_nextCh = nextChar();
            return s_term = DIV;
        }
        else if (s_nextCh == '(') {
            s_nextCh = nextChar();
            return s_term = LPAR;
        }
        else if (s_nextCh == ')') {
            s_nextCh = nextChar();
            return s_term = RPAR;
        }
        else if (s_nextCh == '=') {
            s_nextCh = nextChar();
            if (s_nextCh == '=') {
                s_nextCh = nextChar();
                return s_term = EQ;
            }
            else
                return s_term = ASSIGN;
        }
        else if (s_nextCh == '!') {
            s_nextCh = nextChar();
            if (s_nextCh == '=') {
                s_nextCh = nextChar();
                return s_term = NE;
            }
            else
                error( "Invalid character '%c'", s_nextCh );
        }
        else if (s_nextCh == ';') {
            s_nextCh = nextChar();
            return s_term = SEMI;
        }
        else if (s_nextCh == ',') {
            s_nextCh = nextChar();
            return s_term = COMMA;
        }
        else if (s_nextCh == '{') {
            s_nextCh = nextChar();
            return s_term = LBRACE;
        }
        else if (s_nextCh == '}') {
            s_nextCh = nextChar();
            return s_term = RBRACE;
        }
        else if (s_nextCh == '<') {
            s_nextCh = nextChar();
            return s_term = LT;
        }
        else if (s_nextCh == '>') {
            s_nextCh = nextChar();
            return s_term = GT;
        }
        else if (isdigit(s_nextCh)) {
            s_number = 0;
            do {
                s_number = s_number * 10 + s_nextCh - '0';
                s_nextCh = nextChar();
            } while (isdigit(s_nextCh));
            return s_term = NUMBER;
        }
        else if (isspace(s_nextCh)) {
            do
                s_nextCh = nextChar();
            while (isspace(s_nextCh));
        }
        else if (s_nextCh == EOF) {
            return s_term = _EOF;
        }
        else {
            fprintf( stderr, "Invalid input character '%c'\n", s_nextCh );
            s_nextCh = nextChar();
        }
    }
}


static Expr * parseExpression ();
static Block * parseStatementList ();
static Statement * parseStatement ();
static If * parseIf ();
static Program * parseProgram ();

void initParser ()
{
    initScanner();
    getNextTerm();
}
static void need ( Term term )
{
    if (s_term != term)
        error( "Expected %s", s_termUI[term] );
    getNextTerm();
}

static FunctionCall * parseFunctionCall ( const std::string & name )
{
    std::vector<ExprPtr> args;
    need(LPAR);
    if (s_term != RPAR) {
        args.push_back( ExprPtr(parseExpression()) );
        while (s_term == COMMA) {
            getNextTerm();
            args.push_back( ExprPtr(parseExpression()) );
        }
    }
    need(RPAR);
    return new FunctionCall( name, std::move(args) );
}

static Expr * parseAtom ()
{
    Expr * res;
    if (s_term == IDENT) {
        std::string saveIdent = s_ident;
        getNextTerm();
        if (s_term == LPAR)
            res = parseFunctionCall(saveIdent);
        else
            res = new Ident(saveIdent);
    }
    else if (s_term == LPAR) {
        getNextTerm();
        res = parseExpression();
        need( RPAR );
    }
    else if (s_term == NUMBER) {
        res = new Number(s_number);
        getNextTerm();
    }
    else {
        error( "Unexpected symbol %s", s_termUI[s_term] );
        res = NULL;
    }
    return res;
}

static Expr * parseMul ()
{
    Expr *left = parseAtom();
    while (s_term == MUL || s_term == DIV) {
        Term saveTerm = s_term;
        getNextTerm();
        Expr * right = parseAtom();
        if (saveTerm == MUL)
            left = new BinOp(AstCode::Mul, left, right);
        else
            left = new BinOp(AstCode::Div, left, right);
    }
    return left;
}

static Expr * parseAddition ()
{
    Expr *left = parseMul();
    while (s_term == PLUS || s_term == MINUS) {
        Term saveTerm = s_term;
        getNextTerm();
        Expr * right = parseMul();
        if (saveTerm == PLUS)
            left = new BinOp(AstCode::Add, left, right);
        else
            left = new BinOp(AstCode::Sub, left, right);
    }
    return left;
}

static Expr * parseCond ()
{
    Expr * left = parseAddition();
    while (s_term == LT || s_term == GT || s_term == EQ || s_term == NE) {
        Term saveTerm = s_term;
        getNextTerm();
        Expr * right = parseAddition();
        switch (saveTerm) {
            case LT: left = new BinOp(AstCode::LT, left, right); break;
            case GT: left = new BinOp(AstCode::GT, left, right); break;
            case EQ: left = new BinOp(AstCode::EQ, left, right); break;
            case NE: left = new BinOp(AstCode::NE, left, right); break;
        }
    }
    return left;
}
static Expr * parseExpression ()
{
    return parseCond();
}

static If * parseIf ()
{
    need(IF);
    need(LPAR);
    Expr * cond = parseExpression();
    need(RPAR);
    Statement * thenClause = parseStatement();
    Statement * elseClause = NULL;
    if (s_term == ELSE) {
        getNextTerm();
        elseClause = parseStatement();
    }
    return new If(cond, thenClause, elseClause);
}

static While * parseWhile ()
{
    need(WHILE);
    need(LPAR);
    Expr * cond = parseExpression();
    need(RPAR);
    Statement *body = parseStatement();
    return new While(cond, body);
}

static Function * parseFunction ()
{
    need(FN);
    if (s_term != IDENT)
        error( "Identifier expected after 'fn'" );
    std::string name = s_ident;
    getNextTerm();
    need(LPAR);
    std::vector<std::string> params;
    if (s_term != RPAR) {
        if (s_term != IDENT)
            error( "Identifier expected in function parameter list" );
        params.push_back(s_ident);
        getNextTerm();
        while (s_term == COMMA) {
            getNextTerm();
            if (s_term != IDENT)
                error( "Identifier expected in function parameter list" );
            params.push_back(s_ident);
            getNextTerm();
        }
    }
    need(RPAR);
    need(LBRACE);
    Program * body = parseProgram();
    need(RBRACE);
    return new Function(name, std::move(params), body);
}

static Statement * parseStatement ()
{
    Statement * res;
    switch (s_term) {
        case IDENT: {
            auto saveIdent = s_ident;
            getNextTerm();
            if (s_term == LPAR) {
                res = new StatementExpr( parseFunctionCall( saveIdent ) );
            } else {
                need( ASSIGN );
                Expr * value = parseExpression();
                res = new Assign(saveIdent, value);
            }
            need(SEMI);
        }
        break;

        case LBRACE:
            getNextTerm();
            res = parseStatementList();
            need(RBRACE);
            break;

        case IF:
            res = parseIf();
            break;

        case WHILE:
            res = parseWhile();
            break;

        case FN:
            res = parseFunction();
            break;

        case SEMI:
            res = NULL;
            getNextTerm();
            break;

        default:
            error( "Unexpected '%s' at start of statement", s_termUI[s_term] );
    };
    return res;
}

static Return * parseReturn ()
{
    need(RETURN);
    Expr * value = parseExpression();
    need(SEMI);
    return new Return(value);
}

static Block * parseStatementList ()
{
    std::vector<StatementPtr> list;

    while (s_term == IDENT || s_term == LBRACE || s_term == IF || s_term == WHILE || s_term == SEMI || s_term == FN ) {
        Statement * stmt = parseStatement();
        if (stmt)
            list.push_back( StatementPtr(stmt) );
    }

    return new Block( std::move(list) );
}

static Program * parseProgram ()
{
    Block * body = parseStatementList();
    Return * ret = parseReturn();
    return new Program( body, ret );
}

struct NativeFunction : public Function
{
    long (* const fn)(Env & env, const std::vector<ExprPtr> & args);
    NativeFunction(const char * name, long (*fn)(Env & env, const std::vector<ExprPtr> & args)) :
            Function( name, std::vector<std::string>(), NULL ), fn(fn) {}

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Natuve Function %s (", name.c_str() );
        for ( auto it = params.begin(); it != params.end(); ++it ) {
            if (it != params.begin())
                printf( ", " );
            printf( "%s", it->c_str() );
        }
        printf( ")\n" );
    }

    virtual long eval ( Env & env )
    {
        env.funcs[name] = this;
        return 0;
    }

    virtual long call ( Env & env, const std::vector<ExprPtr> & args )
    {
        return fn( env, args );
    }
};

void registerNativeFunction ( Env & env, const char * name, long (*fn)(Env & env, const std::vector<ExprPtr> & args) )
{
    NativeFunction * n = new NativeFunction( name, fn );
    n->eval( env );
}

static long print ( Env & env, const std::vector<ExprPtr> & args )
{
    for ( auto it = args.begin(); it != args.end(); ++it ) {
        if (it != args.begin())
            printf( ", " );
        printf( "%ld", (*it)->eval( env ) );
    }
    printf( "\n" );
    return 0;
}

int main ()
{
    initParser();
    Program * prog = parseProgram();
    prog->print(0);

    Env env(NULL);
    registerNativeFunction( env, "print", print );

    long result = prog->eval( env );
    for ( const auto & var : env.vars )
        printf( "%s = %ld\n", var.first.c_str(), var.second );
    printf( "\nReturned result: %ld\n", result );
    return 0;
}

