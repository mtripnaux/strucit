%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/Semantic_Analysis/ast.h"
#include "y.tab.h"

/* On indique à Lex que ces fonctions existent dans ast.c */
extern Ast_node *create_int_leaf(int val);
extern Ast_node *create_id_leaf(char* name);
%}

id    [a-zA-Z_][0-9a-zA-Z_]*
const 0|[1-9][0-9]*
comment \/\*([^*]|\*+[^/*])*\*+\/

%%

{comment}       { /* Ignorer les commentaires */ }

"extern"        { return EXTERN; }
"int"           { return INT; }
"void"          { return VOID; }
"struct"        { return STRUCT; }
"sizeof"        { return SIZEOF; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"return"        { return RETURN; }

"&&"            { return AND_OP; }
"||"            { return OR_OP; }
"<="            { return LE_OP; }
">="            { return GE_OP; }
"=="            { return EQ_OP; }
"!="            { return NE_OP; }
"->"            { return PTR_OP; }
"<<"            { return LEFT_OP; }
">>"            { return RIGHT_OP; }
"++"            { return INC_OP; }
"--"            { return DEC_OP; }

{const}         { yylval.node = create_int_leaf(atoi(yytext)); return CONSTANT; }
{id}            { yylval.node = create_id_leaf(strdup(yytext)); return IDENTIFIER; }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"&"             { return '&'; }
"="             { return '='; }
"<"             { return '<'; }
">"             { return '>'; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
"["             { return '['; }
"]"             { return ']'; }
";"             { return ';'; }
","             { return ','; }
"."             { return '.'; }

[ \t\v\n\f]     { /* Ignorer les espaces et sauts de ligne */ }

.               { printf("Caractère inconnu : %s à la ligne %d\n", yytext, yylineno); }

%%
