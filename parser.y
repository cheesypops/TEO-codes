%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h" // Nuestra definición del AST

/* Prototipos */
int yylex(void);
void yyerror(const char *s);
extern int yylineno; // Variable de línea del lexer

ASTNode *raiz_ast = NULL; // Raíz del árbol completo
%}

%union {
    char* cadena;
    double  numero;
    int     booleano;
    TipoDato tipo_dato;
    ASTNode* nodo;
}

/* 1. Definición de Tokens (Sin cambios) */
%token <cadena> ID_TOKEN
%token <numero> NUMERO_TOKEN
%token <cadena> CADENA_TOKEN
%token <booleano> TRUE_TOKEN FALSE_TOKEN

%token TIPO_TOKEN_INT TIPO_TOKEN_BOOLEAN TIPO_TOKEN_FLOAT TIPO_TOKEN_CHAR TIPO_TOKEN_STRING
%token IF_TOKEN ELSE_TOKEN FOR_TOKEN WHILE_TOKEN DO_TOKEN VOID_TOKEN
%token SETUP_TOKEN
%token MOVER_TOKEN GIRARIZQ_TOKEN GIRARDER_TOKEN LEERSENSOR_TOKEN PARAR_TOKEN REVERSA_TOKEN
%token INICIO_TOKEN FIN_TOKEN
%token PAREN_IZQ_TOKEN PAREN_DER_TOKEN
%token CORCH_IZQ_TOKEN CORCH_DER_TOKEN
%token PUNTOYCOMA_TOKEN COMA_TOKEN

/* Operadores */
%token ASIGN_TOKEN /* = */
%token OR_TOKEN /* || */
%token AND_TOKEN /* && */
%token IGUAL_TOKEN /* == */ NO_IGUAL_TOKEN /* != */
%token MENOR_TOKEN /* < */ MENOR_IGUAL_TOKEN /* <= */
%token MAYOR_TOKEN /* > */ MAYOR_IGUAL_TOKEN /* >= */
%token MAS_TOKEN /* + */ MENOS_TOKEN /* - */
%token MULT_TOKEN /* * */ DIV_TOKEN /* / */ MOD_TOKEN /* % */
%token NOT_TOKEN /* ! */
%token INC_TOKEN /* ++ */ DEC_TOKEN /* -- */

/* 2. Definición de Tipos de Nodos (%type) */
/* (Adaptado a la nueva GIC) */
%type <nodo> Programa DeclaracionGlobal SetupDef FuncionDef
%type <nodo> Bloque ListaInstrucciones Instruccion
%type <nodo> Declaracion ListaDeclaradores Declarador DeclaracionInit
%type <nodo> If IfPrima For ForInit ExpresionLogicaFor ForStep DoWhile While
%type <nodo> ListaParametros ListaParametrosCont Parametro TipoRetorno
%type <tipo_dato> Tipo
%type <nodo> ListaArgumentos ListaArgumentosCont

/* Nuevos tipos para la jerarquía de expresión explícita */
%type <nodo> Expresion ExpLogicaOr ExpLogicaAnd ExpComparacion
%type <nodo> ExpAditiva ExpMultiplicativa ExpUnaria ExpPostfija ExpPrimaria


/* 3. JERARQUÍA DE PRECEDENCIA */
/*
 * ¡ELIMINADA!
 * La precedencia ahora es manejada por las propias reglas de la gramática
 * (Camino A). No se necesita %left, %right, ni %prec.
 */

%start Programa
%%

/* ------------------------------------ */
/* 1. Nivel Superior                    */
/* ------------------------------------ */
Programa
    : DeclaracionGlobal SetupDef FuncionDef
    {
        $$ = crear_nodo(NODO_PROGRAMA, @1.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $2;
        $$->hijo3 = $3;
        raiz_ast = $$;
    }
    ;

DeclaracionGlobal
    : /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    | DeclaracionGlobal Declaracion PUNTOYCOMA_TOKEN
    {
        $$ = enlazar_instruccion($1, $2);
    }
    | DeclaracionGlobal Expresion PUNTOYCOMA_TOKEN
    {
        $$ = enlazar_instruccion($1, $2);
    }
    ;

SetupDef
    : VOID_TOKEN SETUP_TOKEN PAREN_IZQ_TOKEN PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_SETUP, @1.first_line);
        $$->hijo1 = crear_nodo_tipo(TIPO_VOID, @1.first_line);
        $$->hijo2 = $5;
    }
    ;
    
/* ------------------------------------ */
/* 2. Bloques y Lista de Instrucciones  */
/* ------------------------------------ */
Bloque
    : INICIO_TOKEN ListaInstrucciones FIN_TOKEN
    {
        $$ = crear_nodo(NODO_BLOQUE, @1.first_line);
        $$->hijo1 = $2;
    }
    ;

/* GIC: ListaInstrucciones -> ListaInstrucciones Instruccion | λ */
ListaInstrucciones
    : /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    | ListaInstrucciones Instruccion
    {
        $$ = enlazar_instruccion($1, $2);
    }
    ;

/* ------------------------------------ */
/* 3. Instrucciones                     */
/* ------------------------------------ */
Instruccion
    : Declaracion PUNTOYCOMA_TOKEN    { $$ = $1; }
    | Expresion PUNTOYCOMA_TOKEN      { $$ = $1; }
    | If                              { $$ = $1; }
    | For                             { $$ = $1; }
    | While                           { $$ = $1; }
    | DoWhile                         { $$ = $1; }
    | Bloque                          { $$ = $1; }
    ;

/* ------------------------------------ */
/* 4. Sentencias Simples (Corregida)    */
/* ------------------------------------ */

/* GIC: Declaracion -> Tipo ListaDeclaradores */
Declaracion
    : Tipo ListaDeclaradores
    {
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);
        $$->hijo2 = $2; /* $2 es la lista de nodos Declarador */
    }
    ;

/* GIC: ListaDeclaradores -> ListaDeclaradores , Declarador | Declarador */
ListaDeclaradores
    : Declarador
    {
        $$ = $1;
    }
    | ListaDeclaradores COMA_TOKEN Declarador
    {
        $$ = enlazar_nodos($1, $3); /* enlazar_nodos es de ast.c */
    }
    ;

/* GIC: Declarador -> id_token DeclaracionInit */
Declarador
    : ID_TOKEN DeclaracionInit
    {
        /* Creamos un nodo 'Declaracion' por cada ID */
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_hoja_id($1, @1.first_line);
        $$->hijo2 = $2;
    }
    ;

/* GIC: DeclaracionInit -> = Expresion | λ */
DeclaracionInit
    : ASIGN_TOKEN Expresion { $$ = $2; }
    | /* lambda */          { $$ = crear_nodo_vacio(); }
    ;
    
/* ------------------------------------ */
/* 5. Estructuras de Control            */
/* ------------------------------------ */
/* (Sin cambios, ya usaban 'Expresion') */
If
    : IF_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque IfPrima
    {
        $$ = crear_nodo(NODO_IF, @1.first_line);
        $$->hijo1 = $3; // Expresion (condición)
        $$->hijo2 = $5; // Bloque (then)
        $$->hijo3 = $6; // Bloque (else) o Vacio
    }
    ;
IfPrima
    : ELSE_TOKEN Bloque   { $$ = $2; }
    | /* lambda */        { $$ = crear_nodo_vacio(); }
    ;
For
    : FOR_TOKEN PAREN_IZQ_TOKEN ForInit PUNTOYCOMA_TOKEN ExpresionLogicaFor PUNTOYCOMA_TOKEN ForStep PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_FOR, @1.first_line);
        $$->hijo1 = $3;
        $$->hijo2 = $5;
        $$->hijo3 = $7;
        $$->hijo4 = $9;
    }
    ;
ForInit
    : Declaracion   { $$ = $1; }
    | Expresion     { $$ = $1; } /* Sigue usando la regla Expresion de alto nivel */
    | /* lambda */  { $$ = crear_nodo_vacio(); }
    ;
ExpresionLogicaFor
    : Expresion     { $$ = $1; }
    | /* lambda */  { $$ = crear_nodo_vacio(); }
    ;
ForStep
    : Expresion     { $$ = $1; }
    | /* lambda */  { $$ = crear_nodo_vacio(); }
    ;
DoWhile
    : DO_TOKEN INICIO_TOKEN Bloque FIN_TOKEN WHILE_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN PUNTOYCOMA_TOKEN
    {
        $$ = crear_nodo(NODO_DO_WHILE, @1.first_line);
        $$->hijo1 = $3;
        $$->hijo2 = $7;
    }
    ;
While
    : WHILE_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_WHILE, @1.first_line);
        $$->hijo1 = $3;
        $$->hijo2 = $5;
    }
    ;
/* ------------------------------------ */
/* 6. Definición de Funciones (Corregida)*/
/* ------------------------------------ */
FuncionDef
    : TipoRetorno ID_TOKEN PAREN_IZQ_TOKEN ListaParametros PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_FUNCION_DEF, @1.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = crear_nodo_hoja_id($2, @2.first_line);
        $$->hijo3 = $4;
        $$->hijo4 = $6;
    }
    ;

/* GIC: ListaParametros -> ListaParametros , Parametro | Parametro | λ */
/* Implementado como una lista opcional (recursiva izq.) */
ListaParametros
    : /* lambda */
    { 
        $$ = crear_nodo_vacio(); 
    }
    | ListaParametrosCont
    {
        $$ = $1;
    }
    ;
ListaParametrosCont
    : Parametro
    {
        $$ = $1;
    }
    | ListaParametrosCont COMA_TOKEN Parametro
    {
        $$ = enlazar_parametro($1, $3);
    }
    ;

Parametro
    : Tipo ID_TOKEN
    {
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);
        $$->hijo2 = crear_nodo_hoja_id($2, @2.first_line);
    }
    ;
TipoRetorno
    : VOID_TOKEN { $$ = crear_nodo_tipo(TIPO_VOID, @1.first_line); }
    | Tipo       { $$ = crear_nodo_tipo($1, @1.first_line); }
    ;
Tipo
    : TIPO_TOKEN_INT      { $$ = TIPO_INT; }
    | TIPO_TOKEN_BOOLEAN  { $$ = TIPO_BOOLEAN; }
    | TIPO_TOKEN_FLOAT    { $$ = TIPO_FLOAT; }
    | TIPO_TOKEN_CHAR     { $$ = TIPO_CHAR; }
    | TIPO_TOKEN_STRING   { $$ = TIPO_STRING; }
    ;
/* ------------------------------------ */
/* 7. Lista de Argumentos (Corregida)   */
/* ------------------------------------ */

/* GIC: ListaArgumentos -> ListaArgumentos , Expresion | Expresion | λ */
ListaArgumentos
    : /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    | ListaArgumentosCont
    {
        $$ = $1;
    }
    ;
ListaArgumentosCont
    : Expresion
    {
        $$ = $1;
    }
    | ListaArgumentosCont COMA_TOKEN Expresion
    {
        $$ = enlazar_argumento($1, $3);
    }
    ;
/* ------------------------------------ */
/* 8. JERARQUÍA DE EXPRESIÓN (NUEVA)    */
/* ------------------------------------ */

/* Nivel 1: Asignación (=) */
Expresion
    : ExpLogicaOr ASIGN_TOKEN Expresion
    {
        $$ = crear_nodo(NODO_ASIGNACION, @2.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $3;
    }
    | ExpLogicaOr
    {
        $$ = $1;
    }
    ;

/* Nivel 2: OR (||) */
ExpLogicaOr
    : ExpLogicaOr OR_TOKEN ExpLogicaAnd
    {
        $$ = crear_nodo_binario("||", $1, $3, @2.first_line);
    }
    | ExpLogicaAnd
    {
        $$ = $1;
    }
    ;

/* Nivel 3: AND (&&) */
ExpLogicaAnd
    : ExpLogicaAnd AND_TOKEN ExpComparacion
    {
        $$ = crear_nodo_binario("&&", $1, $3, @2.first_line);
    }
    | ExpComparacion
    {
        $$ = $1;
    }
    ;

/* Nivel 4: Comparación (==, !=, <, <=, >, >=) */
ExpComparacion
    : ExpComparacion IGUAL_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario("==", $1, $3, @2.first_line);
    }
    | ExpComparacion NO_IGUAL_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario("!=", $1, $3, @2.first_line);
    }
    | ExpComparacion MENOR_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario("<", $1, $3, @2.first_line);
    }
    | ExpComparacion MENOR_IGUAL_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario("<=", $1, $3, @2.first_line);
    }
    | ExpComparacion MAYOR_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario(">", $1, $3, @2.first_line);
    }
    | ExpComparacion MAYOR_IGUAL_TOKEN ExpAditiva
    {
        $$ = crear_nodo_binario(">=", $1, $3, @2.first_line);
    }
    | ExpAditiva
    {
        $$ = $1;
    }
    ;

/* Nivel 5: Adición/Sustracción (+, -) */
ExpAditiva
    : ExpAditiva MAS_TOKEN ExpMultiplicativa
    {
        $$ = crear_nodo_binario("+", $1, $3, @2.first_line);
    }
    | ExpAditiva MENOS_TOKEN ExpMultiplicativa
    {
        $$ = crear_nodo_binario("-", $1, $3, @2.first_line);
    }
    | ExpMultiplicativa
    {
        $$ = $1;
    }
    ;

/* Nivel 6: Multiplicación/División (*, /, %) */
ExpMultiplicativa
    : ExpMultiplicativa MULT_TOKEN ExpUnaria
    {
        $$ = crear_nodo_binario("*", $1, $3, @2.first_line);
    }
    | ExpMultiplicativa DIV_TOKEN ExpUnaria
    {
        $$ = crear_nodo_binario("/", $1, $3, @2.first_line);
    }
    | ExpMultiplicativa MOD_TOKEN ExpUnaria
    {
        $$ = crear_nodo_binario("%", $1, $3, @2.first_line);
    }
    | ExpUnaria
    {
        $$ = $1;
    }
    ;

/* Nivel 7: Prefijos Unarios (!, -, +, ++, --) */
ExpUnaria
    : NOT_TOKEN ExpUnaria
    {
        $$ = crear_nodo_unario("!", $2, @1.first_line);
    }
    | MENOS_TOKEN ExpUnaria
    {
        $$ = crear_nodo_unario("-", $2, @1.first_line);
    }
    | MAS_TOKEN ExpUnaria
    {
        $$ = crear_nodo_unario("+", $2, @1.first_line);
    }
    | INC_TOKEN ExpUnaria
    {
        $$ = crear_nodo_unario("++", $2, @1.first_line);
    }
    | DEC_TOKEN ExpUnaria
    {
        $$ = crear_nodo_unario("--", $2, @1.first_line);
    }
    | ExpPostfija
    {
        $$ = $1;
    }
    ;

/* Nivel 8: Postfijos (llamada, array, ++, --) */
ExpPostfija
    : ExpPrimaria
    {
        $$ = $1;
    }
    | ExpPostfija PAREN_IZQ_TOKEN ListaArgumentos PAREN_DER_TOKEN
    {
        $$ = crear_nodo(NODO_LLAMADA_FUNCION, @2.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $3;
    }
    | ExpPostfija CORCH_IZQ_TOKEN Expresion CORCH_DER_TOKEN
    {
        $$ = crear_nodo_binario("[]", $1, $3, @2.first_line);
    }
    | ExpPostfija INC_TOKEN
    {
        $$ = crear_nodo_postfix($1, "++", @2.first_line);
    }
    | ExpPostfija DEC_TOKEN
    {
        $$ = crear_nodo_postfix($1, "--", @2.first_line);
    }
    ;

/* Nivel 9: Base (Primarios) */
ExpPrimaria
    : ID_TOKEN
        { $$ = crear_nodo_hoja_id($1, @1.first_line); }
    | NUMERO_TOKEN
        { $$ = crear_nodo_hoja_num($1, @1.first_line); }
    | CADENA_TOKEN
        { $$ = crear_nodo_hoja_cadena($1, @1.first_line); }
    | TRUE_TOKEN
        { $$ = crear_nodo_hoja_bool(1, @1.first_line); }
    | FALSE_TOKEN
        { $$ = crear_nodo_hoja_bool(0, @1.first_line); }
    | PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN /* Paréntesis para agrupar */
        { $$ = $2; }
    /* Funciones reservadas */
    | MOVER_TOKEN
        { $$ = crear_nodo_funcion_reservada("mover", @1.first_line); }
    | GIRARIZQ_TOKEN
        { $$ = crear_nodo_funcion_reservada("girarIzq", @1.first_line); }
    | GIRARDER_TOKEN
        { $$ = crear_nodo_funcion_reservada("girarDer", @1.first_line); }
    | LEERSENSOR_TOKEN
        { $$ = crear_nodo_funcion_reservada("leerSensor", @1.first_line); }
    | PARAR_TOKEN
        { $$ = crear_nodo_funcion_reservada("parar", @1.first_line); }
    | REVERSA_TOKEN
        { $$ = crear_nodo_funcion_reservada("reversa", @1.first_line); }
    ;
%%

/* Función de reporte de errores */
void yyerror(const char *s) {
    fprintf(stderr, "Error Sintáctico en línea %d: %s\n", yylineno, s);
}