%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* Declaración externa de yylex y yyerror */
int yylex();
void yyerror(const char *s);
extern int yylineno;

ASTNode *root = NULL;
%}

%union {
    int ival;
    char *sval;
    struct ASTNode *node;
}

/* Tokens definidos en lexer.l */
%token <sval> ID CADENA TYPE_INT TYPE_FLOAT TYPE_BOOL TYPE_CHAR TYPE_STRING TYPE_VOID
%token <ival> NUMERO TRUE FALSE
%token IF ELSE WHILE FOR DO RETURN
%token TOKEN_MOVER TOKEN_RETROCEDER TOKEN_GIRAR_IZQ TOKEN_GIRAR_DER TOKEN_ESPERAR TOKEN_LEER_SENSOR TOKEN_PARAR
%token EQ NEQ LE GE AND OR INC DEC

/* Tipos para los no-terminales */
%type <node> programa elementos elemento funcion var_global lista_params param
%type <node> bloque lista_instrucciones instruccion sentencia_basica resto_decl
%type <node> if_stmt while_stmt do_while_stmt for_stmt
%type <node> expresion exp_or exp_and exp_igualdad exp_rel exp_aditiva exp_mult exp_unaria exp_postfija exp_primaria
%type <node> func_reservada lista_argumentos opcion_llamada
%type <sval> tipo

/* Precedencia */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

/* 1. NIVEL SUPERIOR */
programa: 
    elementos { root = $1; }
    ;

elementos: 
    elemento elementos { $$ = $1; $$->next = $2; }
  | /* vacio */ { $$ = NULL; }
  ;

elemento:
    funcion { $$ = $1; }
  | var_global { $$ = $1; }
  ;

/* Regla para tipos de datos */
tipo:
    TYPE_INT    { $$ = strdup("int"); }
  | TYPE_FLOAT  { $$ = strdup("float"); }
  | TYPE_BOOL   { $$ = strdup("boolean"); }
  | TYPE_CHAR   { $$ = strdup("char"); }
  | TYPE_STRING { $$ = strdup("string"); }
  | TYPE_VOID   { $$ = strdup("void"); }
  ;

var_global:
    tipo ID ';' { 
        $$ = newASTNode(NODE_VAR_DECL); 
        $$->type_name = $1; 
        $$->data.str_val = $2; 
    }
  | tipo ID '=' expresion ';' {
        $$ = newASTNode(NODE_VAR_DECL);
        $$->type_name = $1;
        $$->data.str_val = $2;
        $$->left = $4; 
    }
  ;

funcion:
    tipo ID '(' lista_params ')' bloque {
        $$ = newASTNode(NODE_FUNCTION);
        $$->type_name = $1;       
        $$->data.str_val = $2;    
        $$->left = $4;            
        $$->right = $6;           
    }
  ;

lista_params:
    param ',' lista_params { $$ = $1; $$->next = $3; }
  | param { $$ = $1; }
  | /* vacio */ { $$ = NULL; }
  ;

param:
    tipo ID {
        $$ = newASTNode(NODE_VAR_DECL);
        $$->type_name = $1;
        $$->data.str_val = $2;
    }
  ;

/* 2. BLOQUES E INSTRUCCIONES */
bloque:
    '{' lista_instrucciones '}' {
        $$ = newASTNode(NODE_BLOCK);
        $$->next = $2; 
    }
  ;

lista_instrucciones:
    instruccion lista_instrucciones { $$ = $1; if($1 != NULL) $$->next = $2; }
  | /* vacio */ { $$ = NULL; }
  ;

instruccion:
    bloque { $$ = $1; }
  | if_stmt { $$ = $1; }
  | while_stmt { $$ = $1; }
  | do_while_stmt { $$ = $1; }
  | for_stmt { $$ = $1; }
  | RETURN expresion ';' { $$ = newUnaryNode("return", $2); $$->kind = NODE_RETURN; }
  | RETURN ';' { $$ = newASTNode(NODE_RETURN); }
  | sentencia_basica ';' { $$ = $1; }
  ;

sentencia_basica:
    tipo ID resto_decl {
        $$ = newASTNode(NODE_VAR_DECL);
        $$->type_name = $1;
        $$->data.str_val = $2;
        $$->left = $3;
    }
  | expresion { $$ = $1; }
  ;

resto_decl:
    '=' expresion { $$ = $2; }
  | /* vacio */ { $$ = NULL; }
  ;

/* 3. ESTRUCTURAS DE CONTROL */
if_stmt:
    IF '(' expresion ')' bloque %prec LOWER_THAN_ELSE {
        $$ = newASTNode(NODE_IF);
        $$->left = $3;  
        $$->right = $5; 
    }
  | IF '(' expresion ')' bloque ELSE bloque {
        $$ = newASTNode(NODE_IF);
        $$->left = $3;  
        $$->right = $5; 
        $$->extra = $7; 
    }
  ;

while_stmt:
    WHILE '(' expresion ')' bloque {
        $$ = newASTNode(NODE_WHILE);
        $$->left = $3;
        $$->right = $5;
    }
  ;

do_while_stmt:
    DO bloque WHILE '(' expresion ')' ';' {
        $$ = newASTNode(NODE_DO_WHILE);
        $$->left = $5;
        $$->right = $2;
    }
  ;

for_stmt:
    FOR '(' sentencia_basica ';' expresion ';' expresion ')' bloque {
        $$ = newASTNode(NODE_FOR);
        $$->left = $3;   
        $$->right = $5;  
        $$->extra = $7;  
        $$->next = $9;   
    }
  ;

/* 5. JERARQUÍA DE EXPRESIONES */
expresion:
    exp_or { $$ = $1; }
  | exp_unaria '=' expresion { 
        $$ = newASTNode(NODE_ASSIGN);
        $$->left = $1;
        $$->right = $3;
    }
  ;

exp_or:
    exp_or OR exp_and { $$ = newBinaryNode("||", $1, $3); }
  | exp_and { $$ = $1; }
  ;

exp_and:
    exp_and AND exp_igualdad { $$ = newBinaryNode("&&", $1, $3); }
  | exp_igualdad { $$ = $1; }
  ;

exp_igualdad:
    exp_igualdad EQ exp_rel { $$ = newBinaryNode("==", $1, $3); }
  | exp_igualdad NEQ exp_rel { $$ = newBinaryNode("!=", $1, $3); }
  | exp_rel { $$ = $1; }
  ;

exp_rel:
    exp_rel '<' exp_aditiva { $$ = newBinaryNode("<", $1, $3); }
  | exp_rel LE exp_aditiva  { $$ = newBinaryNode("<=", $1, $3); }
  | exp_rel '>' exp_aditiva { $$ = newBinaryNode(">", $1, $3); }
  | exp_rel GE exp_aditiva  { $$ = newBinaryNode(">=", $1, $3); }
  | exp_aditiva { $$ = $1; }
  ;

exp_aditiva:
    exp_aditiva '+' exp_mult { $$ = newBinaryNode("+", $1, $3); }
  | exp_aditiva '-' exp_mult { $$ = newBinaryNode("-", $1, $3); }
  | exp_mult { $$ = $1; }
  ;

exp_mult:
    exp_mult '*' exp_unaria { $$ = newBinaryNode("*", $1, $3); }
  | exp_mult '/' exp_unaria { $$ = newBinaryNode("/", $1, $3); }
  | exp_mult '%' exp_unaria { $$ = newBinaryNode("%", $1, $3); }
  | exp_unaria { $$ = $1; }
  ;

exp_unaria:
    '!' exp_unaria { $$ = newUnaryNode("!", $2); }
  | '-' exp_unaria { $$ = newUnaryNode("-", $2); }
  | INC exp_unaria { $$ = newUnaryNode("++pre", $2); }
  | DEC exp_unaria { $$ = newUnaryNode("--pre", $2); }
  | exp_postfija { $$ = $1; }
  ;

exp_postfija:
    exp_primaria { $$ = $1; }
  | exp_postfija INC { $$ = newUnaryNode("post++", $1); }
  | exp_postfija DEC { $$ = newUnaryNode("post--", $1); }
  ;

exp_primaria:
    ID opcion_llamada {
        if ($2 == NULL) { 
             $$ = newIDNode($1);
        } else { 
             $$ = newASTNode(NODE_CALL_FUNC);
             $$->data.str_val = $1;
             $$->left = $2; 
        }
    }
  | NUMERO { $$ = newIntNode($1); }
  | CADENA { 
        $$ = newASTNode(NODE_CONST_STR); 
        $$->data.str_val = $1; 
    }
  | TRUE { 
        $$ = newASTNode(NODE_CONST_BOOL); 
        $$->data.int_val = 1; 
    }
  | FALSE { 
        $$ = newASTNode(NODE_CONST_BOOL); 
        $$->data.int_val = 0; 
    }
  | '(' expresion ')' { $$ = $2; }
  | func_reservada { $$ = $1; }
  ;

opcion_llamada:
    '(' lista_argumentos ')' { $$ = $2; }
  | /* vacio */ { $$ = NULL; }
  ;

lista_argumentos:
    expresion ',' lista_argumentos { $$ = $1; $$->next = $3; }
  | expresion { $$ = $1; }
  | /* vacio */ { $$ = NULL; }
  ;

/* 6. FUNCIONES RESERVADAS (ROBOT) */
func_reservada:
    TOKEN_MOVER '(' lista_argumentos ')' { $$ = newRobotNode("mover", $3); }
  | TOKEN_RETROCEDER '(' lista_argumentos ')' { $$ = newRobotNode("retroceder", $3); }
  | TOKEN_GIRAR_IZQ '(' lista_argumentos ')' { $$ = newRobotNode("girarIzq", $3); }
  | TOKEN_GIRAR_DER '(' lista_argumentos ')' { $$ = newRobotNode("girarDer", $3); }
  | TOKEN_ESPERAR '(' lista_argumentos ')' { $$ = newRobotNode("esperar", $3); }
  | TOKEN_LEER_SENSOR '(' lista_argumentos ')' { $$ = newRobotNode("leerSensor", $3); }
  | TOKEN_PARAR '(' lista_argumentos ')' { $$ = newRobotNode("parar", $3); }
  ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error de sintaxis en linea %d: %s\n", yylineno, s);
}