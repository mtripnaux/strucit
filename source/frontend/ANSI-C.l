%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "y.tab.h"

extern Ast_node *create_int_leaf(int val);
extern Ast_node *create_id_leaf(char *name);
%}

id      [a-zA-Z][a-zA-Z0-9_]*
const   0|[1-9][0-9]*
comment \/\*([^*]|\*+[^/*])*\*+\/

%%

{comment}       { }

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

{const}         { yylval.node = create_int_leaf(atoi(yytext)); return CONSTANT; }
{id}            { yylval.node = create_id_leaf(yytext); return IDENTIFIER; }

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
";"             { return ';'; }
","             { return ','; }
"."             { return '.'; }

[ \t\v\n\f]     { }

.               { fprintf(stderr, "Erreur lexicale : '%s' ligne %d\n", yytext, yylineno); }

%%
