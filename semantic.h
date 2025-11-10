#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

typedef struct Simbolo {
    char* nombre;
    TipoDato tipo;
    int ambito;
    int linea;
    
    /* --- NUEVO CAMPO --- */
    /* Guarda el puntero al AST de la lista de parámetros (NODO_DECLARACION) */
    struct ASTNode* parametros; 
    
} Simbolo;


/* Función principal del analizador */
int analizar_semantica(ASTNode* raiz);

#endif // SEMANTIC_H