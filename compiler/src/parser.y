%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast/ast.h"

/* Prototipos de funciones y variables externas utilizadas por el parser. */
int yylex(void);
void yyerror(const char *s);
extern int yylineno; 

/* Puntero global a la raíz del árbol sintáctico abstracto.
 * Se completa en la regla inicial 'Programa' y es consumido
 * por las fases posteriores del compilador.
 */
ASTNode *raiz_ast = NULL;
%}

/* Unión de valores semánticos asociados a los tokens y no terminales.
 * Permite que cada símbolo transporte información de tipos simples y nodos del AST.
 */
%union {
    char* cadena;
    double  numero;
    int     booleano;
    TipoDato tipo_dato;
    ASTNode* nodo;
}

/* 1. Definición de tokens.
 * Cada token corresponde a un patrón léxico reconocido por el lexer
 * y, cuando aplica, lleva un valor semántico asociado.
 */
%token <cadena> ID_TOKEN
%token <numero> NUMERO_TOKEN
%token <cadena> CADENA_TOKEN
%token <booleano> TRUE_TOKEN FALSE_TOKEN

%token TIPO_TOKEN_INT TIPO_TOKEN_BOOLEAN TIPO_TOKEN_FLOAT TIPO_TOKEN_CHAR TIPO_TOKEN_STRING
%token IF_TOKEN ELSE_TOKEN FOR_TOKEN WHILE_TOKEN DO_TOKEN VOID_TOKEN
%token SETUP_TOKEN
%token MOVER_TOKEN GIRARIZQ_TOKEN GIRARDER_TOKEN LEERSENSOR_TOKEN PARAR_TOKEN REVERSA_TOKEN ESPERAR_TOKEN
%token INICIO_TOKEN FIN_TOKEN
%token PAREN_IZQ_TOKEN PAREN_DER_TOKEN
%token CORCH_IZQ_TOKEN CORCH_DER_TOKEN
%token PUNTOYCOMA_TOKEN COMA_TOKEN

/* Conjunto de tokens para los operadores del lenguaje. */
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

/* 2. Definición de tipos para los no terminales (%type).
 * Se indica qué no terminales transportan punteros a nodos del AST
 * y cuáles se asocian a tipos auxiliares (por ejemplo, TipoDato).
 */
%type <nodo> Programa DeclaracionGlobal SetupDef FuncionDef ListaFunciones
%type <nodo> Bloque ListaInstrucciones Instruccion
%type <nodo> Declaracion ListaDeclaradores Declarador DeclaracionInit
%type <nodo> If IfPrima For ForInit ExpresionLogicaFor ForStep DoWhile While
%type <nodo> ListaParametros ListaParametrosCont Parametro TipoRetorno
%type <tipo_dato> Tipo
%type <nodo> ListaArgumentos ListaArgumentosCont

/* Tipos para la jerarquía explícita de expresiones.
 * Esta jerarquía codifica la precedencia y asociatividad mediante
 * la propia estructura de la gramática, en lugar de usar %left/%right.
 */
%type <nodo> Expresion ExpLogicaOr ExpLogicaAnd ExpComparacion
%type <nodo> ExpAditiva ExpMultiplicativa ExpUnaria ExpPostfija ExpPrimaria


/* La precedencia de operadores no se define con directivas de Bison.
 * Se modela de forma explícita mediante la jerarquía de no terminales
 * (ExpLogicaOr, ExpLogicaAnd, ExpComparacion, etc.).
 */

%start Programa
%%

/* --------------------------------------------- */
/* 1. Nivel superior del programa fuente         */
/* --------------------------------------------- */
/* La regla inicial 'Programa' construye la raíz
 * del AST, que encapsula declaraciones globales,
 * la definición de setup y una lista (posiblemente vacía)
 * de funciones definidas por el usuario al final del archivo.
 */
Programa
    : DeclaracionGlobal SetupDef ListaFunciones
    {
        $$ = crear_nodo(NODO_PROGRAMA, @1.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $2;
        $$->hijo3 = $3; /* $3 es la cabeza de la lista de funciones o un NODO_VACIO si no hay funciones. */
        raiz_ast = $$;
    }
    ;

/* DeclaracionGlobal representa las instrucciones ubicadas
 * antes de la definición de setup, modeladas como lista de nodos.
 */
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

/* SetupDef captura la definición especial 'void setup() { ... }'
 * donde se describe la inicialización principal para la plataforma objetivo.
 */
SetupDef
    : VOID_TOKEN SETUP_TOKEN PAREN_IZQ_TOKEN PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_SETUP, @1.first_line);
        $$->hijo1 = crear_nodo_tipo(TIPO_VOID, @1.first_line);
        $$->hijo2 = $5;
    }
    ;
    
/* --------------------------------------------- */
/* 2. Bloques y lista de instrucciones           */
/* --------------------------------------------- */
Bloque
    : INICIO_TOKEN ListaInstrucciones FIN_TOKEN
    {
        $$ = crear_nodo(NODO_BLOQUE, @1.first_line);
        $$->hijo1 = $2;
    }
    ;

/* ListaInstrucciones se representa como una lista enlazada de nodos
 * para recorrer secuencialmente el contenido de un bloque.
 */
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

/* --------------------------------------------- */
/* 3. Instrucciones                              */
/* --------------------------------------------- */
Instruccion
    : Declaracion PUNTOYCOMA_TOKEN    { $$ = $1; }
    | Expresion PUNTOYCOMA_TOKEN      { $$ = $1; }
    | If                              { $$ = $1; }
    | For                             { $$ = $1; }
    | While                           { $$ = $1; }
    | DoWhile                         { $$ = $1; }
    | Bloque                          { $$ = $1; }
    ;

/* --------------------------------------------- */
/* 4. Sentencias simples                         */
/* --------------------------------------------- */

/* Declaracion modela una sentencia de declaración
 * con uno o varios identificadores asociados a un tipo.
 */
Declaracion
    : Tipo ListaDeclaradores
    {
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);
        $$->hijo2 = $2; /* $2 es la lista de nodos Declarador */
    }
    ;

/* ListaDeclaradores enlaza varias ocurrencias de Declarador
 * para representar declaraciones múltiples separadas por comas.
 */
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

/* Declarador asocia un identificador con su posible inicialización
 * y se traduce en un nodo de declaración dentro del AST.
 */
Declarador
    : ID_TOKEN DeclaracionInit
    {
        /* Creamos un nodo 'Declaracion' por cada ID */
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_hoja_id($1, @1.first_line);
        $$->hijo2 = $2;
    }
    ;

/* DeclaracionInit representa la parte opcional de inicialización
 * de una declaración (asignación inicial o ausencia de valor).
 */
DeclaracionInit
    : ASIGN_TOKEN Expresion { $$ = $2; }
    | /* lambda */          { $$ = crear_nodo_vacio(); }
    ;
    
/* --------------------------------------------- */
/* 5. Estructuras de control                     */
/* --------------------------------------------- */
If
    : IF_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque IfPrima
    {
        $$ = crear_nodo(NODO_IF, @1.first_line);
        $$->hijo1 = $3; /* Expresión de condición */
        $$->hijo2 = $5; /* Bloque correspondiente a la rama 'then' */
        $$->hijo3 = $6; /* Bloque correspondiente a la rama 'else' (o nodo vacío) */
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
/* --------------------------------------------- */
/* 6. Definición de funciones                    */
/* --------------------------------------------- */
/* ListaFunciones modela la secuencia opcional de definiciones
 * de funciones de usuario situadas después de setup().
 * Se representa como una lista enlazada usando el campo 'siguiente';
 * la producción vacía se modela mediante un nodo NODO_VACIO.
 */
ListaFunciones
    : /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    | ListaFunciones FuncionDef
    {
        $$ = enlazar_nodos($1, $2);
    }
    ;

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

/* ListaParametros y ListaParametrosCont codifican una lista
 * opcional de parámetros formales enlazados como nodos del AST.
 */
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
/* --------------------------------------------- */
/* 7. Lista de argumentos                        */
/* --------------------------------------------- */

/* ListaArgumentos modela la secuencia de expresiones que se
 * pasan como argumentos en una llamada a función.
 */
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
/* --------------------------------------------- */
/* 8. Jerarquía de expresión                     */
/* --------------------------------------------- */

/* Nivel 1: Asignación (=)
 * Expresion es el nivel más alto e incluye tanto expresiones simples
 * como asignaciones.
 */
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

/* Nivel 7: Prefijos unarios (!, -, +, ++, --) */
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

/* Nivel 8: Postfijos (llamada, acceso a arreglo, ++, --) */
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

/* Nivel 9: Expresiones primarias (identificadores, literales y funciones reservadas) */
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
    | ESPERAR_TOKEN
        { $$ = crear_nodo_funcion_reservada("esperar", @1.first_line); }
    ;
%%

/* Función de reporte de errores sintácticos.
 * Se invoca automáticamente cuando Bison detecta una construcción inválida
 * y muestra la línea del programa fuente junto con un mensaje descriptivo.
 */
void yyerror(const char *s) {
    fprintf(stderr, "Error sintáctico en la línea %d: %s\n", yylineno, s);
}