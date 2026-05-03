%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"      /* Pour construire l'arbre */
#include "symbol.h"   /* Pour la table des symboles */
#include "code.h"     /* Pour générer le code final */

extern int yylineno;
extern FILE *yyin;
int yylex();

/* Pointeur vers la racine de l'arbre final */
Ast_node *racine_ast = NULL;

void yyerror(const char *s) {
    fprintf(stderr, "\033[1;31mErreur syntaxique : %s à la ligne %d\033[0m\n", s, yylineno);
    exit(1);
}
%}

/*  pour stocker les types de données */
%union {
    int value;
    char *id;
    struct _Ast_node *node;
}

%define parse.error verbose

/* Tokens avec leurs types associés */
%token <id> IDENTIFIER
%token <value> CONSTANT
%token LE_OP GE_OP EQ_OP NE_OP
%token EXTERN INT VOID IF RETURN GOTO

/* Types pour les non-terminaux (pour construire l'arbre) */
%type <node> program external_declaration function_definition declaration 
%type <node> compound_statement statement expression primary_expression

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
    : external_declaration { racine_ast = ast_create_node(AST_PROGRAM); ast_add_child(racine_ast, $1); }
    | program external_declaration { ast_add_child(racine_ast, $2); }
    ;

external_declaration
    : function_definition { $$ = $1; }
    | declaration         { $$ = $1; }
    /* Ici tu peux ajouter les appels à tes fonctions francisées */
    ;

/* ===== Expressions ===== */

primary_expression
    : IDENTIFIER { $$ = create_id_leaf($1); }
    | CONSTANT   { $$ = create_int_leaf($1); }
    ;

expression
    : primary_expression { $$ = $1; }
    | expression '+' expression { $$ = create_node(AST_OP, $1, $3); $$->id = strdup("+"); }
    | expression '-' expression { $$ = create_node(AST_OP, $1, $3); $$->id = strdup("-"); }
    | expression '*' expression { $$ = create_node(AST_OP, $1, $3); $$->id = strdup("*"); }
    | expression '/' expression { $$ = create_node(AST_OP, $1, $3); $$->id = strdup("/"); }
    | IDENTIFIER '=' expression { 
        Ast_node *id_node = create_id_leaf($1);
        $$ = create_node(AST_ASSIGNMENT, id_node, $3); 
    }
    | '(' expression ')' { $$ = $2; }
    ;

/* ===== Instructions ===== */

jump_statement
    : RETURN ';' { $$ = ast_create_node(AST_RETURN); }
    | RETURN expression ';' { $$ = create_node(AST_RETURN, $2, NULL); }
    | GOTO IDENTIFIER ';' { $$ = ast_create_node(AST_GOTO); $$->id = $2; }
    ;

/* Les autres règles suivent la même logique d'appel à create_node... */

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

    printf("Début de l'analyse syntaxique...\n");

    if (yyparse() == 0) {
        printf("\033[0;32mAnalyse syntaxique terminée : OK\033[0m\n");
        
        /* Une fois l'arbre construit, on génère le code */
        if (racine_ast != NULL) {
            FILE *f_sortie = fopen("output.c", "w");
            if (f_sortie) {
                printf("Génération du code intermédiaire...\n");
                write_code(racine_ast, f_sortie);
                fclose(f_sortie);
                printf("Code généré dans 'output.c'\n");
            }
        }
    }

    if (argc > 1) fclose(yyin);
    return 0;
}
