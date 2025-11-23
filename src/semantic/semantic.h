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

typedef struct Simbolo {
    char* nombre;                    /* Nombre del símbolo */
    TipoDato tipo;                  /* Tipo de dato (o tipo de retorno para funciones) */
    int ambito;                     /* Nivel de ámbito (0 = global) */
    int linea;                       /* Línea de declaración (para reporte de errores) */
    struct ASTNode* parametros;      /* AST de lista de parámetros (solo para funciones) */
    int indice_vm;                   /* Índice entero para la VM (solo variables y parámetros) */
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

/**
 * Obtiene el índice de variable usado por la VM para un identificador dado.
 *
 * PRECONDICIÓN: El analizador semántico ya se ejecutó y registró la variable
 * en la tabla de símbolos.
 *
 * @param nombre Nombre de la variable a buscar.
 * @return Índice entero (>= 0) si existe, o -1 si no se encuentra o no es variable.
 */
int ts_obtener_indice_variable(const char* nombre);

#endif // SEMANTIC_H