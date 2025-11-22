/*
 * Implementación del Árbol Sintáctico Abstracto (AST).
 * 
 * Este módulo proporciona funciones para crear, manipular y liberar nodos del AST.
 * El AST representa la estructura sintáctica del código fuente como un árbol,
 * donde cada nodo corresponde a una construcción del lenguaje (expresiones,
 * instrucciones, declaraciones, etc.).
 */

#include "ast.h"

/**
 * Crea e inicializa un nuevo nodo del AST.
 * 
 * IMPORTANTE: Todos los punteros hijos se inicializan en NULL y el campo 'data'
 * se deja sin inicializar. Las funciones específicas de creación deben asignar
 * los valores apropiados según el tipo de nodo.
 * 
 * @param tipo Tipo de nodo a crear (NODO_PROGRAMA, NODO_ASIGNACION, etc.)
 * @param linea Número de línea en el código fuente (para reporte de errores)
 * @return Puntero al nodo creado, o NULL si falla la asignación de memoria
 * @note La función termina el programa si falla la asignación de memoria
 */
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

/**
 * Crea un nodo vacío para representar producciones lambda (ε) en la gramática.
 * 
 * Se utiliza cuando una regla gramatical puede ser vacía, permitiendo que el
 * parser maneje listas opcionales (parámetros, argumentos, instrucciones, etc.)
 */
ASTNode* crear_nodo_vacio() {
    return crear_nodo(NODO_VACIO, 0);
}

/**
 * Crea un nodo hoja para un identificador.
 * 
 * IMPORTANTE: Esta función toma posesión de la memoria del string 'nombre',
 * que debe haber sido asignada dinámicamente (típicamente por el lexer).
 * El nodo AST será responsable de liberar esta memoria cuando se destruya.
 * 
 * @param nombre String con el nombre del identificador (se transfiere la propiedad)
 * @param linea Número de línea donde aparece el identificador
 */
ASTNode* crear_nodo_hoja_id(char* nombre, int linea) {
    ASTNode* nodo = crear_nodo(NODO_IDENTIFICADOR, linea);
    nodo->data.cadena = nombre;
    return nodo;
}

/**
 * Crea un nodo hoja para un literal numérico.
 * 
 * El valor se almacena como double para soportar tanto enteros como flotantes.
 * El análisis semántico determinará si es TIPO_INT o TIPO_FLOAT según el valor.
 */
ASTNode* crear_nodo_hoja_num(double valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_NUMERO, linea);
    nodo->data.valor_num = valor;
    return nodo;
}

/**
 * Crea un nodo hoja para un literal de cadena.
 * 
 * IMPORTANTE: Toma posesión de la memoria del string 'valor', que debe haber
 * sido asignada dinámicamente por el lexer. El nodo AST liberará esta memoria.
 */
ASTNode* crear_nodo_hoja_cadena(char* valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_CADENA, linea);
    nodo->data.cadena = valor;
    return nodo;
}

/**
 * Crea un nodo hoja para un literal booleano.
 * 
 * @param valor 1 para true, 0 para false
 */
ASTNode* crear_nodo_hoja_bool(int valor, int linea) {
    ASTNode* nodo = crear_nodo(NODO_BOOLEANO, linea);
    nodo->data.valor_bool = valor;
    return nodo;
}

/**
 * Crea un nodo que representa un tipo de dato.
 * 
 * Se utiliza para almacenar información de tipos en declaraciones y parámetros.
 */
ASTNode* crear_nodo_tipo(TipoDato tipo, int linea) {
    ASTNode* nodo = crear_nodo(NODO_TIPO, linea);
    nodo->data.tipo_dato = tipo;
    return nodo;
}

/**
 * Crea un nodo para un operador unario (prefijo).
 * 
 * Operadores soportados: ! (negación lógica), - (negación aritmética),
 * + (unario positivo), ++ (incremento prefijo), -- (decremento prefijo).
 * 
 * @param op String con el operador (debe ser un literal estático, no se libera)
 * @param hijo Expresión sobre la que opera el operador unario
 */
ASTNode* crear_nodo_unario(char* op, ASTNode* hijo, int linea) {
    ASTNode* nodo = crear_nodo(NODO_UNARIO_OP, linea);
    nodo->data.op_unario = op;
    nodo->hijo1 = hijo;
    return nodo;
}

/**
 * Crea un nodo para un operador binario.
 * 
 * Operadores soportados: +, -, *, /, %, &&, ||, ==, !=, <, <=, >, >=, []
 * (acceso a array).
 * 
 * NOTA: Se reutiliza el campo 'op_unario' de la unión 'data' para almacenar
 * el string del operador binario. Esto es seguro porque ambos tipos de nodos
 * usan el mismo campo de la unión.
 * 
 * @param op String con el operador (debe ser un literal estático)
 * @param izq Operando izquierdo
 * @param der Operando derecho
 */
ASTNode* crear_nodo_binario(char* op, ASTNode* izq, ASTNode* der, int linea) {
    ASTNode* nodo = crear_nodo(NODO_BINARIO_OP, linea);
    /* Reutilizamos op_unario para almacenar el operador binario */
    nodo->data.op_unario = op;
    nodo->hijo1 = izq;
    nodo->hijo2 = der;
    return nodo;
}

/**
 * Crea un nodo para un operador postfijo.
 * 
 * Operadores soportados: ++ (incremento postfijo), -- (decremento postfijo).
 * 
 * IMPORTANTE: La diferencia con el operador prefijo es el orden de evaluación:
 * en postfijo, primero se evalúa la expresión y luego se incrementa/decrementa.
 */
ASTNode* crear_nodo_postfix(ASTNode* hijo, char* op, int linea) {
    ASTNode* nodo = crear_nodo(NODO_POSTFIX_OP, linea);
    nodo->data.op_unario = op;
    nodo->hijo1 = hijo;
    return nodo;
}

/**
 * Crea un nodo para una función reservada del lenguaje.
 * 
 * Funciones reservadas: mover, girarIzq, girarDer, leerSensor, parar, reversa.
 * 
 * NOTA: El string 'nombre' debe ser un literal estático (no se duplica ni se libera).
 */
ASTNode* crear_nodo_funcion_reservada(char* nombre, int linea) {
    ASTNode* nodo = crear_nodo(NODO_FUNCION_RESERVADA, linea);
    /* Los nombres de funciones reservadas son literales estáticos */
    nodo->data.cadena = nombre; 
    return nodo;
}

/**
 * Enlaza un nodo al final de una lista enlazada.
 * 
 * Esta función es la base para todas las operaciones de enlace de listas
 * (instrucciones, parámetros, argumentos, declaraciones).
 * 
 * IMPORTANTE: Si la lista es NULL o contiene solo un NODO_VACIO, retorna
 * el nuevo item como cabeza de la lista. Esto permite construir listas
 * de forma incremental desde vacías.
 * 
 * @param lista Cabeza de la lista enlazada (puede ser NULL o NODO_VACIO)
 * @param item Nodo a agregar al final de la lista
 * @return Cabeza de la lista (puede ser 'item' si la lista estaba vacía)
 */
ASTNode* enlazar_nodos(ASTNode* lista, ASTNode* item) {
    if (!lista || lista->tipo == NODO_VACIO) {
        return item;
    }
    ASTNode* actual = lista;
    while (actual->siguiente) {
        actual = actual->siguiente;
    }
    actual->siguiente = item;
    return lista;
}

/**
 * Enlaza una instrucción a una lista de instrucciones.
 * 
 * Wrapper semántico sobre enlazar_nodos() para mejorar la legibilidad del código.
 */
ASTNode* enlazar_instruccion(ASTNode* lista, ASTNode* instruccion) {
    return enlazar_nodos(lista, instruccion);
}

/**
 * Enlaza un parámetro a una lista de parámetros.
 */
ASTNode* enlazar_parametro(ASTNode* lista, ASTNode* parametro) {
    return enlazar_nodos(lista, parametro);
}

/**
 * Enlaza un argumento a una lista de argumentos.
 */
ASTNode* enlazar_argumento(ASTNode* lista, ASTNode* argumento) {
    return enlazar_nodos(lista, argumento);
}

/**
 * Enlaza una declaración a una lista de declaraciones.
 */
ASTNode* enlazar_declaracion(ASTNode* lista, ASTNode* declaracion_init) {
    return enlazar_nodos(lista, declaracion_init);
}

/**
 * Libera recursivamente toda la memoria asociada al AST.
 * 
 * IMPORTANTE: Solo libera strings para identificadores y literales de cadena,
 * que fueron asignados dinámicamente por el lexer. Los operadores son strings
 * estáticos y NO deben liberarse.
 * 
 * La función recorre el árbol en profundidad primero (post-order) para asegurar
 * que los hijos se liberen antes que sus padres.
 * 
 * @param nodo Raíz del subárbol a liberar (puede ser NULL)
 */
void liberar_arbol(ASTNode* nodo) {
    if (!nodo) return;
    
    /* Liberar recursivamente todos los hijos y el siguiente nodo en la lista */
    liberar_arbol(nodo->hijo1);
    liberar_arbol(nodo->hijo2);
    liberar_arbol(nodo->hijo3);
    liberar_arbol(nodo->hijo4);
    liberar_arbol(nodo->siguiente);

    /* Liberar strings solo para identificadores y literales de cadena */
    /* Estos fueron asignados dinámicamente por el lexer */
    if (nodo->tipo == NODO_IDENTIFICADOR || nodo->tipo == NODO_CADENA) {
        if (nodo->data.cadena) {
            free(nodo->data.cadena);
        }
    }
    /* Los operadores son strings estáticos, NO liberarlos */

    free(nodo);
}

/**
 * Imprime la estructura del AST con indentación para depuración.
 * 
 * Esta función es útil durante el desarrollo para visualizar la estructura
 * del árbol generado por el parser. Muestra el tipo de cada nodo y sus valores
 * cuando aplica (identificadores, números, operadores, etc.).
 * 
 * @param nodo Nodo raíz del subárbol a imprimir
 * @param nivel Nivel de indentación actual (0 para la raíz)
 */
void imprimir_arbol(ASTNode* nodo, int nivel) {
    if (!nodo) return;

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

    if (nodo->hijo1) imprimir_arbol(nodo->hijo1, nivel + 1);
    if (nodo->hijo2) imprimir_arbol(nodo->hijo2, nivel + 1);
    if (nodo->hijo3) imprimir_arbol(nodo->hijo3, nivel + 1);
    if (nodo->hijo4) imprimir_arbol(nodo->hijo4, nivel + 1);
    
    if (nodo->siguiente && nodo->siguiente->tipo != NODO_VACIO) {
        for (int i = 0; i < nivel - 1; i++) printf("  ");
        printf("  (siguiente) ->\n");
        imprimir_arbol(nodo->siguiente, nivel);
    }
}