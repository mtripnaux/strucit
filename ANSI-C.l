D                       [0-9]
L                       [a-zA-Z_]
% {
#include <stdio.h>
#include "y.tab.h"

/* Fonction pour compter les lignes (utile pour les erreurs) */
int line_count = 1;

void count();
% }

%%
"/*"                    { comment(); } // Gestion des commentaires
"//"[^\n]* { /* Commentaire sur une seule ligne */ }

"else"                  { count(); return(ELSE); }
"extern"                { count(); return(EXTERN); }
"for"                   { count(); return(FOR); }
"goto"                  { count(); return(GOTO); }
"if"                    { count(); return(IF); }
"int"                   { count(); return(INT); }
"return"                { count(); return(RETURN); }
"sizeof"                { count(); return(SIZEOF); }
"struct"                { count(); return(STRUCT); }
"void"                  { count(); return(VOID); }
"while"                 { count(); return(WHILE); }

{L}({L}|{D})* { count(); yylval.sval = strdup(yytext); return(IDENTIFIER); }
{D}+                    { count(); yylval.ival = atoi(yytext); return(CONSTANT); }

"->"                    { count(); return(PTR_OP); }
"&&"                    { count(); return(AND_OP); }
"||"                    { count(); return(OR_OP); }
"<="                    { count(); return(LE_OP); }
">="                    { count(); return(GE_OP); }
"=="                    { count(); return(EQ_OP); }
"!="                    { count(); return(NE_OP); }
";"                     { count(); return(';'); }
"{"                     { count(); return('{'); }
"}"                     { count(); return('}'); }
","                     { count(); return(','); }
":"                     { count(); return(':'); }
"="                     { count(); return('='); }
"("                     { count(); return('('); }
")"                     { count(); return(')'); }
"."                     { count(); return('.'); }
"&"                     { count(); return('&'); }
"!"                     { count(); return('!'); }
"-"                     { count(); return('-'); }
"+"                     { count(); return('+'); }
"*"                     { count(); return('*'); }
"/"                     { count(); return('/'); }
"<"                     { count(); return('<'); }
">"                     { count(); return('>'); }

[ \t\v\f]               { count(); }
\n                      { count(); line_count++; }
.                       { /* Ignorer les caracteres inconnus */ }

%%

int yywrap() {
    return(1);
}

/* Fonction pour sauter les commentaires multi-lignes */
void comment() {
    char c, c1;
loop:
    while ((c = input()) != '*' && c != 0);
    if ((c1 = input()) != '/' && c != 0) {
        unput(c1);
        goto loop;
    }
}


