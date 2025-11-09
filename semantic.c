#include "semantic.h"

/* Estructura para la Tabla de Símbolos (muy simplificada) */
/* En un compilador real, esto sería un hash map o un árbol */
#define MAX_SIMBOLOS 1024
Simbolo tabla_simbolos[MAX_SIMBOLOS];
int num_simbolos = 0;
int ambito_actual = 0; // 0 = Global

/* --- Funciones de la Tabla de Símbolos --- */

void ts_entrar_ambito() {
    ambito_actual++;
    printf("[Semantico: Entrando en ambito %d]\n", ambito_actual);
}

void ts_salir_ambito() {
    printf("[Semantico: Saliendo de ambito %d]\n", ambito_actual);
    // Aquí deberíamos eliminar los símbolos de este ámbito
    // (Implementación simplificada: no los eliminamos por ahora)
    ambito_actual--;
}

Simbolo* ts_buscar(char* nombre) {
    // Busca de atrás hacia adelante (del ámbito más interno al más externo)
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

int ts_insertar(char* nombre, TipoDato tipo, int linea) {
    if (num_simbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "Error Semantico: Tabla de simbolos llena.\n");
        return 0;
    }
    
    // Simplificado: no revisamos redeclaraciones en el mismo ámbito
    
    tabla_simbolos[num_simbolos].nombre = strdup(nombre);
    tabla_simbolos[num_simbolos].tipo = tipo;
    tabla_simbolos[num_simbolos].ambito = ambito_actual;
    tabla_simbolos[num_simbolos].linea = linea;
    
    printf("[Semantico: Declarado '%s' (tipo %d) en ambito %d]\n", nombre, tipo, ambito_actual);
    
    num_simbolos++;
    return 1;
}

void ts_liberar() {
    for (int i = 0; i < num_simbolos; i++) {
        free(tabla_simbolos[i].nombre);
    }
    num_simbolos = 0;
}

/* --- Funciones del Recorredor (Walker) del AST --- */

/* Prototipos de funciones del recorredor */
void analizar_nodo(ASTNode* nodo);
TipoDato analizar_expresion(ASTNode* nodo);
void analizar_declaracion(ASTNode* nodo);
void analizar_bloque(ASTNode* nodo);

/* Función principal del analizador semántico */
int analizar_semantica(ASTNode* raiz) {
    if (!raiz) {
        fprintf(stderr, "Error: El arbol AST esta vacio.\n");
        return 0;
    }
    
    printf("\n--- Iniciando Analisis Semantico ---\n");
    analizar_nodo(raiz);
    printf("--- Fin del Analisis Semantico ---\n");
    
    ts_liberar();
    return 1; // Devolver 1 si no hay errores (simplificado)
}

/* Recorredor genérico */
void analizar_nodo(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            analizar_nodo(nodo->hijo1); // DeclaracionGlobal
            analizar_nodo(nodo->hijo2); // SetupDef
            analizar_nodo(nodo->hijo3); // FuncionDef
            break;

        case NODO_DECLARACION_GLOBAL:
            // Esto es una lista, la recorremos
            analizar_nodo(nodo); 
            analizar_nodo(nodo->siguiente);
            break;
            
        case NODO_SETUP:
            // Validar que 'setup' devuelva el tipo correcto (ej. void o int)
            // (Tu GIC lo define como 'Tipo', no 'TipoRetorno')
            printf("[Semantico: Analizando setup()]\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2); // Bloque
            ts_salir_ambito();
            break;
            
        case NODO_FUNCION_DEF:
        {
            char* nombre_func = nodo->hijo2->data.cadena;
            TipoDato tipo_retorno = nodo->hijo1->data.tipo_dato;
            printf("[Semantico: Analizando funcion '%s']\n", nombre_func);
            
            // 1. Declarar la función en el ámbito actual (global)
            ts_insertar(nombre_func, tipo_retorno, nodo->linea); 
            
            // 2. Entrar en el nuevo ámbito de la función
            ts_entrar_ambito();
            
            // 3. Declarar parámetros en el nuevo ámbito
            ASTNode* param = nodo->hijo3;
            while(param && param->tipo != NODO_VACIO) {
                analizar_declaracion(param); // Los parámetros son como declaraciones
                param = param->siguiente;
            }
            
            // 4. Analizar el bloque de la función
            analizar_bloque(nodo->hijo4);
            
            // 5. Salir del ámbito de la función
            ts_salir_ambito();
            break;
        }
            
        case NODO_BLOQUE:
            analizar_bloque(nodo);
            break;
            
        case NODO_ASIGNACION:
        {
            char* nombre_id = nodo->hijo1->data.cadena;
            printf("[Semantico: Analizando asignacion a '%s']\n", nombre_id);
            Simbolo* sym = ts_buscar(nombre_id);
            if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Variable '%s' no declarada.\n", nodo->linea, nombre_id);
            } else {
                // Variable existe. Ahora chequear tipos.
                TipoDato tipo_var = sym->tipo;
                TipoDato tipo_expr = analizar_expresion(nodo->hijo2);
                
                if (tipo_var != tipo_expr) {
                    fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en asignacion a '%s'. Se esperaba %d pero se obtuvo %d.\n",
                        nodo->linea, nombre_id, tipo_var, tipo_expr);
                }
            }
            break;
        }

        case NODO_DECLARACION:
            analizar_declaracion(nodo);
            break;
            
        case NODO_IF:
            printf("[Semantico: Analizando IF]\n");
            // 1. Chequear que la condición sea booleana
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'if' no es booleana.\n", nodo->linea);
            }
            // 2. Analizar el bloque 'then'
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);
            ts_salir_ambito();
            // 3. Analizar el bloque 'else' (si existe)
            if (nodo->hijo3->tipo != NODO_VACIO) {
                ts_entrar_ambito();
                analizar_bloque(nodo->hijo3);
                ts_salir_ambito();
            }
            break;

        case NODO_WHILE:
            printf("[Semantico: Analizando WHILE]\n");
            // 1. Chequear que la condición sea booleana
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'while' no es booleana.\n", nodo->linea);
            }
            // 2. Analizar el bloque
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);
            ts_salir_ambito();
            break;
            
        // TODO: Implementar análisis para NODO_FOR, NODO_DO_WHILE, etc.
            
        default:
            // Para nodos que son expresiones (ej. Expresion;)
            if (nodo->tipo >= NODO_LLAMADA_FUNCION && nodo->tipo <= NODO_BOOLEANO) {
                analizar_expresion(nodo);
            }
            break;
    }
    
    // Si es una lista de instrucciones, seguimos al siguiente
    if (nodo->tipo == NODO_LISTA_INSTRUCCIONES || nodo->tipo == NODO_FUNCION_DEF) {
        analizar_nodo(nodo->siguiente);
    }
}

/* Analiza un bloque (que es una lista de instrucciones) */
void analizar_bloque(ASTNode* nodo_bloque) {
    if (!nodo_bloque || nodo_bloque->tipo != NODO_BLOQUE) return;
    
    ASTNode* instruccion = nodo_bloque->hijo1;
    while (instruccion && instruccion->tipo != NODO_VACIO) {
        analizar_nodo(instruccion);
        instruccion = instruccion->siguiente;
    }
}

/* Analiza una declaración e inserta en la TS */
void analizar_declaracion(ASTNode* nodo) {
    if (nodo->tipo != NODO_DECLARACION) return;
    
    TipoDato tipo_base = nodo->hijo1->data.tipo_dato;
    
    // Recorremos la lista de declaradores (hijo2)
    ASTNode* decl = nodo->hijo2;
    while(decl && decl->tipo != NODO_VACIO) {
        char* nombre_id = decl->hijo1->data.cadena;
        
        // Insertar en la tabla de símbolos
        ts_insertar(nombre_id, tipo_base, decl->linea);
        
        // Si hay una inicialización (ej. int x = 5;)
        if (decl->hijo2->tipo != NODO_VACIO) {
            TipoDato tipo_expr = analizar_expresion(decl->hijo2);
            if (tipo_base != tipo_expr) {
                fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en inicializacion de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                        decl->linea, nombre_id, tipo_base, tipo_expr);
            }
        }
        
        decl = decl->siguiente;
    }
}


/* Analiza una expresión y devuelve su tipo */
TipoDato analizar_expresion(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return TIPO_DESCONOCIDO;

    switch (nodo->tipo) {
        case NODO_NUMERO:
            // Simplificado: asumimos que todos los números son FLOAT
            return TIPO_FLOAT; 
        case NODO_CADENA:
            return TIPO_STRING;
        case NODO_BOOLEANO:
            return TIPO_BOOLEAN;
            
        case NODO_IDENTIFICADOR:
        {
            char* nombre_id = nodo->data.cadena;
            Simbolo* sym = ts_buscar(nombre_id);
            if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Variable '%s' no declarada.\n", nodo->linea, nombre_id);
                return TIPO_DESCONOCIDO;
            }
            return sym->tipo;
        }
        
        case NODO_BINARIO_OP:
        {
            TipoDato tipo_izq = analizar_expresion(nodo->hijo1);
            TipoDato tipo_der = analizar_expresion(nodo->hijo2);
            char* op = nodo->data.op_unario;
            
            // Lógica de Operadores Lógicos y de Comparación
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
                if (tipo_izq != TIPO_BOOLEAN || tipo_der != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos booleanos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                strcmp(op, "<") == 0  || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0  || strcmp(op, ">=") == 0) {
                
                // Simplificado: permitimos comparar solo números
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            
            // Lógica de Operadores Aritméticos
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                
                // Simplificado: solo operamos números
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                // Promoción de tipo: si uno es float, el resultado es float
                if (tipo_izq == TIPO_FLOAT || tipo_der == TIPO_FLOAT) return TIPO_FLOAT;
                return TIPO_INT;
            }
            // TODO: Implementar % (MOD_TOKEN)
            
            return TIPO_DESCONOCIDO;
        }

        case NODO_UNARIO_OP:
            // TODO: Implementar chequeo de unarios (ej. '!' debe ser bool)
            return analizar_expresion(nodo->hijo1);
            
        case NODO_LLAMADA_FUNCION:
        {
            char* nombre_func = nodo->hijo1->data.cadena;
            Simbolo* sym = ts_buscar(nombre_func);
             if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Funcion '%s' no declarada.\n", nodo->linea, nombre_func);
                return TIPO_DESCONOCIDO;
            }
            // TODO: Chequear que el número y tipo de argumentos (nodo->hijo2)
            // coincida con la definición de la función en la TS.
            
            return sym->tipo; // Devuelve el tipo de retorno de la función
        }
            
        default:
            return TIPO_DESCONOCIDO;
    }
}