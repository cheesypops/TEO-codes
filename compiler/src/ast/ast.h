#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enumeración de los tipos de nodos que pueden aparecer en el AST.
typedef enum {
    // Nodos de programa y estructura.
    NODO_PROGRAMA,
    NODO_DECLARACION_GLOBAL,
    NODO_SETUP,
    NODO_FUNCION_DEF,
    NODO_BLOQUE,
    NODO_LISTA_INSTRUCCIONES,

    // Nodos de instrucciones.
    NODO_DECLARACION,
    NODO_IF,
    NODO_FOR,
    NODO_WHILE,
    NODO_DO_WHILE,
    NODO_LLAMADA_FUNCION, // Se usa en expresiones postfijas de llamada.
    NODO_FUNCION_RESERVADA,

    // Nodos de expresión para operadores.
    NODO_ASIGNACION,
    NODO_BINARIO_OP, // Para +, -, *, /, %, &&, ||, ==, !=, <, <=, >, >=
    NODO_UNARIO_OP,  // Para !, -, +, ++, -- (prefijo)
    NODO_POSTFIX_OP, // Para ++, -- (postfijo)
    
    // Nodos de expresión para valores primitivos.
    NODO_IDENTIFICADOR,
    NODO_NUMERO,
    NODO_CADENA,
    NODO_BOOLEANO,

    // Nodos auxiliares.
    NODO_TIPO,
    NODO_LISTA_PARAMETROS,
    NODO_LISTA_ARGUMENTOS,
    NODO_VACIO // Representa producciones vacías (λ) en la gramática.
} TipoNodo;

// Enumeración de tipos de datos admitidos por el lenguaje.
typedef enum {
    TIPO_INT,
    TIPO_BOOLEAN,
    TIPO_FLOAT,
    TIPO_CHAR,
    TIPO_STRING,
    TIPO_VOID,
    TIPO_DESCONOCIDO // Se utiliza para propagar errores semánticos.
} TipoDato;

// Estructura principal de un nodo del AST.
typedef struct ASTNode {
    TipoNodo tipo;
    struct ASTNode *hijo1;     // Hijo izquierdo o única rama (por ejemplo, operador unario).
    struct ASTNode *hijo2;     // Segundo hijo (por ejemplo, rama derecha de un binario o condición de if).
    struct ASTNode *hijo3;     // Tercer hijo (por ejemplo, parte else de un if o paso de un for).
    struct ASTNode *hijo4;     // Cuarto hijo (por ejemplo, cuerpo de un for).
    struct ASTNode *siguiente; // Enlace para listas lineales (instrucciones, parámetros, argumentos, etc.).
    
    // Datos específicos asociados al tipo de nodo.
    union {
        char *cadena;       // Para identificadores y literales de cadena.
        double valor_num;   // Para literales numéricos.
        int valor_bool;     // Para literales booleanos.
        TipoDato tipo_dato; // Para nodos que representan un tipo.
        char op_binario;    // Operador binario simplificado (no se utiliza en la versión actual).
        char *op_unario;    // Representación textual de operadores unarios y binarios.
    } data;
    
    int linea; // Número de línea asociado al nodo, utilizado en mensajes de error.
} ASTNode;

// Funciones para crear nodos del AST (fábrica de nodos).
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

// Funciones para construir listas enlazadas de nodos.
ASTNode* enlazar_instruccion(ASTNode* lista, ASTNode* instruccion);
ASTNode* enlazar_parametro(ASTNode* lista, ASTNode* parametro);
ASTNode* enlazar_argumento(ASTNode* lista, ASTNode* argumento);
ASTNode* enlazar_declaracion(ASTNode* lista, ASTNode* declaracion_init);
ASTNode* enlazar_nodos(ASTNode* lista, ASTNode* item);  

// Funciones auxiliares sobre el árbol completo.
void liberar_arbol(ASTNode* nodo);
void imprimir_arbol(ASTNode* nodo, int nivel); // Útil para depuración visual del AST.

#endif // AST_H