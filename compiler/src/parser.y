%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast/ast.h"

/* Tipos auxiliares internos al parser para listas de operadores y postfijos.
 * Se declaran aquí porque únicamente son usados dentro de parser.y.
 */
typedef struct OpList {
    char* op;
    ASTNode* expr;
    struct OpList* next;
} OpList;

typedef enum {
    PF_CALL,
    PF_INDEX,
    PF_INC,
    PF_DEC
} PostfixKind;

typedef struct PostfixList {
    PostfixKind kind;
    ASTNode* arg;
    ASTNode* index;
    struct PostfixList* next;
} PostfixList;

/* Helpers para listas de operadores. */
static OpList* prepend_op(char* op, ASTNode* expr, OpList* tail) {
    OpList* node = (OpList*)malloc(sizeof(OpList));
    node->op = op;
    node->expr = expr;
    node->next = tail;
    return node;
}

static ASTNode* fold_left(ASTNode* first, OpList* ops) {
    ASTNode* acc = first;
    OpList* it = ops;
    while (it) {
        acc = crear_nodo_binario(it->op, acc, it->expr, acc->linea);
        OpList* tmp = it;
        it = it->next;
        free(tmp);
    }
    return acc;
}

/* Helpers para listas de postfijos. */
static PostfixList* prepend_pf(PostfixKind kind, ASTNode* arg, ASTNode* index, PostfixList* tail) {
    PostfixList* node = (PostfixList*)malloc(sizeof(PostfixList));
    node->kind = kind;
    node->arg = arg;
    node->index = index;
    node->next = tail;
    return node;
}

static ASTNode* apply_postfix(ASTNode* base, PostfixList* list, int linea) {
    ASTNode* acc = base;
    PostfixList* it = list;
    while (it) {
        switch (it->kind) {
            case PF_CALL: {
                ASTNode* call = crear_nodo(NODO_LLAMADA_FUNCION, linea);
                call->hijo1 = acc;
                call->hijo2 = it->arg;
                acc = call;
                break;
            }
            case PF_INDEX: {
                acc = crear_nodo_binario("[]", acc, it->index, linea);
                break;
            }
            case PF_INC: {
                acc = crear_nodo_postfix(acc, "++", linea);
                break;
            }
            case PF_DEC: {
                acc = crear_nodo_postfix(acc, "--", linea);
                break;
            }
        }
        PostfixList* tmp = it;
        it = it->next;
        free(tmp);
    }
    return acc;
}

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
    struct OpList* op_list;
    struct PostfixList* pf_list;
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
%type <nodo> Programa DeclGlobalsOpt DeclGlobalsOpt2 DeclGlobalItem SetupDef FuncionDef ListaFunciones ListaFunciones2
%type <nodo> Bloque ListaInstrucciones ListaInstrucciones2 Instruccion
%type <nodo> Declaracion ListaDeclaradores ListaDeclaradores2 Declarador DeclaracionInit
%type <nodo> If IfPrima For ForInit ExpresionLogicaFor ForStep DoWhile While
%type <nodo> ListaParametrosOpt ListaParametros2 Parametro TipoRetorno
%type <tipo_dato> Tipo
%type <nodo> ListaArgumentosOpt ListaArgumentos2
%type <op_list> ExpLogicaOrTail ExpLogicaAndTail ExpComparacionTail ExpAditivaTail ExpMultiplicativaTail
%type <pf_list> ExpPostfijaTail

/* Tipos para la jerarquía explícita de expresiones.
 * Esta jerarquía codifica la precedencia y asociatividad mediante
 * la propia estructura de la gramática, en lugar de usar %left/%right.
 */
%type <nodo> Expresion ExpresionPrime ExpLogicaOr ExpLogicaAnd ExpComparacion
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
    : DeclGlobalsOpt SetupDef ListaFunciones
    {
        $$ = crear_nodo(NODO_PROGRAMA, @1.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $2;
        $$->hijo3 = $3; /* $3 es la cabeza de la lista de funciones o un NODO_VACIO si no hay funciones. */
        raiz_ast = $$;
    }
    ;

/* Secuencia opcional de declaraciones/expresiones globales. */
DeclGlobalsOpt
    : DeclGlobalItem DeclGlobalsOpt2
    {
        $$ = enlazar_nodos($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

DeclGlobalsOpt2
    : DeclGlobalItem DeclGlobalsOpt2
    {
        $$ = enlazar_nodos($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

DeclGlobalItem
    : Declaracion PUNTOYCOMA_TOKEN { $$ = $1; }
    | Expresion PUNTOYCOMA_TOKEN   { $$ = $1; }
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
    : Instruccion ListaInstrucciones2
    {
        $$ = enlazar_instruccion($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

ListaInstrucciones2
    : Instruccion ListaInstrucciones2
    {
        $$ = enlazar_instruccion($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
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
    : Declarador ListaDeclaradores2
    {
        $$ = enlazar_nodos($1, $2);
    }
    ;

ListaDeclaradores2
    : COMA_TOKEN Declarador ListaDeclaradores2
    {
        $$ = enlazar_nodos($2, $3);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
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
    : FuncionDef ListaFunciones2
    {
        $$ = enlazar_nodos($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

ListaFunciones2
    : FuncionDef ListaFunciones2
    {
        $$ = enlazar_nodos($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

FuncionDef
    : TipoRetorno ID_TOKEN PAREN_IZQ_TOKEN ListaParametrosOpt PAREN_DER_TOKEN Bloque
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
ListaParametrosOpt
    : Parametro ListaParametros2
    {
        $$ = enlazar_parametro($1, $2);
    }
    | /* lambda */
    { 
        $$ = crear_nodo_vacio(); 
    }
    ;
ListaParametros2
    : COMA_TOKEN Parametro ListaParametros2
    {
        $$ = enlazar_parametro($2, $3);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
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
ListaArgumentosOpt
    : Expresion ListaArgumentos2
    {
        $$ = enlazar_argumento($1, $2);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

ListaArgumentos2
    : COMA_TOKEN Expresion ListaArgumentos2
    {
        $$ = enlazar_argumento($2, $3);
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
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
    : ExpLogicaOr ExpresionPrime
    {
        if ($2) {
            $$ = crear_nodo(NODO_ASIGNACION, @2.first_line);
            $$->hijo1 = $1;
            $$->hijo2 = $2;
        } else {
            $$ = $1;
        }
    }
    ;

ExpresionPrime
    : ASIGN_TOKEN Expresion { $$ = $2; }
    | /* lambda */          { $$ = NULL; }
    ;

/* Nivel 2: OR (||) */
ExpLogicaOr
    : ExpLogicaAnd ExpLogicaOrTail
    {
        $$ = fold_left($1, $2);
    }
    ;

ExpLogicaOrTail
    : OR_TOKEN ExpLogicaAnd ExpLogicaOrTail
    {
        $$ = prepend_op("||", $2, $3);
    }
    | /* lambda */
    {
        $$ = NULL;
    }
    ;

/* Nivel 3: AND (&&) */
ExpLogicaAnd
    : ExpComparacion ExpLogicaAndTail
    {
        $$ = fold_left($1, $2);
    }
    ;

ExpLogicaAndTail
    : AND_TOKEN ExpComparacion ExpLogicaAndTail
    {
        $$ = prepend_op("&&", $2, $3);
    }
    | /* lambda */
    {
        $$ = NULL;
    }
    ;

/* Nivel 4: Comparación (==, !=, <, <=, >, >=) */
ExpComparacion
    : ExpAditiva ExpComparacionTail
    {
        $$ = fold_left($1, $2);
    }
    ;

ExpComparacionTail
    : IGUAL_TOKEN ExpAditiva ExpComparacionTail        { $$ = prepend_op("==", $2, $3); }
    | NO_IGUAL_TOKEN ExpAditiva ExpComparacionTail     { $$ = prepend_op("!=", $2, $3); }
    | MENOR_TOKEN ExpAditiva ExpComparacionTail        { $$ = prepend_op("<",  $2, $3); }
    | MENOR_IGUAL_TOKEN ExpAditiva ExpComparacionTail  { $$ = prepend_op("<=", $2, $3); }
    | MAYOR_TOKEN ExpAditiva ExpComparacionTail        { $$ = prepend_op(">",  $2, $3); }
    | MAYOR_IGUAL_TOKEN ExpAditiva ExpComparacionTail  { $$ = prepend_op(">=", $2, $3); }
    | /* lambda */                                     { $$ = NULL; }
    ;

/* Nivel 5: Adición/Sustracción (+, -) */
ExpAditiva
    : ExpMultiplicativa ExpAditivaTail
    {
        $$ = fold_left($1, $2);
    }
    ;

ExpAditivaTail
    : MAS_TOKEN ExpMultiplicativa ExpAditivaTail
    {
        $$ = prepend_op("+", $2, $3);
    }
    | MENOS_TOKEN ExpMultiplicativa ExpAditivaTail
    {
        $$ = prepend_op("-", $2, $3);
    }
    | /* lambda */
    {
        $$ = NULL;
    }
    ;

/* Nivel 6: Multiplicación/División (*, /, %) */
ExpMultiplicativa
    : ExpUnaria ExpMultiplicativaTail
    {
        $$ = fold_left($1, $2);
    }
    ;

ExpMultiplicativaTail
    : MULT_TOKEN ExpUnaria ExpMultiplicativaTail { $$ = prepend_op("*", $2, $3); }
    | DIV_TOKEN ExpUnaria ExpMultiplicativaTail  { $$ = prepend_op("/", $2, $3); }
    | MOD_TOKEN ExpUnaria ExpMultiplicativaTail  { $$ = prepend_op("%", $2, $3); }
    | /* lambda */                               { $$ = NULL; }
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
    : ExpPrimaria ExpPostfijaTail
    {
        $$ = apply_postfix($1, $2, @1.first_line);
    }
    ;

ExpPostfijaTail
    : PAREN_IZQ_TOKEN ListaArgumentosOpt PAREN_DER_TOKEN ExpPostfijaTail
    {
        $$ = prepend_pf(PF_CALL, $2, NULL, $4);
    }
    | CORCH_IZQ_TOKEN Expresion CORCH_DER_TOKEN ExpPostfijaTail
    {
        $$ = prepend_pf(PF_INDEX, NULL, $2, $4);
    }
    | INC_TOKEN ExpPostfijaTail
    {
        $$ = prepend_pf(PF_INC, NULL, NULL, $2);
    }
    | DEC_TOKEN ExpPostfijaTail
    {
        $$ = prepend_pf(PF_DEC, NULL, NULL, $2);
    }
    | /* lambda */
    {
        $$ = NULL;
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
