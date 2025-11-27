/* ==========================================
   codegen.c - Generador de Bytecode (CORREGIDO)
   ========================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "ast.h"

/* OPCODES */
typedef enum {
    OP_PUSH, OP_ADD, OP_SUB, OP_MUL, OP_DIV, 
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GE, OP_LE, 
    OP_AND, OP_OR, OP_NOT,
    OP_LOAD, OP_STORE, OP_JMP, OP_JMP_FALSE,
    OP_CALL, OP_RET,
    OP_MOVER, OP_GIRAR_IZQ, OP_GIRAR_DER, OP_LEER_SENSOR, OP_ESPERAR, OP_PARAR,
    OP_HALT
} Opcode;

const char* opcode_strings[] = {
    "PUSH", "ADD", "SUB", "MUL", "DIV", 
    "EQ", "NEQ", "GT", "LT", "GE", "LE", 
    "AND", "OR", "NOT",
    "LOAD", "STORE", "JMP", "JMP_FALSE",
    "CALL", "RET", 
    "MOVER", "GIRAR_IZQ", "GIRAR_DER", "LEER_SENSOR", "ESPERAR", "PARAR",
    "HALT"
};

typedef struct { Opcode op; int arg; } Instruction;
#define MAX_CODE 2048
static Instruction code_buffer[MAX_CODE];
static int pc = 0;

/* VARIABLES */
typedef struct { char name[32]; int address; } VarMap;
#define MAX_VARS 100
static VarMap vars[MAX_VARS];
static int var_count = 0;

int get_var_address(char *name) {
    for(int i=0; i<var_count; i++) if(strcmp(vars[i].name, name) == 0) return vars[i].address;
    if (var_count < MAX_VARS) {
        strcpy(vars[var_count].name, name);
        vars[var_count].address = var_count;
        var_count++; 
        return vars[var_count-1].address;
    }
    return -1;
}

/* FUNCIONES Y ETIQUETAS */
static int label_map[200]; 
static int label_counter = 0;

typedef struct { char name[32]; int label_id; } FuncMap;
static FuncMap functions[50];
static int func_count = 0;

void init_codegen() {
    pc = 0; var_count = 0; label_counter = 0; func_count = 0;
    for(int i=0; i<200; i++) label_map[i] = -1;
}

int new_label() { return label_counter++; }
void place_label(int lbl) { label_map[lbl] = pc; }

void register_func(char *name, int label) {
    strcpy(functions[func_count].name, name);
    functions[func_count].label_id = label;
    func_count++;
}

int get_func_label(char *name) {
    for(int i=0; i<func_count; i++) if(strcmp(functions[i].name, name) == 0) return functions[i].label_id;
    return -1;
}

void emit(Opcode op, int arg) {
    if (pc < MAX_CODE) { code_buffer[pc].op = op; code_buffer[pc].arg = arg; pc++; }
    else { fprintf(stderr, "Error: Buffer lleno.\n"); exit(1); }
}

void gen_node(ASTNode *node);
/* gen_list recorre la lista enlazada (next) */
void gen_list(ASTNode *node) { while(node) { gen_node(node); node = node->next; } }

void gen_node(ASTNode *node) {
    if (!node) return;
    int l1, l2;

    switch(node->kind) {
        /* En caso de que exista un nodo programa contenedor */
        case NODE_PROGRAM: gen_list(node->next); break;
        case NODE_BLOCK:   gen_list(node->next); break;

        case NODE_FUNCTION: 
        {
            int lbl_func = new_label();
            place_label(lbl_func);
            register_func(node->data.str_val, lbl_func);
            if (node->right) gen_node(node->right);
            emit(OP_RET, 0); 
        }
        break;

        case NODE_CALL_FUNC:
        {
            if (node->left) gen_list(node->left);
            int lbl = get_func_label(node->data.str_val);
            if (lbl == -1) {
                lbl = new_label();
                register_func(node->data.str_val, lbl);
            }
            emit(OP_CALL, lbl);
        }
        break;

        case NODE_VAR_DECL:
            if(node->left) { gen_node(node->left); emit(OP_STORE, get_var_address(node->data.str_val)); }
            break;
        case NODE_ASSIGN:
            gen_node(node->right);
            if(node->left && node->left->kind == NODE_ID) emit(OP_STORE, get_var_address(node->left->data.str_val));
            break;
        case NODE_ID: emit(OP_LOAD, get_var_address(node->data.str_val)); break;
        case NODE_CONST_INT: emit(OP_PUSH, node->data.int_val); break;

        case NODE_IF:
            l1 = new_label(); l2 = new_label();
            gen_node(node->left); emit(OP_JMP_FALSE, l1);
            gen_node(node->right); emit(OP_JMP, l2);
            place_label(l1);
            if(node->extra) gen_node(node->extra);
            place_label(l2);
            break;
        case NODE_WHILE:
            l1 = new_label(); l2 = new_label();
            place_label(l1);
            gen_node(node->left); emit(OP_JMP_FALSE, l2);
            gen_node(node->right); emit(OP_JMP, l1);
            place_label(l2);
            break;

        case NODE_CALL_ROBOT:
            if(node->left) gen_list(node->left);
            char *f = node->data.str_val;
            if(!strcmp(f,"mover")) emit(OP_MOVER, 0);
            else if(!strcmp(f,"girarIzq")) emit(OP_GIRAR_IZQ, 0);
            else if(!strcmp(f,"girarDer")) emit(OP_GIRAR_DER, 0);
            else if(!strcmp(f,"leerSensor")) emit(OP_LEER_SENSOR, 0);
            else if(!strcmp(f,"esperar")) emit(OP_ESPERAR, 0);
            else if(!strcmp(f,"parar")) emit(OP_PARAR, 0);
            break;

        case NODE_BIN_OP:
            gen_node(node->left); gen_node(node->right);
            char *op = node->data.str_val;
            if(!strcmp(op,"+")) emit(OP_ADD,0); else if(!strcmp(op,"-")) emit(OP_SUB,0);
            else if(!strcmp(op,"*")) emit(OP_MUL,0); else if(!strcmp(op,"/")) emit(OP_DIV,0);
            else if(!strcmp(op,"==")) emit(OP_EQ,0); else if(!strcmp(op,"!=")) emit(OP_NEQ,0);
            else if(!strcmp(op,">")) emit(OP_GT,0); else if(!strcmp(op,"<")) emit(OP_LT,0);
            else if(!strcmp(op,"&&")) emit(OP_AND,0); else if(!strcmp(op,"||")) emit(OP_OR,0);
            break;
            
        case NODE_RETURN:
            if(node->left) gen_node(node->left);
            emit(OP_RET, 0);
            break;

        default: break;
    }
}

void generate_code(ASTNode *node, FILE *fp) {
    init_codegen();
    
    /* RESERVAMOS INSTRUCCIÓN 0 PARA SALTO A MAIN */
    emit(OP_JMP, 0); 

    /* CORRECCIÓN CRÍTICA:
       El parser entrega 'node' como el INICIO de una lista enlazada (root = elementos).
       No es un nodo contenedor. Si usamos gen_node(node), solo procesa el primero.
       Debemos usar gen_list(node) para recorrer Variables -> Funciones -> Main. */
    
    gen_list(node);

    /* BACKPATCHING DEL MAIN */
    int main_lbl = get_func_label("main");
    if (main_lbl != -1) {
        code_buffer[0].arg = main_lbl; 
    } else {
        code_buffer[0].op = OP_HALT;
    }
    emit(OP_HALT, 0);

    /* RESOLUCIÓN Y ESCRITURA */
    for (int i = 0; i < pc; i++) {
        if (code_buffer[i].op == OP_JMP || code_buffer[i].op == OP_JMP_FALSE || code_buffer[i].op == OP_CALL) {
            int lbl_id = code_buffer[i].arg;
            int target = label_map[lbl_id];
            if (target != -1) code_buffer[i].arg = target;
        }
        fprintf(fp, "%s %d\n", opcode_strings[code_buffer[i].op], code_buffer[i].arg);
    }
    printf("Generacion completada. Instrucciones generadas: %d\n", pc);
}