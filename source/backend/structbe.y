%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
extern int yylineno;
extern FILE *yyin;
int yylex();

void yyerror(const char *s) {
    fprintf(stderr, "\033[1;31mErreur syntaxique : %s à la ligne %d\033[0m\n", s, yylineno);
    exit(1);
}
%}

%union {
    int   value;
    char *id;
}

%define parse.error verbose

%token <id>    IDENTIFIER
%token <value> CONSTANT
%token LE_OP GE_OP EQ_OP NE_OP LSHIFT_OP RSHIFT_OP
%token EXTERN INT VOID IF RETURN GOTO

%right '='

%start program

%%

primary_expression
    : IDENTIFIER
    | CONSTANT
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
    : '&'
    | '*'
    | '-'
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
    | primary_expression LSHIFT_OP primary_expression
    | primary_expression RSHIFT_OP primary_expression
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
    | unary_operator primary_expression '=' primary_expression
    | primary_expression '=' additive_expression
    ;

declaration
    : declaration_specifiers declarator ';'
    ;

declaration_specifiers
    : EXTERN type_specifier
    | type_specifier
    ;

type_specifier
    : VOID
    | INT
    ;

declarator
    : '*' direct_declarator
    | direct_declarator
    ;

direct_declarator
    : IDENTIFIER
    | direct_declarator '(' parameter_list ')'
    | direct_declarator '(' VOID ')'
    | direct_declarator '(' ')'
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

parameter_declaration
    : declaration_specifiers declarator
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
    | '{' declaration_list '}'
    | '{' declaration_list statement_list '}'
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

statement_list
    : statement
    | statement_list statement
    ;

labeled_statement
    : IDENTIFIER ':' statement
    ;

expression_statement
    : ';'
    | expression ';'
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

function_definition
    : declaration_specifiers declarator compound_statement
    ;

%%

int main(int argc, char **argv)
{
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Erreur d'ouverture du fichier source");
            return 1;
        }
    }

    if (yyparse() == 0) {
        printf("\033[0;32mAnalyse syntaxique backend : OK\033[0m\n");
    }

    if (argc > 1) fclose(yyin);
    return 0;
}
