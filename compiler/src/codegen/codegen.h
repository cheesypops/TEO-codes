#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "../ast/ast.h"
#include "../semantic/semantic.h"

// Tamaño máximo del programa compilado (número de enteros en el búfer de bytecode).
#define MAX_CODE_SIZE 2048

// Conjunto de códigos de operación soportados por la máquina virtual del robot.
typedef enum {
    OP_HALT = 0,
    
    // Operaciones sobre pila y memoria.
    OP_CONST,       // 1
    OP_LOAD,        // 2
    OP_STORE,       // 3
    
    // Operaciones aritméticas.
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, // 4, 5, 6, 7, 8
    OP_INC, OP_DEC, // 9, 10
    OP_NEG,         // 11
    
    // Operadores lógicos y de comparación.
    OP_AND, OP_OR, OP_NOT, // 12, 13, 14
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE, // 15..20
    
    // Instrucciones de salto de control de flujo.
    OP_JMP, // 21: salto incondicional
    OP_JZ,  // 22: salto si falso (jump if zero)

    // Instrucciones específicas del robot seguidor de línea.
    OP_MOVER, OP_GIRAR_IZQ, OP_GIRAR_DER, 
    OP_LEER_SENSOR, OP_PARAR, OP_REVERSA,

    OP_DELAY
    
} OpCode;

// Genera el bytecode correspondiente al AST y lo escribe en el archivo indicado.
void generar_codigo(ASTNode* raiz, const char* nombre_archivo);

#endif // CODEGEN_H