/* ==========================================
   VM ROBOT ESP32 - SOPORTE DE FUNCIONES
   ========================================== */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// --- CONFIGURACIÓN DE HARDWARE ---
#define PIN_SD_CS 21
#define ARCHIVO_PROGRAMA "/codigo.txt"

// Pines Sensores
const int pinSensorIzquierdo = 32;
const int pinSensorDerecho = 33;

// Pines Motores
const int pinENA = 25; const int pinIN1 = 26; const int pinIN2 = 27;
const int pinENB = 13; const int pinIN3 = 14; const int pinIN4 = 12;

// PWM
const int freqPWM = 25000;
const int canalPWM_A = 0;
const int canalPWM_B = 1;
const int resolucionPWM = 8;
const int velocidadBase = 175;
const int velocidadGiro = 170;

/* --- 1. OPCODES (Deben coincidir con codegen.c) --- */
enum Opcode {
    OP_PUSH, OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GE, OP_LE,
    OP_AND, OP_OR, OP_NOT,
    OP_LOAD, OP_STORE,
    OP_JMP, OP_JMP_FALSE,
    OP_CALL, OP_RET, 
    OP_MOVER, OP_GIRAR_IZQ, OP_GIRAR_DER, OP_LEER_SENSOR, OP_ESPERAR, OP_PARAR,
    OP_HALT
};

/* --- 2. MEMORIA DE LA VM --- */
#define MAX_CODE 512
#define STACK_SIZE 64
#define MEM_SIZE 100
#define CALL_STACK_SIZE 32 // Profundidad máxima de llamadas

byte progOps[MAX_CODE];
int  progArgs[MAX_CODE];
int  progSize = 0;

int stack[STACK_SIZE];     // Pila de Datos
int sp = 0;
int memory[MEM_SIZE];      // Variables

int returnStack[CALL_STACK_SIZE]; // Pila de Retorno (Call Stack)
int rp = 0;                       // Puntero de retorno

int ip = 0;                // Instruction Pointer
bool running = false;

/* --- PROTOTIPOS DE HARDWARE --- */
void avanzar(int vI, int vD);
void detenerse();
void soloMotorDerecho(int v);
void soloMotorIzquierdo(int v);

/* --- FUNCIONES AUXILIARES VM --- */
void push(int v) { if(sp < STACK_SIZE) stack[sp++] = v; }
int pop() { return (sp > 0) ? stack[--sp] : 0; }

void loadProgram(const char* fname) {
    File f = SD.open(fname);
    if(!f) { Serial.println("Error SD"); return; }
    progSize = 0;
    while(f.available() && progSize < MAX_CODE) {
        String line = f.readStringUntil('\n'); line.trim();
        if(line.length()==0) continue;
        int spIdx = line.indexOf(' ');
        String opS = (spIdx==-1)?line:line.substring(0,spIdx);
        int arg = (spIdx==-1)?0:line.substring(spIdx+1).toInt();
        
        byte op=0; // Mapeo simple
        if(opS=="PUSH") op=OP_PUSH; else if(opS=="ADD") op=OP_ADD;
        else if(opS=="SUB") op=OP_SUB; else if(opS=="MUL") op=OP_MUL;
        else if(opS=="EQ") op=OP_EQ; else if(opS=="NEQ") op=OP_NEQ;
        else if(opS=="GT") op=OP_GT; else if(opS=="LT") op=OP_LT;
        else if(opS=="LOAD") op=OP_LOAD; else if(opS=="STORE") op=OP_STORE;
        else if(opS=="JMP") op=OP_JMP; else if(opS=="JMP_FALSE") op=OP_JMP_FALSE;
        else if(opS=="CALL") op=OP_CALL; else if(opS=="RET") op=OP_RET; // <---
        else if(opS=="MOVER") op=OP_MOVER; else if(opS=="ESPERAR") op=OP_ESPERAR;
        else if(opS=="GIRAR_IZQ") op=OP_GIRAR_IZQ; else if(opS=="GIRAR_DER") op=OP_GIRAR_DER;
        else if(opS=="LEER_SENSOR") op=OP_LEER_SENSOR; else if(opS=="PARAR") op=OP_PARAR;
        else if(opS=="HALT") op=OP_HALT;

        if(op!=0 || opS=="PUSH") { // Op 0 suele ser valido si es el primero del enum, cuidado
             progOps[progSize] = op; progArgs[progSize] = arg; progSize++;
        }
    }
    f.close();
    Serial.printf("Programa cargado: %d instrucciones\n", progSize);
    running = true;
}

void setup() {
    Serial.begin(115200);
    
    // Config Pines
    pinMode(pinSensorIzquierdo, INPUT); pinMode(pinSensorDerecho, INPUT);
    pinMode(pinIN1, OUTPUT); pinMode(pinIN2, OUTPUT);
    pinMode(pinIN3, OUTPUT); pinMode(pinIN4, OUTPUT);
    
    ledcSetup(canalPWM_A, freqPWM, resolucionPWM);
    ledcSetup(canalPWM_B, freqPWM, resolucionPWM);
    ledcAttachPin(pinENA, canalPWM_A); ledcAttachPin(pinENB, canalPWM_B);
    
    detenerse();

    if(!SD.begin(PIN_SD_CS)) { Serial.println("Fallo SD"); return; }
    loadProgram(ARCHIVO_PROGRAMA);
}

void loop() {
    if(!running) return;
    if(ip < 0 || ip >= progSize) { detenerse(); running=false; return; }

    byte op = progOps[ip];
    int arg = progArgs[ip];
    int a, b;

    switch(op) {
        case OP_PUSH: push(arg); break;
        case OP_ADD: b=pop(); a=pop(); push(a+b); break;
        case OP_SUB: b=pop(); a=pop(); push(a-b); break;
        case OP_MUL: b=pop(); a=pop(); push(a*b); break;
        case OP_EQ: b=pop(); a=pop(); push(a==b); break;
        case OP_NEQ: b=pop(); a=pop(); push(a!=b); break;
        case OP_GT: b=pop(); a=pop(); push(a>b); break;
        case OP_LT: b=pop(); a=pop(); push(a<b); break;
        
        case OP_LOAD: if(arg<MEM_SIZE) push(memory[arg]); break;
        case OP_STORE: if(arg<MEM_SIZE) memory[arg]=pop(); break;

        // --- SALTOS Y FUNCIONES ---
        case OP_JMP: ip = arg - 1; break;
        case OP_JMP_FALSE: if(pop()==0) ip = arg - 1; break;
        
        case OP_CALL:
            if(rp < CALL_STACK_SIZE) {
                returnStack[rp++] = ip; // Guardamos posicion actual
                ip = arg - 1;           // Saltamos a la funcion
            } else {
                Serial.println("Stack Overflow (CALL)"); running=false;
            }
            break;
            
        case OP_RET:
            if(rp > 0) {
                ip = returnStack[--rp]; // Recuperamos posicion
                // No restamos 1 aqui, porque queremos ejecutar la SIGUIENTE instrucción
                // al CALL original. Como al final del loop hay ip++, si no hacemos nada,
                // ip pasará a la siguiente. Correcto.
            } else {
                Serial.println("Fin programa (Main RET)"); running=false;
            }
            break;

        // --- HARDWARE ---
        case OP_MOVER: 
            if(pop() > 0) avanzar(velocidadBase, velocidadBase); 
            else detenerse(); 
            break;
            
        case OP_GIRAR_IZQ: pop(); soloMotorDerecho(velocidadGiro); break;
        case OP_GIRAR_DER: pop(); soloMotorIzquierdo(velocidadGiro); break;
        
        case OP_LEER_SENSOR: 
            {
                int izq = digitalRead(pinSensorIzquierdo);
                int der = digitalRead(pinSensorDerecho);
                push((izq*2) + der); // 0=BB, 1=BN, 2=NB, 3=NN
            }
            break;
            
        case OP_ESPERAR: delay(pop()); break;
        case OP_PARAR: detenerse(); break;
        case OP_HALT: detenerse(); running=false; break;
    }
    ip++;
}

// --- IMPLEMENTACIÓN FÍSICA ---
void detenerse() {
    digitalWrite(pinIN1, LOW); digitalWrite(pinIN2, LOW); ledcWrite(canalPWM_A, 0);
    digitalWrite(pinIN3, LOW); digitalWrite(pinIN4, LOW); ledcWrite(canalPWM_B, 0);
}
void avanzar(int vi, int vd) {
    digitalWrite(pinIN1, HIGH); digitalWrite(pinIN2, LOW); ledcWrite(canalPWM_A, vd);
    digitalWrite(pinIN3, HIGH); digitalWrite(pinIN4, LOW); ledcWrite(canalPWM_B, vi);
}
void soloMotorDerecho(int v) { // Gira IZQ
    digitalWrite(pinIN1, HIGH); digitalWrite(pinIN2, LOW); ledcWrite(canalPWM_A, v);
    digitalWrite(pinIN3, LOW); digitalWrite(pinIN4, LOW); ledcWrite(canalPWM_B, 0);
}
void soloMotorIzquierdo(int v) { // Gira DER
    digitalWrite(pinIN1, LOW); digitalWrite(pinIN2, LOW); ledcWrite(canalPWM_A, 0);
    digitalWrite(pinIN3, HIGH); digitalWrite(pinIN4, LOW); ledcWrite(canalPWM_B, v);
}