#ifndef CALC_AST_H
#define CALC_AST_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdio.h>
#include <assert.h>

void runtimeError ( const char * msg, ... );

struct Function;

struct Env
{
    Env * const parent;
    std::map<std::string,long> vars;
    std::map<std::string,Function*> funcs;

    Env(Env *const parent) : parent(parent) { }

    long getVar ( const std::string name )
    {
        auto it = vars.find( name );
        if (it != vars.end())
            return it->second;
        if (parent)
            return parent->getVar(name);
        runtimeError( "Undefined variable %s", name.c_str() );
        return 0;
    }

    Function * getFunc ( const std::string name )
    {
        auto it = funcs.find( name );
        if (it != funcs.end())
            return it->second;
        if (parent)
            return parent->getFunc(name);
        runtimeError( "Undefined function %s", name.c_str() );
        return 0;
    }
};

#define AST_CODES \
  _ACODE(Number) \
  _ACODE(Ident) \
  _ACODE(FunctionCall) \
  _ACODE(Expr) \
  _ACODE(BinOp) \
  _ACODE(Return) \
  _ACODE(If) \
  _ACODE(While) \
  _ACODE(Assign) \
  _ACODE(Block) \
  _ACODE(Function) \
  _ACODE(Program) \
  _ACODE(Mul) _ACODE(Div) _ACODE(Add) _ACODE(Sub) _ACODE(LT) _ACODE(GT) _ACODE(EQ) _ACODE(NE)

struct AstCode {
#define _ACODE(t) t,
enum T { AST_CODES };
#undef _ACODE
};

extern const char * const AstCodeNames[];

#define INDENT_STEP 4

inline void printIndent ( int indent )
{
    printf( "%*s", indent, "" );
}


struct Ast
{
    const AstCode::T code;
    Ast(AstCode::T code) : code(code) { }

    virtual void print ( int indent ) = 0;
    virtual long eval ( Env & env ) = 0;
};
typedef std::shared_ptr<Ast> AstPtr;

struct Expr : public Ast
{
    Expr(const AstCode::T &code) : Ast(code) { }
};
typedef std::shared_ptr<Expr> ExprPtr;

struct Atom : public Expr
{
    Atom(AstCode::T code) : Expr(code) { }
};

struct Number : public Atom
{
    const long value;
    Number(const long value) : Atom(AstCode::Number), value(value) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Number: %ld\n", value );
    }
    virtual long eval ( Env & )
    {
        return value;
    }
};

struct Ident : public Atom
{
    const std::string name;
    Ident(const std::string &name) : Atom(AstCode::Ident), name(name) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Ident: %s\n", name.c_str() );
    }
    virtual long eval ( Env & env )
    {
        return env.getVar( name );
    }
};

struct BinOp : public Expr
{
    Expr * const left;
    Expr * const right;

    BinOp(const AstCode::T &code, Expr *const left, Expr *const right) : Expr(code), left(left), right(right) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "BinOp: %s\n", AstCodeNames[code] );
        left->print( indent + INDENT_STEP );
        right->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        long l = left->eval( env );
        long r = right->eval( env );
        switch (code)
        {
            case AstCode::Add: return l + r;
            case AstCode::Sub: return l - r;
            case AstCode::Mul: return l * r;
            case AstCode::Div: return l / r;
            case AstCode::LT: return l < r;
            case AstCode::GT: return l > r;
            case AstCode::EQ: return l == r;
            case AstCode::NE: return l != r;
        }
        assert( false );
    }
};

struct Return : public Ast
{
    Expr * const value;

    Return(Expr *const value) : Ast(AstCode::Return), value(value) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Return\n" );
        value->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        return value->eval( env );
    }
};

struct Statement : public Ast
{
    Statement(const AstCode::T &code) : Ast(code) { }
};
typedef std::shared_ptr<Statement> StatementPtr;

struct If : public Statement
{
    Expr * const cond;
    Statement * const thenClause;
    Statement * const elseClause;

    If(Expr *const cond, Statement *const thenClause, Statement *const elseClause) :
       Statement(AstCode::If), cond(cond), thenClause(thenClause), elseClause(elseClause) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "If\n" );
        cond->print( indent + INDENT_STEP );
        thenClause->print( indent + INDENT_STEP );
        if (elseClause)
            elseClause->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        if (cond->eval(env))
           return thenClause->eval( env );
        else if (elseClause)
            return elseClause->eval( env );
        else
            return 0;
    }
};

struct While : public Statement
{
    Expr * const cond;
    Statement * const body;

    While(Expr *const cond, Statement *const body) :
            Statement(AstCode::If), cond(cond), body(body) {}

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "While\n" );
        cond->print( indent + INDENT_STEP );
        body->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        long result = 0;
        while (cond->eval(env))
            result = body->eval( env );
        return result;
    }
};
struct Assign : public Statement
{
    const std::string name;
    Expr * const value;

    Assign(const std::string &name, Expr *const value) : Statement(AstCode::Assign), name(name), value(value) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Assign %s\n", name.c_str() );
        value->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        long v = value->eval( env );
        env.vars[name] = v;
        return v;
    }
};

struct Block : public Statement
{
    std::vector<StatementPtr> list;
    Block ( std::vector<StatementPtr> && aList ) : Statement(AstCode::Block), list(aList) { };

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Block\n" );
        for ( const auto & sp : list )
            sp->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        long result = 0;
        for ( const auto & sp : list )
            result = sp->eval( env );
        return result;
    }
};

struct Program : public Ast
{
    Block * const body;
    Return * const returnStmt;

    Program(Block *const body, Return *const returnStmt) :
            Ast(AstCode::Program), body(body), returnStmt(returnStmt) { }

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Program\n" );
        body->print(indent + INDENT_STEP);
        returnStmt->print(indent + INDENT_STEP);
    }

    virtual long eval ( Env & env )
    {
        body->eval( env );
        return returnStmt->eval( env );
    }
};

struct Function : public Statement
{
    const std::string name;
    std::vector<std::string> params;
    Program * const body;

    Function ( const std::string & name, std::vector<std::string> && params, Program * body ) :
        Statement(AstCode::Function), name(name), params(params), body(body) {};

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "Function %s (", name.c_str() );
        for ( auto it = params.begin(); it != params.end(); ++it ) {
            if (it != params.begin())
                printf( ", " );
            printf( "%s", it->c_str() );
        }
        printf( ")\n" );
        body->print( indent + INDENT_STEP );
    }

    virtual long eval ( Env & env )
    {
        env.funcs[name] = this;
        return 0;
    }

    virtual long call ( Env & env, const std::vector<ExprPtr> & args )
    {
        long result;
        Env funcEnv( &env );
        for ( int i = 0, e = params.size(); i < e; ++i ) {
           long v = i < args.size() ? args[i]->eval( env ) : 0;
           funcEnv.vars[params[i]] = v;
        }
        return body->eval( funcEnv );
    }
};

struct FunctionCall : public Atom
{
    const std::string name;
    const std::vector<ExprPtr> args;
    FunctionCall ( const std::string & name, std::vector<ExprPtr> && args ) :
            Atom(AstCode::FunctionCall), name(name), args(args) {}

    virtual void print ( int indent )
    {
        printIndent(indent);
        printf( "call %s\n", name.c_str() );
        for ( const auto & a : args )
            a->print( indent + INDENT_STEP );
    }
    virtual long eval ( Env & env )
    {
        return env.getFunc( name )->call( env, args );
    }
};


struct AstVisitor
{
    virtual void visitNumber ( Number * );
    virtual void visitIdent ( Ident * );
};

#endif //CALC_AST_H
