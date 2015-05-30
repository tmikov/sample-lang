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
    TERM(EQ,"==")\
    TERM(NE,"!=")\
    TERM(LT,"<")\
    TERM(GT,">")\
    TERM(LBRACE,"{")\
    TERM(RBRACE,"}")\
    TERM(NUMBER,"number") \
    TERM(SEMI,";") \
    TERM(ASSIGN,"=") \
    TERM(IF,"if") \
    TERM(ELSE,"else") \
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
    s_kw["if"] = IF;
    s_kw["else"] = ELSE;
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


static long s_result;
static bool s_performOperations;
static std::map<std::string,long> s_vars;

static void parseExpression ();
static void parseStatementList ();
static void parseStatement ();
static void parseIf ();

void initParser ()
{
    initScanner();
    getNextTerm();
    s_performOperations = true;
}
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
        getNextTerm();
    }
    else if (s_term == LPAR) {
        getNextTerm();
        parseExpression();
        need( RPAR );
    }
    else if (s_term == NUMBER) {
        s_result = s_number;
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
            tmp *= s_result;
        else
            tmp /= s_result;
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
            tmp += s_result;
        else
            tmp -= s_result;
    }
    s_result = tmp;
}

static void parseCond ()
{
    parseAddition();
    long tmp = s_result;
    while (s_term == LT || s_term == GT || s_term == EQ || s_term == NE) {
        Term saveTerm = s_term;
        getNextTerm();
        parseAddition();
        switch (saveTerm) {
            case LT: tmp = tmp < s_result; break;
            case GT: tmp = tmp > s_result; break;
            case EQ: tmp = tmp == s_result; break;
            case NE: tmp = tmp != s_result; break;
        }
    }
    s_result = tmp;
}
static void parseExpression ()
{
    parseCond();
}

static void parseIf ()
{
    need(IF);
    need(LPAR);
    parseExpression();
    long cond = s_result;
    bool perform = s_performOperations;
    need(RPAR);
    s_performOperations = perform & (cond != 0);
    parseStatement();
    if (s_term == ELSE) {
        s_performOperations = perform & (cond == 0);
        getNextTerm();
        parseStatement();
    }
    s_performOperations = perform;
}

static void parseStatement ()
{
    switch (s_term) {
        case IDENT: {
            auto saveIdent = s_ident;
            getNextTerm();
            need( ASSIGN );
            parseExpression();
            if (s_performOperations)
                s_vars[saveIdent] = s_result;
            need(SEMI);
        }
        break;

        case LBRACE:
            getNextTerm();
            parseStatementList();
            need(RBRACE);
            break;

        case IF:
            parseIf();
            break;

        case SEMI:
            getNextTerm();
            break;

        default:
            error( "Unexpected '%s' at start of statement", s_termUI[s_term] );
    };
}

static void parseReturn ()
{
    need(RETURN);
    parseExpression();
    need(SEMI);
}

static void parseStatementList ()
{
    while (s_term == IDENT || s_term == LBRACE || s_term == IF || s_term == SEMI) {
        parseStatement();
    }
}

static void parseProgram ()
{
    parseStatementList();
    parseReturn();
}

int main ()
{
    initParser();
    parseProgram();
    for ( const auto & v : s_vars ) {
        printf( "%s = %ld\n", v.first.c_str(), v.second );
    }
    printf( "Result: %ld\n", s_result );
    return 0;
}

