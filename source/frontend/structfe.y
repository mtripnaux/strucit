%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "code.h"

extern int yylineno;
extern FILE *yyin;
int yylex();

static Ast_node *racine_ast = NULL;

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
%token LSHIFT_OP RSHIFT_OP INC_OP DEC_OP
%token EXTERN INT VOID STRUCT IF ELSE WHILE FOR RETURN

%type <node> program external_declaration
%type <node> struct_specifier struct_declaration_list struct_declaration
%type <node> declaration declaration_specifiers type_specifier declarator direct_declarator
%type <node> parameter_list parameter_declaration
%type <node> function_definition
%type <node> compound_statement declaration_list statement_list
%type <node> matched_statement unmatched_statement
%type <node> expression_statement jump_statement
%type <node> primary_expression postfix_expression unary_expression unary_operator
%type <node> multiplicative_expression additive_expression
%type <node> shift_expression relational_expression equality_expression
%type <node> logical_and_expression logical_or_expression
%type <node> expression
%type <node> argument_expression_list

%right '='
%left OR_OP
%left AND_OP
%left EQ_OP NE_OP
%left '<' '>' LE_OP GE_OP
%left LSHIFT_OP RSHIFT_OP
%left '+' '-'
%left '*' '/'
%right UMINUS

%start program

%%

// Racine AST avec déclarations externes
program
    : external_declaration
    {
        $$ = ast_create_node(AST_PROGRAM);
        ast_add_child($$, $1);
        racine_ast = $$;
    }
    | program external_declaration
    {
        ast_add_child($1, $2);
        $$ = $1;
        racine_ast = $$;
    }
    ;

// Externes fonction / déclaration globale
external_declaration
    : function_definition    { $$ = $1; }
    | declaration            { $$ = $1; }
    ;

// Déclaration type + déclarateur ou struct seul
declaration
    : declaration_specifiers declarator ';'
    {
        // EXTERN marqué avec value=1 dans declaration_specifiers
        if ($1->value == 1) {
            $$ = ast_create_node(AST_EXTERN_DECLARATION);
            ast_add_child($$, $1);
            ast_add_child($$, $2);
        } else {
            $$ = ast_create_node(AST_DECLARATION);
            ast_add_child($$, $1);
            ast_add_child($$, $2);
        }
    }
    | struct_specifier ';'
    {
        // Déclaration de struct seule (struct Foo { ... };)
        $$ = $1;
    }
    ;

// Extern optionnel + type primitif/struct
declaration_specifiers
    : type_specifier
    {
        $$ = $1;
        $$->value = 0;  // regular
    }
    | EXTERN type_specifier
    {
        $$ = $2;
        $$->value = 1;  // extern
    }
    ;

// void, int, ou struct
type_specifier
    : VOID
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id = strdup("void");
        $$->size = 0;
    }
    | INT
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id = strdup("int");
        $$->size = 4;
    }
    | struct_specifier
    {
        $$ = $1;
    }
    ;

// Définition struct avec corps ou référence par nom
struct_specifier
    : STRUCT IDENTIFIER '{' struct_declaration_list '}'
    {
        // Définition complète struct Foo { ... }
        $$ = ast_create_node(AST_STRUCT_DEFINITION);
        ast_add_child($$, $2);
        ast_add_child($$, $4);
    }
    | STRUCT '{' struct_declaration_list '}'
    {
        // Struct anonyme (rare)
        $$ = ast_create_node(AST_STRUCT_DEFINITION);
        ast_add_child($$, $3);
    }
    | STRUCT IDENTIFIER
    {
        // Référence struct Foo (type reference)
        $$ = ast_create_node(AST_STRUCT);
        ast_add_child($$, $2);
    }
    ;

struct_declaration_list
    : struct_declaration
    {
        $$ = ast_create_node(AST_STRUCT_FIELD_LIST);
        ast_add_child($$, $1);
    }
    | struct_declaration_list struct_declaration
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

struct_declaration
    : type_specifier declarator ';'
    {
        $$ = ast_create_node(AST_STRUCT_FIELD);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

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
    | direct_declarator '(' parameter_list ')'
    {
        $$ = ast_create_node(AST_FUNC_DECLARATOR);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

parameter_list
    : parameter_declaration
    {
        $$ = ast_create_node(AST_PARAM_LIST);
        ast_add_child($$, $1);
    }
    | parameter_list ',' parameter_declaration
    {
        ast_add_child($1, $3);
        $$ = $1;
    }
    ;

parameter_declaration
    : declaration_specifiers declarator
    {
        $$ = ast_create_node(AST_PARAM);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    ;

// Def de fonction
function_definition
    : declaration_specifiers declarator compound_statement
    {
        $$ = ast_create_node(AST_FUNCTION_DEFINITION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

// Bloc {} avec déclarations optionnelles + statements
compound_statement
    : '{' '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
    }
    | '{' declaration_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
    }
    | '{' statement_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
    }
    | '{' declaration_list statement_list '}'
    {
        $$ = ast_create_node(AST_COMPOUND_STATEMENT);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

declaration_list
    : declaration
    {
        $$ = ast_create_node(AST_STATEMENT_LIST);
        ast_add_child($$, $1);
    }
    | declaration_list declaration
    {
        ast_add_child($1, $2);
        $$ = $1;
    }
    ;

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

// Instruction avec else appairé (pas de dangling else)
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

// Sans else apparié (dangling else possible)
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

// Expression primaire : identifiant, constante, (expr)
primary_expression
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
    ;

// Expression postfix : appels fonction, accès struct
postfix_expression
    : primary_expression
    {
        $$ = $1;
    }
    | postfix_expression '(' ')'
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
    }
    | postfix_expression '(' argument_expression_list ')'
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | postfix_expression PTR_OP IDENTIFIER
    {
        $$ = ast_create_node(AST_POSTFIX_POINTER);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | postfix_expression INC_OP
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
        ast_add_child($$, create_id_leaf("++"));
    }
    | postfix_expression DEC_OP
    {
        $$ = ast_create_node(AST_POSTFIX);
        ast_add_child($$, $1);
        ast_add_child($$, create_id_leaf("--"));
    }
    ;

// Opérateurs unaires, sizeof
unary_expression
    : postfix_expression
    {
        $$ = $1;
    }
    | unary_operator unary_expression
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
    }
    | SIZEOF unary_expression
    {
        $$ = ast_create_node(AST_UNARY_SIZEOF);
        ast_add_child($$, $2);
    }
    | SIZEOF '(' INT ')'
    {
        $$ = ast_create_node(AST_UNARY_SIZEOF);
        ast_add_child($$, create_id_leaf("int"));
    }
    | SIZEOF '(' VOID ')'
    {
        $$ = ast_create_node(AST_UNARY_SIZEOF);
        ast_add_child($$, create_id_leaf("void"));
    }
    | INC_OP unary_expression
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, create_id_leaf("++"));
        ast_add_child($$, $2);
    }
    | DEC_OP unary_expression
    {
        $$ = ast_create_node(AST_UNARY);
        ast_add_child($$, create_id_leaf("--"));
        ast_add_child($$, $2);
    }
    ;

// &, *, -
unary_operator
    : '&'
    {
        $$ = create_id_leaf("&");
    }
    | '*'
    {
        $$ = create_id_leaf("*");
    }
    | '-'
    {
        $$ = create_id_leaf("-");
    }
    ;

multiplicative_expression
    : unary_expression
    {
        $$ = $1;
    }
    | multiplicative_expression '*' unary_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("*");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | multiplicative_expression '/' unary_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("/");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

additive_expression
    : multiplicative_expression
    {
        $$ = $1;
    }
    | additive_expression '+' multiplicative_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("+");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | additive_expression '-' multiplicative_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("-");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

shift_expression
    : additive_expression
    {
        $$ = $1;
    }
    | shift_expression LSHIFT_OP additive_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup("<<");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | shift_expression RSHIFT_OP additive_expression
    {
        $$ = ast_create_node(AST_OP);
        $$->id = strdup(">>");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

relational_expression
    : shift_expression
    {
        $$ = $1;
    }
    | relational_expression '<' shift_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression '>' shift_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup(">");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression LE_OP shift_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression GE_OP shift_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup(">=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

equality_expression
    : relational_expression
    {
        $$ = $1;
    }
    | equality_expression EQ_OP relational_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("==");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | equality_expression NE_OP relational_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("!=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

logical_and_expression
    : equality_expression
    {
        $$ = $1;
    }
    | logical_and_expression AND_OP equality_expression
    {
        $$ = ast_create_node(AST_BOOL_LOGIC);
        $$->id = strdup("&&");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

logical_or_expression
    : logical_and_expression
    {
        $$ = $1;
    }
    | logical_or_expression OR_OP logical_and_expression
    {
        $$ = ast_create_node(AST_BOOL_LOGIC);
        $$->id = strdup("||");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

expression
    : logical_or_expression
    {
        $$ = $1;
    }
    | unary_expression '=' expression
    {
        $$ = ast_create_node(AST_ASSIGNMENT);
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
        fprintf(stderr, "Usage: %s <source.c> [sortie.c]\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) { perror("Erreur ouverture fichier"); return 1; }

    if (yyparse() != 0) { fclose(yyin); return 1; }
    fclose(yyin);

    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) { perror("Erreur création fichier sortie"); return 1; }
    }
    write_code(racine_ast, out);
    if (out != stdout) fclose(out);
    return 0;
}