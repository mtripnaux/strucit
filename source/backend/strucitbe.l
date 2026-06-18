/*
* Difference notable avec le frontend : l'identifiant accepte un '_' en
* tete ([a-zA-Z_]... au lieu de [a-zA-Z]...). C'est volontaire : codegen.c
* nomme ses temporaires "_temp_0", "_temp_1", etc. Comme un identifiant 
* utilisateur ne peut jamais commencer par '_', ce choix ne cree aucune
* ambiguite ni collision possible.
*/

%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strucitbe.tab.h"
#include "erreurs.h"
%}

%%

"/*"([^*]|\*+[^*/])*\*+"/"  { /* commentaires ignorés */ }

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

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.id = strdup(yytext); return IDENTIFIER; }

0|[1-9][0-9]*           { yylval.value = atoi(yytext); return CONSTANT; }

[ \t\v\n\r\f]   { /* espaces ignorés */ }

"+"       { return '+'; }
"-"       { return '-'; }
"*"       { return '*'; }
"/"       { return '/'; }
"&"       { return '&'; }
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

.               { erreur_lexicale(yylineno, yytext); }

%%
