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

/* * Unión de tipos de YACC. 
 * Cada símbolo (terminal o no-terminal) tiene un valor de este tipo.
 */
%union {
    char* cadena;     // Para id_token, cadena_token
    double  numero;     // Para numero_token
    int     booleano;   // Para true/false
    TipoDato tipo_dato; // Para Tipo
    ASTNode* nodo;      // Para la mayoría de los no-terminales (construyen el AST)
}

/* * TOKENS (Terminales)
 * Se definen los tipos de la unión para los tokens que llevan valor.
 */
%token <cadena> ID_TOKEN
%token <numero> NUMERO_TOKEN
%token <cadena> CADENA_TOKEN
%token <booleano> TRUE_TOKEN FALSE_TOKEN

%token TIPO_TOKEN_INT TIPO_TOKEN_BOOLEAN TIPO_TOKEN_FLOAT TIPO_TOKEN_CHAR TIPO_TOKEN_STRING
%token IF_TOKEN ELSE_TOKEN FOR_TOKEN WHILE_TOKEN DO_TOKEN VOID_TOKEN
%token INICIO_TOKEN FIN_TOKEN
%token PAREN_IZQ_TOKEN PAREN_DER_TOKEN
%token CORCH_IZQ_TOKEN CORCH_DER_TOKEN
%token PUNTOYCOMA_TOKEN COMA_TOKEN

/* Operadores (se usan en las reglas de precedencia) */
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

/* * TIPOS (No-Terminales)
 * Se definen los tipos de la unión para los no-terminales que devuelven un valor.
 */
%type <nodo> Programa DeclaracionGlobal SetupDef FuncionDef
%type <nodo> Bloque ListaInstrucciones Instruccion
%type <nodo> Asignacion Declaracion DeclaracionInit DeclaracionPrima
%type <nodo> If IfPrima For ForInit ExpresionLogicaFor ForStep DoWhile While
%type <nodo> ListaParametros ListaParametrosPrima Parametro TipoRetorno
%type <tipo_dato> Tipo
%type <nodo> ListaArgumentos ListaArgumentosPrima
%type <nodo> Expresion ExpOr ExpAnd ExpIgualdad ExpRel ExpAditiva ExpMult ExpUnitaria ExpPostfix ExpPostfixPrima ExpPrimaria

/* * PRECEDENCIA Y ASOCIATIVIDAD (Opción A)
 * Define la jerarquía de expresiones de Nivel 2 a 7.
 * De menor a mayor precedencia (de arriba hacia abajo).
 */
 
/* Nivel 1: Asignación (tu GIC la maneja como Instrucción, no expresión, lo respetamos) */
/* %right ASIGN_TOKEN */ /* No se usa aquí, 'Asignacion' es una regla separada */

/* Nivel 2: Lógicos */
%left OR_TOKEN
%left AND_TOKEN

/* Nivel 3: Comparación */
%left IGUAL_TOKEN NO_IGUAL_TOKEN
%left MENOR_TOKEN MENOR_IGUAL_TOKEN MAYOR_TOKEN MAYOR_IGUAL_TOKEN

/* Nivel 4: Aritméticos */
%left MAS_TOKEN MENOS_TOKEN
%left MULT_TOKEN DIV_TOKEN MOD_TOKEN

/* Nivel 5: Unarios (Prefijos) */
%right NOT_TOKEN /* ! */
%right UMENOS /* 'dummy' token for unary minus */
%right UMAS   /* 'dummy' token for unary plus */
%right INC_TOKEN /* ++ prefijo */
%right DEC_TOKEN /* -- prefijo */

/* Nivel 6: Postfijos (Tienen la mayor precedencia) */
%left PAREN_IZQ_TOKEN PAREN_DER_TOKEN /* Llamada a función */
%left CORCH_IZQ_TOKEN CORCH_DER_TOKEN /* Acceso a array */
%left INC_TOKEN /* ++ postfijo */
%left DEC_TOKEN /* -- postfijo */


/* Símbolo inicial de la gramática */
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
        raiz_ast = $$; // Guardamos la raíz
    }
    ;

DeclaracionGlobal
    : Declaracion PUNTOYCOMA_TOKEN DeclaracionGlobal
    {
        $1->siguiente = $3; // Enlazamos la lista
        $$ = $1;
    }
    | Asignacion PUNTOYCOMA_TOKEN DeclaracionGlobal
    {
        $1->siguiente = $3; // Enlazamos la lista
        $$ = $1;
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

SetupDef
    : Tipo ID_TOKEN PAREN_IZQ_TOKEN PAREN_DER_TOKEN Bloque
    {
        if (strcmp($2, "setup") != 0) {
            yyerror("Error: Se esperaba la funcion 'setup'");
            YYERROR;
        }
        $$ = crear_nodo(NODO_SETUP, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);
        $$->data.cadena = $2; // Guardamos "setup"
        $$->hijo2 = $5; // Bloque
    }
    ;

/* ------------------------------------ */
/* 2. Bloques y Lista de Instrucciones  */
/* ------------------------------------ */

Bloque
    : INICIO_TOKEN ListaInstrucciones FIN_TOKEN
    {
        $$ = crear_nodo(NODO_BLOQUE, @1.first_line);
        $$->hijo1 = $2; // ListaInstrucciones
    }
    ;

ListaInstrucciones
    : Instruccion ListaInstrucciones
    {
        $$ = $1;
        $$->siguiente = $2; // Enlazamos la lista
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

/* ------------------------------------ */
/* 3. Instrucciones                     */
/* ------------------------------------ */

Instruccion
    : Asignacion PUNTOYCOMA_TOKEN      { $$ = $1; }
    | Declaracion PUNTOYCOMA_TOKEN    { $$ = $1; }
    | Expresion PUNTOYCOMA_TOKEN      { $$ = $1; }
    | If                              { $$ = $1; }
    | For                             { $$ = $1; }
    | While                           { $$ = $1; }
    | DoWhile                         { $$ = $1; }
    | Bloque                          { $$ = $1; }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;

/* ------------------------------------ */
/* 4. Sentencias Simples                */
/* ------------------------------------ */

Asignacion
    : ID_TOKEN ASIGN_TOKEN Expresion
    {
        $$ = crear_nodo(NODO_ASIGNACION, @1.first_line);
        $$->hijo1 = crear_nodo_hoja_id($1, @1.first_line);
        $$->hijo2 = $3;
    }
    ;

Declaracion
    : Tipo ID_TOKEN DeclaracionInit DeclaracionPrima
    {
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line);
        $$->hijo1 = crear_nodo_tipo($1, @1.first_line);
        
        // Creamos el nodo para 'ID_TOKEN DeclaracionInit'
        ASTNode* decl_nodo = crear_nodo(NODO_DECLARACION, @2.first_line); // Nodo auxiliar
        decl_nodo->hijo1 = crear_nodo_hoja_id($2, @2.first_line); // ID
        decl_nodo->hijo2 = $3; // Expresion o Vacio
        
        // Enlazamos con el resto (DeclaracionPrima)
        decl_nodo->siguiente = $4;
        $$->hijo2 = decl_nodo;
    }
    ;

DeclaracionInit
    : ASIGN_TOKEN Expresion { $$ = $2; }
    | /* lambda */          { $$ = crear_nodo_vacio(); }
    ;

DeclaracionPrima
    : COMA_TOKEN ID_TOKEN DeclaracionInit DeclaracionPrima
    {
        // Creamos el nodo para 'ID_TOKEN DeclaracionInit'
        $$ = crear_nodo(NODO_DECLARACION, @2.first_line); // Nodo auxiliar
        $$->hijo1 = crear_nodo_hoja_id($2, @2.first_line); // ID
        $$->hijo2 = $3; // Expresion o Vacio
        $$->siguiente = $4; // Enlazamos con el resto
    }
    | /* lambda */
    {
        $$ = crear_nodo_vacio();
    }
    ;
    
/* ------------------------------------ */
/* 5. Estructuras de Control            */
/* ------------------------------------ */

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
        $$->hijo1 = $3; // ForInit
        $$->hijo2 = $5; // ExpresionLogicaFor
        $$->hijo3 = $7; // ForStep
        $$->hijo4 = $9; // Bloque
    }
    ;

ForInit
    : Declaracion   { $$ = $1; }
    | Expresion     { $$ = $1; }
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
        $$->hijo1 = $3; // Bloque
        $$->hijo2 = $7; // Expresion
    }
    ;

While
    : WHILE_TOKEN PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_WHILE, @1.first_line);
        $$->hijo1 = $3; // Expresion
        $$->hijo2 = $5; // Bloque
    }
    ;

/* ------------------------------------ */
/* 6. Definición de Funciones y Tipos   */
/* ------------------------------------ */

FuncionDef
    : TipoRetorno ID_TOKEN PAREN_IZQ_TOKEN ListaParametros PAREN_DER_TOKEN Bloque
    {
        $$ = crear_nodo(NODO_FUNCION_DEF, @1.first_line);
        $$->hijo1 = $1; // TipoRetorno
        $$->hijo2 = crear_nodo_hoja_id($2, @2.first_line); // ID
        $$->hijo3 = $4; // ListaParametros
        $$->hijo4 = $6; // Bloque
    }
    ;

ListaParametros
    : Parametro ListaParametrosPrima
    {
        $$ = $1;
        $$->siguiente = $2;
    }
    | /* lambda */ { $$ = crear_nodo_vacio(); }
    ;

ListaParametrosPrima
    : COMA_TOKEN Parametro ListaParametrosPrima
    {
        $$ = $2;
        $$->siguiente = $3;
    }
    | /* lambda */ { $$ = crear_nodo_vacio(); }
    ;

Parametro
    : Tipo ID_TOKEN
    {
        $$ = crear_nodo(NODO_DECLARACION, @1.first_line); // Usamos NODO_DECLARACION
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
/* 7. Lista de Argumentos (llamadas)    */
/* ------------------------------------ */

ListaArgumentos
    : Expresion ListaArgumentosPrima
    {
        $$ = $1;
        $$->siguiente = $2;
    }
    | /* lambda */ { $$ = crear_nodo_vacio(); }
    ;

ListaArgumentosPrima
    : COMA_TOKEN Expresion ListaArgumentosPrima
    {
        $$ = $2;
        $$->siguiente = $3;
    }
    | /* lambda */ { $$ = crear_nodo_vacio(); }
    ;
    
/* ------------------------------------ */
/* 8. JERARQUÍA DE EXPRESIÓN (YACC)     */
/* ------------------------------------ */

/* * Usamos la precedencia definida en %left, %right.
 * Esto reemplaza tu GIC Nivel 2-7.
 */

Expresion
    : ExpPrimaria
        { $$ = $1; }
    
    /* Nivel 6: Postfijos */
    | Expresion PAREN_IZQ_TOKEN ListaArgumentos PAREN_DER_TOKEN
    {
        // Llamada a función
        // $1 es el ID (o ExpPrimaria)
        $$ = crear_nodo(NODO_LLAMADA_FUNCION, @2.first_line);
        $$->hijo1 = $1;
        $$->hijo2 = $3; // ListaArgumentos
    }
    | Expresion CORCH_IZQ_TOKEN Expresion CORCH_DER_TOKEN
    {
        // Acceso a Array (no estaba en tu GIC postfij, pero sí en ExpPostfix')
        $$ = crear_nodo_binario("[]", $1, $3, @2.first_line);
    }
    | Expresion INC_TOKEN
    {
        $$ = crear_nodo_postfix($1, "++", @2.first_line);
    }
    | Expresion DEC_TOKEN
    {
        $$ = crear_nodo_postfix($1, "--", @2.first_line);
    }

    /* Nivel 5: Unarios (Prefijos) */
    | NOT_TOKEN Expresion
    {
        $$ = crear_nodo_unario("!", $2, @1.first_line);
    }
    | MENOS_TOKEN Expresion %prec UMENOS
    {
        $$ = crear_nodo_unario("-", $2, @1.first_line);
    }
    | MAS_TOKEN Expresion %prec UMAS
    {
        $$ = crear_nodo_unario("+", $2, @1.first_line);
    }
    | INC_TOKEN Expresion
    {
        $$ = crear_nodo_unario("++", $2, @1.first_line);
    }
    | DEC_TOKEN Expresion
    {
        $$ = crear_nodo_unario("--", $2, @1.first_line);
    }
    
    /* Nivel 4: Aritméticos */
    | Expresion MULT_TOKEN Expresion
    {
        $$ = crear_nodo_binario("*", $1, $3, @2.first_line);
    }
    | Expresion DIV_TOKEN Expresion
    {
        $$ = crear_nodo_binario("/", $1, $3, @2.first_line);
    }
    | Expresion MOD_TOKEN Expresion
    {
        $$ = crear_nodo_binario("%", $1, $3, @2.first_line);
    }
    | Expresion MAS_TOKEN Expresion
    {
        $$ = crear_nodo_binario("+", $1, $3, @2.first_line);
    }
    | Expresion MENOS_TOKEN Expresion
    {
        $$ = crear_nodo_binario("-", $1, $3, @2.first_line);
    }
    
    /* Nivel 3: Comparación */
    | Expresion MENOR_TOKEN Expresion
    {
        $$ = crear_nodo_binario("<", $1, $3, @2.first_line);
    }
    | Expresion MENOR_IGUAL_TOKEN Expresion
    {
        $$ = crear_nodo_binario("<=", $1, $3, @2.first_line);
    }
    | Expresion MAYOR_TOKEN Expresion
    {
        $$ = crear_nodo_binario(">", $1, $3, @2.first_line);
    }
    | Expresion MAYOR_IGUAL_TOKEN Expresion
    {
        $$ = crear_nodo_binario(">=", $1, $3, @2.first_line);
    }
    | Expresion IGUAL_TOKEN Expresion
    {
        $$ = crear_nodo_binario("==", $1, $3, @2.first_line);
    }
    | Expresion NO_IGUAL_TOKEN Expresion
    {
        $$ = crear_nodo_binario("!=", $1, $3, @2.first_line);
    }

    /* Nivel 2: Lógicos */
    | Expresion AND_TOKEN Expresion
    {
        $$ = crear_nodo_binario("&&", $1, $3, @2.first_line);
    }
    | Expresion OR_TOKEN Expresion
    {
        $$ = crear_nodo_binario("||", $1, $3, @2.first_line);
    }
    ;

/* Nivel 7: Base Primaria */
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
    | PAREN_IZQ_TOKEN Expresion PAREN_DER_TOKEN
        { $$ = $2; } /* Devolvemos la expresión interna */
    ;

%%

/* Función de reporte de errores */
void yyerror(const char *s) {
    fprintf(stderr, "Error Sintáctico en línea %d: %s\n", yylineno, s);
}