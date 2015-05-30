#ifndef CALC_AST_H
#define CALC_AST_H

#include <string>
#include <vector>
#include <memory>
#include <stdio.h>

#define AST_CODES \
  _ACODE(Number) \
  _ACODE(Ident) \
  _ACODE(Expr) \
  _ACODE(BinOp) \
  _ACODE(Return) \
  _ACODE(If) \
  _ACODE(Assign) \
  _ACODE(Block) \
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
};
typedef std::shared_ptr<Ast> AstPtr;

struct Expr : public Ast
{
    Expr(const AstCode::T &code) : Ast(code) { }
};

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
};

struct AstVisitor
{
    virtual void visitNumber ( Number * );
    virtual void visitIdent ( Ident * );
};

#endif //CALC_AST_H
