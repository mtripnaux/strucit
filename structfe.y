%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylineno;
extern FILE *yyin;
int yylex();

void yyerror(const char *s) {
    fprintf(stderr, "\033[0;31mErreur syntaxique : %s ligne %d\033[0m\n", s, yylineno);
    exit(1);
}
%}

%union {
    Ast_node *node;
}

%token <node> IDENTIFIER CONSTANT
%token SIZEOF PTR_OP
%token LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
%token EXTERN INT VOID STRUCT IF ELSE WHILE FOR RETURN

%type <node> program external_declaration
%type <node> struct_definition struct_field_list struct_field
%type <node> declaration extern_declaration function_definition
%type <node> declaration_specifiers declarator direct_declarator
%type <node> param_list param_declaration
%type <node> argument_expression_list expression
%type <node> compound_statement local_decl_list local_decl
%type <node> statement_list matched_statement unmatched_statement
%type <node> expression_statement jump_statement

%right '='
%left OR_OP
%left AND_OP
%left EQ_OP NE_OP
%left '<' '>' LE_OP GE_OP
%left '+' '-'
%left '*' '/'
%right UMINUS
%left POSTFIX '(' PTR_OP

%start program

%%

/* ===== Programme ===== */

program
    : external_declaration
    {
        $$ = ast_create_node(AST_PROGRAM);
        ast_add_child($$, $1);
    }
    | program external_declaration
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

external_declaration
    : function_definition    { $$ = $1; }
    | declaration            { $$ = $1; }
    | struct_definition ';'  { $$ = $1; }
    | extern_declaration ';' { $$ = $1; }
    ;

/* ===== Structures ===== */

struct_definition
    : STRUCT IDENTIFIER '{' struct_field_list '}'
    {
        $$ = ast_create_node(AST_STRUCT_DEFINITION);
        ast_add_child($$, $2);
        ast_add_child($$, $4);
    }
    ;

struct_field_list
    : struct_field
    {
        $$ = ast_create_node(AST_STRUCT_FIELD_LIST);
        ast_add_child($$, $1);
    }
    | struct_field_list struct_field
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

struct_field
    : declaration_specifiers declarator ';'
    {
        $$ = ast_create_node(AST_STRUCT_FIELD);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

/* ===== Déclarations ===== */

extern_declaration
    : EXTERN declaration_specifiers declarator
    {
        $$ = ast_create_node(AST_EXTERN_DECLARATION);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

declaration
    : declaration_specifiers declarator ';'
    {
        $$ = ast_create_node(AST_DECLARATION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

declaration_specifiers
    : INT
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id   = strdup("int");
        $$->size = 4;
    }
    | VOID
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id   = strdup("void");
        $$->size = 0;
    }
    | STRUCT IDENTIFIER
    {
        $$ = ast_create_node(AST_STRUCT);
        ast_add_child($$, $2);
    }
    ;

/* ===== Déclarateurs ===== */

declarator
    : '*' direct_declarator
    {
        $$ = ast_create_node(AST_STAR_DECLARATOR);
        ast_add_child($$, $2);
    }
    | direct_declarator
    {
        $$ = $1;
    }
    ;

direct_declarator
    : IDENTIFIER
    {
        $$ = $1;
    }
    | '(' declarator ')'
    {
        $$ = $2;
    }
    | direct_declarator '(' ')'
    {
        $$ = ast_create_node(AST_DIRECT_DECLARATOR);
        ast_add_child($$, $1);
    }
    | direct_declarator '(' param_list ')'
    {
        $$ = ast_create_node(AST_FUNC_DECLARATOR);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

/* ===== Paramètres ===== */

param_list
    : param_declaration
    {
        $$ = ast_create_node(AST_PARAM_LIST);
        ast_add_child($$, $1);
    }
    | param_list ',' param_declaration
    {
        ast_add_child($1, $3);
        $$ = $1;
    }
    ;

param_declaration
    : declaration_specifiers declarator
    {
        $$ = ast_create_node(AST_PARAM);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

/* ===== Définition de fonction ===== */

function_definition
    : declaration_specifiers declarator compound_statement
    {
        $$ = ast_create_node(AST_FUNCTION_DEFINITION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

/* ===== Blocs ===== */

compound_statement
    : '{' '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
    }
    | '{' local_decl_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
    }
    | '{' statement_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
    }
    | '{' local_decl_list statement_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

local_decl_list
    : local_decl
    {
        $$ = ast_create_node(AST_STATEMENT_LIST);
        ast_add_child($$, $1);
    }
    | local_decl_list local_decl
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

local_decl
    : declaration_specifiers declarator ';'
    {
        $$ = ast_create_node(AST_DECLARATION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

/* ===== Instructions ===== */

statement_list
    : matched_statement
    {
        $$ = ast_create_node(AST_STATEMENT_LIST);
        ast_add_child($$, $1);
    }
    | unmatched_statement
    {
        $$ = ast_create_node(AST_STATEMENT_LIST);
        ast_add_child($$, $1);
    }
    | statement_list matched_statement
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    | statement_list unmatched_statement
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

matched_statement
    : expression_statement
    {
        $$ = $1;
    }
    | compound_statement
    {
        $$ = $1;
    }
    | jump_statement
    {
        $$ = $1;
    }
    | IF '(' expression ')' matched_statement ELSE matched_statement
    {
        $$ = ast_create_node(AST_IF_ELSE);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
        ast_add_child($$, $7);
    }
    | WHILE '(' expression ')' matched_statement
    {
        $$ = ast_create_node(AST_WHILE);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
    }
    | FOR '(' expression_statement expression_statement expression ')' matched_statement
    {
        $$ = ast_create_node(AST_FOR);
        ast_add_child($$, $3);
        ast_add_child($$, $4);
        ast_add_child($$, $5);
        ast_add_child($$, $7);
    }
    ;

unmatched_statement
    : IF '(' expression ')' matched_statement
    {
        $$ = ast_create_node(AST_IF);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
    }
    | IF '(' expression ')' unmatched_statement
    {
        $$ = ast_create_node(AST_IF);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
    }
    | IF '(' expression ')' matched_statement ELSE unmatched_statement
    {
        $$ = ast_create_node(AST_IF_ELSE);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
        ast_add_child($$, $7);
    }
    | WHILE '(' expression ')' unmatched_statement
    {
        $$ = ast_create_node(AST_WHILE);
        ast_add_child($$, $3);
        ast_add_child($$, $5);
    }
    | FOR '(' expression_statement expression_statement expression ')' unmatched_statement
    {
        $$ = ast_create_node(AST_FOR);
        ast_add_child($$, $3);
        ast_add_child($$, $4);
        ast_add_child($$, $5);
        ast_add_child($$, $7);
    }
    ;

expression_statement
    : ';'
    {
        $$ = ast_create_node(AST_EXPRESSION_STATEMENT);
    }
    | expression ';'
    {
        $$ = ast_create_node(AST_EXPRESSION_STATEMENT);
        ast_add_child($$, $1);
    }
    ;

jump_statement
    : RETURN ';'
    {
        $$ = ast_create_node(AST_RETURN);
    }
    | RETURN expression ';'
    {
        $$ = ast_create_node(AST_RETURN);
        ast_add_child($$, $2);
    }
    ;

/* ===== Expressions (grammaire plate, précédences déclarées) ===== */

expression
    : IDENTIFIER
    {
        $$ = $1;
    }
    | CONSTANT
    {
        $$ = $1;
    }
    | '(' expression ')'
    {
        $$ = $2;
    }
    | expression '=' expression
    {
        $$ = ast_create_node(AST_ASSIGNMENT);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression OR_OP expression
    {
        $$ = ast_create_node(AST_BOOL_LOGIC);
        $$->id = strdup("||");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression AND_OP expression
    {
        $$ = ast_create_node(AST_BOOL_LOGIC);
        $$->id = strdup("&&");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression EQ_OP expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("==");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression NE_OP expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("!=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '<' expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '>' expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup(">");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression LE_OP expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression GE_OP expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup(">=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '+' expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("+");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '-' expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("-");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '*' expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("*");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression '/' expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("/");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | '-' expression %prec UMINUS
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, create_id_leaf("-"));
        ast_add_child($$, $2);
    }
    | '&' expression %prec UMINUS
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, create_id_leaf("&"));
        ast_add_child($$, $2);
    }
    | '*' expression %prec UMINUS
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, create_id_leaf("*"));
        ast_add_child($$, $2);
    }
    | SIZEOF '(' expression ')' %prec UMINUS
    {
        $$ = ast_create_node(AST_UNARY_SIZEOF);
        ast_add_child($$, $3);
    }
    | expression '(' ')' %prec POSTFIX
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
    }
    | expression '(' argument_expression_list ')' %prec POSTFIX
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | expression PTR_OP IDENTIFIER %prec POSTFIX
    {
        $$ = ast_create_node(AST_POSTFIX_POINTER);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

argument_expression_list
    : expression
    {
        $$ = ast_create_node(AST_ARGUMENT_EXPRESSION_LIST);
        ast_add_child($$, $1);
    }
    | argument_expression_list ',' expression
    {
        ast_add_child($1, $3);
        $$ = $1;
    }
    ;

%%

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fichier.c>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("Erreur ouverture fichier");
        return 1;
    }
    if (yyparse() == 0)
        printf("Analyse syntaxique : OK\n");
    fclose(yyin);
    return 0;
}
