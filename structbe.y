%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"

extern int yylineno;
int yylex();

int yyerror(const char *s ) {
    fprintf(stderr, "Erreur Backend : %s à la ligne %d\n", s, yylineno);
    exit(1);
}
%}

%define parse.error verbose

%token IDENTIFIER CONSTANT
%token LE_OP GE_OP EQ_OP NE_OP
%token EXTERN
%token INT VOID
%token IF RETURN GOTO

%start program

%%

primary_expression
        : IDENTIFIER
        | CONSTANT
        | '(' expression ')'
        ;

postfix_expression
        : primary_expression
        | postfix_expression '(' ')'
        | postfix_expression '(' argument_expression_list ')'
        ;

argument_expression_list
        : primary_expression
        | argument_expression_list ',' primary_expression
        ;

unary_expression
        : postfix_expression
        | unary_operator primary_expression
        ;

unary_operator
        : '&' | '*' | '-'
        ;

multiplicative_expression
        : unary_expression
        | primary_expression '*' primary_expression
        | primary_expression '/' primary_expression
        ;

additive_expression
        : multiplicative_expression
        | primary_expression '+' primary_expression
        | primary_expression '-' primary_expression
        ;

relational_expression
        : additive_expression
        | primary_expression '<' primary_expression
        | primary_expression '>' primary_expression
        | primary_expression LE_OP primary_expression
        | primary_expression GE_OP primary_expression
        ;

equality_expression
        : relational_expression
        | primary_expression EQ_OP primary_expression
        | primary_expression NE_OP primary_expression
        ;

expression
        : equality_expression
        | unary_expression '=' expression
        ;

statement
        : compound_statement
        | labeled_statement
        | expression_statement
        | selection_statement
        | jump_statement 
        ;

compound_statement
        : '{' '}'
        | '{' statement_list '}'
        ;

labeled_statement
        : IDENTIFIER ':' statement
        ;

selection_statement
        : IF '(' equality_expression ')' GOTO IDENTIFIER ';'
        ;

jump_statement
        : RETURN ';'
        | RETURN expression ';'
        | GOTO IDENTIFIER ';'
        ;

program
        : external_declaration
        | program external_declaration
        ;

external_declaration
        : function_definition
        | declaration
        ;

/* On réutilise les règles standards pour les fonctions et déclarations */
function_definition
        : type_specifier declarator compound_statement
        ;

type_specifier : INT | VOID ;
declarator : '*' IDENTIFIER | IDENTIFIER ;
declaration : type_specifier IDENTIFIER ';' ;
statement_list : statement | statement_list statement ;

%%

int main() {
    yyparse();
    return 0;
}
