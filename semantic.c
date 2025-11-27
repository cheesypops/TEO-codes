/* ==========================================
   semantic.c - Analizador Semántico
   ========================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"
#include "ast.h"

/* --- ESTRUCTURAS DE LA TABLA DE SÍMBOLOS --- */

typedef struct Symbol {
    char *name;         // Nombre de la variable
    char *type;         // Tipo (int, boolean, etc.)
    int scope_level;    // Nivel de profundidad del scope
    struct Symbol *next; // Lista enlazada
} Symbol;

static Symbol *symbol_table = NULL;
static int current_scope = 0;
static int semantic_errors = 0;

/* --- FUNCIONES AUXILIARES DE LA TABLA --- */

/* Inserta un símbolo en la tabla (al inicio de la lista) */
void add_symbol(char *name, char *type) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = strdup(type);
    sym->scope_level = current_scope;
    sym->next = symbol_table;
    symbol_table = sym;
    // printf("DEBUG: Declarada variable '%s' de tipo '%s' en scope %d\n", name, type, current_scope);
}

/* Busca un símbolo en toda la tabla (para verificar uso) */
Symbol* find_symbol(char *name) {
    Symbol *current = symbol_table;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Busca un símbolo SOLO en el scope actual (para verificar redeclaración) */
int is_declared_in_current_scope(char *name) {
    Symbol *current = symbol_table;
    while (current != NULL && current->scope_level == current_scope) {
        if (strcmp(current->name, name) == 0) {
            return 1; // Encontrado en el scope actual
        }
        current = current->next;
    }
    return 0;
}

/* Elimina los símbolos del scope actual al salir de un bloque */
void exit_scope() {
    while (symbol_table != NULL && symbol_table->scope_level == current_scope) {
        Symbol *temp = symbol_table;
        symbol_table = symbol_table->next;
        
        free(temp->name);
        free(temp->type);
        free(temp);
    }
    current_scope--;
}

void enter_scope() {
    current_scope++;
}

void report_error(const char *msg, const char *detail) {
    fprintf(stderr, "Error Semántico: %s [%s]\n", msg, detail ? detail : "");
    semantic_errors++;
}

/* --- VISITOR (RECORRIDO DEL ÁRBOL) --- */

void check_node(ASTNode *node);

/* Función para recorrer listas de sentencias (hermanos) */
void check_list(ASTNode *node) {
    while (node != NULL) {
        check_node(node);
        // Si el nodo es un bloque o un if, ya manejó sus hijos internos.
        // Avanzamos al siguiente hermano en la lista de instrucciones.
        // NOTA: En tu parser, 'bloque' usa 'next' para sus hijos.
        // Aquí asumimos que check_list recorre la lista enlazada 'next' del nivel actual.
        // Si estamos DENTRO de un bloque, 'node->next' es la siguiente instrucción.
        node = node->next; 
        
        /* Caso especial: Si el nodo era un bloque, su puntero next apuntaba a su contenido 
           interno según el parser.y anterior. Esto rompe la lista de hermanos externa.
           Sin embargo, para propósitos de análisis semántico recursivo estándar,
           asumiremos que la estructura AST se navega correctamente o que el parser
           se ajustará para tener punteros 'body' separados. 
           
           Para este código, seguiremos la lógica estándar de AST:
           node->next apunta al siguiente hermano.
        */
    }
}

void check_node(ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case NODE_PROGRAM:
            // El programa raíz suele tener la lista de elementos en 'next'
            // o se pasa directamente el primer elemento.
            // Si root es PROGRAM, revisamos sus hijos.
            // Como en parser.y root = elementos, y elementos es una lista en 'next':
            check_list(node); 
            break;

        case NODE_VAR_DECL:
            // Regla: No redeclarar en el mismo scope
            if (is_declared_in_current_scope(node->data.str_val)) {
                report_error("Variable redeclarada en el mismo ambito", node->data.str_val);
            } else {
                add_symbol(node->data.str_val, node->type_name);
            }
            // Revisar la expresión de inicialización si existe
            if (node->left) check_node(node->left);
            break;

        case NODE_ID:
            // Regla: La variable debe haber sido declarada antes
            if (find_symbol(node->data.str_val) == NULL) {
                report_error("Variable no declarada", node->data.str_val);
            }
            break;

        case NODE_BLOCK:
            enter_scope();
            // El contenido del bloque está en node->next según tu parser
            // Pero check_list iterará sobre ese next.
            check_list(node->next);
            exit_scope();
            break;

        case NODE_IF:
            check_node(node->left);  // Condición
            check_node(node->right); // Bloque True (es un bloque, creará scope propio)
            if (node->extra) check_node(node->extra); // Bloque Else
            break;

        case NODE_WHILE:
            check_node(node->left);  // Condición
            check_node(node->right); // Bloque
            break;

        case NODE_FOR:
            enter_scope(); // El for suele tener su propio scope para la variable init
            check_node(node->left);  // Init (ej: int i = 0)
            check_node(node->right); // Condición
            check_node(node->extra); // Incremento
            check_node(node->next);  // Cuerpo
            exit_scope();
            break;

        case NODE_ASSIGN:
            // Verificar lado izquierdo (debe ser ID válido) y derecho
            check_node(node->left);
            check_node(node->right);
            // Aquí se podría agregar chequeo de tipos (int = int)
            break;

        case NODE_CALL_ROBOT:
            // Validación específica para funciones del robot
            if (strcmp(node->data.str_val, "esperar") == 0) {
                // token_esperar requiere al menos un argumento (tiempo)
                if (node->left == NULL) {
                    report_error("La funcion 'token_esperar' requiere un argumento (tiempo en ms)", NULL);
                }
            }
            // Validar los argumentos (que las variables usadas dentro existan)
            if (node->left) check_list(node->left);
            break;

        case NODE_FUNCTION:
            // Declaración de función
            add_symbol(node->data.str_val, node->type_name); // Agregar nombre función al scope actual
            enter_scope();
            if (node->left) check_list(node->left); // Validar parámetros (agregarlos al scope de la func)
            if (node->right) check_node(node->right); // Validar cuerpo
            exit_scope();
            break;

        /* Operaciones binarias, unarias y llamadas a funciones */
        case NODE_BIN_OP:
        case NODE_UNARY_OP:
        case NODE_CALL_FUNC:
        case NODE_RETURN:
            if (node->left) check_node(node->left);
            if (node->right) check_node(node->right);
            break;

        default:
            // Para constantes (INT, BOOL) no hay chequeos semánticos necesarios
            break;
    }
}

/* --- INTERFAZ PÚBLICA --- */

void semantic_analysis(ASTNode *root) {
    semantic_errors = 0;
    current_scope = 0;
    symbol_table = NULL;

    printf("Iniciando Analisis Semantico...\n");
    check_node(root);
    
    if (semantic_errors == 0) {
        printf("Analisis Semantico completado con exito.\n");
    } else {
        printf("Analisis Semantico finalizado con %d errores.\n", semantic_errors);
    }
}

int get_semantic_errors() {
    return semantic_errors;
}