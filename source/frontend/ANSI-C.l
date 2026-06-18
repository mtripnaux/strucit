%option noyywrap nounput noinput batch
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "strucitfe.tab.h"

extern Ast_node *create_constant_leaf(int val);
extern Ast_node *create_identifier_leaf(char *name);
%}

id      [a-zA-Z][a-zA-Z0-9_]*
const   0|[1-9][0-9]*
comment \/\*([^*]|\*+[^/*])*\*+\/

%%

{comment}       { /* commentaires ignorés */ }

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

    /* Les mots-cles ci-dessus sont testés avant la regle generique {id} :
    flex priorise toujours la regle la plus longue, et en cas d'egalite
    (un mot-cle EST un identifiant valide), la regle ecrite en premier dans
    le fichier. C'est ce qui garantit qu'un identifiant ne peut jamais
    porter le nom d'un mot-cle reserve, sans avoir besoin de le verifier
    ailleurs. */

"&&"            { return AND_OP; }
"||"            { return OR_OP; }
"<="            { return LE_OP; }
">="            { return GE_OP; }
"=="            { return EQ_OP; }
"!="            { return NE_OP; }
"->"            { return PTR_OP; }


{const}         { yylval.node = create_constant_leaf(atoi(yytext)); 
                  yylval.node->line = yylineno; return CONSTANT; }
{id}            { yylval.node = create_identifier_leaf(yytext); 
                yylval.node->line = yylineno; return IDENTIFIER; }

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

[ \t\v\n\r\f]   { /* espaces : ignores */ }

.               { fprintf(stderr, "Erreur lexicale : '%s' ligne %d\n", yytext, yylineno); }

%%
