%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/Semantic_Analysis/ast.h"
#include "../../src/Semantic_Analysis/symbol.h"
#include "../../src/Semantic_Analysis/stack.h"
#include "../../src/Semantic_Analysis/hashmap.h"

extern int yylineno;
extern FILE *yyin;
extern FILE *yyout;
int yylex();

char *current_function;
Stack *stack;

void yyerror(const char *s) {
    fprintf(stderr, "\033[0;31mErreur Syntaxique : %s à la ligne %d\033[0m\n", s, yylineno);
    exit(1);
}
%}

%union {
    struct _Ast_node *node;
}

/* Tokens */
%token <node> IDENTIFIER CONSTANT
%token SIZEOF PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP
%token LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
%token EXTERN INT VOID STRUCT IF ELSE WHILE FOR RETURN

/* Types des non-terminaux */
%type <node> program external_declaration function_definition declaration
%type <node> primary_expression postfix_expression argument_expression_list 
%type <node> unary_expression unary_operator multiplicative_expression 
%type <node> additive_expression shift_expression relational_expression 
%type <node> equality_expression logical_and_expression logical_or_expression expression 
%type <node> compound_statement statement_list statement expression_statement
%type <node> selection_statement iteration_statement jump_statement
%type <node> declarator direct_declarator declaration_specifiers

%start program

%%

primary_expression
    : IDENTIFIER { $$ = $1; }
    | CONSTANT   { $$ = $1; }
    | '(' expression ')' { $$ = $2; }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression '(' ')' { $$ = ast_create_node(AST_POSTFIX); ast_add_child($$, $1); }
    | postfix_expression '(' argument_expression_list ')' { $$ = ast_create_node(AST_POSTFIX); ast_add_child($$, $1); ast_add_child($$, $3); }
    | postfix_expression '.' IDENTIFIER { $$ = ast_create_node(AST_POSTFIX); ast_add_child($$, $1); ast_add_child($$, $3); }
    | postfix_expression PTR_OP IDENTIFIER { $$ = ast_create_node(AST_POSTFIX_POINTER); ast_add_child($$, $1); ast_add_child($$, $3); }
    | postfix_expression INC_OP { $$ = ast_create_node(AST_POSTFIX); ast_add_child($$, $1); ast_add_child($$, create_id_leaf("++")); }
    | postfix_expression DEC_OP { $$ = ast_create_node(AST_POSTFIX); ast_add_child($$, $1); ast_add_child($$, create_id_leaf("--")); }
    ;

argument_expression_list
    : expression { $$ = ast_create_node(AST_ARGUMENT_EXPRESSION_LIST); ast_add_child($$, $1); }
    | argument_expression_list ',' expression { ast_add_child($1, $3); $$ = $1; }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | unary_operator unary_expression { $$ = ast_create_node(AST_UNARY); ast_add_child($$, $1); ast_add_child($$, $2); }
    | SIZEOF unary_expression { $$ = ast_create_node(AST_UNARY_SIZEOF); ast_add_child($$, $2); }
    ;

unary_operator
    : '&' { $$ = create_id_leaf("&"); }
    | '*' { $$ = create_id_leaf("*"); }
    | '-' { $$ = create_id_leaf("-"); }
    ;

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression '*' unary_expression { $$ = ast_create_node(AST_OP); $$->id = "*"; ast_add_child($$, $1); ast_add_child($$, $3); }
    | multiplicative_expression '/' unary_expression { $$ = ast_create_node(AST_OP); $$->id = "/"; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression { $$ = ast_create_node(AST_OP); $$->id = "+"; ast_add_child($$, $1); ast_add_child($$, $3); }
    | additive_expression '-' multiplicative_expression { $$ = ast_create_node(AST_OP); $$->id = "-"; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression LEFT_OP additive_expression { $$ = ast_create_node(AST_OP); $$->id = "<<"; ast_add_child($$, $1); ast_add_child($$, $3); }
    | shift_expression RIGHT_OP additive_expression { $$ = ast_create_node(AST_OP); $$->id = ">>"; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = "<"; ast_add_child($$, $1); ast_add_child($$, $3); }
    | relational_expression '>' shift_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = ">"; ast_add_child($$, $1); ast_add_child($$, $3); }
    | relational_expression LE_OP shift_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = "<="; ast_add_child($$, $1); ast_add_child($$, $3); }
    | relational_expression GE_OP shift_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = ">="; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ_OP relational_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = "=="; ast_add_child($$, $1); ast_add_child($$, $3); }
    | equality_expression NE_OP relational_expression { $$ = ast_create_node(AST_BOOL_OP); $$->id = "!="; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

logical_and_expression
    : equality_expression { $$ = $1; }
    | logical_and_expression AND_OP equality_expression { $$ = ast_create_node(AST_BOOL_LOGIC); $$->id = "&&"; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OR_OP logical_and_expression { $$ = ast_create_node(AST_BOOL_LOGIC); $$->id = "||"; ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

expression
    : logical_or_expression { $$ = $1; }
    | unary_expression '=' expression { $$ = ast_create_node(AST_ASSIGNMENT); ast_add_child($$, $1); ast_add_child($$, $3); }
    ;

/* Déclarations */
declaration
    : declaration_specifiers declarator ';' { $$ = ast_create_node(AST_DECLARATION); ast_add_child($$, $1); ast_add_child($$, $2); }
    ;

declaration_specifiers
    : INT  { $$ = ast_create_node(AST_TYPE_SPECIFIER); $$->id = "int"; $$->size = 4; }
    | VOID { $$ = ast_create_node(AST_TYPE_SPECIFIER); $$->id = "void"; $$->size = 0; }
    | STRUCT IDENTIFIER { $$ = ast_create_node(AST_STRUCT); ast_add_child($$, $2); }
    ;

declarator
    : '*' direct_declarator { $$ = ast_create_node(AST_STAR_DECLARATOR); ast_add_child($$, $2); }
    | direct_declarator { $$ = $1; }
    ;

direct_declarator
    : IDENTIFIER { $$ = $1; }
    | '(' declarator ')' { $$ = $2; }
    | direct_declarator '(' ')' { $$ = ast_create_node(AST_DIRECT_DECLARATOR); ast_add_child($$, $1); }
    ;

compound_statement
    : '{' '}' { $$ = ast_create_node(AST_COMPOUND_STATEMENT); }
    | '{' { push(stack, create_hash_map()); } statement_list '}' { $$ = ast_create_node(AST_COMPOUND_STATEMENT); ast_add_child($$, $3); pop(stack); }
    ;

statement_list
    : statement { $$ = ast_create_node(AST_STATEMENT_LIST); ast_add_child($$, $1); }
    | statement_list statement { ast_add_child($1, $2); $$ = $1; }
    ;

statement
    : compound_statement { $$ = $1; }
    | expression_statement { $$ = $1; }
    | selection_statement { $$ = $1; }
    | iteration_statement { $$ = $1; }
    | jump_statement { $$ = $1; }
    ;

expression_statement
    : ';' { $$ = ast_create_node(AST_EXPRESSION_STATEMENT); }
    | expression ';' { $$ = ast_create_node(AST_EXPRESSION_STATEMENT); ast_add_child($$, $1); }
    ;

selection_statement
    : IF '(' expression ')' statement { $$ = ast_create_node(AST_IF); ast_add_child($$, $3); ast_add_child($$, $5); }
    | IF '(' expression ')' statement ELSE statement { $$ = ast_create_node(AST_IF_ELSE); ast_add_child($$, $3); ast_add_child($$, $5); ast_add_child($$, $7); }
    ;

iteration_statement
    : WHILE '(' expression ')' statement { $$ = ast_create_node(AST_WHILE); ast_add_child($$, $3); ast_add_child($$, $5); }
    | FOR '(' expression_statement expression_statement expression ')' statement 
      { 
        $$ = ast_create_node(AST_FOR); 
        ast_add_child($$, $3); ast_add_child($$, $4); ast_add_child($$, $5); ast_add_child($$, $7); 
      }
    ;

jump_statement
    : RETURN ';' { $$ = ast_create_node(AST_RETURN); }
    | RETURN expression ';' { $$ = ast_create_node(AST_RETURN); ast_add_child($$, $2); }
    ;

program
    : external_declaration { $$ = ast_create_node(AST_PROGRAM); ast_add_child($$, $1); }
    | program external_declaration { ast_add_child($1, $2); $$ = $1; }
    ;

external_declaration
    : function_definition { $$ = $1; }
    | declaration { $$ = $1; }
    ;

function_definition
    : declaration_specifiers declarator compound_statement
    {
        $$ = ast_create_node(AST_FUNCTION_DEFINITION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

%%

int main(int argc, char **argv)
{
    stack = create_stack();
    push(stack, create_hash_map());

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) { perror("Erreur ouverture fichier"); return 1; }

    printf("--- Analyse Syntaxique et Sémantique en cours ---\n");
    if (yyparse() == 0) {
        printf("--- Analyse terminée : Succès ---\n");
    }

    return 0;
}
