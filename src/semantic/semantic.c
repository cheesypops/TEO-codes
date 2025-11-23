/*
 * Implementación del Analizador Semántico.
 * 
 * Este módulo realiza el análisis semántico del código fuente mediante dos fases:
 * 
 * FASE 1 - Recolección de Símbolos:
 *   Recorre el AST para registrar todas las declaraciones globales y firmas de funciones.
 *   Esto permite detectar funciones antes de su uso y manejar referencias hacia adelante.
 * 
 * FASE 2 - Análisis de Cuerpos:
 *   Verifica tipos, valida uso de símbolos, gestiona ámbitos (scopes) y realiza
 *   comprobaciones semánticas sobre las expresiones e instrucciones.
 * 
 * GESTIÓN DE ÁMBITOS:
 *   Utiliza un sistema de ámbitos anidados donde cada bloque y función crea un nuevo
 *   ámbito. Los símbolos se buscan desde el ámbito más interno hacia el global (shadowing).
 */

#include "semantic.h"

/* ========== Tabla de Símbolos ========== */

/**
 * Tabla de símbolos basada en arreglo.
 * 
 * NOTA: Para producción, sería mejor usar una tabla hash o estructura más eficiente.
 * La implementación actual es simple pero suficiente para el alcance del proyecto.
 * 
 * ESTRATEGIA DE ÁMBITOS:
 *   Los símbolos se almacenan en orden de inserción. Al salir de un ámbito,
 *   se eliminan todos los símbolos de ese nivel (que están al final del arreglo).
 */
#define MAX_SIMBOLOS 1024
Simbolo tabla_simbolos[MAX_SIMBOLOS];
int num_simbolos = 0;
int ambito_actual = 0;  /* Nivel de ámbito actual (0 = ámbito global) */

/* Índice incremental para mapear variables (y parámetros) al arreglo vars[] de la VM. */
static int siguiente_indice_vm = 0;

Simbolo* ts_buscar_en_ambito_actual(char* nombre);

/* ========== Operaciones sobre la Tabla de Símbolos ========== */

/**
 * Entra en un nuevo ámbito.
 * 
 * Se llama al entrar en bloques, funciones, estructuras de control, etc.
 * Incrementa el contador de ámbito para que los nuevos símbolos se asocien
 * al nivel correcto.
 */
void ts_entrar_ambito() {
    ambito_actual++;
    printf("[Semantico: Entrando en ambito %d]\n", ambito_actual);
}

/**
 * Sale del ámbito actual.
 * 
 * Elimina todos los símbolos del ámbito actual (que están al final del arreglo
 * debido al orden de inserción) y decrementa el contador de ámbito.
 * 
 * IMPORTANTE: Esta función libera la memoria de los nombres de los símbolos
 * que se eliminan, ya que fueron asignados con strdup().
 */
void ts_salir_ambito() {
    printf("[Semantico: Saliendo de ambito %d]\n", ambito_actual);

    /* Eliminar todos los símbolos del ámbito actual (están al final del arreglo) */
    while (num_simbolos > 0 && tabla_simbolos[num_simbolos - 1].ambito == ambito_actual) {
        num_simbolos--;
        printf("[Semantico: Liberando '%s' (ambito %d)]\n", 
               tabla_simbolos[num_simbolos].nombre, 
               ambito_actual);
        free(tabla_simbolos[num_simbolos].nombre); 
        tabla_simbolos[num_simbolos].nombre = NULL;
    }
    
    ambito_actual--;
}

/**
 * Busca un símbolo en la tabla desde el ámbito más interno hacia el global.
 * 
 * Esta función implementa el shadowing (ocultamiento): si una variable local
 * tiene el mismo nombre que una global, se retorna la local.
 * 
 * @param nombre Nombre del símbolo a buscar
 * @return Puntero al símbolo encontrado, o NULL si no existe
 */
Simbolo* ts_buscar(char* nombre) {
    /* Buscar desde el final (ámbito más reciente) hacia el inicio */
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

/**
 * Busca un símbolo solo en el ámbito actual.
 * 
 * Se utiliza para detectar redeclaraciones: no se permite declarar dos veces
 * la misma variable en el mismo ámbito, pero sí se permite shadowing entre ámbitos.
 * 
 * @param nombre Nombre del símbolo a buscar
 * @return Puntero al símbolo encontrado en el ámbito actual, o NULL
 */
Simbolo* ts_buscar_en_ambito_actual(char* nombre) {
    /* Buscar solo en el ámbito actual (símbolos con ambito == ambito_actual) */
    for (int i = num_simbolos - 1; i >= 0; i--) {
        if (tabla_simbolos[i].ambito < ambito_actual) {
            /* Ya salimos del ámbito actual, no hay más símbolos aquí */
            break; 
        }
        if (strcmp(tabla_simbolos[i].nombre, nombre) == 0) {
            return &tabla_simbolos[i];
        }
    }
    return NULL;
}

/**
 * Inserta un símbolo en la tabla de símbolos.
 * 
 * IMPORTANTE: Para funciones, el parámetro 'params' debe contener el AST de
 * la lista de parámetros formales. Para variables, debe ser NULL.
 * 
 * @param nombre Nombre del símbolo (se duplica con strdup)
 * @param tipo Tipo de dato del símbolo
 * @param linea Línea de declaración (para reporte de errores)
 * @param params AST de parámetros (solo para funciones, NULL para variables)
 * @return 1 si se insertó correctamente, 0 si hay error (redeclaración o tabla llena)
 */
int ts_insertar(char* nombre, TipoDato tipo, int linea, ASTNode* params) {
    /* Verificar que hay espacio en la tabla */
    if (num_simbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "Error Semantico: Tabla de simbolos llena.\n");
        return 0;
    }
    
    /* Verificar que no hay redeclaración en el mismo ámbito */
    Simbolo* existente = ts_buscar_en_ambito_actual(nombre);
    if (existente) {
        fprintf(stderr, "Error Semantico (linea %d): Redeclaracion de '%s'. Previamente declarado en linea %d.\n",
                linea, nombre, existente->linea);
        return 0;
    }
    
    /* Insertar el nuevo símbolo */
    tabla_simbolos[num_simbolos].nombre = strdup(nombre);
    tabla_simbolos[num_simbolos].tipo = tipo;
    tabla_simbolos[num_simbolos].ambito = ambito_actual;
    tabla_simbolos[num_simbolos].linea = linea;
    tabla_simbolos[num_simbolos].parametros = params;
    /* Asignar índice de VM solo a variables/parámetros (no a funciones). */
    if (params == NULL) {
        tabla_simbolos[num_simbolos].indice_vm = siguiente_indice_vm++;
    } else {
        tabla_simbolos[num_simbolos].indice_vm = -1;
    }
    
    printf("[Semantico: Declarado '%s' (tipo %d) en ambito %d]\n", nombre, tipo, ambito_actual);
    
    num_simbolos++;
    return 1;
}

/**
 * Libera toda la memoria asociada a la tabla de símbolos.
 * 
 * IMPORTANTE: Solo libera los nombres de los símbolos (asignados con strdup).
 * Los nodos AST de parámetros NO se liberan aquí, ya que pertenecen al AST principal.
 */
void ts_liberar() {
    for (int i = 0; i < num_simbolos; i++) {
        /* Verificar NULL antes de liberar (por seguridad) */
        if (tabla_simbolos[i].nombre) {
            free(tabla_simbolos[i].nombre);
        }
    }
    num_simbolos = 0;
    siguiente_indice_vm = 0;
}

/* ========== Prototipos de Funciones Recorredoras ========== */
void recolectar_simbolos(ASTNode* nodo);
void analizar_cuerpos(ASTNode* nodo);
TipoDato analizar_expresion(ASTNode* nodo);
void analizar_declaracion(ASTNode* nodo);
void analizar_bloque(ASTNode* nodo);

/* ========== FASE 1: Recolección de Símbolos ========== */

/**
 * Fase 1: Recolección de símbolos.
 * 
 * Recorre el AST para registrar todas las declaraciones globales y firmas de funciones.
 * Esta fase permite:
 * - Detectar funciones antes de su uso (referencias hacia adelante)
 * - Validar que no hay redeclaraciones globales
 * - Preparar la tabla de símbolos para la fase 2
 * 
 * IMPORTANTE: En esta fase NO se analizan los cuerpos de las funciones, solo
 * se registran sus firmas (nombre, tipo de retorno, parámetros).
 */
void recolectar_simbolos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            /* Recorrer declaraciones globales y definiciones de funciones */
            recolectar_simbolos(nodo->hijo1);  /* DeclaracionGlobal */
            recolectar_simbolos(nodo->hijo3);   /* FuncionDef */
            break;

        case NODO_DECLARACION:
            /* Registrar variables declaradas globalmente */
            analizar_declaracion(nodo);
            break;
            
        case NODO_ASIGNACION:
            /* Las asignaciones no declaran símbolos, se ignoran en esta fase */
            break;

        case NODO_FUNCION_DEF:
        {
            /* Registrar la firma de la función (nombre, tipo retorno, parámetros) */
            char* nombre_func = nodo->hijo2->data.cadena;
            TipoDato tipo_retorno = nodo->hijo1->data.tipo_dato;
            printf("[Fase 1: Registrando funcion '%s']\n", nombre_func);
            
            /* Guardar la lista de parámetros en el símbolo para validación posterior */
            ts_insertar(nombre_func, tipo_retorno, nodo->linea, nodo->hijo3);
            
            break;
        }

        default:
            break;
    }
    
    /* Si es una lista enlazada, continuar con el siguiente elemento */
    if (nodo->tipo == NODO_DECLARACION || 
        nodo->tipo == NODO_ASIGNACION || 
        nodo->tipo == NODO_FUNCION_DEF) {
        recolectar_simbolos(nodo->siguiente);
    }
}

/* ========== FASE 2: Análisis de Cuerpos ========== */

/**
 * Fase 2: Análisis de cuerpos.
 * 
 * Recorre el AST para realizar verificaciones semánticas sobre las expresiones
 * e instrucciones:
 * - Verificación de tipos en expresiones y asignaciones
 * - Validación de uso de símbolos (variables y funciones deben estar declaradas)
 * - Gestión de ámbitos (entrar/salir de bloques)
 * - Validación de argumentos en llamadas a funciones
 * - Verificación de tipos en estructuras de control
 * 
 * IMPORTANTE: Esta fase asume que la fase 1 ya registró todos los símbolos.
 */
void analizar_cuerpos(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            analizar_cuerpos(nodo->hijo2); // SetupDef
            analizar_cuerpos(nodo->hijo3); // FuncionDef (lista)
            break;
            
        case NODO_SETUP:
            /* Analizar el cuerpo de setup() en un nuevo ámbito */
            printf("[Fase 2: Analizando setup()]\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);  /* Bloque */
            ts_salir_ambito();
            break;
            
        case NODO_FUNCION_DEF:
        {
            char* nombre_func = nodo->hijo2->data.cadena;
            printf("[Fase 2: Analizando cuerpo de funcion '%s']\n", nombre_func);
            ts_entrar_ambito();

            /* Registrar parámetros formales como variables locales */
            ASTNode* param = nodo->hijo3;
            while(param && param->tipo != NODO_VACIO) {
                TipoDato tipo_param = param->hijo1->data.tipo_dato;
                char* nombre_param = param->hijo2->data.cadena;
                
                /* Insertar parámetro como variable local (NULL porque no es función) */
                ts_insertar(nombre_param, tipo_param, param->linea, NULL);
                
                param = param->siguiente;
            }

            /* Analizar el cuerpo de la función */
            analizar_bloque(nodo->hijo4);
            ts_salir_ambito();
            
            /* Continuar con la siguiente función en la lista */
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
            /* Verificar que la condición es booleana y analizar bloques */
            printf("[Fase 2: Analizando IF]\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'if' no es booleana.\n", nodo->linea);
            }
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);  /* Bloque then */
            ts_salir_ambito();
            
            /* Analizar bloque else si existe */
            if (nodo->hijo3->tipo != NODO_VACIO) {
                ts_entrar_ambito();
                analizar_bloque(nodo->hijo3);  /* Bloque else */
                ts_salir_ambito();
            }
            break;

        case NODO_WHILE:
            /* Verificar condición booleana y analizar cuerpo */
            printf("[Fase 2: Analizando WHILE]\n");
            if (analizar_expresion(nodo->hijo1) != TIPO_BOOLEAN) {
                 fprintf(stderr, "Error Semantico (linea %d): La condicion del 'while' no es booleana.\n", nodo->linea);
            }
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo2);  /* Cuerpo del bucle */
            ts_salir_ambito();
            break;
            
        case NODO_FOR:
            /* Analizar inicialización, condición, paso y cuerpo */
            printf("[Fase 2: Analizando FOR]\n");
            ts_entrar_ambito();
            analizar_cuerpos(nodo->hijo1);  /* Inicialización */
            
            /* Verificar condición si existe */
            if (nodo->hijo2->tipo != NODO_VACIO) {
                if (analizar_expresion(nodo->hijo2) != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): La condicion del 'for' no es booleana.\n", nodo->hijo2->linea);
                }
            }
            analizar_cuerpos(nodo->hijo3);  /* Paso */
            analizar_bloque(nodo->hijo4);   /* Cuerpo del bucle */
            ts_salir_ambito();
            break;

        case NODO_DO_WHILE:
            /* Analizar cuerpo primero, luego verificar condición */
            printf("[Fase 2: Analizando DO-WHILE]\n");
            ts_entrar_ambito();
            analizar_bloque(nodo->hijo1);  /* Cuerpo del bucle */
            ts_salir_ambito();
            
            /* Verificar que la condición es booleana */
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

/* ========== Función Principal del Analizador Semántico ========== */

/**
 * Función principal del analizador semántico.
 * 
 * Ejecuta el análisis semántico completo en dos fases:
 * 1. Recolección de símbolos (declaraciones globales y firmas de funciones)
 * 2. Análisis de cuerpos (verificación de tipos y validación de uso)
 * 
 * @param raiz Raíz del AST a analizar
 * @return 1 si el análisis fue exitoso, 0 si hay errores críticos
 */
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

    /* No liberamos la tabla de símbolos aquí para permitir que fases
     * posteriores (como codegen) reutilicen la información de variables.
     * El proceso termina poco después, por lo que el impacto en memoria
     * es aceptable para el alcance del proyecto.
     */
    return 1;
}

int ts_obtener_indice_variable(const char* nombre) {
    if (!nombre) return -1;
    Simbolo* sym = ts_buscar((char*)nombre);
    if (!sym) return -1;
    /* Solo consideramos variables/parámetros (no funciones). */
    if (sym->parametros != NULL) {
        return -1;
    }
    return sym->indice_vm;
}

/**
 * Analiza un bloque de código.
 * 
 * Recorre todas las instrucciones del bloque y las analiza semánticamente.
 * Los bloques crean nuevos ámbitos, pero esta función NO gestiona ámbitos
 * (eso lo hace analizar_cuerpos() cuando procesa NODO_BLOQUE).
 * 
 * @param nodo_bloque Nodo NODO_BLOQUE a analizar
 */
void analizar_bloque(ASTNode* nodo_bloque) {
    if (!nodo_bloque || nodo_bloque->tipo != NODO_BLOQUE) return;
    
    /* Recorrer la lista enlazada de instrucciones */
    ASTNode* instruccion = nodo_bloque->hijo1;
    while (instruccion && instruccion->tipo != NODO_VACIO) {
        analizar_cuerpos(instruccion);
        instruccion = instruccion->siguiente;
    }
}

/**
 * Analiza una declaración de variables.
 * 
 * Registra las variables en la tabla de símbolos y verifica que las
 * inicializaciones (si existen) sean compatibles con el tipo declarado.
 * 
 * IMPORTANTE: Permite conversión implícita de INT a FLOAT en inicializaciones.
 * 
 * @param nodo Nodo NODO_DECLARACION a analizar
 */
void analizar_declaracion(ASTNode* nodo) {
    if (nodo->tipo != NODO_DECLARACION) return;
    
    TipoDato tipo_base = nodo->hijo1->data.tipo_dato;
    
    /* Recorrer la lista de declaradores (ej: int x, y = 5, z;) */
    ASTNode* decl = nodo->hijo2;
    while(decl && decl->tipo != NODO_VACIO) {
        char* nombre_id = decl->hijo1->data.cadena;
        
        /* Registrar la variable en la tabla de símbolos */
        ts_insertar(nombre_id, tipo_base, decl->linea, NULL);
        
        /* Verificar inicialización si existe */
        if (decl->hijo2->tipo != NODO_VACIO) {
            TipoDato tipo_expr = analizar_expresion(decl->hijo2);
            
            /* Verificar compatibilidad de tipos */
            if (tipo_base != tipo_expr) {
                /* Permitir conversión implícita de INT a FLOAT */
                if (tipo_base == TIPO_FLOAT && tipo_expr == TIPO_INT) {
                    /* Es válido (ej: float f = 5;) */
                } else {
                    fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en inicializacion de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                            decl->linea, nombre_id, tipo_base, tipo_expr);
                }
            }
        }
        
        decl = decl->siguiente;
    }
}

/* ========== Análisis de Expresiones ========== */

/**
 * Analiza una expresión y retorna su tipo.
 * 
 * Esta función realiza verificación de tipos recursiva sobre el AST de expresiones:
 * - Valida que los operandos de operadores sean compatibles
 * - Verifica que las variables estén declaradas
 * - Valida llamadas a funciones (número y tipos de argumentos)
 * - Determina el tipo resultante de la expresión
 * 
 * IMPORTANTE: Permite conversión implícita de INT a FLOAT en operaciones aritméticas
 * y en argumentos de funciones.
 * 
 * @param nodo Nodo raíz de la expresión a analizar
 * @return Tipo de dato de la expresión, o TIPO_DESCONOCIDO si hay error
 */
TipoDato analizar_expresion(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return TIPO_DESCONOCIDO;

    switch (nodo->tipo) {
        case NODO_NUMERO:
        {
            /* Determinar si el número es entero o flotante */
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
            /* Verificar que el lado izquierdo es un identificador (l-value) */
            ASTNode* lado_izq = nodo->hijo1;
            
            if (lado_izq->tipo != NODO_IDENTIFICADOR) {
                fprintf(stderr, "Error Semantico (linea %d): El lado izquierdo de la asignacion no es una variable.\n", nodo->linea);
                return TIPO_DESCONOCIDO;
            }

            /* Buscar la variable en la tabla de símbolos */
            char* nombre_id = lado_izq->data.cadena;
            Simbolo* sym = ts_buscar(nombre_id);
            if (!sym) {
                fprintf(stderr, "Error Semantico (linea %d): Variable '%s' no declarada.\n", lado_izq->linea, nombre_id);
                return TIPO_DESCONOCIDO;
            }
            
            /* Verificar compatibilidad de tipos */
            TipoDato tipo_var = sym->tipo;
            TipoDato tipo_expr = analizar_expresion(nodo->hijo2);
            
            if (tipo_var != tipo_expr) {
                 /* Permitir conversión implícita de INT a FLOAT */
                 if (tipo_var == TIPO_FLOAT && tipo_expr == TIPO_INT) {
                    /* Válido (ej: f = 5;) */
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
            
            /* Operadores lógicos: &&, || */
            if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
                if (tipo_izq != TIPO_BOOLEAN || tipo_der != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos booleanos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            
            /* Operadores de comparación: ==, !=, <, <=, >, >= */
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                strcmp(op, "<") == 0  || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0  || strcmp(op, ">=") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                return TIPO_BOOLEAN;
            }
            
            /* Operadores aritméticos: +, -, *, / */
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                
                if ((tipo_izq != TIPO_FLOAT && tipo_izq != TIPO_INT) || 
                    (tipo_der != TIPO_FLOAT && tipo_der != TIPO_INT)) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere operandos numericos.\n", nodo->linea, op);
                }
                /* Si algún operando es FLOAT, el resultado es FLOAT */
                if (tipo_izq == TIPO_FLOAT || tipo_der == TIPO_FLOAT) return TIPO_FLOAT;
                return TIPO_INT;
            }

            /* Operador módulo: % (solo enteros) */
            if (strcmp(op, "%") == 0) {
                if (tipo_izq != TIPO_INT || tipo_der != TIPO_INT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '%%' requiere operandos enteros.\n", nodo->linea);
                }
                return TIPO_INT;
            }

            /* Acceso a array: [] */
            if (strcmp(op, "[]") == 0) {
                /* TODO: Implementar lógica completa de tipos de array */
                /* Por ahora, solo verificamos que el índice sea entero */
                TipoDato tipo_indice = analizar_expresion(nodo->hijo2);
                if (tipo_indice != TIPO_INT) {
                     fprintf(stderr, "Error Semantico (linea %d): El indice de acceso a array debe ser un entero (TIPO_INT).\n", nodo->linea);
                }
                /* NOTA: Asumimos que el tipo del acceso es el tipo base del array */
                /* Esto es una simplificación - en un compilador completo se debería */
                /* mantener información del tipo base del array */
                return tipo_izq; 
            }

            return TIPO_DESCONOCIDO;
        }

        case NODO_UNARIO_OP:
        {
            char* op = nodo->data.op_unario;
            TipoDato tipo_hijo = analizar_expresion(nodo->hijo1);
            
            /* Negación lógica: ! */
            if (strcmp(op, "!") == 0) {
                if (tipo_hijo != TIPO_BOOLEAN) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador '!' requiere un operando booleano.\n", nodo->linea);
                }
                return TIPO_BOOLEAN;
            }
            
            /* Operadores unarios aritméticos: +, - */
            if (strcmp(op, "-") == 0 || strcmp(op, "+") == 0) {
                if (tipo_hijo != TIPO_INT && tipo_hijo != TIPO_FLOAT) {
                    fprintf(stderr, "Error Semantico (linea %d): Operador unario '%s' requiere un operando numerico.\n", nodo->linea, op);
                }
                return tipo_hijo;
            }
            
            /* Incremento/decremento prefijo: ++, -- */
            if (strcmp(op, "++") == 0 || strcmp(op, "--") == 0) {
                /* Verificar que el operando es un l-value (variable) */
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
            
            /* Verificar que el operando es un l-value (variable) */
            if (nodo->hijo1->tipo != NODO_IDENTIFICADOR) {
                fprintf(stderr, "Error Semantico (linea %d): El operador postfijo '%s' requiere un l-value (variable modificable).\n", nodo->linea, op);
            }

            /* Verificar que el tipo es numérico */
            if (tipo_hijo != TIPO_INT && tipo_hijo != TIPO_FLOAT) {
                fprintf(stderr, "Error Semantico (linea %d): Operador '%s' requiere un operando numerico.\n", nodo->linea, op);
            }
            return tipo_hijo; 
        }
            
        case NODO_LLAMADA_FUNCION:
        {
            ASTNode* callee = nodo->hijo1;
            
            /* Llamada a función definida por el usuario */
            if (callee->tipo == NODO_IDENTIFICADOR) {
                char* nombre_func = callee->data.cadena;
                Simbolo* sym = ts_buscar(nombre_func);
                if (!sym) {
                    fprintf(stderr, "Error Semantico (linea %d): Funcion '%s' no declarada.\n", nodo->linea, nombre_func);
                    return TIPO_DESCONOCIDO;
                }
            
                /* Comparar parámetros formales con argumentos reales */
                ASTNode* param = sym->parametros;  /* Lista de parámetros esperados (de la TS) */
                ASTNode* arg = nodo->hijo2;        /* Lista de argumentos dados (del AST) */
                int arg_count = 0;
            
                /* Verificar tipos de argumentos uno por uno */
                while ((param && param->tipo != NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                    arg_count++;
                    
                    /* param es un NODO_DECLARACION (hijo1=Tipo, hijo2=ID) */
                    TipoDato tipo_param = param->hijo1->data.tipo_dato;
                    TipoDato tipo_arg = analizar_expresion(arg);
            
                    /* Verificar compatibilidad de tipos */
                    if (tipo_param != tipo_arg) {
                        /* Permitir conversión implícita de INT a FLOAT */
                        if (tipo_param == TIPO_FLOAT && tipo_arg == TIPO_INT) {
                           /* Válido */
                        } else {
                            fprintf(stderr, "Error Semantico (linea %d): Incompatibilidad de tipos en argumento %d de '%s'. Se esperaba %d pero se obtuvo %d.\n",
                                    arg->linea, arg_count, nombre_func, tipo_param, tipo_arg);
                        }
                    }
            
                    param = param->siguiente;
                    arg = arg->siguiente;
                }
            
                /* Verificar número de argumentos */
                if ((param && param->tipo != NODO_VACIO) && (!arg || arg->tipo == NODO_VACIO)) {
                    fprintf(stderr, "Error Semantico (linea %d): No hay suficientes argumentos para la funcion '%s'.\n", nodo->linea, nombre_func);
                } else if ((!param || param->tipo == NODO_VACIO) && (arg && arg->tipo != NODO_VACIO)) {
                     fprintf(stderr, "Error Semantico (linea %d): Demasiados argumentos para la funcion '%s'.\n", nodo->linea, nombre_func);
                }
                
                return sym->tipo;  /* Retornar el tipo de retorno de la función */
            
            /* Llamada a función reservada del lenguaje */
            } else if (callee->tipo == NODO_FUNCION_RESERVADA) {
                char* nombre_func = callee->data.cadena;
                printf("[Fase 2: Analizando llamada a func reservada '%s']\n", nombre_func);

                ASTNode* arg = nodo->hijo2;

                /* Función reservada leerSensor(): sin argumentos, retorna int */
                if (strcmp(nombre_func, "leerSensor") == 0) {
                    if (arg && arg->tipo != NODO_VACIO) {
                        fprintf(stderr, "Error Semantico (linea %d): '%s()' no toma argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_INT;
                }

                /* Función reservada parar(): sin argumentos, retorna void */
                if (strcmp(nombre_func, "parar") == 0) {
                    if (arg && arg->tipo != NODO_VACIO) {
                        fprintf(stderr, "Error Semantico (linea %d): '%s()' no toma argumentos.\n", nodo->linea, nombre_func);
                    }
                    return TIPO_VOID;
                }

                /* Funciones reservadas de movimiento con UN argumento entero:
                 *   mover(int)
                 *   girarIzq(int)
                 *   girarDer(int)
                 *   reversa(int)
                 */
                if (strcmp(nombre_func, "mover") == 0 ||
                    strcmp(nombre_func, "girarIzq") == 0 ||
                    strcmp(nombre_func, "girarDer") == 0 ||
                    strcmp(nombre_func, "reversa") == 0) {

                    /* Debe haber exactamente un argumento. */
                    if (!arg || arg->tipo == NODO_VACIO) {
                        fprintf(stderr, "Error Semantico (linea %d): '%s()' requiere un argumento entero.\n",
                                nodo->linea, nombre_func);
                        return TIPO_VOID;
                    }

                    /* Analizar el primer argumento y comprobar que es entero. */
                    TipoDato tipo_arg = analizar_expresion(arg);
                    if (tipo_arg != TIPO_INT) {
                        fprintf(stderr, "Error Semantico (linea %d): El argumento de '%s()' debe ser entero (TIPO_INT).\n",
                                arg->linea, nombre_func);
                    }

                    /* Verificar que no haya más argumentos en la lista. */
                    if (arg->siguiente && arg->siguiente->tipo != NODO_VACIO) {
                        fprintf(stderr, "Error Semantico (linea %d): '%s()' solo acepta un argumento.\n",
                                nodo->linea, nombre_func);
                    }

                    return TIPO_VOID;
                }
                
                return TIPO_DESCONOCIDO;
            }
            
            /* Error: se intenta llamar a algo que no es una función */
            fprintf(stderr, "Error Semantico (linea %d): No se puede llamar a este tipo de expresion.\n", nodo->linea);
            return TIPO_DESCONOCIDO;
        }
            
        default:
            return TIPO_DESCONOCIDO;
    }
}