/*
 * Implementación del generador de bytecode (.vmcode) para la VM TEO.
 *
 * La VM es de pila y trabaja con instrucciones en formato:
 *
 *   MNEMONIC arg1 arg2
 *
 * donde los argumentos son enteros (0 si no se usan).
 */

#include "codegen.h"
#include "../semantic/semantic.h"

/* Representación interna de una instrucción antes de volcarla al archivo. */
typedef struct {
    const char *mnemonic;
    int arg1;
    int arg2;
} Instruction;

/* Programa en memoria para permitir backpatching de saltos. */
static Instruction *program = NULL;
static int program_size = 0;
static int program_cap = 0;

/* Contador de instrucciones emitidas (equivale al PC actual en la VM). */
static int pc_actual = 0;

/* --- Declaraciones adelantadas de funciones internas de generación --- */
static void gen_programa(ASTNode *nodo);
static void gen_bloque(ASTNode *bloque);
static void gen_lista_instrucciones(ASTNode *lista);
static void gen_instruccion(ASTNode *nodo);
static void gen_declaracion(ASTNode *nodo);
static void gen_expresion(ASTNode *nodo);
static void gen_llamada_funcion(ASTNode *nodo);

/* Control de flujo (se implementa completamente en tareas posteriores) */
static void gen_if(ASTNode *nodo);
static void gen_while(ASTNode *nodo);
static void gen_for(ASTNode *nodo);
static void gen_do_while(ASTNode *nodo);

/* Helpers internos */
static int emit(const char *mnemonic, int arg1, int arg2);
static void ensure_capacity(void);
static int obtener_indice_variable(ASTNode *id_node);

void generar_bytecode(ASTNode *raiz, FILE *out) {
    if (!raiz || !out) {
        return;
    }

    /* Reiniciar estado interno. */
    pc_actual = 0;
    program_size = 0;

    gen_programa(raiz);

    /* Asegurar finalización del programa. */
    emit("HALT", 0, 0);

    /* Volcar el programa al archivo de salida en el formato acordado. */
    for (int i = 0; i < program_size; i++) {
        fprintf(out, "%s %d %d\n", program[i].mnemonic, program[i].arg1, program[i].arg2);
    }
}

/* ========================= Helpers internos ========================= */

static void ensure_capacity(void) {
    if (program_size >= program_cap) {
        int nuevo_cap = (program_cap == 0) ? 128 : program_cap * 2;
        Instruction *nuevo = (Instruction *)realloc(program, nuevo_cap * sizeof(Instruction));
        if (!nuevo) {
            fprintf(stderr, "Error: no se pudo reservar memoria para el programa de bytecode.\n");
            exit(1);
        }
        program = nuevo;
        program_cap = nuevo_cap;
    }
}

/**
 * Inserta una instrucción en el programa en memoria.
 *
 * @return Índice de la instrucción recién insertada (su PC).
 */
static int emit(const char *mnemonic, int arg1, int arg2) {
    if (!mnemonic) return -1;
    ensure_capacity();
    program[program_size].mnemonic = mnemonic;
    program[program_size].arg1 = arg1;
    program[program_size].arg2 = arg2;
    pc_actual = program_size;
    return program_size++;
}

static int obtener_indice_variable(ASTNode *id_node) {
    if (!id_node || id_node->tipo != NODO_IDENTIFICADOR || !id_node->data.cadena) {
        return -1;
    }
    int idx = ts_obtener_indice_variable(id_node->data.cadena);
    if (idx < 0) {
        fprintf(stderr, "[codegen] Advertencia: variable '%s' sin indice de VM.\n",
                id_node->data.cadena);
    }
    return idx;
}

/* ========================= Generación de alto nivel ========================= */

/**
 * Genera el flujo principal de instrucciones del programa.
 *
 * Actualmente se asume que:
 *  - NODO_PROGRAMA.hijo1 son declaraciones/expresiones globales.
 *  - NODO_PROGRAMA.hijo2 es el nodo NODO_SETUP con el bloque principal.
 *  - NODO_SETUP.hijo2 es el bloque (NODO_BLOQUE) que contiene la lista
 *    de instrucciones principales.
 *
 * Las funciones definidas por el usuario (hijo3) se ignoran en esta primera
 * versión del backend, ya que la VM aún no soporta llamadas a funciones
 * de usuario.
 */
static void gen_programa(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_PROGRAMA) return;

    /* 1) Declaraciones y expresiones globales (se ejecutan antes de setup). */
    gen_lista_instrucciones(nodo->hijo1);

    /* 2) Cuerpo de setup() como flujo principal. */
    ASTNode *setup = nodo->hijo2;
    if (setup && setup->tipo == NODO_SETUP) {
        ASTNode *bloque = setup->hijo2;
        gen_bloque(bloque);
    }
}

static void gen_bloque(ASTNode *bloque) {
    if (!bloque || bloque->tipo != NODO_BLOQUE) return;
    gen_lista_instrucciones(bloque->hijo1);
}

static void gen_lista_instrucciones(ASTNode *lista) {
    ASTNode *actual = lista;
    while (actual && actual->tipo != NODO_VACIO) {
        gen_instruccion(actual);
        actual = actual->siguiente;
    }
}

static void gen_instruccion(ASTNode *nodo) {
    if (!nodo) return;

    switch (nodo->tipo) {
        case NODO_DECLARACION:
            gen_declaracion(nodo);
            break;

        case NODO_IF:
            gen_if(nodo);
            break;

        case NODO_WHILE:
            gen_while(nodo);
            break;

        case NODO_FOR:
            gen_for(nodo);
            break;

        case NODO_DO_WHILE:
            gen_do_while(nodo);
            break;

        case NODO_LLAMADA_FUNCION:
            gen_llamada_funcion(nodo);
            break;

        case NODO_BLOQUE:
            gen_bloque(nodo);
            break;

        default:
            /* Expresiones que aparecen como instrucción (asignaciones, ops, etc.) */
            if (nodo->tipo >= NODO_ASIGNACION && nodo->tipo <= NODO_BOOLEANO) {
                gen_expresion(nodo);
            }
            break;
    }
}

/* ========================= Declaraciones y asignaciones ========================= */

static void gen_declaracion(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_DECLARACION) return;

    /* nodo->hijo1 es el tipo (no se usa directamente en bytecode) */
    ASTNode *decl = nodo->hijo2;  /* Lista de declaradores */
    while (decl && decl->tipo != NODO_VACIO) {
        ASTNode *id_node = decl->hijo1;   /* NODO_IDENTIFICADOR */
        ASTNode *init = decl->hijo2;      /* Expresión o NODO_VACIO */

        if (init && init->tipo != NODO_VACIO) {
            /* Evaluar expresión de inicialización (deja valor en la pila). */
            gen_expresion(init);

            int idx = obtener_indice_variable(id_node);
            if (idx >= 0) {
                /* Guardar el valor inicial en la variable. */
                emit("STORE", idx, 0);
            }
        }

        decl = decl->siguiente;
    }
}

/* ========================= Expresiones ========================= */

static void gen_expresion(ASTNode *nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_NUMERO: {
            int v = (int)nodo->data.valor_num;
            emit("LOADI", v, 0);
            break;
        }

        case NODO_BOOLEANO: {
            int v = nodo->data.valor_bool ? 1 : 0;
            emit("LOADI", v, 0);
            break;
        }

        case NODO_IDENTIFICADOR: {
            int idx = obtener_indice_variable(nodo);
            if (idx >= 0) {
                emit("LOAD", idx, 0);
            }
            break;
        }

        case NODO_ASIGNACION: {
            ASTNode *lhs = nodo->hijo1;
            ASTNode *rhs = nodo->hijo2;

            /* Evaluar primero la expresión del lado derecho. */
            gen_expresion(rhs);

            if (lhs && lhs->tipo == NODO_IDENTIFICADOR) {
                int idx = obtener_indice_variable(lhs);
                if (idx >= 0) {
                    emit("STORE", idx, 0);
                }
            } else {
                fprintf(stderr, "[codegen] Asignacion con lado izquierdo no soportado (linea %d).\n",
                        nodo->linea);
            }
            break;
        }

        case NODO_BINARIO_OP: {
            char *op = nodo->data.op_unario;

            /* Evaluar operandos en orden izquierda-derecha. */
            gen_expresion(nodo->hijo1);
            gen_expresion(nodo->hijo2);

            if (strcmp(op, "+") == 0) {
                emit("ADD", 0, 0);
            } else if (strcmp(op, "-") == 0) {
                emit("SUB", 0, 0);
            } else if (strcmp(op, "*") == 0) {
                emit("MUL", 0, 0);
            } else if (strcmp(op, "/") == 0) {
                emit("DIV", 0, 0);
            } else if (strcmp(op, "&&") == 0) {
                emit("AND", 0, 0);
            } else if (strcmp(op, "||") == 0) {
                emit("OR", 0, 0);
            } else if (strcmp(op, "==") == 0) {
                emit("EQ", 0, 0);
            } else if (strcmp(op, "!=") == 0) {
                emit("NEQ", 0, 0);
            } else if (strcmp(op, "<") == 0) {
                emit("LT", 0, 0);
            } else if (strcmp(op, "<=") == 0) {
                emit("LE", 0, 0);
            } else if (strcmp(op, ">") == 0) {
                emit("GT", 0, 0);
            } else if (strcmp(op, ">=") == 0) {
                emit("GE", 0, 0);
            } else if (strcmp(op, "%") == 0) {
                fprintf(stderr, "[codegen] Operador '%%' no soportado en VM actual (linea %d).\n",
                        nodo->linea);
            } else if (strcmp(op, "[]") == 0) {
                fprintf(stderr, "[codegen] Acceso a arrays '[]' no implementado en VM (linea %d).\n",
                        nodo->linea);
            } else {
                fprintf(stderr, "[codegen] Operador binario desconocido '%s' (linea %d).\n",
                        op, nodo->linea);
            }
            break;
        }

        case NODO_UNARIO_OP: {
            char *op = nodo->data.op_unario;

            if (strcmp(op, "!") == 0) {
                gen_expresion(nodo->hijo1);
                emit("NOT", 0, 0);
            } else if (strcmp(op, "+") == 0) {
                gen_expresion(nodo->hijo1);
            } else if (strcmp(op, "-") == 0) {
                /* -x  =>  x; LOADI -1; MUL */
                gen_expresion(nodo->hijo1);
                emit("LOADI", -1, 0);
                emit("MUL", 0, 0);
            } else if (strcmp(op, "++") == 0 || strcmp(op, "--") == 0) {
                ASTNode *id = nodo->hijo1;
                int idx = obtener_indice_variable(id);
                if (idx >= 0) {
                    /* LOAD idx; LOADI 1; ADD/SUB; STORE idx */
                    emit("LOAD", idx, 0);
                    emit("LOADI", 1, 0);
                    if (strcmp(op, "++") == 0) {
                        emit("ADD", 0, 0);
                    } else {
                        emit("SUB", 0, 0);
                    }
                    emit("STORE", idx, 0);
                }
            } else {
                fprintf(stderr, "[codegen] Operador unario '%s' no soportado (linea %d).\n",
                        op, nodo->linea);
            }
            break;
        }

        case NODO_POSTFIX_OP: {
            /* Aproximación: tratar postfijos como prefijos para efectos de VM. */
            char *op = nodo->data.op_unario;
            ASTNode *id = nodo->hijo1;
            int idx = obtener_indice_variable(id);
            if (idx >= 0) {
                emit("LOAD", idx, 0);
                emit("LOADI", 1, 0);
                if (strcmp(op, "++") == 0) {
                    emit("ADD", 0, 0);
                } else if (strcmp(op, "--") == 0) {
                    emit("SUB", 0, 0);
                }
                emit("STORE", idx, 0);
            }
            break;
        }

        case NODO_LLAMADA_FUNCION:
            gen_llamada_funcion(nodo);
            break;

        default:
            /* Otros tipos se ignoran por ahora. */
            break;
    }
}

/* ========================= Llamadas a funciones ========================= */

static void gen_llamada_funcion(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_LLAMADA_FUNCION) return;

    ASTNode *callee = nodo->hijo1;
    ASTNode *args = nodo->hijo2;

    /* Llamadas a funciones reservadas del lenguaje (carro / ESP32). */
    if (callee && callee->tipo == NODO_FUNCION_RESERVADA) {
        const char *nombre = callee->data.cadena;

        /* Evaluar argumentos (si hay) antes de la llamada. */
        ASTNode *arg = args;
        while (arg && arg->tipo != NODO_VACIO) {
            gen_expresion(arg);
            arg = arg->siguiente;
        }

        if (strcmp(nombre, "mover") == 0) {
            emit("CALL_MOVER", 0, 0);
        } else if (strcmp(nombre, "girarIzq") == 0) {
            emit("CALL_GIRAR_IZQ", 0, 0);
        } else if (strcmp(nombre, "girarDer") == 0) {
            emit("CALL_GIRAR_DER", 0, 0);
        } else if (strcmp(nombre, "reversa") == 0) {
            emit("CALL_REVERSA", 0, 0);
        } else if (strcmp(nombre, "leerSensor") == 0) {
            emit("CALL_LEER_SENSOR", 0, 0);
        } else if (strcmp(nombre, "parar") == 0) {
            emit("CALL_PARAR", 0, 0);
        } else {
            fprintf(stderr, "[codegen] Funcion reservada '%s' no reconocida (linea %d).\n",
                    nombre, nodo->linea);
        }
        return;
    }

    /* Llamadas a funciones de usuario aún no soportadas en la VM. */
    fprintf(stderr, "[codegen] Llamadas a funciones de usuario no soportadas (linea %d).\n",
            nodo->linea);
}

/* ========================= Control de flujo (stubs) ========================= */

static void gen_if(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_IF) return;

    ASTNode *cond = nodo->hijo1;
    ASTNode *then_bloque = nodo->hijo2;
    ASTNode *else_bloque = nodo->hijo3;

    /* Generar condición: deja 0/1 en la pila. */
    gen_expresion(cond);

    /* Salto condicional hacia el bloque else (o al final si no hay else). */
    int idx_jz = emit("JZ", 0, 0);  /* destino se parchea luego */

    /* Bloque then. */
    gen_bloque(then_bloque);

    if (else_bloque && else_bloque->tipo != NODO_VACIO) {
        /* Saltar al final del if después de ejecutar el then. */
        int idx_jmp_fin = emit("JMP", 0, 0);

        /* El inicio del bloque else es la siguiente instrucción. */
        int pc_else = program_size;
        program[idx_jz].arg1 = pc_else;

        /* Bloque else. */
        gen_bloque(else_bloque);

        /* Parchear salto incondicional al final del if. */
        int pc_fin = program_size;
        program[idx_jmp_fin].arg1 = pc_fin;
    } else {
        /* Sin else: el destino de JZ es la instrucción siguiente al then. */
        int pc_fin = program_size;
        program[idx_jz].arg1 = pc_fin;
    }
}

static void gen_while(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_WHILE) return;

    ASTNode *cond = nodo->hijo1;
    ASTNode *cuerpo = nodo->hijo2;

    /* Punto de inicio del bucle (donde se evalúa la condición). */
    int pc_inicio = program_size;

    /* Evaluar condición. */
    gen_expresion(cond);

    /* Si la condición es 0, saltar al final del bucle. */
    int idx_jz = emit("JZ", 0, 0);

    /* Cuerpo del bucle. */
    gen_bloque(cuerpo);

    /* Volver a evaluar la condición. */
    emit("JMP", pc_inicio, 0);

    /* Parchear destino de JZ al final del bucle. */
    int pc_fin = program_size;
    program[idx_jz].arg1 = pc_fin;
}

static void gen_for(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_FOR) return;

    ASTNode *init = nodo->hijo1;
    ASTNode *cond = nodo->hijo2;
    ASTNode *step = nodo->hijo3;
    ASTNode *cuerpo = nodo->hijo4;

    /* Inicialización (se ejecuta una sola vez antes del bucle). */
    if (init && init->tipo != NODO_VACIO) {
        if (init->tipo == NODO_DECLARACION) {
            gen_declaracion(init);
        } else {
            gen_instruccion(init);
        }
    }

    /* Punto de inicio del bucle. */
    int pc_inicio = program_size;
    int idx_jz = -1;

    /* Condición opcional. */
    if (cond && cond->tipo != NODO_VACIO) {
        gen_expresion(cond);
        idx_jz = emit("JZ", 0, 0);  /* se parchará más tarde */
    }

    /* Cuerpo del bucle. */
    gen_bloque(cuerpo);

    /* Paso opcional (ej. i++, i -= 1, etc.). */
    if (step && step->tipo != NODO_VACIO) {
        gen_expresion(step);
    }

    /* Volver al inicio del bucle. */
    emit("JMP", pc_inicio, 0);

    /* Si había condición, parchar su salto al final del bucle. */
    if (idx_jz >= 0) {
        int pc_fin = program_size;
        program[idx_jz].arg1 = pc_fin;
    }
}

static void gen_do_while(ASTNode *nodo) {
    if (!nodo || nodo->tipo != NODO_DO_WHILE) return;

    ASTNode *cuerpo = nodo->hijo1;
    ASTNode *cond = nodo->hijo2;

    /* El cuerpo se ejecuta al menos una vez. */
    int pc_inicio = program_size;
    gen_bloque(cuerpo);

    /* Evaluar la condición al final. */
    gen_expresion(cond);

    /* Si condición es 0, salir; si no, volver al inicio del cuerpo. */
    int idx_jz = emit("JZ", 0, 0);
    emit("JMP", pc_inicio, 0);

    int pc_fin = program_size;
    program[idx_jz].arg1 = pc_fin;
}



