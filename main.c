/* ==========================================
   main.c - Punto de entrada del Compilador
   ========================================== */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "y.tab.h"      /* Definiciones de tokens generadas por Bison */
#include "semantic.h"   /* Analizador Semántico */
#include "codegen.h"    /* Generador de Código */

/* Variables externas provenientes de Flex y Bison */
extern int yylineno;
extern FILE *yyin;
extern int yyparse();
extern ASTNode *root;   /* Definido en parser.y */

int main(int argc, char** argv) {
    /* 1. Validación de argumentos */
    if (argc != 2) {
        fprintf(stderr, "Uso incorrecto.\nSintaxis: %s <archivo_fuente>\n", argv[0]);
        return 1;
    }

    /* 2. Abrir archivo de entrada */
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("Error al abrir el archivo de entrada");
        return 1;
    }

    /* Asignar el archivo a Flex */
    yyin = input;

    /* 3. Iniciar Parsing (Análisis Sintáctico) */
    printf("--- [1/3] Iniciando Analisis Sintactico ---\n");
    if (yyparse() != 0) {
        fprintf(stderr, "Error: Fallo en el analisis sintactico.\n");
        fclose(input);
        return 1;
    }

    /* Verificar si se generó el árbol raíz */
    if (root == NULL) {
        fprintf(stderr, "Error: No se genero el arbol AST (Programa vacio o error).\n");
        fclose(input);
        return 1;
    }
    printf("Analisis Sintactico completado.\n");

    /* 4. Iniciar Análisis Semántico */
    printf("\n--- [2/3] Iniciando Analisis Semantico ---\n");
    semantic_analysis(root);

    if (get_semantic_errors() > 0) {
        fprintf(stderr, "\nError: Se encontraron %d errores semanticos. Compilacion detenida.\n", get_semantic_errors());
        
        /* Limpieza */
        fclose(input);
        freeAST(root); 
        return 1;
    }

    /* 5. Generación de Código */
    printf("\n--- [3/3] Generando Codigo Intermedio ---\n");
    
    FILE *output = fopen("codigo.txt", "w");
    if (!output) {
        perror("Error al crear archivo de salida 'codigo.txt'");
        fclose(input);
        freeAST(root);
        return 1;
    }

    generate_code(root, output);

    /* 6. Finalización y Limpieza */
    fclose(output);
    fclose(input);
    freeAST(root); // Liberar memoria del árbol

    printf("\n=========================================\n");
    printf(" COMPILACION EXITOSA \n");
    printf(" Codigo generado en: codigo.txt\n");
    printf("=========================================\n");

    return 0;
}