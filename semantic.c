#include "semantic.h"

/* Estructura para la Tabla de Símbolos (muy simplificada) */
/* En un compilador real, esto sería un hash map o un árbol */
#define MAX_SIMBOLOS 1024
Simbolo tabla_simbolos[MAX_SIMBOLOS];
int num_simbolos = 0;
int ambito_actual = 0; // 0 = Global

/* --- Prototipos Internos --- */
Simbolo* ts_buscar_en_ambito_actual(char* nombre);

/* --- Funciones de la Tabla de Símbolos --- */

void ts_entrar_ambito() {
    ambito_actual++;
    printf("[Semantico: Entrando en ambito %d]\n", ambito_actual);
}

void ts_salir_ambito() {
    printf("[Semantico: Saliendo de ambito %d]\n", ambito_actual);

    // Limpieza real de la tabla de símbolos
    while (num_simbolos > 0 && tabla_simbolos[num_simbolos - 1].ambito == ambito_actual) {
        
        num_simbolos--; // "Borra" el símbolo del conteo
        
        printf("[Semantico: Liberando '%s' (ambito %d)]\n", 
               tabla_simbolos[num_simbolos].nombre, 
               ambito_actual);
        
        // Libera la memoria del string duplicado
        free(tabla_simbolos[num_simbolos].nombre); 
        tabla_simbolos[num_simbolos].nombre = NULL;
    }
    
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

/**
 * NUEVA FUNCIÓN:
 * Busca un símbolo ÚNICAMENTE en el ámbito actual.
 * Es usada para detectar redeclaraciones.
 */
Simbolo* ts_buscar_en_ambito_actual(char* nombre) {
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (tabla_simbolos[i].ambito < ambito_actual) {
            break; 
        }
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

/*
 * CAMBIO: La firma de la función ahora acepta un ASTNode*
 * para la lista de parámetros (será NULL si no es una función).
 */
int ts_insertar(char* nombre, TipoDato tipo, int linea, ASTNode* params) {
    if (num_simbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "Error Semantico: Tabla de simbolos llena.\n");
        return 0;
    }
    
    Simbolo* existente = ts_buscar_en_ambito_actual(nombre);
    if (existente) {
        fprintf(stderr, "Error Semantico (linea %d): Redeclaracion de '%s'. Previamente declarado en linea %d.\n",
                linea, nombre, existente->linea);
        return 0; // Fallo al insertar
    }
    
    tabla_simbolos[num_simbolos].nombre = strdup(nombre);
    tabla_simbolos[num_simbolos].tipo = tipo;
    tabla_simbolos[num_simbolos].ambito = ambito_actual;
    tabla_simbolos[num_simbolos].linea = linea;
    tabla_simbolos[num_simbolos].parametros = params; // Guardar la lista de parámetros
    
    printf("[Semantico: Declarado '%s' (tipo %d) en ambito %d]\n", nombre, tipo, ambito_actual);
    
    num_simbolos++;
    return 1;
}

void ts_liberar() {
    for (int i = 0; i < num_simbolos; i++) {
        // CAMBIO: Añadida comprobación de NULL
        if (tabla_simbolos[i].nombre) {
            free(tabla_simbolos[i].nombre);
        }
    }
    num_simbolos = 0;
}

/* --- Prototipos de los Recorredores (Walkers) --- */
void recolectar_simbolos(ASTNode* nodo);
void analizar_cuerpos(ASTNode* nodo);
TipoDato analizar_expresion(ASTNode* nodo);
void analizar_declaracion(ASTNode* nodo);
void analizar_bloque(ASTNode* nodo);


/* ====================================================== */
/* FASE 1: RECOLECCIÓN DE SÍMBOLOS                        */
/* ====================================================== */

void recolectar_simbolos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            recolectar_simbolos(nodo->hijo1); // DeclaracionGlobal
            recolectar_simbolos(nodo->hijo3); // FuncionDef
            break;

        case NODO_DECLARACION:
            analizar_declaracion(nodo);
            break;
            
        case NODO_ASIGNACION:
            break;

        case NODO_FUNCION_DEF:
        {
            char* nombre_func = nodo->hijo2->data.cadena;
            TipoDato tipo_retorno = nodo->hijo1->data.tipo_dato;
            printf("[Fase 1: Registrando funcion '%s']\n", nombre_func);
            
            /*
             * CAMBIO: Ahora pasamos nodo->hijo3 (la lista de parámetros)
             * a ts_insertar para que se guarde en el símbolo.
             */
            ts_insertar(nombre_func, tipo_retorno, nodo->linea, nodo->hijo3);
            
            break;
        }

        default:
            break;
    }
    
    // Si es una lista, seguir al siguiente
    if (nodo->tipo == NODO_DECLARACION || 
        nodo->tipo == NODO_ASIGNACION || 
        nodo->tipo == NODO_FUNCION_DEF) {
        recolectar_simbolos(nodo->siguiente);
    }
}


/* ====================================================== */
/* FASE 2: ANÁLISIS DE CUERPOS                            */
/* ====================================================== */

void analizar_cuerpos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            analizar_cuerpos(nodo->hijo2); // SetupDef
            analizar_cuerpos(nodo->hijo3); // FuncionDef (lista)
            break;
            
        case NODO_SETUP:
            printf("[Fase 2: Analizando setup()]\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2); // Bloque
            ts_salir_ambito();
            break;
            
        case NODO_FUNCION_DEF:
        {
            char* nombre_func = nodo->hijo2->data.cadena;
            printf("[Fase 2: Analizando cuerpo de funcion '%s']\n", nombre_func);
            ts_entrar_ambito();

            ASTNode* param = nodo->hijo3;
            while(param && param->tipo != NODO_VACIO) {
                TipoDato tipo_param = param->hijo1->data.tipo_dato;
                char* nombre_param = param->hijo2->data.cadena;
                
                /* CAMBIO: Pasamos NULL como 4to argumento, ya que esto NO es una función */
                ts_insertar(nombre_param, tipo_param, param->linea, NULL);
                
                param = param->siguiente;
            }

            analizar_bloque(nodo->hijo4);
            ts_salir_ambito();
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
            printf("[Fase 2: Analizando IF]\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'if' no es booleana.\n", nodo->linea);
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
            printf("[Fase 2: Analizando WHILE]\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'while' no es booleana.\n", nodo->linea);
            }
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);
            ts_salir_ambito();
            break;
            
        case NODO_FOR:
            printf("[Fase 2: Analizando FOR]\n");
            ts_entrar_ambito();
            analizar_cuerpos(nodo->hijo1); // Init
            
            if (nodo->hijo2->tipo != NODO_VACIO) {
                if (analizar_expresion(nodo->hijo2) != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): La condicion del 'for' no es booleana.\n", nodo->hijo2->linea);
                }
            }
            analizar_cuerpos(nodo->hijo3); // Step
            analizar_bloque(nodo->hijo4); // Bloque
            ts_salir_ambito();
            break;

        case NODO_DO_WHILE:
            printf("[Fase 2: Analizando DO-WHILE]\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo1);
            ts_salir_ambito();
            
            if (analizar_expresion(nodo->hijo2) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'do-while' no es booleana.\n", nodo->hijo2->linea);
            }
            break;
            
        default:
            if (nodo->tipo >= NODO_LLAMADA_FUNCION && nodo->tipo <= NODO_BOOLEANO) {
                analizar_expresion(nodo);
            }
            break;
    }
}

/* ====================================================== */
/* FUNCIONES AUXILIARES (COMPARTIDAS)                     */
/* ====================================================== */

int analizar_semantica(ASTNode* raiz) {
    if (!raiz) {
        fprintf(stderr, "Error: El arbol AST esta vacio.\n");
        return 0;
    }
    
    printf("\n--- Iniciando Analisis Semantico (FASE 1: Recoleccion) ---\n");
    recolectar_simbolos(raiz);
    
    printf("\n--- Iniciando Analisis Semantico (FASE 2: Cuerpos) ---\n");
    analizar_cuerpos(raiz);
    
    printf("\n--- Fin del Analisis Semantico ---\n");
    
    ts_liberar();
    return 1;
}

void analizar_bloque(ASTNode* nodo_bloque) {
    if (!nodo_bloque || nodo_bloque->tipo != NODO_BLOQUE) return;
    
    ASTNode* instruccion = nodo_bloque->hijo1;
    while (instruccion && instruccion->tipo != NODO_VACIO) {
        analizar_cuerpos(instruccion);
        instruccion = instruccion->siguiente;
    }
}

void analizar_declaracion(ASTNode* nodo) {
    if (nodo->tipo != NODO_DECLARACION) return;
    
    TipoDato tipo_base = nodo->hijo1->data.tipo_dato;
    
    ASTNode* decl = nodo->hijo2;
    while(decl && decl->tipo != NODO_VACIO) {
        char* nombre_id = decl->hijo1->data.cadena;
        
        /* CAMBIO: Pasamos NULL como 4to argumento, ya que esto NO es una función */
        ts_insertar(nombre_id, tipo_base, decl->linea, NULL);
        
        if (decl->hijo2->tipo != NODO_VACIO) {
            TipoDato tipo_expr = analizar_expresion(decl->hijo2);
            
            if (tipo_base != tipo_expr) {
                // Permitir asignación implícita de INT a FLOAT
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


TipoDato analizar_expresion(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return TIPO_DESCONOCIDO;

    switch (nodo->tipo) {
        case NODO_NUMERO:
        {
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
            ASTNode* lado_izq = nodo->hijo1;
            
            if (lado_izq->tipo != NODO_IDENTIFICADOR) {
                fprintf(stderr, "Error Semantico (linea %d): El lado izquierdo de la asignacion no es una variable.\n", nodo->linea);
                return TIPO_DESCONOCIDO;
            }

            
            char* nombre_id = lado_izq->data.cadena;
            Simbolo* sym = ts_buscar(nombre_id);
            if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Variable '%s' no declarada.\n", lado_izq->linea, nombre_id);
                return TIPO_DESCONOCIDO;
            }
            
            TipoDato tipo_var = sym->tipo;
            TipoDato tipo_expr = analizar_expresion(nodo->hijo2);
            
            if (tipo_var != tipo_expr) {
                 if (tipo_var == TIPO_FLOAT && tipo_expr == TIPO_INT) {
                    // Válido (ej. f = 5;)
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
            
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
                if (tipo_izq != TIPO_BOOLEAN || tipo_der != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos booleanos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                strcmp(op, "<") == 0  || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0  || strcmp(op, ">=") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                if (tipo_izq == TIPO_FLOAT || tipo_der == TIPO_FLOAT) return TIPO_FLOAT;
                return TIPO_INT;
            }

            if (strcmp(op, "%") == 0) {
                if (tipo_izq != TIPO_INT || tipo_der != TIPO_INT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%%' requiere operandos enteros.\n", nodo->linea);
                }
                return TIPO_INT;
            }

            
            if (strcmp(op, "[]") == 0) {
                // TODO: Implementar lógica de tipo array
                // Por ahora, solo analizamos el índice para encontrar errores
                TipoDato tipo_indice = analizar_expresion(nodo->hijo2);
                if (tipo_indice != TIPO_INT) {
                     fprintf(stderr, "Error Semantico (linea %d): El indice de acceso a array debe ser un entero (TIPO_INT).\n", nodo->linea);
                }
                // HACK: Asumimos que el tipo del acceso es el tipo base
                // (esto es incorrecto, pero es lo mejor que podemos hacer)
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
            
            if (callee->tipo == NODO_IDENTIFICADOR) {
                char* nombre_func = callee->data.cadena;
                Simbolo* sym = ts_buscar(nombre_func);
                if (!sym) {
                    fprintf(stderr, "Error Semantico (linea %d): Funcion '%s' no declarada.\n", nodo->linea, nombre_func);
                    return TIPO_DESCONOCIDO;
                }

                ASTNode* param = sym->parametros; // Lista de parámetros esperados (de la TS)
                ASTNode* arg = nodo->hijo2;    // Lista de argumentos dados (del AST)
                int arg_count = 0;

                while ((param && param->tipo != NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                    arg_count++;
                    
                    // param es un NODO_DECLARACION (hijo1=Tipo, hijo2=ID)
                    TipoDato tipo_param = param->hijo1->data.tipo_dato;
                    TipoDato tipo_arg = analizar_expresion(arg);

                    if (tipo_param != tipo_arg) {
                        // Permitir INT -> FLOAT
                        if (tipo_param == TIPO_FLOAT && tipo_arg == TIPO_INT) {
                           // Válido
                        } else {
                            fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en argumento %d de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                                    arg->linea, arg_count, nombre_func, tipo_param, tipo_arg);
                        }
                    }

                    param = param->siguiente;
                    arg = arg->siguiente;
                }

                // Comprobar desajuste de cantidad de argumentos
                if ((param && param->tipo != NODO_VACIO) && (!arg || arg->tipo == NODO_VACIO)) {
                    fprintf(stderr, "Error Semantico (linea %d): No hay suficientes argumentos para la funcion '%s'.\n", nodo->linea, nombre_func);
                } else if ((!param || param->tipo == NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                     fprintf(stderr, "Error Semantico (linea %d): Demasiados argumentos para la funcion '%s'.\n", nodo->linea, nombre_func);
                }
                
                return sym->tipo; // Devuelve el tipo de retorno de la función
            
            } else if (callee->tipo == NODO_FUNCION_RESERVADA) {
                char* nombre_func = callee->data.cadena;
                printf("[Fase 2: Analizando llamada a func reservada '%s']\n", nombre_func);
                
                if (strcmp(nombre_func, "parar") == 0 ||
                    strcmp(nombre_func, "mover") == 0 ||
                    strcmp(nombre_func, "girarIzq") == 0 ||
                    strcmp(nombre_func, "girarDer") == 0 ||
                    strcmp(nombre_func, "reversa") == 0) 
                {
                    if (nodo->hijo2->tipo != NODO_VACIO) {
                         fprintf(stderr, "Error Semantico (linea %d): '%s()' no toma argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_VOID;
                }
                
                if (strcmp(nombre_func, "leerSensor") == 0) {
                     if (nodo->hijo2->tipo != NODO_VACIO) {
                         fprintf(stderr, "Error Semantico (linea %d): '%s()' no toma argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_INT;
                }
                
                return TIPO_DESCONOCIDO;
            }
            
            fprintf(stderr, "Error Semantico (linea %d): No se puede llamar a este tipo de expresion.\n", nodo->linea);
            return TIPO_DESCONOCIDO;
        }
            
        default:
            return TIPO_DESCONOCIDO;
    }
}