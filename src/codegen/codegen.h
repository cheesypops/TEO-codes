/*
 * Generador de Bytecode para la VM TEO basada en pila.
 *
 * Recorre el AST verificado por el analizador semántico y emite instrucciones
 * en formato de texto plano:
 *
 *   MNEMONIC arg1 arg2
 *
 * Una instrucción por línea, sin índices explícitos ni comentarios.
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "../ast/ast.h"

/**
 * Genera bytecode en formato .vmcode a partir del AST raíz.
 *
 * PRECONDICIÓN: El análisis semántico ya se ejecutó sobre el AST y no hubo
 * errores críticos.
 *
 * @param raiz Raíz del AST del programa completo.
 * @param out  Archivo de salida ya abierto en modo texto para escribir
 *             el bytecode (por ejemplo, "program.vmcode").
 */
void generar_bytecode(ASTNode *raiz, FILE *out);

#endif /* CODEGEN_H */


