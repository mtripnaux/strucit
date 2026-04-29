%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"

extern int yylineno;
extern FILE *yyin;
int yylex();

void yyerror(const char *s) {
    fprintf(stderr, "\033[0;31mErreur syntaxique backend : %s ligne %d\033[0m\n", s, yylineno);
    exit(1);
}
%}

%define parse.error verbose

%token IDENTIFIER CONSTANT
%token LE_OP GE_OP EQ_OP NE_OP
%token EXTERN INT VOID IF RETURN GOTO

%right '='
%left EQ_OP NE_OP
%left '<' '>' LE_OP GE_OP
%left '+' '-'
%left '*' '/'
%right UNARY
%left POSTFIX '('

%start program

%%

/* ===== Programme ===== */

program
    : external_declaration
    | program external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    | extern_declaration
    ;

/* ===== Types et déclarateurs ===== */

type_specifier
    : INT
    | VOID
    ;

declarator
    : '*' direct_declarator
    | direct_declarator
    ;

direct_declarator
    : IDENTIFIER
    | direct_declarator '(' ')'
    | direct_declarator '(' param_list ')'
    ;

param_list
    : param_declaration
    | param_list ',' param_declaration
    ;

param_declaration
    : type_specifier declarator
    ;

/* ===== Déclarations ===== */

extern_declaration
    : EXTERN type_specifier declarator ';'
    ;

declaration
    : type_specifier declarator ';'
    ;

/* ===== Définition de fonction ===== */

function_definition
    : type_specifier declarator compound_statement
    ;

/* ===== Blocs ===== */

compound_statement
    : '{' '}'
    | '{' local_decl_list '}'
    | '{' statement_list '}'
    | '{' local_decl_list statement_list '}'
    ;

local_decl_list
    : local_decl
    | local_decl_list local_decl
    ;

local_decl
    : type_specifier declarator ';'
    ;

/* ===== Instructions ===== */

statement_list
    : statement
    | statement_list statement
    ;

statement
    : compound_statement
    | labeled_statement
    | expression_statement
    | selection_statement
    | jump_statement
    ;

labeled_statement
    : IDENTIFIER ':' statement
    ;

selection_statement
    : IF '(' condition ')' GOTO IDENTIFIER ';'
    ;

jump_statement
    : RETURN ';'
    | RETURN primary_expression ';'
    | GOTO IDENTIFIER ';'
    ;

expression_statement
    : expression ';'
    | ';'
    ;

/* ===== Conditions (sans && ni ||) ===== */

condition
    : primary_expression EQ_OP primary_expression
    | primary_expression NE_OP primary_expression
    | primary_expression '<'   primary_expression
    | primary_expression '>'   primary_expression
    | primary_expression LE_OP primary_expression
    | primary_expression GE_OP primary_expression
    ;

/* ===== Expressions (grammaire plate, précédences déclarées) ===== */

primary_expression
    : IDENTIFIER
    | CONSTANT
    ;

expression
    : IDENTIFIER
    | CONSTANT
    | '(' expression ')'
    | expression '=' expression
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression EQ_OP expression
    | expression NE_OP expression
    | expression '<'   expression
    | expression '>'   expression
    | expression LE_OP expression
    | expression GE_OP expression
    | '-' expression %prec UNARY
    | '&' expression %prec UNARY
    | '*' expression %prec UNARY
    | expression '(' ')' %prec POSTFIX
    | expression '(' call_args ')' %prec POSTFIX
    ;

call_args
    : primary_expression
    | call_args ',' primary_expression
    ;

%%

int main(int argc, char **argv)
{
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Erreur ouverture fichier");
            return 1;
        }
    }
    if (yyparse() == 0)
        printf("Analyse syntaxique backend : OK\n");
    if (argc > 1)
        fclose(yyin);
    return 0;
}
