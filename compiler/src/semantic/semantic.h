#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../ast/ast.h"

/* Cabeceras del módulo de análisis semántico.
 * Declara la estructura de símbolo y las funciones públicas que operan sobre el AST.
 */
typedef struct Simbolo {
    char* nombre;
    TipoDato tipo;
    int ambito;
    int linea;
    
    struct ASTNode* parametros; 
    
    // Dirección de memoria virtual (0, 1, 2, ...) asociada en la tabla de símbolos para el bytecode.
    int direccion;
    
} Simbolo;


// Función principal del analizador semántico; coordina las fases sobre el AST completo.
int analizar_semantica(ASTNode* raiz);

// Permite buscar un símbolo por nombre, reutilizado por la fase de generación de código.
Simbolo* ts_buscar(char* nombre);

// Libera la memoria asociada a la tabla de símbolos al final del compilador.
void ts_liberar_total();

#endif // SEMANTIC_H