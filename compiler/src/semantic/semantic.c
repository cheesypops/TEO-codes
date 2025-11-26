#include "semantic.h"

/* Módulo de análisis semántico.
 * Gestiona la tabla de símbolos y recorre el AST para verificar tipos,
 * ámbitos y llamadas a funciones antes de la generación de código.
 */

/* Estructura para la Tabla de Símbolos (muy simplificada).
 * En un compilador real, normalmente se implementaría como tabla hash o árbol.
 */
#define MAX_SIMBOLOS 1024
Simbolo tabla_simbolos[MAX_SIMBOLOS];
int num_simbolos = 0;
int ambito_actual = 0; // 0 = ámbito global

/* Contador global de direcciones de memoria virtual usado por el generador de bytecode. */
int contador_direcciones = 0;

/* Prototipos de funciones internas de la tabla de símbolos. */
Simbolo* ts_buscar_en_ambito_actual(char* nombre);

/* ====================================================== */
/* FUNCIONES DE TABLA DE SÍMBOLOS                         */
/* ====================================================== */

void ts_entrar_ambito() {
    ambito_actual++;
    // printf("[Semántico] Entrando en ámbito %d.\n", ambito_actual);
}

void ts_salir_ambito() {
    /* Cambio importante orientado a la generación de código:
     * los símbolos ya no se eliminan al salir de un ámbito para que
     * el backend pueda seguir resolviendo direcciones de memoria.
     * La comprobación de visibilidad continúa siendo correcta porque
     * ts_buscar recorre desde el ámbito más interno al más externo.
     */
    // printf("[Semántico] Saliendo de ámbito %d.\n", ambito_actual);
    ambito_actual--;
}

Simbolo* ts_buscar(char* nombre) {
    // Busca de atrás hacia adelante (del ámbito más interno al más externo).
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

/* Busca un símbolo únicamente en el ámbito actual.
 * Se utiliza para detectar redeclaraciones en el mismo nivel.
 */
Simbolo* ts_buscar_en_ambito_actual(char* nombre) {
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (tabla_simbolos[i].ambito < ambito_actual) {
            // Hemos salido del ámbito actual
            break; 
        }
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

/* Inserta un símbolo en la tabla.
 * params indica la lista de parámetros (ASTNode*) si se trata de una función,
 * o NULL cuando se inserta una variable.
 */
int ts_insertar(char* nombre, TipoDato tipo, int linea, ASTNode* params) {
    if (num_simbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "Error semántico: la tabla de símbolos está llena.\n");
        return 0;
    }
    
    // Valida redeclaraciones en el mismo ámbito.
    Simbolo* existente = ts_buscar_en_ambito_actual(nombre);
    if (existente) {
        fprintf(stderr, "Error semántico (línea %d): redeclaración de '%s'. Declarado previamente en la línea %d.\n",
                linea, nombre, existente->linea);
        return 0; // Fallo al insertar
    }
    
    tabla_simbolos[num_simbolos].nombre = strdup(nombre);
    tabla_simbolos[num_simbolos].tipo = tipo;
    tabla_simbolos[num_simbolos].ambito = ambito_actual;
    tabla_simbolos[num_simbolos].linea = linea;
    tabla_simbolos[num_simbolos].parametros = params; // Guarda la lista de parámetros asociada.
    
    /* Asigna la siguiente dirección de memoria virtual disponible
     * y avanza el contador para futuras declaraciones.
     */
    tabla_simbolos[num_simbolos].direccion = contador_direcciones++;
    
    printf("[Semántico] Declarado '%s' (addr=%d, tipo=%d) en ámbito %d.\n", 
           nombre, tabla_simbolos[num_simbolos].direccion, tipo, ambito_actual);
    
    num_simbolos++;
    return 1;
}

/* Función de limpieza obsoleta para uso interno.
 * La liberación real de la tabla se delega en ts_liberar_total() al final del main.
 */
void ts_liberar() {
    // No hace nada intencionalmente ahora, para preservar símbolos para el codegen.
}

/* Libera la memoria asociada a los símbolos.
 * Se invoca una sola vez al final del compilador, después de la generación de código.
 */
void ts_liberar_total() {
    for (int i = 0; i < num_simbolos; i++) {
        if (tabla_simbolos[i].nombre) {
            free(tabla_simbolos[i].nombre);
        }
        // No se liberan 'parametros' aquí porque son nodos del AST;
        // se liberan de forma unificada mediante liberar_arbol() en el main.
    }
    num_simbolos = 0;
    contador_direcciones = 0;
}

/* Prototipos de las funciones que recorren el AST en las distintas fases. */
void recolectar_simbolos(ASTNode* nodo);
void analizar_cuerpos(ASTNode* nodo);
TipoDato analizar_expresion(ASTNode* nodo);
void analizar_declaracion(ASTNode* nodo);
void analizar_bloque(ASTNode* nodo);


/* ====================================================== */
/* FASE 1: RECOLECCIÓN DE SÍMBOLOS                        */
/* ====================================================== */

/* Recorre el AST recogiendo las declaraciones de alto nivel
 * (variables globales y funciones) para registrarlas en la tabla de símbolos.
 */
void recolectar_simbolos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            // Recorre las listas de declaraciones globales y definiciones de función.
            recolectar_simbolos(nodo->hijo1); // DeclaracionGlobal
            recolectar_simbolos(nodo->hijo3); // FuncionDef
            break;

        case NODO_DECLARACION:
            // Se trata de una declaración global gestionada por la lista.
            analizar_declaracion(nodo);
            break;
            
        case NODO_ASIGNACION:
            // Asignación global sin efectos semánticos adicionales en la fase 1.
            break;

        case NODO_FUNCION_DEF:
        {
            // Registra únicamente el encabezado de la función en la tabla.
            char* nombre_func = nodo->hijo2->data.cadena;
            TipoDato tipo_retorno = nodo->hijo1->data.tipo_dato;
            printf("[Fase 1] Registrando función '%s'.\n", nombre_func);
            
            /* Guarda en el símbolo la lista de parámetros declarados para la función. */
            ts_insertar(nombre_func, tipo_retorno, nodo->linea, nodo->hijo3);
            
            break;
        }

        default:
            break;
    }
    
    // Si pertenece a una lista de nodos, continúa con el siguiente elemento.
    if (nodo->tipo == NODO_DECLARACION || 
        nodo->tipo == NODO_ASIGNACION || 
        nodo->tipo == NODO_FUNCION_DEF) {
        recolectar_simbolos(nodo->siguiente);
    }
}


/* ====================================================== */
/* FASE 2: ANÁLISIS DE CUERPOS                            */
/* ====================================================== */

/* Recorre el AST analizando los cuerpos de setup y de las funciones,
 * comprobando ámbitos, tipos y uso correcto de variables y llamadas.
 */
void analizar_cuerpos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            analizar_cuerpos(nodo->hijo2); // SetupDef
            analizar_cuerpos(nodo->hijo3); // FuncionDef (lista)
            break;
            
        case NODO_SETUP:
            printf("[Fase 2] Analizando cuerpo de setup().\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2); // Bloque
            ts_salir_ambito();
            break;
            
        case NODO_FUNCION_DEF:
        {
            char* nombre_func = nodo->hijo2->data.cadena;
            printf("[Fase 2] Analizando cuerpo de la función '%s'.\n", nombre_func);

            // 1. Entrar en el nuevo ámbito correspondiente a la función.
            ts_entrar_ambito();

            // 2. Declarar parámetros en el nuevo ámbito (como variables locales).
            ASTNode* param = nodo->hijo3;
            while(param && param->tipo != NODO_VACIO) {
                TipoDato tipo_param = param->hijo1->data.tipo_dato;
                char* nombre_param = param->hijo2->data.cadena;
                
                // params es NULL porque los parámetros se tratan como variables, no como funciones.
                ts_insertar(nombre_param, tipo_param, param->linea, NULL);
                
                param = param->siguiente;
            }

            // 3. Analizar el bloque de la función.
            analizar_bloque(nodo->hijo4);

            // 4. Salir del ámbito de la función.
            ts_salir_ambito();
            
            // Continúa con la siguiente definición de función en la lista.
            analizar_cuerpos(nodo->siguiente);
            break;
        }
            
        case NODO_BLOQUE:
            analizar_bloque(nodo);
            break;
            
        case NODO_ASIGNACION:
            analizar_expresion(nodo);
            break;

        case NODO_DECLARACION:
            analizar_declaracion(nodo);
            break;
            
        case NODO_IF:
            printf("[Fase 2] Analizando sentencia IF.\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error semántico (línea %d): la condición del 'if' no es booleana.\n", nodo->linea);
            }
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);
            ts_salir_ambito();
            
            if (nodo->hijo3->tipo != NODO_VACIO) {
                ts_entrar_ambito();
                analizar_bloque(nodo->hijo3);
                ts_salir_ambito();
            }
            break;

        case NODO_WHILE:
            printf("[Fase 2] Analizando ciclo WHILE.\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error semántico (línea %d): la condición del 'while' no es booleana.\n", nodo->linea);
            }
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);
            ts_salir_ambito();
            break;
            
        case NODO_FOR:
            printf("[Fase 2] Analizando ciclo FOR.\n");
            ts_entrar_ambito();
            analizar_cuerpos(nodo->hijo1); // Init
            
            if (nodo->hijo2->tipo != NODO_VACIO) {
                if (analizar_expresion(nodo->hijo2) != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error semántico (línea %d): la condición del 'for' no es booleana.\n", nodo->hijo2->linea);
                }
            }
            analizar_cuerpos(nodo->hijo3); // Step
            analizar_bloque(nodo->hijo4); // Bloque
            ts_salir_ambito();
            break;

        case NODO_DO_WHILE:
            printf("[Fase 2] Analizando ciclo DO-WHILE.\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo1);
            ts_salir_ambito();
            
            if (analizar_expresion(nodo->hijo2) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error semántico (línea %d): la condición del 'do-while' no es booleana.\n", nodo->hijo2->linea);
            }
            break;
            
        default:
            // Expresiones sueltas, como llamadas a función sin asignación.
            if (nodo->tipo >= NODO_LLAMADA_FUNCION && nodo->tipo <= NODO_BOOLEANO) {
                analizar_expresion(nodo);
            }
            break;
    }
}

/* ====================================================== */
/* FUNCIONES AUXILIARES Y PUNTO DE ENTRADA                */
/* ====================================================== */

/* Función principal del analizador semántico.
 * Coordina las dos fases: recolección de símbolos y análisis de cuerpos.
 */
int analizar_semantica(ASTNode* raiz) {
    if (!raiz) {
        fprintf(stderr, "Error: el árbol AST está vacío.\n");
        return 0;
    }
    
    printf("\n--- Iniciando análisis semántico (fase 1: recolección de símbolos) ---\n");
    recolectar_simbolos(raiz);
    
    printf("\n--- Iniciando análisis semántico (fase 2: cuerpos) ---\n");
    analizar_cuerpos(raiz);
    
    printf("\n--- Fin del análisis semántico ---\n");
    
    /* No se llama a ts_liberar() en esta función.
     * La liberación global de la tabla se difiere hasta el final del main
     * mediante ts_liberar_total(), una vez generados todos los códigos.
     */
    
    return 1;
}

/* Analiza un bloque de instrucciones recorrido como lista lineal. */
void analizar_bloque(ASTNode* nodo_bloque) {
    if (!nodo_bloque || nodo_bloque->tipo != NODO_BLOQUE) return;
    
    ASTNode* instruccion = nodo_bloque->hijo1;
    while (instruccion && instruccion->tipo != NODO_VACIO) {
        analizar_cuerpos(instruccion); 
        instruccion = instruccion->siguiente;
    }
}

/* Analiza una declaración y registra sus identificadores en la tabla de símbolos. */
void analizar_declaracion(ASTNode* nodo) {
    if (nodo->tipo != NODO_DECLARACION) return;
    
    TipoDato tipo_base = nodo->hijo1->data.tipo_dato;
    
    // Recorre la lista de declaradores asociada a la sentencia.
    ASTNode* decl = nodo->hijo2;
    while(decl && decl->tipo != NODO_VACIO) {
        char* nombre_id = decl->hijo1->data.cadena;
        
        // Inserta la variable actual en la tabla de símbolos.
        ts_insertar(nombre_id, tipo_base, decl->linea, NULL);
        
        // Si existe inicialización (por ejemplo, int x = 5;), se valida su tipo.
        if (decl->hijo2->tipo != NODO_VACIO) {
            TipoDato tipo_expr = analizar_expresion(decl->hijo2);
            
            if (tipo_base != tipo_expr) {
                // Permite asignación implícita de INT a FLOAT.
                if (tipo_base == TIPO_FLOAT && tipo_expr == TIPO_INT) {
                    // Es válido (ej. float f = 5;)
                } else {
                    fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en inicializacion de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                            decl->linea, nombre_id, tipo_base, tipo_expr);
                }
            }
        }
        
        decl = decl->siguiente;
    }
}


/* Analiza una expresión y devuelve el tipo estático resultante. */
TipoDato analizar_expresion(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return TIPO_DESCONOCIDO;

    switch (nodo->tipo) {
        case NODO_NUMERO:
        {
            // Si el número no tiene parte decimal, se trata como TIPO_INT.
            double val = nodo->data.valor_num;
            if (val == (long)val) {
                return TIPO_INT;
            }
            return TIPO_FLOAT;
        } 
        case NODO_CADENA:
            return TIPO_STRING;
        case NODO_BOOLEANO:
            return TIPO_BOOLEAN;

        case NODO_ASIGNACION:
        {
            // Valida que el lado izquierdo (hijo1) sea un l-value.
            ASTNode* lado_izq = nodo->hijo1;
            
            if (lado_izq->tipo != NODO_IDENTIFICADOR) {
                fprintf(stderr, "Error Semantico (linea %d): El lado izquierdo de la asignacion no es una variable.\n", nodo->linea);
                return TIPO_DESCONOCIDO;
            }
            
            // Obtiene el símbolo asociado a la variable del lado izquierdo.
            char* nombre_id = lado_izq->data.cadena;
            Simbolo* sym = ts_buscar(nombre_id);
            if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Variable '%s' no declarada.\n", lado_izq->linea, nombre_id);
                return TIPO_DESCONOCIDO;
            }
            
            // Analiza el lado derecho de la asignación.
            TipoDato tipo_var = sym->tipo;
            TipoDato tipo_expr = analizar_expresion(nodo->hijo2);
            
            // Comprueba compatibilidad de tipos entre variable y expresión.
            if (tipo_var != tipo_expr) {
                 if (tipo_var == TIPO_FLOAT && tipo_expr == TIPO_INT) {
                    // Conversión implícita válida (por ejemplo, f = 5;).
                } else {
                    fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en asignacion a '%s'. Se esperaba %d pero se obtuvo %d.\n",
                            nodo->linea, nombre_id, tipo_var, tipo_expr);
                }
            }
            
            return tipo_var;
        }
            
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
            
            // Operadores lógicos.
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
                if (tipo_izq != TIPO_BOOLEAN || tipo_der != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos booleanos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            // Operadores de comparación.
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                strcmp(op, "<") == 0  || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0  || strcmp(op, ">=") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            
            // Operadores aritméticos.
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                if (tipo_izq == TIPO_FLOAT || tipo_der == TIPO_FLOAT) return TIPO_FLOAT;
                return TIPO_INT;
            }

            // Operador módulo.
            if (strcmp(op, "%") == 0) {
                if (tipo_izq != TIPO_INT || tipo_der != TIPO_INT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%%' requiere operandos enteros.\n", nodo->linea);
                }
                return TIPO_INT;
            }

            // Acceso simplificado a arrays (implementación temporal).
            if (strcmp(op, "[]") == 0) {
                // Comprueba que el índice sea entero.
                TipoDato tipo_indice = analizar_expresion(nodo->hijo2);
                if (tipo_indice != TIPO_INT) {
                     fprintf(stderr, "Error Semantico (linea %d): El indice de acceso a array debe ser un entero (TIPO_INT).\n", nodo->linea);
                }
                // Se asume que el tipo del acceso coincide con el tipo base de la variable izquierda.
                return tipo_izq; 
            }
            
            return TIPO_DESCONOCIDO;
        }

        case NODO_UNARIO_OP:
        {
            char* op = nodo->data.op_unario;
            TipoDato tipo_hijo = analizar_expresion(nodo->hijo1);
            
            if (strcmp(op, "!") == 0) {
                if (tipo_hijo != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '!' requiere un operando booleano.\n", nodo->linea);
                }
                return TIPO_BOOLEAN;
            }
            
            if (strcmp(op, "-") == 0 || strcmp(op, "+") == 0) {
                if (tipo_hijo != TIPO_INT && tipo_hijo != TIPO_FLOAT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador unario '%s' requiere un operando numerico.\n", nodo->linea, op);
                }
                return tipo_hijo; 
            }
            
            if (strcmp(op, "++") == 0 || strcmp(op, "--") == 0) {
                // Valida que el operando sea un l-value.
                if (nodo->hijo1->tipo != NODO_IDENTIFICADOR) {
                    fprintf(stderr, "Error Semantico (linea %d): El operador prefijo '%s' requiere un l-value (variable modificable).\n", nodo->linea, op);
                }

                if (tipo_hijo != TIPO_INT && tipo_hijo != TIPO_FLOAT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere un operando numerico.\n", nodo->linea, op);
                }
                return tipo_hijo;
            }
            
            return TIPO_DESCONOCIDO;
        }

        case NODO_POSTFIX_OP:
        {
            char* op = nodo->data.op_unario;
            TipoDato tipo_hijo = analizar_expresion(nodo->hijo1);
            
            // Valida que el operando sea un l-value.
            if (nodo->hijo1->tipo != NODO_IDENTIFICADOR) {
                fprintf(stderr, "Error Semantico (linea %d): El operador postfijo '%s' requiere un l-value (variable modificable).\n", nodo->linea, op);
            }

            if (tipo_hijo != TIPO_INT && tipo_hijo != TIPO_FLOAT) {
                fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere un operando numerico.\n", nodo->linea, op);
            }
            return tipo_hijo; 
        }
            
        case NODO_LLAMADA_FUNCION:
        {
            ASTNode* callee = nodo->hijo1;
            
            // Caso 1: funciones declaradas por el usuario.
            if (callee->tipo == NODO_IDENTIFICADOR) {
                char* nombre_func = callee->data.cadena;
                Simbolo* sym = ts_buscar(nombre_func);
                if (!sym) {
                    fprintf(stderr, "Error Semantico (linea %d): Funcion '%s' no declarada.\n", nodo->linea, nombre_func);
                    return TIPO_DESCONOCIDO;
                }

                // Valida los argumentos reales frente a la lista de parámetros formales.
                ASTNode* param = sym->parametros; // Lista de parámetros esperados.
                ASTNode* arg = nodo->hijo2;       // Lista de argumentos proporcionados.
                int arg_count = 0;

                while ((param && param->tipo != NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                    arg_count++;
                    
                    // param es un NODO_DECLARACION (hijo1 = tipo, hijo2 = identificador).
                    TipoDato tipo_param = param->hijo1->data.tipo_dato;
                    TipoDato tipo_arg = analizar_expresion(arg);

                    if (tipo_param != tipo_arg) {
                        if (tipo_param == TIPO_FLOAT && tipo_arg == TIPO_INT) {
                           // Conversión implícita válida de INT a FLOAT.
                        } else {
                            fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en argumento %d de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                                    arg->linea, arg_count, nombre_func, tipo_param, tipo_arg);
                        }
                    }

                    param = param->siguiente;
                    arg = arg->siguiente;
                }

                // Valida el número total de argumentos recibidos.
                if ((param && param->tipo != NODO_VACIO) && (!arg || arg->tipo == NODO_VACIO)) {
                    fprintf(stderr, "Error semántico (línea %d): no hay suficientes argumentos para la función '%s'.\n", nodo->linea, nombre_func);
                } else if ((!param || param->tipo == NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                     fprintf(stderr, "Error semántico (línea %d): hay demasiados argumentos para la función '%s'.\n", nodo->linea, nombre_func);
                }
                
                return sym->tipo; // Devuelve el tipo de retorno
            
            // Caso 2: funciones reservadas del sistema.
            } else if (callee->tipo == NODO_FUNCION_RESERVADA) {
                char* nombre_func = callee->data.cadena;
                printf("[Fase 2] Analizando llamada a función reservada '%s'.\n", nombre_func);
                
                if (strcmp(nombre_func, "parar") == 0 ||
                    strcmp(nombre_func, "mover") == 0 ||
                    strcmp(nombre_func, "girarIzq") == 0 ||
                    strcmp(nombre_func, "girarDer") == 0 ||
                    strcmp(nombre_func, "reversa") == 0) 
                {
                    if (nodo->hijo2->tipo != NODO_VACIO) {
                         fprintf(stderr, "Error semántico (línea %d): '%s()' no admite argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_VOID;
                }

                if (strcmp(nombre_func, "esperar") == 0) {
                     // Verifica que se haya proporcionado al menos un argumento.
                     if (nodo->hijo2->tipo == NODO_VACIO) {
                         fprintf(stderr, "Error semántico (línea %d): 'esperar()' requiere un argumento con la cantidad de milisegundos.\n", nodo->linea);
                         return TIPO_VOID;
                     }
                     
                     // Verifica el tipo del argumento principal.
                     TipoDato tipo_arg = analizar_expresion(nodo->hijo2); // Primer argumento.
                     if (tipo_arg != TIPO_INT && tipo_arg != TIPO_FLOAT) {
                         fprintf(stderr, "Error semántico (línea %d): 'esperar()' requiere un valor numérico entero (milisegundos).\n", nodo->linea);
                     }
                     
                     return TIPO_VOID; // No devuelve valor
                }
                
                if (strcmp(nombre_func, "leerSensor") == 0) {
                     if (nodo->hijo2->tipo != NODO_VACIO) {
                         fprintf(stderr, "Error semántico (línea %d): '%s()' no admite argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_INT;
                }
                
                return TIPO_DESCONOCIDO;
            }
            
            fprintf(stderr, "Error semántico (línea %d): no se puede invocar este tipo de expresión.\n", nodo->linea);
            return TIPO_DESCONOCIDO;
        }
            
        default:
            return TIPO_DESCONOCIDO;
    }
}