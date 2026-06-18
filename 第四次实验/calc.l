%{
#include <stdio.h>
#include <stdlib.h>
#include "calc.tab.h"

extern int yylval;
%}

%%

[ \t\r\n]+        { /* skip whitespace */ }
[0-9]+            { yylval = atoi(yytext); return NUM; }
"+"               { return PLUS; }
"-"               { return MINUS; }
"*"               { return TIMES; }
"/"               { return DIVIDE; }
"("               { return LPAREN; }
")"               { return RPAREN; }
.                 { /* skip unknown */ }

%%

int yywrap(void) { return 1; }
