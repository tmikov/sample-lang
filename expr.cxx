#include <stdio.h>
#include <ctype.h>
#include <string>
#include <stdlib.h>
#include <stdarg.h>
#include <map>


#define TERMS \
    TERM(_EOF,"<end of file>")\
    TERM(IDENT,"identifier")\
    TERM(PLUS,"+")\
    TERM(MINUS,"-")\
    TERM(MUL,"*")\
    TERM(DIV,"/")\
    TERM(LPAR,"(")\
    TERM(RPAR,")")\
    TERM(NUMBER,"number") \
    TERM(SEMI,";") \
    TERM(ASSIGN,"=") \
    TERM(RETURN,"return")


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
    s_line = 1; s_col = 0;
    s_nextCh = nextChar();
}

static void saveStart ()
{
    s_startLine = s_line;
    s_startCol = s_col;
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
            return s_term = ASSIGN;
        }
        else if (s_nextCh == ';') {
            s_nextCh = nextChar();
            return s_term = SEMI;
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

void initParser ()
{
    initScanner();
    getNextTerm();
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

static long s_result;
static std::map<std::string,long> s_vars;

static void parseExpression ();

static void need ( Term term )
{
    if (s_term != term)
        error( "Expected %s", s_termUI[term] );
    getNextTerm();
}

static void parseAtom ()
{
    if (s_term == IDENT) {
        auto it = s_vars.find( s_ident );
        if (it == s_vars.end())
            error( "Undefined variable %s", s_ident.c_str() );
        s_result = it->second;
        printf( "push %s\n", it->first.c_str() );
        getNextTerm();
    }
    else if (s_term == LPAR) {
        getNextTerm();
        parseExpression();
        need( RPAR );
    }
    else if (s_term == NUMBER) {
        s_result = s_number;
        printf( "push %ld\n", s_number );
        getNextTerm();
    } else
        error( "Unexpected symbol %s", s_termUI[s_term] );
}

static void parseMul ()
{
    parseAtom();
    long tmp = s_result;
    while (s_term == MUL || s_term == DIV) {
        Term saveTerm = s_term;
        getNextTerm();
        parseAtom();
        if (saveTerm == MUL)
            printf( "pop eax\npop ebx\nmul ebx\npush eax\n" );
        else
            printf( "pop eax\npop ebx\nidiv ebx\npush eax\n" );
    }
    s_result = tmp;
}

static void parseAddition ()
{
    parseMul();
    long tmp = s_result;
    while (s_term == PLUS || s_term == MINUS) {
        Term saveTerm = s_term;
        getNextTerm();
        parseMul();
        if (saveTerm == PLUS)
            printf( "pop eax\nadd [esp],eax\n" );
        else
            printf( "pop eax\nsub [esp],eax\n" );
    }
    s_result = tmp;
}

static void parseExpression ()
{
    parseAddition();
}

static void parseStatement ()
{
    if (s_term != IDENT)
        error( "Identifier expected at start of statement instead of '%s'", s_termUI[s_term] );
    auto saveIdent = s_ident;
    getNextTerm();
    need( ASSIGN );
    parseExpression();
    s_vars[saveIdent] = s_result;
    printf( "pop $%s\n", saveIdent.c_str() );
}

static void parseReturn ()
{
    need(RETURN);
    parseExpression();
    need(SEMI);
    printf( "ret\n" );
}

static void parseProgram ()
{
    while (s_term != _EOF && s_term != RETURN) {
        parseStatement();
        need( SEMI );
    }
    parseReturn();
}

int main ()
{
    initParser();
    parseProgram();
//    for ( const auto & v : s_vars ) {
//        printf( "%s = %ld\n", v.first.c_str(), v.second );
//    }
//    printf( "Result: %ld\n", s_result );
    return 0;
}

