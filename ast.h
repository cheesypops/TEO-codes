/* ==========================================
   ast.h - Definiciones del Árbol de Sintaxis Abstracta
   ========================================== */

#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Tipos de Nodos */
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,      /* Declaración de función */
    NODE_VAR_DECL,      /* Declaración de variable */
    NODE_BLOCK,         /* Bloque de código { ... } */
    
    /* Sentencias de Control */
    NODE_IF,
    NODE_WHILE,
    NODE_DO_WHILE,
    NODE_FOR,
    NODE_RETURN,
    
    /* Operaciones y Asignaciones */
    NODE_ASSIGN,        /* = */
    NODE_BIN_OP,        /* +, -, *, /, &&, ||, ==, !=, <, etc. */
    NODE_UNARY_OP,      /* -, !, ++, -- (prefijo/postfijo) */
    
    /* Valores y Referencias */
    NODE_CONST_INT,
    NODE_CONST_FLOAT,
    NODE_CONST_BOOL,
    NODE_CONST_STR,
    NODE_ID,            /* Uso de variable */
    
    /* Especiales Robot */
    NODE_CALL_ROBOT,    /* token_mover, token_girar, etc. */
    NODE_CALL_FUNC      /* Llamada a función de usuario */
} NodeKind;

/* Estructura del Nodo */
typedef struct ASTNode {
    NodeKind kind;      /* Tipo de nodo */
    
    /* Datos del nodo */
    char *type_name;    /* "int", "void", etc. (para declaraciones) */
    
    union {
        int int_val;
        float float_val;
        char *str_val;  /* Para IDs, cadenas o el operador (+, -, mover) */
    } data;

    /* Estructura de árbol */
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *extra; /* Para el 3er componente del for o el 'else' del if */
    
    /* Estructura de lista (para sentencias secuenciales o argumentos) */
    struct ASTNode *next; 
    
} ASTNode;

/* Constructores */
ASTNode* newASTNode(NodeKind kind);
ASTNode* newBinaryNode(char *op, ASTNode *left, ASTNode *right);
ASTNode* newUnaryNode(char *op, ASTNode *child);
ASTNode* newIntNode(int val);
ASTNode* newIDNode(char *name);
ASTNode* newRobotNode(char *action, ASTNode *args);

/* Utilidades */
void freeAST(ASTNode *node);
void printAST(ASTNode *node, int level);

#endif