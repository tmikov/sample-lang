%token<numval> IDENT "<ident>"
%token PLUS "+"
%token MINUS "-"
%token LPAR "("
%token RPAR ")"
%token MUL "*"
%token DIV "/" 
%token<numval> NUMBER "number"
%token SEMI ";"
%token ASSIGN "="
%token RETURN "return"

%type<numval> expression addition mul atom

%%

program:
    statements_opt return
  ;

statements_opt:
    %empty
  | statements_opt statement ";"
  ;

statement:
    IDENT "=" expression
 ;

return: RETURN expression ";"

expression: addition ;

addition:
    addition[a] "+" mul { $$ = $a + $mul; }
  | addition[a] "-" mul { $$ = $a - $mul; }
  | mul
  ;

mul:
    mul[m] "*" atom     { $$ = $m * $atom; }
  | mul[m] "/" atom     { $$ = $m / $atom; }
  | atom
  ;

atom:
    IDENT
  | NUMBER
  | "(" expression ")" { $$ = $expression; }
  ;

%%
