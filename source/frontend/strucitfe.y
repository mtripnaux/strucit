%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"
#include "semantic.h"
#include "erreurs.h"

extern int yylineno;
extern FILE *yyin;
int yylex();

static Ast_node *racine_ast = NULL;

// yyerror() ne fait plus exit(1) : grace aux regles de recuperation
// "error ';'" plus bas (panic-mode error recovery, cf dragon book 4.8.3),
// le parseur resynchronise sur le ';' suivant et continue, ce qui permet
// de detecter et d'afficher TOUTES les erreurs syntaxiques (et lexicales)
// d'un fichier en une seule passe plutot que de s'arreter a la premiere.
// erreur_syntaxique() (erreurs.c) incremente deja g_total_erreurs : pas
// besoin d'un compteur separe ici, main() le consultera directement.
void yyerror(const char *s) {
    erreur_syntaxique(yylineno, s);
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
%type <node> struct_specifier struct_declaration_list struct_declaration
%type <node> declaration declaration_specifiers type_specifier declarator direct_declarator
%type <node> parameter_list parameter_declaration
%type <node> function_definition
%type <node> compound_statement declaration_list statement_list
%type <node> matched_statement unmatched_statement
%type <node> expression_statement jump_statement
%type <node> primary_expression postfix_expression unary_expression unary_operator
%type <node> multiplicative_expression additive_expression
%type <node> relational_expression equality_expression
%type <node> logical_and_expression logical_or_expression
%type <node> expression
%type <node> argument_expression_list

%start program

%%

// Racine de l'AST : la liste de toutes les declarations/fonctions du
// fichier, dans leur ordre d'apparition. racine_ast est mise a jour a
// chaque reduction pour rester valide meme si bison re-applique cette
// regle plusieurs fois (programme a plusieurs declarations).
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

// Au plus haut niveau du fichier : soit une fonction complete (avec corps),
// soit une simple declaration (variable, extern, ou struct).
external_declaration
    : function_definition    { $$ = $1; }
    | declaration            { $$ = $1; }
    // Recuperation panic-mode au niveau global (cf expression_statement
    // pour la meme idee a l'interieur d'un corps de fonction) : une
    // declaration ou un en-tete de fonction mal forme est ignore jusqu'au
    // ';' suivant, et le parseur reprend la suite du fichier. Placee ici
    // (et pas dans "declaration", qui est aussi utilisee a l'interieur des
    // fonctions) pour ne jamais entrer en concurrence avec la recuperation
    // de expression_statement, ce qui creerait un conflit reduce/reduce.
    | error ';'
    {
        yyerrok;
        $$ = ast_create_node(AST_EXPRESSION_STATEMENT);
    }
    ;

// declaration_specifiers->value distingue "extern" (1) de "normal" (0),
// cf la regle declaration_specifiers ci-dessous : c'est ce flag qui decide
// si on produit un AST_EXTERN_DECLARATION ou un AST_DECLARATION.
declaration
    : declaration_specifiers declarator ';'
    {
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
        // "struct Foo { ... };" sans variable associee : juste la
        // definition de la structure, qui se suffit a elle-meme.
        $$ = $1;
    }
    ;

// Le champ ->value du noeud de type est reutilise comme drapeau
// "declare extern ?" (0/1), lu par la regle 'declaration' ci-dessus.
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

// Type de base (int/void) ou reference/definition de structure.
type_specifier
    : VOID
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id = strdup("void");
    }
    | INT
    {
        $$ = ast_create_node(AST_TYPE_SPECIFIER);
        $$->id = strdup("int");
    }
    | struct_specifier
    {
        $$ = $1;
    }
    ;

// Trois usages possibles du mot-cle struct : definir un nouveau type avec
// son corps (le cas normal), un corps anonyme (rare, sans nom), ou juste
// reference un type struct deja defini ailleurs (ex: "struct Foo *p;").
// Seul le premier cas produit AST_STRUCT_DEFINITION (a enregistrer dans la
// table des symboles) ; le troisieme produit AST_STRUCT (juste un nom de
// type, semantic.c verifiera que la structure existe bien).
struct_specifier
    : STRUCT IDENTIFIER '{' struct_declaration_list '}'
    {
        $$ = ast_create_node(AST_STRUCT_DEFINITION);
        ast_add_child($$, $2);
        ast_add_child($$, $4);
    }
    | STRUCT '{' struct_declaration_list '}'
    {
        $$ = ast_create_node(AST_STRUCT_DEFINITION);
        ast_add_child($$, $3);
    }
    | STRUCT IDENTIFIER
    {
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

// Un declarateur est soit un nom simple, soit un nom precede d'une etoile
// (pointeur). C'est la seule facon de mettre une etoile : ast_est_pointeur
// (ast.c) reconnait precisement ce noeud AST_STAR_DECLARATOR.
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

// direct_declarator gere : un identifiant nu, un declarateur parenthese
// (les parentheses ne creent pas de noeud, $$ = $2 directement : elles ne
// servent qu'a lever une ambiguite de priorite, ex pour un pointeur de
// fonction "(*f)(...)"), et les deux formes de declarateur de fonction
// (avec ou sans parametres).
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

function_definition
    : declaration_specifiers declarator compound_statement
    {
        $$ = ast_create_node(AST_FUNCTION_DEFINITION);
        ast_add_child($$, $1);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    ;

// Un bloc { } peut contenir des declarations locales, des instructions,
// les deux, ou rien. Les quatre cas sont enumeres explicitement plutot que
// rendus optionnels pour rester simple a lire (pas de regle vide
// ambigue ici).
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

// Une liste d'instructions peut commencer par une instruction "fermee"
// (matched) ou "ouverte" (unmatched, qui contient un if sans else) : voir
// le commentaire en tete de fichier sur le dangling-else.
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

// matched_statement : toute instruction qui ne peut PAS "absorber" un else
// venant apres elle (soit parce qu'elle n'a pas de if du tout, soit parce
// que son if a deja un else). C'est en restreignant le corps du if/while/for
// a matched_statement (jamais unmatched_statement) qu'on empeche le else
// de la regle IF...ELSE de s'attacher au mauvais if.
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

// unmatched_statement : un if SANS else (donc qui peut encore "absorber"
// un else qui suivrait), ou une boucle dont le corps est lui-meme un tel
// if. C'est exactement le else de la 3e alternative qui s'attache
// toujours au if le plus proche non encore ferme (sans ambiguite, donc
// sans conflit shift/reduce).
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
    // Recuperation panic-mode (dragon book 4.8.3) : une instruction
    // mal formee est remplacee par une instruction vide, et le parseur
    // reprend juste apres le ';' qui suit. Ca permet de continuer a
    // chercher d'autres erreurs dans le reste du fichier au lieu de
    // s'arreter a la premiere.
    | error ';'
    {
        yyerrok;
        $$ = ast_create_node(AST_EXPRESSION_STATEMENT);
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

// primary_expression est la base de toute expression : un identifiant, une
// constante, ou une sous-expression parenthesee (les parentheses ne
// produisent pas de noeud, elles servent juste a regrouper).
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

// Appels de fonction "f(...)" et acces a un champ "p.champ"/"p->champ".
// ->line est renseigne explicitement sur les appels (et nulle part
// ailleurs dans ce fichier) car c'est le seul endroit ou semantic.c a
// besoin de reporter une ligne precise pour un noeud qui n'est ni
// IDENTIFIER ni CONSTANT (cf verifier_appel dans semantic.c).
postfix_expression
    : primary_expression
    {
        $$ = $1;
    }
    | postfix_expression '(' ')'
    {
        $$ = ast_create_node(AST_POSTFIX);
        $$->line = yylineno;
        ast_add_child($$, $1);
    }
    | postfix_expression '(' argument_expression_list ')'
    {
        $$ = ast_create_node(AST_POSTFIX);
        $$->line = yylineno;
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | postfix_expression '.' IDENTIFIER
    {
        $$ = ast_create_node(AST_POSTFIX_POINTER);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | postfix_expression PTR_OP IDENTIFIER
    {
        $$ = ast_create_node(AST_POSTFIX_POINTER);
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    ;

// Operateurs unaires (-, &, *) et sizeof. unary_operator (juste en-dessous)
// produit une feuille AST_IDENTIFIER dont ->id est litteralement "-", "&"
// ou "*" : c'est ce qui permet a codegen.c/semantic.c de distinguer
// l'operateur applique sans avoir besoin d'un type de noeud different
// par operateur.
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
    ;

unary_operator
    : '&'
    {
        $$ = create_identifier_leaf("&");
    }
    | '*'
    {
        $$ = create_identifier_leaf("*");
    }
    | '-'
    {
        $$ = create_identifier_leaf("-");
    }
    ;

// A partir d'ici : la chaine classique de non-terminaux en cascade qui
// encode la priorite des operateurs directement dans la structure de la
// grammaire (multiplicatif avant additif avant relationnel avant egalite
// avant && avant ||), sans avoir besoin de declarations %left/%right.
// Chaque niveau ne peut combiner que des operandes du niveau immediatement
// inferieur (ou de lui-meme, pour la recursivite gauche), ce qui force
// bison a reduire dans le bon ordre.

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

relational_expression
    : additive_expression
    {
        $$ = $1;
    }
    | relational_expression '<' additive_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression '>' additive_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup(">");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression LE_OP additive_expression
    {
        $$ = ast_create_node(AST_BOOL_OP);
        $$->id = strdup("<=");
        ast_add_child($$, $1);
        ast_add_child($$, $3);
    }
    | relational_expression GE_OP additive_expression
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

// expression est la racine de toute la cascade ci-dessus, plus
// l'affectation. L'affectation est volontairement recursive a DROITE
// ("unary_expression '=' expression", pas "expression '=' expression") :
// ca rend "a = b = c;" valide et associatif a droite (a = (b = c)), sans
// ambiguite et sans besoin de declarer %right '='.
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

// Point d'entree du compilateur frontend : structit <source.c> [sortie.c]
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.c> [sortie.c]\n", argv[0]);
        return 1;
    }
    g_fichier_source = argv[1];
    yyin = fopen(argv[1], "r");
    if (!yyin) { erreur_systeme("Ouverture du fichier source"); return 1; }

    // yyparse() construit racine_ast (via les actions ci-dessus). Grace aux
    // regles "error ';'" (panic-mode recovery), une erreur syntaxique ne
    // provoque plus un exit() immediat : le parseur resynchronise et
    // continue, donc g_total_erreurs (erreurs.c) peut compter PLUSIEURS
    // erreurs (lexicales et/ou syntaxiques) trouvees dans tout le fichier
    // en une seule passe. yyparse() ne renvoie non-nul que si meme la
    // recuperation echoue (ex: erreur juste avant la fin du fichier, rien
    // a resynchroniser).
    int parse_status = yyparse();
    fclose(yyin);

    if (parse_status != 0 || g_total_erreurs > 0) {
        rapporter_echec_compilation();
        if (racine_ast) ast_free(racine_ast);
        return 1;
    }

    // Analyse semantique complete AVANT toute generation de code, pour ne
    // jamais produire un fichier de sortie partiel/incorrect si le
    // programme source contient une erreur de type quelque part. On ne
    // l'execute que si la syntaxe est propre : un AST issu d'une
    // recuperation d'erreur ne serait pas fiable a verifier semantiquement.
    sem_analyse(racine_ast);
    if (g_total_erreurs > 0) {
        rapporter_echec_compilation();
        sem_liberer();
        ast_free(racine_ast);
        return 1;
    }

    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) { erreur_systeme("Création du fichier de sortie"); return 1; }
    }
    write_code(racine_ast, out);
    if (out != stdout) fclose(out);

    // Ordre important : codegen_liberer() avant sem_liberer(), car
    // codegen.c lit (sans le posseder) table_globale construit par
    // semantic.c — c'est sem_liberer() (donc symtable_liberer()) qui en
    // est responsable, pas codegen_liberer().
    codegen_liberer();
    sem_liberer();
    ast_free(racine_ast);

    return 0;
}
