/*
 * Punto de Entrada Principal del Compilador.
 * 
 * Este programa orquesta las fases de compilación:
 * 1. Análisis Léxico: tokenización del código fuente (Flex)
 * 2. Análisis Sintáctico: construcción del AST (Bison)
 * 3. Análisis Semántico: verificación de tipos y validación (semantic.c)
 * 
 * FLUJO DE EJECUCIÓN:
 *   - Abre el archivo de entrada
 *   - Flex tokeniza el código y Bison construye el AST
 *   - Se imprime el AST (modo depuración)
 *   - Se realiza el análisis semántico
 *   - Se libera la memoria del AST
 */

#include <stdio.h>
#include "../ast/ast.h"
#include "../semantic/semantic.h"

/* Función generada por Bison - parsea la entrada y construye el AST */
extern int yyparse(void);
/* Raíz del AST completo - establecida por el parser */
extern ASTNode *raiz_ast; 
/* Archivo de entrada para Flex */
extern FILE *yyin;

int main(int argc, char *argv[]) {
    /* Verificar argumentos de línea de comandos */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo_entrada>\n", argv[0]);
        return 1;
    }

    /* Abrir archivo de entrada */
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", argv[1]);
        return 1;
    }

    printf("Iniciando analisis lexico y sintactico de: %s\n", argv[1]);

    /* FASE 1: Análisis Léxico y Sintáctico */
    /* Flex tokeniza el código y Bison construye el AST */
    int resultado_parse = yyparse();
    fclose(yyin);

    if (resultado_parse != 0) {
        fprintf(stderr, "Analisis fallido.\n");
        return 1;
    }

    printf("Analisis lexico y sintactico completado con exito.\n\n");
    
    /* Salida de depuración: imprimir estructura del AST */
    printf("--- Arbol Sintactico Abstracto (AST) ---\n");
    imprimir_arbol(raiz_ast, 0);
    printf("---------------------------------------\n");
    
    /* FASE 2: Análisis Semántico */
    /* Verificación de tipos, validación de símbolos, gestión de ámbitos */
    analizar_semantica(raiz_ast);
    
    /* Liberar memoria del AST */
    liberar_arbol(raiz_ast);
    return 0;
}