#include "codegen.h"
#include <stdlib.h>
#include <string.h>

/* Módulo de generación de código.
 * Transforma el AST validado en una secuencia de instrucciones de bytecode
 * que será consumida por la máquina virtual del robot seguidor de línea.
 */

/* 1. BUFFER DE CÓDIGO (INT)
 * Se utiliza un búfer de enteros para representar el programa compilado;
 * los valores en coma flotante se truncan a entero (por ejemplo, 3.14 -> 3).
 */
int code_buffer[MAX_CODE_SIZE];
int pc = 0; 

/* 2. NOMBRES DE INSTRUCCIONES 
 * Mapea cada opcode simbólico a la representación textual que se escribe en el archivo.
 */
const char* OP_NAMES[] = {
    "HALT",
    "CONST", "LOAD", "STORE",
    "ADD", "SUB", "MUL", "DIV", "MOD",
    "INC", "DEC", "NEG",
    "AND", "OR", "NOT",
    "EQ", "NEQ", "GT", "LT", "GTE", "LTE",
    "JMP", "JZ",
    "MOVER", "GIRAR_IZQ", "GIRAR_DER", 
    "LEER_SENSOR", "PARAR", "REVERSA",
    "DELAY" // Instrucción de espera en milisegundos (argumento en la pila)
};

/* 3. CANTIDAD DE ARGUMENTOS
 * Indica cuántos operandos enteros acompaña a cada instrucción en el flujo de bytecode.
 */
const int OP_ARGS[] = {
    0, // HALT
    1, 1, 1, // CONST, LOAD, STORE
    0, 0, 0, 0, 0, // Aritmética
    0, 0, 0, // Unarios
    0, 0, 0, // Lógica
    0, 0, 0, 0, 0, 0, // Comparación
    1, 1, // JMP, JZ
    0, 0, 0, // MOVER, GIRAR_IZQ, GIRAR_DER
    0, 0, 0, // LEER_SENSOR, PARAR, REVERSA
    0        // DELAY (argumento leído desde la pila)
};

/* 4. FUNCIONES DE EMISIÓN (INT)
 * Encapsulan la escritura segura en el búfer de bytecode y permiten correcciones posteriores.
 */

void emit(int valor) {
    if (pc >= MAX_CODE_SIZE) {
        fprintf(stderr, "Error en la generación de código: desbordamiento del búfer de bytecode.\n");
        exit(1);
    }
    code_buffer[pc++] = valor;
}

void patch(int direccion, int valor) {
    if (direccion >= 0 && direccion < pc) code_buffer[direccion] = valor;
}

/* 5. GENERADOR RECURSIVO
 * Recorre el AST y emite las instrucciones correspondientes para cada tipo de nodo.
 */

void generar_nodo(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            // 1. Genera inicializaciones y expresiones globales.
            ASTNode* global = nodo->hijo1;
            while (global) {
                generar_nodo(global);
                global = global->siguiente;
            }
            // 2. Genera el cuerpo de setup().
            generar_nodo(nodo->hijo2);
            // 3. Genera el código de las funciones definidas por el usuario.
            ASTNode* func = nodo->hijo3;
            while (func) {
                generar_nodo(func);
                func = func->siguiente;
            }
            break;

        case NODO_SETUP:
            generar_nodo(nodo->hijo2); 
            // Marca el fin del programa principal ejecutado por la VM.
            emit(OP_HALT); 
            break;

        case NODO_FUNCION_DEF:
            generar_nodo(nodo->hijo4); 
            break;

        case NODO_BLOQUE: {
            ASTNode* instr = nodo->hijo1;
            while (instr) { generar_nodo(instr); instr = instr->siguiente; }
            break;
        }
        
        case NODO_DECLARACION: {
            ASTNode* decl = nodo->hijo2; 
            while(decl) {
                if (decl->hijo2->tipo != NODO_VACIO) {
                    generar_nodo(decl->hijo2); 
                    Simbolo* sym = ts_buscar(decl->hijo1->data.cadena);
                    if (sym) { emit(OP_STORE); emit(sym->direccion); }
                }
                decl = decl->siguiente;
            }
            break;
        }

        case NODO_ASIGNACION: {
            generar_nodo(nodo->hijo2); 
            Simbolo* sym = ts_buscar(nodo->hijo1->data.cadena);
            if (sym) { emit(OP_STORE); emit(sym->direccion); }
            break;
        }

        case NODO_NUMERO: 
            emit(OP_CONST); 
            // Trunca explícitamente el valor numérico a entero antes de almacenarlo.
            emit((int)nodo->data.valor_num); 
            break;
            
        case NODO_BOOLEANO: 
            emit(OP_CONST); 
            emit(nodo->data.valor_bool); 
            break;

        case NODO_IDENTIFICADOR: {
            Simbolo* sym = ts_buscar(nodo->data.cadena);
            if (sym) { emit(OP_LOAD); emit(sym->direccion); }
            break;
        }
        
        case NODO_BINARIO_OP: {
            generar_nodo(nodo->hijo1); generar_nodo(nodo->hijo2);
            char* op = nodo->data.op_unario;
            if (strcmp(op, "+") == 0) emit(OP_ADD);
            else if (strcmp(op, "-") == 0) emit(OP_SUB);
            else if (strcmp(op, "*") == 0) emit(OP_MUL);
            else if (strcmp(op, "/") == 0) emit(OP_DIV);
            else if (strcmp(op, "%") == 0) emit(OP_MOD);
            else if (strcmp(op, "&&") == 0) emit(OP_AND);
            else if (strcmp(op, "||") == 0) emit(OP_OR);
            else if (strcmp(op, "==") == 0) emit(OP_EQ);
            else if (strcmp(op, "!=") == 0) emit(OP_NEQ);
            else if (strcmp(op, ">") == 0) emit(OP_GT);
            else if (strcmp(op, "<") == 0) emit(OP_LT);
            else if (strcmp(op, ">=") == 0) emit(OP_GTE);
            else if (strcmp(op, "<=") == 0) emit(OP_LTE);
            break;
        }
        
        case NODO_UNARIO_OP: {
            char* op = nodo->data.op_unario;
            if (strcmp(op, "++") == 0 || strcmp(op, "--") == 0) {
                char* nombre = nodo->hijo1->data.cadena;
                Simbolo* sym = ts_buscar(nombre);
                if(sym) {
                    emit(OP_LOAD); emit(sym->direccion);
                    emit(OP_CONST); emit(1);
                    emit((strcmp(op, "++") == 0) ? OP_ADD : OP_SUB);
                    emit(OP_STORE); emit(sym->direccion);
                    emit(OP_LOAD); emit(sym->direccion); 
                }
            } else {
                generar_nodo(nodo->hijo1);
                if (strcmp(op, "!") == 0) emit(OP_NOT);
                else if (strcmp(op, "-") == 0) emit(OP_NEG);
            }
            break;
        }

        case NODO_IF: {
            generar_nodo(nodo->hijo1); emit(OP_JZ); int addr_else = pc; emit(0); 
            generar_nodo(nodo->hijo2);
            if (nodo->hijo3->tipo != NODO_VACIO) {
                emit(OP_JMP); int addr_fin = pc; emit(0); 
                patch(addr_else, pc); generar_nodo(nodo->hijo3); patch(addr_fin, pc);
            } else { patch(addr_else, pc); }
            break;
        }
        
        case NODO_WHILE: {
            int addr_start = pc; generar_nodo(nodo->hijo1); emit(OP_JZ); int addr_end = pc; emit(0);
            generar_nodo(nodo->hijo2); emit(OP_JMP); emit(addr_start); patch(addr_end, pc);
            break;
        }
        
        case NODO_FOR: {
            generar_nodo(nodo->hijo1); int addr_start = pc;
            if (nodo->hijo2->tipo != NODO_VACIO) generar_nodo(nodo->hijo2);
            else { emit(OP_CONST); emit(1); }
            emit(OP_JZ); int addr_end = pc; emit(0);
            generar_nodo(nodo->hijo4); generar_nodo(nodo->hijo3);
            emit(OP_JMP); emit(addr_start); patch(addr_end, pc);
            break;
        }
        
        case NODO_LLAMADA_FUNCION: {
            if (nodo->hijo1->tipo == NODO_FUNCION_RESERVADA) {
                char* func = nodo->hijo1->data.cadena;

                /* Para las funciones reservadas del robot, la convención de paso
                 * de parámetros es la siguiente:
                 *   1) Se evalúan los argumentos de izquierda a derecha y se
                 *      dejan sus valores en la pila.
                 *   2) Para mover/girar/reversa se empuja un contador de
                 *      argumentos (OP_CONST n) antes del opcode.
                 *   3) La VM lee el contador y consume los valores de la pila
                 *      según la firma concreta de cada función.
                 *   4) Para esperar(), el argumento se deja directamente en la
                 *      pila y la instrucción DELAY lo consume sin contador
                 *      adicional para preservar el diseño original.
                 */

                int arg_count = 0;
                ASTNode* arg = nodo->hijo2;
                while (arg && arg->tipo != NODO_VACIO) {
                    generar_nodo(arg);
                    arg_count++;
                    arg = arg->siguiente;
                }

                if (strcmp(func, "mover") == 0 ||
                    strcmp(func, "girarIzq") == 0 ||
                    strcmp(func, "girarDer") == 0 ||
                    strcmp(func, "reversa") == 0) {
                    /* Inserta el contador de argumentos para que la VM pueda
                     * adaptar su comportamiento sin necesidad de nuevos opcodes.
                     */
                    emit(OP_CONST);
                    emit(arg_count);

                    if (strcmp(func, "mover") == 0) emit(OP_MOVER);
                    else if (strcmp(func, "girarIzq") == 0) emit(OP_GIRAR_IZQ);
                    else if (strcmp(func, "girarDer") == 0) emit(OP_GIRAR_DER);
                    else if (strcmp(func, "reversa") == 0) emit(OP_REVERSA);
                } else if (strcmp(func, "parar") == 0) {
                    /* 'parar()' no utiliza parámetros; la fase semántica se
                     * encarga de garantizar que no existan argumentos.
                     */
                    emit(OP_PARAR);
                } else if (strcmp(func, "leerSensor") == 0) {
                    /* 'leerSensor()' tampoco recibe argumentos y deja el valor
                     * leído en la pila.
                     */
                    emit(OP_LEER_SENSOR);
                } else if (strcmp(func, "esperar") == 0) {
                    /* Para 'esperar(ms)', el argumento ya se encuentra en la
                     * pila; DELAY consume directamente el tiempo en
                     * milisegundos sin contador extra.
                     */
                    emit(OP_DELAY);
                }
            }
            break;
        }
        default: break;
    }
}

/* 6. FUNCIÓN PRINCIPAL DE ESCRITURA */
void generar_codigo(ASTNode* raiz, const char* nombre_archivo) {
    pc = 0;
    printf("\n[CodeGen] Generando bytecode en memoria a partir del AST...\n");
    generar_nodo(raiz);
    
    FILE* f = fopen(nombre_archivo, "w");
    if (!f) {
        fprintf(stderr, "Error en la generación de código: no se pudo abrir el archivo '%s' para escritura.\n", nombre_archivo);
        return;
    }
    
    int i = 0;
    while (i < pc) {
        int opcode = code_buffer[i];
        if (opcode < 0 || opcode > 29) break; // Finaliza si se encuentra un opcode fuera de rango.

        fprintf(f, "%s", OP_NAMES[opcode]);
        
        int args = OP_ARGS[opcode];
        if (args > 0) {
            int val = code_buffer[i + 1];
            fprintf(f, " %d", val); // Escribe el argumento entero asociado a la instrucción.
            i++; 
        }
        fprintf(f, "\n");
        i++;
    }
    fclose(f);
    printf("[CodeGen] Archivo de bytecode '%s' generado correctamente.\n", nombre_archivo);
}