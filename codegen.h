#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "semantic.h"

/* Opcodes para la Máquina Virtual Arduino */
typedef enum {
    OP_HALT = 0,
    
    // Pila y Memoria
    OP_CONST,       // [OP] [VALOR]
    OP_LOAD,        // [OP] [DIRECCION]
    OP_STORE,       // [OP] [DIRECCION]
    
    // Aritmética
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_INC, OP_DEC, // ++, --
    OP_NEG,         // - unario
    
    // Lógica
    OP_AND, OP_OR, OP_NOT,
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE,
    
    // Saltos (Pendiente para fase avanzada, usaremos placeholders)
    OP_JMP, OP_JZ,

    // Funciones Robot
    OP_MOVER, OP_GIRAR_IZQ, OP_GIRAR_DER, 
    OP_LEER_SENSOR, OP_PARAR, OP_REVERSA
    
} OpCode;

void generar_codigo(ASTNode* raiz, const char* nombre_archivo);

#endif // CODEGEN_H