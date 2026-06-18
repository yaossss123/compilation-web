%{
#include <stdio.h>
#include <stdlib.h>

extern int yylex(void);
void yyerror(const char *s);
%}

%token NUM
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN

%left PLUS MINUS
%left TIMES DIVIDE

%%

program:
    expr               { printf("[bison] result = %d\n", $1); }
  ;

expr:
    NUM                { $$ = $1; printf("[bison] reduce NUM -> %d\n", $1); }
  | expr PLUS expr     { $$ = $1 + $3; printf("[bison] reduce %d + %d = %d\n", $1, $3, $$); }
  | expr MINUS expr    { $$ = $1 - $3; printf("[bison] reduce %d - %d = %d\n", $1, $3, $$); }
  | expr TIMES expr    { $$ = $1 * $3; printf("[bison] reduce %d * %d = %d\n", $1, $3, $$); }
  | expr DIVIDE expr   { if ($3==0){yyerror("divide by zero");$$=0;} else {$$=$1/$3; printf("[bison] reduce %d / %d = %d\n",$1,$3,$$);} }
  | LPAREN expr RPAREN { $$ = $2; printf("[bison] reduce (expr) = %d\n", $$); }
  ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "[bison error] %s\n", s);
}

int main(void) {
    printf("=== flex/bison calculator ===\n");
    printf("input: ");
    yyparse();
    return 0;
}
