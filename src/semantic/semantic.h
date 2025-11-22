/*
 * Analizador Semántico - Verificación de Tipos y Gestión de Tabla de Símbolos.
 * 
 * Este módulo proporciona la interfaz para realizar el análisis semántico del código.
 * El análisis se realiza en dos fases:
 * 1. Recolección de símbolos (declaraciones globales y firmas de funciones)
 * 2. Análisis de cuerpos (verificación de tipos y validación de uso)
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../ast/ast.h"

/**
 * Entrada en la tabla de símbolos.
 * 
 * Almacena información sobre variables y funciones declaradas en el programa.
 * Para funciones, el campo 'parametros' contiene el AST de la lista de parámetros
 * formales. Para variables, 'parametros' es NULL.
 */
typedef struct Simbolo {
    char* nombre;                    /* Nombre del símbolo */
    TipoDato tipo;                  /* Tipo de dato (o tipo de retorno para funciones) */
    int ambito;                     /* Nivel de ámbito (0 = global) */
    int linea;                       /* Línea de declaración (para reporte de errores) */
    struct ASTNode* parametros;      /* AST de lista de parámetros (solo para funciones) */
} Simbolo;

/**
 * Función principal del analizador semántico.
 * 
 * Realiza el análisis semántico completo del AST en dos fases:
 * - Fase 1: Recolección de símbolos
 * - Fase 2: Análisis de cuerpos y verificación de tipos
 * 
 * @param raiz Raíz del AST a analizar
 * @return 1 si el análisis fue exitoso, 0 si hay errores críticos
 */
int analizar_semantica(ASTNode* raiz);

#endif // SEMANTIC_H