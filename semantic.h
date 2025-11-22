#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

typedef struct Simbolo {
    char* nombre;
    TipoDato tipo;
    int ambito;
    int linea;
    
    struct ASTNode* parametros; 
    
    /* --- NUEVO CAMPO --- */
    /* Dirección de memoria virtual (0, 1, 2...) para el Bytecode */
    int direccion;
    
} Simbolo;


/* Función principal del analizador */
int analizar_semantica(ASTNode* raiz);

/* Función para buscar símbolos (necesaria para codegen) */
Simbolo* ts_buscar(char* nombre);

/* Nueva función para limpiar explícitamente al final */
void ts_liberar_total();

#endif // SEMANTIC_H