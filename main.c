#include <stdio.h>
#include <stdlib.h> /* Para exit() */
#include "ast.h"
#include "semantic.h"
#include "codegen.h" /* [NUEVO] Incluimos el generador de código */

/* Prototipo de la función yyparse() generada por Bison */
extern int yyparse(void);
/* Variable global donde yyparse() guardará el AST */
extern ASTNode *raiz_ast; 
/* Variable global que Flex usará para leer el archivo */
extern FILE *yyin;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo_entrada>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", argv[1]);
        return 1;
    }

    printf("Iniciando analisis lexico y sintactico de: %s\n", argv[1]);

    /* 1. Llamada principal al parser (Sintaxis) */
    int resultado_parse = yyparse();
    
    fclose(yyin);

    if (resultado_parse != 0) {
        fprintf(stderr, "Analisis fallido (Errores Sintacticos).\n");
        /* Si falla el parser, liberamos lo que se haya podido crear y salimos */
        if (raiz_ast) liberar_arbol(raiz_ast);
        return 1;
    }

    printf("Analisis lexico y sintactico completado con exito.\n\n");
    
    /* Imprimir el AST (Opcional, útil para depuración) */
    printf("--- Arbol Sintactico Abstracto (AST) ---\n");
    imprimir_arbol(raiz_ast, 0);
    printf("---------------------------------------\n");
    
    /* 2. Llamada al analizador semántico */
    /* devuelve 1 si todo fue bien (o advertencias no fatales), 0 si hubo errores graves */
    if (analizar_semantica(raiz_ast)) {
        
        /* 3. Generación de Código Intermedio (Backend) */
        /* Solo generamos código si la semántica fue correcta */
        /* Guardamos el resultado en "codigo.txt" para la SD */
        generar_codigo(raiz_ast, "codigo.txt");
        
    } else {
        fprintf(stderr, "No se genero codigo debido a errores semanticos.\n");
    }
    
    /* 4. Limpieza de Memoria (Garbage Collection) */
    printf("\n[Main] Liberando memoria...\n");
    
    /* Liberamos la tabla de símbolos (que preservamos durante codegen) */
    ts_liberar_total(); 
    
    /* Liberamos el árbol AST completo */
    liberar_arbol(raiz_ast);

    printf("[Main] Finalizado.\n");
    return 0;
}