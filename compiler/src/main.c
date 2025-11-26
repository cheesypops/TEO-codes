#include <stdio.h>
#include <stdlib.h> /* Biblioteca estándar utilizada por el compilador */
#include "ast/ast.h"
#include "semantic/semantic.h"
#include "codegen/codegen.h" /* Módulo encargado de transformar el AST en bytecode */

/* Función principal del parser generada por Bison.
 * Se encarga de realizar el análisis sintáctico y de construir el AST en la variable global raiz_ast.
 */
extern int yyparse(void);

/* Puntero global al nodo raíz del AST construido por yyparse.
 * Se reutiliza en las etapas de análisis semántico y generación de código.
 */
extern ASTNode *raiz_ast; 

/* Flujo de entrada que Flex utiliza como fuente de tokens.
 * Se asocia al archivo de programa fuente que se desea compilar.
 */
extern FILE *yyin;

/* Función principal del compilador.
 * Orquesta el pipeline completo: abre el archivo fuente, invoca al parser para construir el AST,
 * ejecuta el análisis semántico, genera el bytecode para la máquina virtual y libera los recursos.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso esperado: %s <archivo_fuente>\n", argv[0]);
        return 1;
    }

    /* Se abre el archivo fuente y se conecta como entrada del lexer.
     * Sin un archivo válido no tiene sentido iniciar el proceso de compilación.
     */
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Error: no se pudo abrir el archivo '%s'.\n", argv[1]);
        return 1;
    }

    printf("Iniciando análisis léxico y sintáctico del archivo: %s\n", argv[1]);

    /* 1. Análisis léxico y sintáctico.
     * En esta etapa se recorre el archivo de entrada, se reconocen los tokens
     * y se construye el AST que representará el programa.
     */
    int resultado_parse = yyparse();
    
    fclose(yyin);

    if (resultado_parse != 0) {
        fprintf(stderr, "Análisis sintáctico fallido: se encontraron errores de sintaxis.\n");
        /* Si el parser informa errores, no se continúa con las etapas posteriores. */
        if (raiz_ast) liberar_arbol(raiz_ast);
        return 1;
    }

    printf("Análisis léxico y sintáctico completado con éxito.\n\n");
    
    /* Impresión del AST para inspección y depuración.
     * Esta salida permite visualizar la estructura producida por el parser.
     */
    printf("--- Árbol Sintáctico Abstracto (AST) ---\n");
    imprimir_arbol(raiz_ast, 0);
    printf("---------------------------------------\n");
    
    /* 2. Análisis semántico sobre el AST.
     * Verifica tipos, declaraciones y reglas del lenguaje.
     * Devuelve 1 si todo fue correcto (o solo hubo advertencias no fatales) y 0 si hubo errores graves.
     */
    if (analizar_semantica(raiz_ast)) {
        
        /* 3. Generación de código intermedio (bytecode para la máquina virtual).
         * Solo se genera código si el análisis semántico fue satisfactorio,
         * y se guarda el resultado en "codigo.txt" para ser consumido por la VM en la ESP32.
         */
        generar_codigo(raiz_ast, "codigo.txt");
        
    } else {
        fprintf(stderr, "No se generó código debido a errores semánticos.\n");
    }
    
    /* 4. Liberación de recursos del compilador.
     * Se limpia la memoria asociada a estructuras globales para evitar fugas.
     */
    printf("\nLiberando memoria empleada por el compilador...\n");
    
    /* Se libera la tabla de símbolos, que se mantuvo viva hasta después de la generación de código. */
    ts_liberar_total(); 
    
    /* Se libera el árbol AST completo construido durante el análisis. */
    liberar_arbol(raiz_ast);

    printf("Compilación finalizada correctamente.\n");
    return 0;
}