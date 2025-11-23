#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "semantic.h"

/* Tamaño máximo del programa compilado */
#define MAX_CODE_SIZE 2048

typedef enum {
    OP_HALT = 0,
    
    // Pila y Memoria
    OP_CONST,       // 1
    OP_LOAD,        // 2
    OP_STORE,       // 3
    
    // Aritmética
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, // 4, 5, 6, 7, 8
    OP_INC, OP_DEC, // 9, 10
    OP_NEG,         // 11
    
    // Lógica
    OP_AND, OP_OR, OP_NOT, // 12, 13, 14
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE, // 15..20
    
    // Saltos
    OP_JMP, // 21: Salto incondicional
    OP_JZ,  // 22: Salto si falso (Jump if Zero)

    // Funciones Robot
    OP_MOVER, OP_GIRAR_IZQ, OP_GIRAR_DER, 
    OP_LEER_SENSOR, OP_PARAR, OP_REVERSA,

    OP_DELAY
    
} OpCode;

void generar_codigo(ASTNode* raiz, const char* nombre_archivo);

#endif // CODEGEN_H