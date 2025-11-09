#include <stdio.h>
#include "ast.h"
#include "semantic.h"

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

    /* Llamada principal al parser */
    int resultado_parse = yyparse();
    
    fclose(yyin);

    if (resultado_parse != 0) {
        fprintf(stderr, "Analisis fallido.\n");
        return 1;
    }

    printf("Analisis lexico y sintactico completado con exito.\n\n");
    
    /* Imprimir el AST (para depuración) */
    printf("--- Arbol Sintactico Abstracto (AST) ---\n");
    imprimir_arbol(raiz_ast, 0);
    printf("---------------------------------------\n");
    
    /* Llamada al analizador semántico */
    analizar_semantica(raiz_ast);
    
    /* Liberar memoria del AST */
    liberar_arbol(raiz_ast);

    return 0;
}