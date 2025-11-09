#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Enum para los tipos de nodos del AST */
typedef enum {
    /* Nodos de Programa y Estructura */
    NODO_PROGRAMA,
    NODO_DECLARACION_GLOBAL,
    NODO_SETUP,
    NODO_FUNCION_DEF,
    NODO_BLOQUE,
    NODO_LISTA_INSTRUCCIONES,

    /* Nodos de Instrucciones */
    NODO_ASIGNACION,
    NODO_DECLARACION,
    NODO_IF,
    NODO_FOR,
    NODO_WHILE,
    NODO_DO_WHILE,
    NODO_LLAMADA_FUNCION, // Se usa en ExpPostfix

    /* Nodos de Expresión (Operadores) */
    NODO_BINARIO_OP, // Para +, -, *, /, %, &&, ||, ==, !=, <, <=, >, >=
    NODO_UNARIO_OP,  // Para !, -, +, ++, -- (prefijo)
    NODO_POSTFIX_OP, // Para ++, -- (postfijo)
    
    /* Nodos de Expresión (Primitivos) */
    NODO_IDENTIFICADOR,
    NODO_NUMERO,
    NODO_CADENA,
    NODO_BOOLEANO,

    /* Nodos Auxiliares */
    NODO_TIPO,
    NODO_LISTA_PARAMETROS,
    NODO_LISTA_ARGUMENTOS,
    NODO_VACIO // Para producciones lambda (λ)
} TipoNodo;

/* Enum para los tipos de datos del lenguaje */
typedef enum {
    TIPO_INT,
    TIPO_BOOLEAN,
    TIPO_FLOAT,
    TIPO_CHAR,
    TIPO_STRING,
    TIPO_VOID,
    TIPO_DESCONOCIDO // Para errores semánticos
} TipoDato;

/* Estructura principal de un nodo del AST */
typedef struct ASTNode {
    TipoNodo tipo;
    struct ASTNode *hijo1;     // Hijo izquierdo, o única rama (ej. unario)
    struct ASTNode *hijo2;     // Hijo derecho, o segunda rama (ej. if)
    struct ASTNode *hijo3;     // Tercera rama (ej. for, if-else)
    struct ASTNode *hijo4;     // Cuarta rama (ej. for)
    struct ASTNode *siguiente; // Para listas (Instrucciones, Parámetros, etc.)
    
    /* Datos específicos del nodo */
    union {
        char *cadena;       // Para id, cadena_token
        double valor_num;   // Para numero_token
        int valor_bool;     // Para true/false
        TipoDato tipo_dato; // Para NODO_TIPO
        char op_binario;    // '+', '*', etc. (simplificado)
        char *op_unario;    // "!", "++", etc.
    } data;
    
    int linea; // Para reportar errores
} ASTNode;

/* Funciones para crear nodos del AST (la "fábrica" de nodos) */
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

/* Funciones para enlazar listas */
ASTNode* enlazar_instruccion(ASTNode* lista, ASTNode* instruccion);
ASTNode* enlazar_parametro(ASTNode* lista, ASTNode* parametro);
ASTNode* enlazar_argumento(ASTNode* lista, ASTNode* argumento);
ASTNode* enlazar_declaracion(ASTNode* lista, ASTNode* declaracion_init);

/* Funciones del Árbol */
void liberar_arbol(ASTNode* nodo);
void imprimir_arbol(ASTNode* nodo, int nivel); // Para depuración

#endif // AST_H