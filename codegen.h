#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

/* Genera el código intermedio recorriendo el AST proporcionado.
   Escribe el resultado en formato de texto (2 columnas) en el archivo fp.
   
   Formato de salida: OPCODE ARGUMENTO
   Ejemplo:
   PUSH 10
   ADD 0
*/
void generate_code(ASTNode* node, FILE* fp);

#endif