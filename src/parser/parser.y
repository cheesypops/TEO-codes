/*
 * Analizador Sintáctico (Parser).
 * 
 * Este archivo contiene la especificación de la gramática en formato Bison.
 * El parser construye un Árbol Sintáctico Abstracto (AST) a partir del flujo
 * de tokens generado por el analizador léxico.
 * 
 * GENERADO POR: Bison (yacc) - este archivo se compila para generar parser.tab.c
 * 
 * ESTRATEGIA: La gramática utiliza recursión por la izquierda para manejar
 * listas y establece la precedencia de operadores mediante la estructura
 * jerárquica de las reglas gramaticales.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ast/ast.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;

/* Raíz del AST completo - se establece al completar el parseo del programa */
ASTNode *raiz_ast = NULL;
%}

/**
 * Unión de tipos para valores semánticos de tokens y no terminales.
 * 
 * Bison utiliza esta unión para almacenar los valores asociados a cada token
 * y no terminal durante el parseo.
 */
%union {
    char* cadena;        /* Identificadores y literales de cadena */
    double  numero;      /* Literales numéricos */
    int     booleano;    /* Literales booleanos */
    TipoDato tipo_dato;  /* Tipos de datos */
    ASTNode* nodo;       /* Nodos del AST */
}

/* ========== Definición de Tokens ========== */

/* Tokens con valores asociados */
%token <cadena> ID_TOKEN
%token <numero> NUMERO_TOKEN
%token <cadena> CADENA_TOKEN
%token <booleano> TRUE_TOKEN FALSE_TOKEN

/* Palabras reservadas de tipos */
%token TIPO_TOKEN_INT TIPO_TOKEN_BOOLEAN TIPO_TOKEN_FLOAT TIPO_TOKEN_CHAR TIPO_TOKEN_STRING

/* Palabras reservadas de control */
%token IF_TOKEN ELSE_TOKEN FOR_TOKEN WHILE_TOKEN DO_TOKEN VOID_TOKEN

/* Función especial del lenguaje */
%token SETUP_TOKEN

/* Funciones reservadas del lenguaje */
%token MOVER_TOKEN GIRARIZQ_TOKEN GIRARDER_TOKEN LEERSENSOR_TOKEN PARAR_TOKEN REVERSA_TOKEN

/* Delimitadores */
%token INICIO_TOKEN FIN_TOKEN
%token PAREN_IZQ_TOKEN PAREN_DER_TOKEN
%token CORCH_IZQ_TOKEN CORCH_DER_TOKEN
%token PUNTOYCOMA_TOKEN COMA_TOKEN

/* Operadores */
%token ASIGN_TOKEN OR_TOKEN AND_TOKEN
%token IGUAL_TOKEN NO_IGUAL_TOKEN
%token MENOR_TOKEN MENOR_IGUAL_TOKEN MAYOR_TOKEN MAYOR_IGUAL_TOKEN
%token MAS_TOKEN MENOS_TOKEN MULT_TOKEN DIV_TOKEN MOD_TOKEN
%token NOT_TOKEN INC_TOKEN DEC_TOKEN

/* ========== Tipos de No Terminales ========== */

/* No terminales que producen nodos AST */
%type <nodo> Programa DeclaracionGlobal SetupDef FuncionDef
%type <nodo> Bloque ListaInstrucciones Instruccion
%type <nodo> Declaracion ListaDeclaradores Declarador DeclaracionInit
%type <nodo> If IfPrima For ForInit ExpresionLogicaFor ForStep DoWhile While
%type <nodo> ListaParametros ListaParametrosCont Parametro TipoRetorno
%type <nodo> ListaArgumentos ListaArgumentosCont
%type <nodo> Expresion ExpLogicaOr ExpLogicaAnd ExpComparacion
%type <nodo> ExpAditiva ExpMultiplicativa ExpUnaria ExpPostfija ExpPrimaria

/* No terminales que producen tipos de datos */
%type <tipo_dato> Tipo

/* NOTA: La precedencia de operadores se maneja mediante la estructura
 * jerárquica de la gramática, no mediante directivas %left/%right.
 * Los niveles de precedencia (de menor a mayor) son:
 * 1. Asignación (=)
 * 2. OR lógico (||)
 * 3. AND lógico (&&)
 * 4. Comparaciones (==, !=, <, <=, >, >=)
 * 5. Suma/Resta (+, -)
 * 6. Multiplicación/División/Módulo (*, /, %)
 * 7. Operadores unarios (!, -, +, ++, --)
 * 8. Postfijos (++, --, llamadas, acceso a array)
 * 9. Primitivos (literales, identificadores, paréntesis)
 */

%start Programa
%%

/* ========== Reglas Gramaticales ========== */

/* Regla inicial: un programa consiste en declaraciones globales, setup y funciones */
Programa
    : DeclaracionGlobal SetupDef FuncionDef
    {
        /* Construir nodo raíz del programa */
        $$ = crear_nodo(NODO_PROGRAMA, @1.first_line);
        $$->hijo1 = $1;  /* Declaraciones globales */
        $$->hijo2 = $2;  /* Función setup() */
        $$->hijo3 = $3;  /* Definiciones de funciones */
        raiz_ast = $$;   /* Guardar referencia global para acceso posterior */
    }
    ;

/* Declaraciones globales: lista de declaraciones y expresiones al nivel superior */
DeclaracionGlobal
    : /* lambda - puede estar vacío */
    {
        $$ = crear_nodo_vacio();
    }
    | DeclaracionGlobal Declaracion PUNTOYCOMA_TOKEN
    {
        /* Agregar declaración a la lista */
        $$ = enlazar_instruccion($1, $2);
    }
    | DeclaracionGlobal Expresion PUNTOYCOMA_TOKEN
    {
        /* Permitir expresiones como instrucciones (asignaciones, llamadas) */
        $$ = enlazar_instruccion($1, $2);
    }
    ;

/* Función setup(): función especial sin parámetros que se ejecuta al inicio */
SetupDef
    : VOID_TOKEN SETUP_TOKEN PAREN_IZQ_TOKEN PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_SETUP, @1.first_line);
        $$->hijo1 = crear_nodo_tipo(TIPO_VOID, @1.first_line);  /* Tipo de retorno */
        $$->hijo2 = $5;  /* Cuerpo de la función */
    }
    ;
    
/* Bloques y listas de instrucciones */
Bloque
    : INICIO_TOKEN ListaInstrucciones FIN_TOKEN
    {
        /* Bloque delimitado por llaves { } */
        $$ = crear_nodo(NODO_BLOQUE, @1.first_line);
        $$->hijo1 = $2;  /* Lista de instrucciones dentro del bloque */
    }
    ;

/* Lista de instrucciones: recursión por la izquierda para construir lista enlazada */
ListaInstrucciones
    : /* lambda - lista vacía */
    {
        $$ = crear_nodo_vacio();
    }
    | ListaInstrucciones Instruccion
    {
        /* Agregar instrucción al final de la lista */
        $$ = enlazar_instruccion($1, $2);
    }
    ;

/* Instrucciones: cualquier construcción que puede aparecer como statement */
Instruccion
    : Declaracion PUNTOYCOMA_TOKEN    { $$ = $1; }
    | Expresion PUNTOYCOMA_TOKEN      { $$ = $1; }  /* Asignaciones, llamadas */
    | If                              { $$ = $1; }
    | For                             { $$ = $1; }
    | While                           { $$ = $1; }
    | DoWhile                         { $$ = $1; }
    | Bloque                          { $$ = $1; }  /* Bloque anidado */
    ;

/* Declaraciones: tipo seguido de lista de declaradores */
Declaracion
    : Tipo ListaDeclaradores
    {
        /* Ejemplo: int x, y = 5; */
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);  /* Tipo base */
        $$->hijo2 = $2;  /* Lista de variables declaradas */
    }
    ;

/* Lista de declaradores: permite múltiples variables en una declaración */
ListaDeclaradores
    : Declarador
    {
        /* Caso base: un solo declarador */
        $$ = $1;
    }
    | ListaDeclaradores COMA_TOKEN Declarador
    {
        /* Agregar declarador a la lista (ej: int x, y, z;) */
        $$ = enlazar_nodos($1, $3);
    }
    ;

/* Declarador: identificador con inicialización opcional */
Declarador
    : ID_TOKEN DeclaracionInit
    {
        /* Ejemplo: x o x = 5 */
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_hoja_id($1, @1.first_line);  /* Nombre de la variable */
        $$->hijo2 = $2;  /* Inicialización (puede ser NODO_VACIO) */
    }
    ;

/* Inicialización opcional en declaración */
DeclaracionInit
    : ASIGN_TOKEN Expresion { $$ = $2; }  /* Variable con valor inicial */
    | /* lambda */          { $$ = crear_nodo_vacio(); }  /* Variable sin inicializar */
    ;
    
/* ========== Estructuras de Control ========== */

/* Estructura condicional if-else */
If
    : IF_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque IfPrima
    {
        /* if (condición) { bloque } else { bloque } */
        $$ = crear_nodo(NODO_IF, @1.first_line);
        $$->hijo1 = $3;  /* Condición */
        $$->hijo2 = $5;  /* Bloque then */
        $$->hijo3 = $6;  /* Bloque else (puede ser NODO_VACIO) */
    }
    ;
IfPrima
    : ELSE_TOKEN Bloque   { $$ = $2; }  /* Cláusula else presente */
    | /* lambda */        { $$ = crear_nodo_vacio(); }  /* Sin else */
    ;

/* Bucle for: for (init; condición; paso) { cuerpo } */
For
    : FOR_TOKEN PAREN_IZQ_TOKEN ForInit PUNTOYCOMA_TOKEN ExpresionLogicaFor PUNTOYCOMA_TOKEN ForStep PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_FOR, @1.first_line);
        $$->hijo1 = $3;  /* Inicialización (puede ser declaración o expresión) */
        $$->hijo2 = $5;  /* Condición de continuación */
        $$->hijo3 = $7;  /* Expresión de paso */
        $$->hijo4 = $9;  /* Cuerpo del bucle */
    }
    ;
ForInit
    : Declaracion   { $$ = $1; }  /* Declaración de variable de control */
    | Expresion     { $$ = $1; }  /* Expresión de inicialización */
    | /* lambda */  { $$ = crear_nodo_vacio(); }  /* Sin inicialización */
    ;
ExpresionLogicaFor
    : Expresion     { $$ = $1; }  /* Condición de continuación */
    | /* lambda */  { $$ = crear_nodo_vacio(); }  /* Bucle infinito */
    ;
ForStep
    : Expresion     { $$ = $1; }  /* Expresión de incremento/decremento */
    | /* lambda */  { $$ = crear_nodo_vacio(); }  /* Sin paso */
    ;

/* Bucle do-while: do { bloque } while (condición); */
DoWhile
    : DO_TOKEN INICIO_TOKEN Bloque FIN_TOKEN WHILE_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN PUNTOYCOMA_TOKEN
    {
        $$ = crear_nodo(NODO_DO_WHILE, @1.first_line);
        $$->hijo1 = $3;  /* Cuerpo del bucle */
        $$->hijo2 = $7;  /* Condición (se evalúa después de ejecutar el cuerpo) */
    }
    ;

/* Bucle while: while (condición) { bloque } */
While
    : WHILE_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_WHILE, @1.first_line);
        $$->hijo1 = $3;  /* Condición (se evalúa antes del cuerpo) */
        $$->hijo2 = $5;  /* Cuerpo del bucle */
    }
    ;

/* ========== Definiciones de Funciones ========== */

/* Definición de función: tipo_retorno nombre (parámetros) { cuerpo } */
FuncionDef
    : TipoRetorno ID_TOKEN PAREN_IZQ_TOKEN ListaParametros PAREN_DER_TOKEN Bloque
    {
        /* Ejemplo: int suma(int a, int b) { ... } */
        $$ = crear_nodo(NODO_FUNCION_DEF, @1.first_line);
        $$->hijo1 = $1;  /* Tipo de retorno */
        $$->hijo2 = crear_nodo_hoja_id($2, @2.first_line);  /* Nombre de la función */
        $$->hijo3 = $4;  /* Lista de parámetros */
        $$->hijo4 = $6;  /* Cuerpo de la función */
    }
    ;

/* Lista de parámetros formales de una función */
ListaParametros
    : /* lambda - función sin parámetros */
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
        /* Caso base: un solo parámetro */
        $$ = $1;
    }
    | ListaParametrosCont COMA_TOKEN Parametro
    {
        /* Agregar parámetro a la lista (ej: int a, int b, int c) */
        $$ = enlazar_parametro($1, $3);
    }
    ;

/* Parámetro formal: tipo nombre */
Parametro
    : Tipo ID_TOKEN
    {
        /* Ejemplo: int x */
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);  /* Tipo del parámetro */
        $$->hijo2 = crear_nodo_hoja_id($2, @2.first_line);  /* Nombre del parámetro */
    }
    ;

/* Tipo de retorno de función: void o tipo primitivo */
TipoRetorno
    : VOID_TOKEN { $$ = crear_nodo_tipo(TIPO_VOID, @1.first_line); }
    | Tipo       { $$ = crear_nodo_tipo($1, @1.first_line); }
    ;

/* Tipos de datos primitivos del lenguaje */
Tipo
    : TIPO_TOKEN_INT      { $$ = TIPO_INT; }
    | TIPO_TOKEN_BOOLEAN  { $$ = TIPO_BOOLEAN; }
    | TIPO_TOKEN_FLOAT    { $$ = TIPO_FLOAT; }
    | TIPO_TOKEN_CHAR     { $$ = TIPO_CHAR; }
    | TIPO_TOKEN_STRING   { $$ = TIPO_STRING; }
    ;

/* ========== Jerarquía de Expresiones (por Precedencia) ========== */

/* Lista de argumentos en llamada a función */
ListaArgumentos
    : /* lambda - llamada sin argumentos */
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
        /* Caso base: un solo argumento */
        $$ = $1;
    }
    | ListaArgumentosCont COMA_TOKEN Expresion
    {
        /* Agregar argumento a la lista (ej: f(a, b, c)) */
        $$ = enlazar_argumento($1, $3);
    }
    ;

/* ========== Jerarquía de Expresiones por Precedencia ========== */
/* 
 * La precedencia se establece mediante la estructura jerárquica de la gramática.
 * Cada nivel representa un nivel de precedencia, de menor a mayor:
 */

/* Nivel 1: Asignación (=) - menor precedencia, asociativa por la derecha */
Expresion
    : ExpLogicaOr ASIGN_TOKEN Expresion
    {
        /* Asignación: x = y (asociativa por la derecha) */
        $$ = crear_nodo(NODO_ASIGNACION, @2.first_line);
        $$->hijo1 = $1;  /* L-value (debe ser identificador) */
        $$->hijo2 = $3;  /* Expresión a asignar */
    }
    | ExpLogicaOr
    {
        /* Pasar al siguiente nivel si no hay asignación */
        $$ = $1;
    }
    ;

/* Nivel 2: OR lógico (||) - asociativo por la izquierda */
ExpLogicaOr
    : ExpLogicaOr OR_TOKEN ExpLogicaAnd
    {
        /* Evaluación cortocircuitada: si izquierda es true, no evalúa derecha */
        $$ = crear_nodo_binario("||", $1, $3, @2.first_line);
    }
    | ExpLogicaAnd
    {
        $$ = $1;
    }
    ;

/* Nivel 3: AND lógico (&&) - asociativo por la izquierda */
ExpLogicaAnd
    : ExpLogicaAnd AND_TOKEN ExpComparacion
    {
        /* Evaluación cortocircuitada: si izquierda es false, no evalúa derecha */
        $$ = crear_nodo_binario("&&", $1, $3, @2.first_line);
    }
    | ExpComparacion
    {
        $$ = $1;
    }
    ;

/* Nivel 4: Operadores de comparación (==, !=, <, <=, >, >=) */
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
        /* Pasar al siguiente nivel si no hay comparación */
        $$ = $1;
    }
    ;

/* Nivel 5: Suma y Resta (+, -) - asociativos por la izquierda */
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

/* Nivel 6: Multiplicación, División y Módulo (*, /, %) - asociativos por la izquierda */
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
        /* Módulo solo aplica a enteros */
        $$ = crear_nodo_binario("%", $1, $3, @2.first_line);
    }
    | ExpUnaria
    {
        $$ = $1;
    }
    ;

/* Nivel 7: Operadores unarios prefijos (!, -, +, ++, --) */
ExpUnaria
    : NOT_TOKEN ExpUnaria
    {
        /* Negación lógica: !expresión */
        $$ = crear_nodo_unario("!", $2, @1.first_line);
    }
    | MENOS_TOKEN ExpUnaria
    {
        /* Negación aritmética: -expresión */
        $$ = crear_nodo_unario("-", $2, @1.first_line);
    }
    | MAS_TOKEN ExpUnaria
    {
        /* Operador unario positivo: +expresión (raro pero válido) */
        $$ = crear_nodo_unario("+", $2, @1.first_line);
    }
    | INC_TOKEN ExpUnaria
    {
        /* Incremento prefijo: ++variable (incrementa antes de usar) */
        $$ = crear_nodo_unario("++", $2, @1.first_line);
    }
    | DEC_TOKEN ExpUnaria
    {
        /* Decremento prefijo: --variable (decrementa antes de usar) */
        $$ = crear_nodo_unario("--", $2, @1.first_line);
    }
    | ExpPostfija
    {
        $$ = $1;
    }
    ;

/* Nivel 8: Postfijos y acceso (llamadas, arrays, ++, --) */
ExpPostfija
    : ExpPrimaria
    {
        /* Caso base: expresión primaria */
        $$ = $1;
    }
    | ExpPostfija PAREN_IZQ_TOKEN ListaArgumentos PAREN_DER_TOKEN
    {
        /* Llamada a función: función(argumentos) */
        $$ = crear_nodo(NODO_LLAMADA_FUNCION, @2.first_line);
        $$->hijo1 = $1;  /* Función a llamar */
        $$->hijo2 = $3;  /* Lista de argumentos */
    }
    | ExpPostfija CORCH_IZQ_TOKEN Expresion CORCH_DER_TOKEN
    {
        /* Acceso a array: array[índice] */
        $$ = crear_nodo_binario("[]", $1, $3, @2.first_line);
    }
    | ExpPostfija INC_TOKEN
    {
        /* Incremento postfijo: variable++ (usa valor, luego incrementa) */
        $$ = crear_nodo_postfix($1, "++", @2.first_line);
    }
    | ExpPostfija DEC_TOKEN
    {
        /* Decremento postfijo: variable-- (usa valor, luego decrementa) */
        $$ = crear_nodo_postfix($1, "--", @2.first_line);
    }
    ;

/* Nivel 9: Expresiones primarias (literales, identificadores, paréntesis) - mayor precedencia */
ExpPrimaria
    : ID_TOKEN
        { /* Identificador: nombre de variable o función */
          $$ = crear_nodo_hoja_id($1, @1.first_line); }
    | NUMERO_TOKEN
        { /* Literal numérico: entero o flotante */
          $$ = crear_nodo_hoja_num($1, @1.first_line); }
    | CADENA_TOKEN
        { /* Literal de cadena: "texto" */
          $$ = crear_nodo_hoja_cadena($1, @1.first_line); }
    | TRUE_TOKEN
        { /* Literal booleano true */
          $$ = crear_nodo_hoja_bool(1, @1.first_line); }
    | FALSE_TOKEN
        { /* Literal booleano false */
          $$ = crear_nodo_hoja_bool(0, @1.first_line); }
    | PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN
        { /* Expresión entre paréntesis: (expresión) - cambia precedencia */
          $$ = $2; }
    /* Funciones reservadas del lenguaje - se reconocen como identificadores especiales */
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

/**
 * Función de reporte de errores llamada por Bison cuando detecta un error sintáctico.
 * 
 * Se invoca automáticamente cuando el parser encuentra un token inesperado o
 * cuando la pila de análisis no puede reducir ninguna regla.
 * 
 * @param s Mensaje de error proporcionado por Bison
 */
void yyerror(const char *s) {
    fprintf(stderr, "Error Sintáctico en línea %d: %s\n", yylineno, s);
}