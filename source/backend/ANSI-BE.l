%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structbe.tab.h"
%}

%%

"/*"([^*]|\*+[^*/])*\*+"/"  { /* Ignorer les commentaires */ }

"extern"  { return EXTERN; }
"int"     { return INT; }
"void"    { return VOID; }
"goto"    { return GOTO; }
"if"      { return IF; }
"return"  { return RETURN; }

"<="      { return LE_OP; }
">="      { return GE_OP; }
"=="      { return EQ_OP; }
"!="      { return NE_OP; }
"&&"      { return AND_OP; }
"||"      { return OR_OP; }

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.id = strdup(yytext); return IDENTIFIER; }

0|[1-9][0-9]*           { yylval.value = atoi(yytext); return CONSTANT; }

[ \t\v\n\r\f]   { }

"*"       { return '*'; }
"/"       { return '/'; }
"+"       { return '+'; }
"-"       { return '-'; }
"<"       { return '<'; }
">"       { return '>'; }
"="       { return '='; }
"("       { return '('; }
")"       { return ')'; }
"{"       { return '{'; }
"}"       { return '}'; }
";"       { return ';'; }
":"       { return ':'; }
","       { return ','; }

.               { fprintf(stderr, "Erreur lexicale : caractère '%s' inconnu à la ligne %d\n", yytext, yylineno); }

%%
