#include "FS.h"
#include "SD.h"
#include "SPI.h"

/* ==========================================
   1. CONFIGURACIÓN DE HARDWARE
   ========================================== */
// --- Pines SD ---
#define PIN_SD_CS 21

// --- Pines Sensores IR ---
const int pinSensorIzquierdo = 32;
const int pinSensorDerecho = 33;

// --- Pines Motores (L298N) ---
const int pinENA = 25;
const int pinIN1 = 26;
const int pinIN2 = 27;
const int pinENB = 13;
const int pinIN3 = 14;
const int pinIN4 = 12;

// --- Configuración PWM ---
const int freqPWM = 25000;
const int canalPWM_A = 0;
const int canalPWM_B = 1;
const int resPWM = 8;
const int velocidadBase = 175;
const int velocidadGiro = 170;

/* ==========================================
   2. DEFINICIÓN DE LA MÁQUINA VIRTUAL
   ========================================== */

// Opcodes (Deben coincidir con codegen.h)
enum OpCode
{
  OP_HALT = 0,
  OP_CONST,
  OP_LOAD,
  OP_STORE,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_INC,
  OP_DEC,
  OP_NEG,
  OP_AND,
  OP_OR,
  OP_NOT,
  OP_EQ,
  OP_NEQ,
  OP_GT,
  OP_LT,
  OP_GTE,
  OP_LTE,
  OP_JMP,
  OP_JZ,
  OP_MOVER,
  OP_GIRAR_IZQ,
  OP_GIRAR_DER,
  OP_LEER_SENSOR,
  OP_PARAR,
  OP_REVERSA,
  OP_DELAY // <--- NUEVO
};

// Memoria de la VM (Enteros)
#define MAX_STACK 128
#define MAX_MEMORY 128
#define MAX_CODE 1024

int stack[MAX_STACK]; // Pila
int sp = 0;           // Puntero de pila

int memory[MAX_MEMORY]; // Variables globales
int prog[MAX_CODE];     // Memoria de programa
int progSize = 0;       // Tamaño
int pc = 0;             // Contador de programa

bool running = false; // Estado

/* ==========================================
   3. FUNCIONES FÍSICAS DEL ROBOT
   ========================================== */

void detenerse()
{
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
  ledcWrite(canalPWM_A, 0);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalPWM_B, 0);
}

void avanzar()
{
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  ledcWrite(canalPWM_A, velocidadBase);
  digitalWrite(pinIN3, HIGH);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalPWM_B, velocidadBase);
}

void reversa()
{
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  ledcWrite(canalPWM_A, velocidadBase);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalPWM_B, velocidadBase);
}

void girarIzquierda()
{
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
  ledcWrite(canalPWM_A, 0);
  digitalWrite(pinIN3, HIGH);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalPWM_B, velocidadGiro);
}

void girarDerecha()
{
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  ledcWrite(canalPWM_A, velocidadGiro);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalPWM_B, 0);
}

int leerSensores()
{
  int izq = digitalRead(pinSensorIzquierdo);
  int der = digitalRead(pinSensorDerecho);
  // 0: Blanco, 1: Negro. Retorna código:
  if (izq == 0 && der == 0)
    return 0;
  if (izq == 1 && der == 0)
    return 1;
  if (izq == 0 && der == 1)
    return 2;
  return 3;
}

/* ==========================================
   4. CARGADOR (TEXTO -> ENTEROS)
   ========================================== */

int stringToOpcode(String str)
{
  str.trim();
  if (str == "HALT")
    return OP_HALT;
  if (str == "CONST")
    return OP_CONST;
  if (str == "LOAD")
    return OP_LOAD;
  if (str == "STORE")
    return OP_STORE;
  if (str == "ADD")
    return OP_ADD;
  if (str == "SUB")
    return OP_SUB;
  if (str == "MUL")
    return OP_MUL;
  if (str == "DIV")
    return OP_DIV;
  if (str == "MOD")
    return OP_MOD;
  if (str == "INC")
    return OP_INC;
  if (str == "DEC")
    return OP_DEC;
  if (str == "NEG")
    return OP_NEG;
  if (str == "AND")
    return OP_AND;
  if (str == "OR")
    return OP_OR;
  if (str == "NOT")
    return OP_NOT;
  if (str == "EQ")
    return OP_EQ;
  if (str == "NEQ")
    return OP_NEQ;
  if (str == "GT")
    return OP_GT;
  if (str == "LT")
    return OP_LT;
  if (str == "GTE")
    return OP_GTE;
  if (str == "LTE")
    return OP_LTE;
  if (str == "JMP")
    return OP_JMP;
  if (str == "JZ")
    return OP_JZ;
  if (str == "MOVER")
    return OP_MOVER;
  if (str == "GIRAR_IZQ")
    return OP_GIRAR_IZQ;
  if (str == "GIRAR_DER")
    return OP_GIRAR_DER;
  if (str == "LEER_SENSOR")
    return OP_LEER_SENSOR;
  if (str == "PARAR")
    return OP_PARAR;
  if (str == "REVERSA")
    return OP_REVERSA;
  if (str == "DELAY")
    return OP_DELAY; // <--- NUEVO
  return -1;
}

void cargarCodigoDesdeSD()
{
  if (!SD.begin(PIN_SD_CS))
  {
    Serial.println("Error SD");
    return;
  }
  File file = SD.open("/codigo.txt");
  if (!file)
  {
    Serial.println("Error codigo.txt");
    return;
  }

  Serial.println("--- Cargando Programa ---");
  progSize = 0;

  while (file.available() && progSize < MAX_CODE)
  {
    // 1. Leer línea completa
    String linea = file.readStringUntil('\n');
    linea.trim();
    if (linea.length() == 0)
      continue;

    // 2. Separar instrucción
    int espacioIdx = linea.indexOf(' ');
    String opStr = (espacioIdx == -1) ? linea : linea.substring(0, espacioIdx);

    int opcode = stringToOpcode(opStr);
    if (opcode == -1)
      continue; // Ignorar desconocidos

    // 3. Guardar Opcode
    prog[progSize++] = opcode;

    // 4. Si requiere argumento, leerlo
    if (opcode == OP_CONST || opcode == OP_LOAD || opcode == OP_STORE ||
        opcode == OP_JMP || opcode == OP_JZ)
    {

      if (espacioIdx != -1)
      {
        String argStr = linea.substring(espacioIdx + 1);
        prog[progSize++] = argStr.toInt(); // <--- Usamos ENTEROS
      }
      else
      {
        prog[progSize++] = 0;
      }
    }
  }
  file.close();

  Serial.print("Instrucciones: ");
  Serial.println(progSize);

  sp = 0;
  pc = 0;
  running = true;
}

/* ==========================================
   5. CPU VIRTUAL
   ========================================== */

void ejecutarCiclo()
{
  if (!running || pc >= progSize)
  {
    if (running)
    {
      detenerse();
      Serial.println("Fin.");
    }
    running = false;
    return;
  }

  int opcode = prog[pc++];

  switch (opcode)
  {
  case OP_HALT:
    running = false;
    detenerse();
    break;

  // Pila y Memoria
  case OP_CONST:
    stack[sp++] = prog[pc++];
    break;
  case OP_LOAD:
    stack[sp++] = memory[prog[pc++]];
    break;
  case OP_STORE:
    memory[prog[pc++]] = stack[--sp];
    break;

  // Aritmética (Entera)
  case OP_ADD:
    sp--;
    stack[sp - 1] += stack[sp];
    break;
  case OP_SUB:
    sp--;
    stack[sp - 1] -= stack[sp];
    break;
  case OP_MUL:
    sp--;
    stack[sp - 1] *= stack[sp];
    break;
  case OP_DIV:
    sp--;
    if (stack[sp] != 0)
      stack[sp - 1] /= stack[sp];
    break;
  case OP_MOD:
    sp--;
    if (stack[sp] != 0)
      stack[sp - 1] %= stack[sp];
    break;
  case OP_INC:
    stack[sp - 1]++;
    break;
  case OP_DEC:
    stack[sp - 1]--;
    break;
  case OP_NEG:
    stack[sp - 1] = -stack[sp - 1];
    break;

  // Lógica
  case OP_AND:
    sp--;
    stack[sp - 1] = stack[sp - 1] && stack[sp];
    break;
  case OP_OR:
    sp--;
    stack[sp - 1] = stack[sp - 1] || stack[sp];
    break;
  case OP_NOT:
    stack[sp - 1] = !stack[sp - 1];
    break;

  // Comparación
  case OP_EQ:
    sp--;
    stack[sp - 1] = (stack[sp - 1] == stack[sp]);
    break;
  case OP_NEQ:
    sp--;
    stack[sp - 1] = (stack[sp - 1] != stack[sp]);
    break;
  case OP_GT:
    sp--;
    stack[sp - 1] = (stack[sp - 1] > stack[sp]);
    break;
  case OP_LT:
    sp--;
    stack[sp - 1] = (stack[sp - 1] < stack[sp]);
    break;
  case OP_GTE:
    sp--;
    stack[sp - 1] = (stack[sp - 1] >= stack[sp]);
    break;
  case OP_LTE:
    sp--;
    stack[sp - 1] = (stack[sp - 1] <= stack[sp]);
    break;

  // Saltos
  case OP_JMP:
    pc = prog[pc];
    break;
  case OP_JZ:
  {
    int target = prog[pc++];
    int val = stack[--sp];
    if (val == 0)
      pc = target;
  }
  break;

  // Robot y Control
  case OP_MOVER:
    avanzar();
    break;
  case OP_PARAR:
    detenerse();
    break;
  case OP_GIRAR_IZQ:
    girarIzquierda();
    break;
  case OP_GIRAR_DER:
    girarDerecha();
    break;
  case OP_REVERSA:
    reversa();
    break;

  case OP_LEER_SENSOR:
    stack[sp++] = leerSensores();
    break;

  case OP_DELAY:
  {                       // <--- NUEVO
    int ms = stack[--sp]; // Sacar tiempo de la pila
    delay(ms);
  }
  break;
  }
}

/* ==========================================
   6. MAIN ARDUINO
   ========================================== */

void setup()
{
  Serial.begin(9600);

  pinMode(pinSensorIzquierdo, INPUT);
  pinMode(pinSensorDerecho, INPUT);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);

  ledcSetup(canalPWM_A, freqPWM, resPWM);
  ledcSetup(canalPWM_B, freqPWM, resPWM);
  ledcAttachPin(pinENA, canalPWM_A);
  ledcAttachPin(pinENB, canalPWM_B);

  detenerse();
  delay(1000);

  cargarCodigoDesdeSD();
}

void loop()
{
  if (running)
  {
    ejecutarCiclo();
  }
}
