#include "codegen.h"
#include <stdlib.h>

FILE* archivo_salida = NULL;

/* Función auxiliar para emitir una instrucción simple */
void emitir(int opcode) {
    fprintf(archivo_salida, "%d\n", opcode);
}

/* Función auxiliar para emitir instrucción con argumento */
void emitir_arg(int opcode, double arg) {
    // Imprimimos como entero si no tiene decimales para ahorrar espacio, o float si los tiene
    if (arg == (long)arg) {
        fprintf(archivo_salida, "%d %ld\n", opcode, (long)arg);
    } else {
        fprintf(archivo_salida, "%d %f\n", opcode, arg);
    }
}

/* Recorrido recursivo para generar código */
void generar_nodo(ASTNode* nodo) {
    if (!nodo || nodo->tipo == NODO_VACIO) return;

    switch (nodo->tipo) {
        case NODO_PROGRAMA:
            // Generamos primero setup, luego funciones (simplificado)
            // En una VM real, saltaríamos al main/setup primero.
            generar_nodo(nodo->hijo2); // Setup
            break;

        case NODO_SETUP:
            // Setup es simplemente un bloque de instrucciones
            generar_nodo(nodo->hijo2); // Bloque
            emitir(OP_HALT); // Al terminar setup, detenemos
            break;

        case NODO_BLOQUE:
        {
            ASTNode* instr = nodo->hijo1;
            while (instr) {
                generar_nodo(instr);
                instr = instr->siguiente;
            }
            break;
        }

        /* --- EXPRESIONES --- */
        case NODO_NUMERO:
            emitir_arg(OP_CONST, nodo->data.valor_num);
            break;
            
        case NODO_BOOLEANO:
            emitir_arg(OP_CONST, nodo->data.valor_bool); // 1 o 0
            break;

        case NODO_IDENTIFICADOR:
        {
            // Buscamos la dirección en la tabla de símbolos
            Simbolo* sym = ts_buscar(nodo->data.cadena);
            if (sym) {
                emitir_arg(OP_LOAD, sym->direccion);
            } else {
                fprintf(stderr, "Error CodeGen: Variable '%s' no encontrada.\n", nodo->data.cadena);
            }
            break;
        }

        case NODO_ASIGNACION:
        {
            // 1. Generar código para la expresión derecha (RHS) -> Deja resultado en pila
            generar_nodo(nodo->hijo2);
            
            // 2. Buscar dirección de la variable izquierda (LHS)
            char* nombre = nodo->hijo1->data.cadena;
            Simbolo* sym = ts_buscar(nombre);
            if (sym) {
                emitir_arg(OP_STORE, sym->direccion);
            }
            break;
        }

        case NODO_BINARIO_OP:
        {
            // Post-orden: Izq, Der, Operador
            generar_nodo(nodo->hijo1);
            generar_nodo(nodo->hijo2);
            
            char* op = nodo->data.op_unario;
            if (strcmp(op, "+") == 0) emitir(OP_ADD);
            else if (strcmp(op, "-") == 0) emitir(OP_SUB);
            else if (strcmp(op, "*") == 0) emitir(OP_MUL);
            else if (strcmp(op, "/") == 0) emitir(OP_DIV);
            else if (strcmp(op, "&&") == 0) emitir(OP_AND);
            else if (strcmp(op, "||") == 0) emitir(OP_OR);
            else if (strcmp(op, "==") == 0) emitir(OP_EQ);
            else if (strcmp(op, "!=") == 0) emitir(OP_NEQ);
            else if (strcmp(op, ">") == 0) emitir(OP_GT);
            else if (strcmp(op, "<") == 0) emitir(OP_LT);
            // ... agregar el resto
            break;
        }
        
        case NODO_LLAMADA_FUNCION:
        {
            // Verificamos si es función reservada del robot
            if (nodo->hijo1->tipo == NODO_FUNCION_RESERVADA) {
                char* func = nodo->hijo1->data.cadena;
                if (strcmp(func, "mover") == 0) emitir(OP_MOVER);
                else if (strcmp(func, "parar") == 0) emitir(OP_PARAR);
                else if (strcmp(func, "girarIzq") == 0) emitir(OP_GIRAR_IZQ);
                else if (strcmp(func, "girarDer") == 0) emitir(OP_GIRAR_DER);
                else if (strcmp(func, "reversa") == 0) emitir(OP_REVERSA);
                else if (strcmp(func, "leerSensor") == 0) emitir(OP_LEER_SENSOR);
            }
            // Funciones de usuario pendientes (requiere saltos)
            break;
        }
        
        /* --- ESTRUCTURAS DE CONTROL (IF/WHILE) --- */
        /* NOTA: Escribir saltos en un archivo de texto secuencial es complejo
           porque no sabemos la línea de destino todavía.
           Por ahora, los omitimos para verificar aritmética y variables primero.
        */
        case NODO_IF:
        case NODO_WHILE:
        case NODO_FOR:
            printf("Advertencia: Generacion de IF/WHILE/FOR aun no implementada en archivo de texto.\n");
            break;

        default:
            // Ignorar otros nodos por ahora
            break;
    }
}

void generar_codigo(ASTNode* raiz, const char* nombre_archivo) {
    archivo_salida = fopen(nombre_archivo, "w");
    if (!archivo_salida) {
        fprintf(stderr, "Error: No se pudo crear el archivo de salida '%s'\n", nombre_archivo);
        return;
    }

    printf("\n--- Generando Codigo Intermedio ---\n");
    generar_nodo(raiz);
    printf("--- Codigo Generado en '%s' ---\n", nombre_archivo);

    fclose(archivo_salida);
}