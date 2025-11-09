#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* Estructura para un símbolo en la Tabla de Símbolos */
typedef struct {
    char* nombre;
    TipoDato tipo;
    int ambito;
    int linea;
    // ... aquí iría más info (ej. lista de parámetros si es función)
} Simbolo;


/* Función principal del analizador */
int analizar_semantica(ASTNode* raiz);

#endif // SEMANTIC_H    