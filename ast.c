#include "ast.h"

/* Función base para crear un nodo */
ASTNode* crear_nodo(TipoNodo tipo, int linea) {
    ASTNode* nodo = (ASTNode*)malloc(sizeof(ASTNode));
    if (!nodo) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nodo AST\n");
        exit(1);
    }
    nodo->tipo = tipo;
    nodo->hijo1 = NULL;
    nodo->hijo2 = NULL;
    nodo->hijo3 = NULL;
    nodo->hijo4 = NULL;
    nodo->siguiente = NULL;
    nodo->data.cadena = NULL;
    nodo->linea = linea;
    return nodo;
}

ASTNode* crear_nodo_vacio() {
    return crear_nodo(NODO_VACIO, 0);
}

ASTNode* crear_nodo_hoja_id(char* nombre, int linea) {
    ASTNode* nodo = crear_nodo(NODO_IDENTIFICADOR, linea);
    nodo->data.cadena = nombre; // yylval se encargará de la memoria
    return nodo;
}

ASTNode* crear_nodo_hoja_num(double valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_NUMERO, linea);
    nodo->data.valor_num = valor;
    return nodo;
}

ASTNode* crear_nodo_hoja_cadena(char* valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_CADENA, linea);
    nodo->data.cadena = valor; // yylval se encargará de la memoria
    return nodo;
}

ASTNode* crear_nodo_hoja_bool(int valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_BOOLEANO, linea);
    nodo->data.valor_bool = valor;
    return nodo;
}

ASTNode* crear_nodo_tipo(TipoDato tipo, int linea) {
    ASTNode* nodo = crear_nodo(NODO_TIPO, linea);
    nodo->data.tipo_dato = tipo;
    return nodo;
}

ASTNode* crear_nodo_unario(char* op, ASTNode* hijo, int linea) {
    ASTNode* nodo = crear_nodo(NODO_UNARIO_OP, linea);
    nodo->data.op_unario = op;
    nodo->hijo1 = hijo;
    return nodo;
}

ASTNode* crear_nodo_binario(char* op, ASTNode* izq, ASTNode* der, int linea) {
    ASTNode* nodo = crear_nodo(NODO_BINARIO_OP, linea);
    nodo->data.op_unario = op; // Usamos op_unario para guardar el string del op
    nodo->hijo1 = izq;
    nodo->hijo2 = der;
    return nodo;
}

ASTNode* crear_nodo_postfix(ASTNode* hijo, char* op, int linea) {
    ASTNode* nodo = crear_nodo(NODO_POSTFIX_OP, linea);
    nodo->data.op_unario = op;
    nodo->hijo1 = hijo;
    return nodo;
}

ASTNode* crear_nodo_funcion_reservada(char* nombre, int linea) {
    ASTNode* nodo = crear_nodo(NODO_FUNCION_RESERVADA, linea);
    /* Asignamos el string literal estático. No necesitamos strdup() */
    nodo->data.cadena = nombre; 
    return nodo;
}

/* Enlaza dos nodos en una lista usando el campo 'siguiente' */
ASTNode* enlazar_nodos(ASTNode* lista, ASTNode* item) {
    if (!lista || lista->tipo == NODO_VACIO) {
        /* Si la lista es nula o vacía, el item es la nueva lista */
        return item;
    }
    ASTNode* actual = lista;
    while (actual->siguiente) {
        actual = actual->siguiente;
    }
    actual->siguiente = item;
    return lista;
}

ASTNode* enlazar_instruccion(ASTNode* lista, ASTNode* instruccion) {
    return enlazar_nodos(lista, instruccion);
}

ASTNode* enlazar_parametro(ASTNode* lista, ASTNode* parametro) {
    return enlazar_nodos(lista, parametro);
}

ASTNode* enlazar_argumento(ASTNode* lista, ASTNode* argumento) {
    return enlazar_nodos(lista, argumento);
}

ASTNode* enlazar_declaracion(ASTNode* lista, ASTNode* declaracion_init) {
    // Para Declaracion' -> , id Init Declaracion'
    // 'lista' es el nodo (id, init) y 'declaracion_init' es el resto de la lista
    return enlazar_nodos(lista, declaracion_init);
}


/* Libera recursivamente el árbol */
void liberar_arbol(ASTNode* nodo) {
    if (!nodo) return;
    
    // Libera hijos
    liberar_arbol(nodo->hijo1);
    liberar_arbol(nodo->hijo2);
    liberar_arbol(nodo->hijo3);
    liberar_arbol(nodo->hijo4);
    liberar_arbol(nodo->siguiente);

    // Libera datos internos si es necesario (IDs y Cadenas)
    if (nodo->tipo == NODO_IDENTIFICADOR || nodo->tipo == NODO_CADENA) {
        if (nodo->data.cadena) {
            free(nodo->data.cadena);
        }
    }
    // Nota: los operadores (op_unario) son strings estáticos, no se liberan.

    free(nodo);
}

/* Función auxiliar para imprimir el árbol (para depuración) */
void imprimir_arbol(ASTNode* nodo, int nivel) {
    if (!nodo) return;

    // Imprimir indentación
    for (int i = 0; i < nivel; i++) printf("  ");

    switch (nodo->tipo) {
        case NODO_PROGRAMA: printf("Programa\n"); break;
        case NODO_DECLARACION_GLOBAL: printf("DeclaracionGlobal\n"); break;
        case NODO_SETUP: printf("SetupDef\n"); break;
        case NODO_FUNCION_DEF: printf("FuncionDef\n"); break;
        case NODO_BLOQUE: printf("Bloque\n"); break;
        case NODO_LISTA_INSTRUCCIONES: printf("ListaInstrucciones\n"); break;
        case NODO_ASIGNACION: printf("Asignacion\n"); break;
        case NODO_DECLARACION: printf("Declaracion\n"); break;
        case NODO_IF: printf("If\n"); break;
        case NODO_FOR: printf("For\n"); break;
        case NODO_WHILE: printf("While\n"); break;
        case NODO_DO_WHILE: printf("DoWhile\n"); break;
        case NODO_LLAMADA_FUNCION: printf("LlamadaFuncion\n"); break;
        case NODO_FUNCION_RESERVADA: printf("FuncReservada: %s\n", nodo->data.cadena); break;
        case NODO_BINARIO_OP: printf("OpBinario (%s)\n", nodo->data.op_unario); break;
        case NODO_UNARIO_OP: printf("OpUnario (%s)\n", nodo->data.op_unario); break;
        case NODO_POSTFIX_OP: printf("OpPostfix (%s)\n", nodo->data.op_unario); break;
        case NODO_IDENTIFICADOR: printf("Id: %s\n", nodo->data.cadena); break;
        case NODO_NUMERO: printf("Num: %f\n", nodo->data.valor_num); break;
        case NODO_CADENA: printf("Str: \"%s\"\n", nodo->data.cadena); break;
        case NODO_BOOLEANO: printf("Bool: %s\n", nodo->data.valor_bool ? "true" : "false"); break;
        case NODO_TIPO: printf("Tipo (%d)\n", nodo->data.tipo_dato); break;
        case NODO_LISTA_PARAMETROS: printf("ListaParametros\n"); break;
        case NODO_LISTA_ARGUMENTOS: printf("ListaArgumentos\n"); break;
        case NODO_VACIO: printf("Vacio (λ)\n"); break;
        default: printf("Nodo Desconocido (%d)\n", nodo->tipo); break;
    }

    // Recorrer hijos
    if (nodo->hijo1) imprimir_arbol(nodo->hijo1, nivel + 1);
    if (nodo->hijo2) imprimir_arbol(nodo->hijo2, nivel + 1);
    if (nodo->hijo3) imprimir_arbol(nodo->hijo3, nivel + 1);
    if (nodo->hijo4) imprimir_arbol(nodo->hijo4, nivel + 1);
    
    // Recorrer listas
    if (nodo->siguiente && nodo->siguiente->tipo != NODO_VACIO) {
        for (int i = 0; i < nivel - 1; i++) printf("  "); // Indentación de lista
        printf("  (siguiente) ->\n");
        imprimir_arbol(nodo->siguiente, nivel);
    }
}