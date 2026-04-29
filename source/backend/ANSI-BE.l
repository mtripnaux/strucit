%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"
%}

id      [a-zA-Z_][a-zA-Z0-9_]*
const   0|[1-9][0-9]*
comment \/\*([^*]|\*+[^/*])*\*+\/

%%

{comment}       { }

"extern"        { return EXTERN; }
"int"           { return INT; }
"void"          { return VOID; }
"goto"          { return GOTO; }
"if"            { return IF; }
"return"        { return RETURN; }

"<="            { return LE_OP; }
">="            { return GE_OP; }
"=="            { return EQ_OP; }
"!="            { return NE_OP; }

{id}            { return IDENTIFIER; }
{const}         { return CONSTANT; }

[ \t\v\n\r\f]   { }

"*"             { return '*'; }
"/"             { return '/'; }
"+"             { return '+'; }
"-"             { return '-'; }
"<"             { return '<'; }
">"             { return '>'; }
"&"             { return '&'; }
"="             { return '='; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
";"             { return ';'; }
":"             { return ':'; }
","             { return ','; }

.               { fprintf(stderr, "Erreur lexicale backend : '%s' ligne %d\n", yytext, yylineno); }

%%
