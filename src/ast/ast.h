/*
 * Definiciones del Árbol Sintáctico Abstracto (AST).
 * 
 * El AST representa la estructura sintáctica del código fuente como un árbol,
 * donde cada nodo corresponde a una construcción del lenguaje. Esta estructura
 * intermedia permite al compilador realizar análisis semántico y generar código
 * de forma más eficiente que trabajando directamente con tokens.
 */

#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Tipos de nodos en el AST.
 * 
 * Cada tipo corresponde a una construcción específica del lenguaje:
 * - Nodos de programa: estructura global, funciones, bloques
 * - Nodos de instrucciones: declaraciones, asignaciones, estructuras de control
 * - Nodos de expresiones: operadores, literales, identificadores
 * - Nodos auxiliares: tipos, listas, nodos vacíos
 */
typedef enum {
    /* Nodos de Programa y Estructura */
    NODO_PROGRAMA,              /* Raíz del programa completo */
    NODO_DECLARACION_GLOBAL,    /* Lista de declaraciones globales */
    NODO_SETUP,                 /* Función setup() especial del lenguaje */
    NODO_FUNCION_DEF,           /* Definición de función */
    NODO_BLOQUE,                /* Bloque de código entre { } */
    NODO_LISTA_INSTRUCCIONES,   /* Lista enlazada de instrucciones */

    /* Nodos de Instrucciones */
    NODO_DECLARACION,           /* Declaración de variable(s) */
    NODO_IF,                    /* Estructura condicional if-else */
    NODO_FOR,                   /* Bucle for */
    NODO_WHILE,                 /* Bucle while */
    NODO_DO_WHILE,              /* Bucle do-while */
    NODO_LLAMADA_FUNCION,      /* Llamada a función */
    NODO_FUNCION_RESERVADA,     /* Función reservada del lenguaje */

    /* Nodos de Expresiones (operadores) */
    NODO_ASIGNACION,            /* Asignación (=) */
    NODO_BINARIO_OP,           /* Operadores binarios: +, -, *, /, %, &&, ||, ==, !=, <, <=, >, >=, [] */
    NODO_UNARIO_OP,            /* Operadores unarios prefijos: !, -, +, ++, -- */
    NODO_POSTFIX_OP,           /* Operadores postfijos: ++, -- */
    
    /* Nodos de Expresiones (primitivos) */
    NODO_IDENTIFICADOR,         /* Nombre de variable o función */
    NODO_NUMERO,                /* Literal numérico (int o float) */
    NODO_CADENA,                /* Literal de cadena */
    NODO_BOOLEANO,              /* Literal booleano (true/false) */

    /* Nodos Auxiliares */
    NODO_TIPO,                  /* Especificación de tipo de dato */
    NODO_LISTA_PARAMETROS,      /* Lista de parámetros de función */
    NODO_LISTA_ARGUMENTOS,      /* Lista de argumentos en llamada */
    NODO_VACIO                  /* Producción vacía (lambda/ε) */
} TipoNodo;

/**
 * Tipos de datos del lenguaje.
 * 
 * Representa los tipos primitivos y especiales que puede tener una variable
 * o expresión. TIPO_DESCONOCIDO se usa para indicar errores semánticos.
 */
typedef enum {
    TIPO_INT,                   /* Entero */
    TIPO_BOOLEAN,               /* Booleano (true/false) */
    TIPO_FLOAT,                 /* Número de punto flotante */
    TIPO_CHAR,                  /* Carácter (no usado actualmente) */
    TIPO_STRING,                /* Cadena de caracteres */
    TIPO_VOID,                  /* Sin tipo (para funciones sin retorno) */
    TIPO_DESCONOCIDO            /* Tipo desconocido (indica error semántico) */
} TipoDato;

/**
 * Estructura de un nodo del AST.
 * 
 * DISEÑO: Se utilizan múltiples punteros hijos (hijo1-4) para diferentes tipos
 * de nodos, en lugar de un arreglo variable. Esto simplifica el acceso y mejora
 * la legibilidad del código.
 * 
 * DISTRIBUCIÓN DE HIJOS POR TIPO DE NODO:
 * - Operadores unarios: hijo1 (operando)
 * - Operadores binarios: hijo1 (izquierda), hijo2 (derecha)
 * - If: hijo1 (condición), hijo2 (then), hijo3 (else, puede ser NODO_VACIO)
 * - For: hijo1 (inicialización), hijo2 (condición), hijo3 (paso), hijo4 (cuerpo)
 * - While/DoWhile: hijo1 (condición o cuerpo), hijo2 (cuerpo o condición)
 * - Llamada función: hijo1 (función), hijo2 (argumentos)
 * 
 * El puntero 'siguiente' encadena nodos en listas enlazadas para representar
 * secuencias (instrucciones, parámetros, argumentos, declaraciones).
 * 
 * MEMORIA: Los strings en 'data.cadena' pueden ser:
 * - Asignados dinámicamente (identificadores, literales de cadena) -> deben liberarse
 * - Literales estáticos (operadores, funciones reservadas) -> NO liberar
 */
typedef struct ASTNode {
    TipoNodo tipo;                  /* Tipo de nodo (determina la estructura) */
    struct ASTNode *hijo1;          /* Primer hijo (uso depende del tipo) */
    struct ASTNode *hijo2;          /* Segundo hijo */
    struct ASTNode *hijo3;          /* Tercer hijo */
    struct ASTNode *hijo4;          /* Cuarto hijo */
    struct ASTNode *siguiente;      /* Siguiente nodo en lista enlazada */
    
    /**
     * Unión para almacenar datos del nodo según su tipo.
     * 
     * - cadena: Para identificadores, literales de cadena, operadores (string)
     * - valor_num: Para literales numéricos
     * - valor_bool: Para literales booleanos
     * - tipo_dato: Para nodos de tipo
     * - op_unario: Reutilizado también para operadores binarios (string)
     */
    union {
        char *cadena;
        double valor_num;
        int valor_bool;
        TipoDato tipo_dato;
        char op_binario;    /* No usado (mantenido por compatibilidad) */
        char *op_unario;    /* Operadores como string: "!", "++", "||", etc. */
    } data;
    
    int linea;  /* Número de línea en el código fuente (para reporte de errores) */
} ASTNode;

/* ========== Funciones de Creación de Nodos ========== */

ASTNode* crear_nodo(TipoNodo tipo, int linea);
ASTNode* crear_nodo_vacio();
ASTNode* crear_nodo_hoja_id(char* nombre, int linea);
ASTNode* crear_nodo_hoja_num(double valor, int linea);
ASTNode* crear_nodo_hoja_cadena(char* valor, int linea);
ASTNode* crear_nodo_hoja_bool(int valor, int linea);
ASTNode* crear_nodo_tipo(TipoDato tipo, int linea);
ASTNode* crear_nodo_unario(char* op, ASTNode* hijo, int linea);
ASTNode* crear_nodo_binario(char* op, ASTNode* izq, ASTNode* der, int linea);
ASTNode* crear_nodo_postfix(ASTNode* hijo, char* op, int linea);
ASTNode* crear_nodo_funcion_reservada(char* nombre, int linea);

/* ========== Funciones de Enlace de Listas ========== */

ASTNode* enlazar_instruccion(ASTNode* lista, ASTNode* instruccion);
ASTNode* enlazar_parametro(ASTNode* lista, ASTNode* parametro);
ASTNode* enlazar_argumento(ASTNode* lista, ASTNode* argumento);
ASTNode* enlazar_declaracion(ASTNode* lista, ASTNode* declaracion_init);
ASTNode* enlazar_nodos(ASTNode* lista, ASTNode* item);  

/* ========== Operaciones sobre el Árbol ========== */

void liberar_arbol(ASTNode* nodo);
void imprimir_arbol(ASTNode* nodo, int nivel);  /* Depuración: imprime estructura del AST */

#endif // AST_H