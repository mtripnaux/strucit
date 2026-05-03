%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h" // Généré par Yacc/Bison
%}

id      [a-zA-Z_][a-zA-Z0-9_]*
const   0|[1-9][0-9]*
comment \/\*([^*]|\*+[^/*])*\*+\/

%%

{comment}       { /* Ignorer les commentaires */ }

/* Mots-clés */
"extern"        { return EXTERN; }
"int"           { return INT; }
"void"          { return VOID; }
"goto"          { return GOTO; }
"if"            { return IF; }
"else"          { return ELSE; }   
"while"         { return WHILE; }   
"for"           { return FOR; }    
"return"        { return RETURN; }

/* Opérateurs complexes */
"<="            { return LE_OP; }
">="            { return GE_OP; }
"=="            { return EQ_OP; }
"!="            { return NE_OP; }
"&&"            { return AND_OP; }
"||"            { return OR_OP; }

/* Identifiants et Constantes */
{id}            { 
                  yylval.id = strdup(yytext); // On transmet le nom au Parser
                  return IDENTIFIER; 
                }

{const}         { 
                  yylval.value = atoi(yytext); // On transmet la valeur au Parser
                  return CONSTANT; 
                }

/* Espaces et sauts de ligne (ignorés) */
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

/* Erreur par défaut */
.               { fprintf(stderr, "Erreur lexicale : caractère '%s' inconnu à la ligne %d\n", yytext, yylineno); }

%%
