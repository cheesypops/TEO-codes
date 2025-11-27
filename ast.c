/* ==========================================
   ast.c - Implementación del AST
   ========================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* ==========================================
   CONSTRUCTORES DE NODOS
   ========================================== */

/* Función base para crear un nodo genérico e inicializarlo */
ASTNode* newASTNode(NodeKind kind) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: No hay memoria suficiente para crear nodo AST.\n");
        exit(1);
    }
    
    node->kind = kind;
    node->type_name = NULL;
    node->left = NULL;
    node->right = NULL;
    node->extra = NULL;
    node->next = NULL;
    
    // Inicializamos la unión data a 0/NULL
    node->data.str_val = NULL; 
    
    return node;
}

/* Constructor para Operaciones Binarias (Suma, AND, Igualdad, etc.) */
ASTNode* newBinaryNode(char *op, ASTNode *left, ASTNode *right) {
    ASTNode* node = newASTNode(NODE_BIN_OP);
    node->data.str_val = strdup(op); // Copiamos la cadena del operador
    node->left = left;
    node->right = right;
    return node;
}

/* Constructor para Operaciones Unarias (Negación, Incremento, etc.) */
ASTNode* newUnaryNode(char *op, ASTNode *child) {
    ASTNode* node = newASTNode(NODE_UNARY_OP);
    node->data.str_val = strdup(op);
    node->left = child; // Usamos left para el único operando
    return node;
}

/* Constructor para Literales Enteros */
ASTNode* newIntNode(int val) {
    ASTNode* node = newASTNode(NODE_CONST_INT);
    node->data.int_val = val;
    return node;
}

/* Constructor para Identificadores (Variables) */
ASTNode* newIDNode(char *name) {
    ASTNode* node = newASTNode(NODE_ID);
    node->data.str_val = strdup(name);
    return node;
}

/* Constructor para Funciones del Robot */
ASTNode* newRobotNode(char *action, ASTNode *args) {
    ASTNode* node = newASTNode(NODE_CALL_ROBOT);
    node->data.str_val = strdup(action); // Ej: "mover", "girar"
    node->left = args; // Lista de argumentos en el hijo izquierdo
    return node;
}

/* ==========================================
   UTILIDADES DE VISUALIZACIÓN
   ========================================== */

/* Helper para imprimir indentación */
void printIndent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

/* Función recursiva para imprimir el árbol */
void printAST(ASTNode *node, int level) {
    if (!node) return;

    /* Imprimimos el nodo actual y sus hermanos (lista next) */
    ASTNode *current = node;
    while (current != NULL) {
        printIndent(level);

        switch (current->kind) {
            case NODE_PROGRAM:
                printf("[PROGRAMA]\n");
                // Los hijos del programa están en next, el while lo maneja, 
                // pero si tiene estructura jerárquica interna:
                break;

            case NODE_FUNCTION:
                printf("[FUNCION] %s %s\n", 
                       current->type_name ? current->type_name : "void",
                       current->data.str_val);
                printIndent(level + 1); printf("PARAMS:\n");
                printAST(current->left, level + 2);
                printIndent(level + 1); printf("CUERPO:\n");
                printAST(current->right, level + 2);
                break;

            case NODE_VAR_DECL:
                printf("[DECLARACION] %s %s\n", 
                       current->type_name, 
                       current->data.str_val);
                if (current->left) {
                    printIndent(level + 1); printf("Inicializacion:\n");
                    printAST(current->left, level + 2);
                }
                break;

            case NODE_BLOCK:
                printf("[BLOQUE]\n");
                // El contenido del bloque está en 'next', pero como estamos en un loop while
                // sobre 'current', necesitamos diferenciar si 'next' es hermano o hijo.
                // En el parser definimos: $$->next = $2.
                // Para visualizar mejor, llamamos recursivamente a next aqui
                // y rompemos el while del nivel actual si es necesario, 
                // o iteramos manualmente.
                printAST(current->next, level + 1);
                return; // El contenido del bloque se maneja aqui, salimos.

            case NODE_IF:
                printf("[IF]\n");
                printIndent(level + 1); printf("Condicion:\n");
                printAST(current->left, level + 2);
                printIndent(level + 1); printf("Then:\n");
                printAST(current->right, level + 2);
                if (current->extra) {
                    printIndent(level + 1); printf("Else:\n");
                    printAST(current->extra, level + 2);
                }
                break;

            case NODE_WHILE:
                printf("[WHILE]\n");
                printIndent(level + 1); printf("Condicion:\n");
                printAST(current->left, level + 2);
                printIndent(level + 1); printf("Cuerpo:\n");
                printAST(current->right, level + 2);
                break;
            
            case NODE_FOR:
                printf("[FOR]\n");
                printIndent(level + 1); printf("Init:\n");
                printAST(current->left, level + 2);
                printIndent(level + 1); printf("Cond:\n");
                printAST(current->right, level + 2);
                printIndent(level + 1); printf("Step:\n");
                printAST(current->extra, level + 2);
                printIndent(level + 1); printf("Cuerpo:\n");
                // El cuerpo del for lo guardamos en 'next' en el parser
                printAST(current->next, level + 2); 
                return; // Salimos para no reimprimir next en el while principal

            case NODE_ASSIGN:
                printf("[ASIGNACION (=)]\n");
                printAST(current->left, level + 1);
                printAST(current->right, level + 1);
                break;

            case NODE_BIN_OP:
                printf("[OP BINARIA] %s\n", current->data.str_val);
                printAST(current->left, level + 1);
                printAST(current->right, level + 1);
                break;

            case NODE_CONST_INT:
                printf("[INT] %d\n", current->data.int_val);
                break;
            
            case NODE_CONST_STR:
                printf("[STRING] %s\n", current->data.str_val);
                break;

            case NODE_CONST_BOOL:
                printf("[BOOL] %s\n", current->data.int_val ? "true" : "false");
                break;

            case NODE_ID:
                printf("[ID] %s\n", current->data.str_val);
                break;

            case NODE_CALL_ROBOT:
                printf("[ROBOT ACCION] %s\n", current->data.str_val);
                if (current->left) {
                    printIndent(level + 1); printf("Argumentos:\n");
                    printAST(current->left, level + 2);
                }
                break;
                
            case NODE_RETURN:
                printf("[RETURN]\n");
                if (current->left) printAST(current->left, level + 1);
                break;

            default:
                printf("[NODO DESCONOCIDO %d]\n", current->kind);
        }

        // Avanzamos al siguiente nodo en la lista (hermanos/instrucciones siguientes)
        // NOTA: Para IF, WHILE, BLOQUE, ya manejamos sus hijos internos. 
        // Este next es para la siguiente instrucción al mismo nivel.
        if (current->kind == NODE_BLOCK || current->kind == NODE_FOR) {
            // Estos nodos usaron 'next' para sus hijos/contenido, 
            // así que no iteramos aqui para evitar duplicados.
            break; 
        }
        
        current = current->next;
    }
}

/* ==========================================
   GESTIÓN DE MEMORIA
   ========================================== */

void freeAST(ASTNode *node) {
    if (!node) return;

    // Liberar hijos primero
    freeAST(node->left);
    freeAST(node->right);
    freeAST(node->extra);
    
    // Si no es un bloque (que usa next como hijos), liberar el siguiente hermano
    // Esto depende de cómo quieras limpiar la lista.
    // Generalmente es seguro llamar recursivo a next.
    freeAST(node->next);

    // Liberar cadenas asignadas dinámicamente
    if (node->type_name) free(node->type_name); // Ojo: si type_name viene de yylval.sval (strdup), liberar.
    
    // Liberar data.str_val si aplica (IDs, Operadores, Strings)
    if (node->kind == NODE_ID || node->kind == NODE_CONST_STR || 
        node->kind == NODE_BIN_OP || node->kind == NODE_UNARY_OP || 
        node->kind == NODE_CALL_ROBOT || node->kind == NODE_FUNCTION || 
        node->kind == NODE_VAR_DECL) {
        
        if (node->data.str_val) free(node->data.str_val);
    }

    free(node);
}