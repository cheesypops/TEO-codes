## Lenguaje y VM para carro seguidor de línea (ESP32)

Este repositorio contiene un **lenguaje tipo C/C++**, su **compilador a
bytecode** y una **máquina virtual en Arduino/ESP32** para controlar un carro
seguidor / evita–líneas con ESP32 y micro‑SD.

- **Idea principal**: escribir programas en un lenguaje sencillo (similar a
  C++), con algunas **funciones reservadas** para mover el carro, y compilar
  esos programas a un **bytecode** que luego ejecuta la VM en el ESP32.

---

## Estructura del proyecto

- **`compiler/`** – Compilador del lenguaje

  - **`src/`**
    - `main.c` – Punto de entrada del compilador.
    - `lexer.l` – Reglas de **Flex** (análisis léxico).
    - `parser.y` – Gramática de **Bison** (análisis sintáctico).
    - `gramatica.txt` – Descripción de la gramática a alto nivel.
    - `ast/ast.{h,c}` – Definición y manejo del **AST** (árbol sintáctico
      abstracto).
    - `semantic/semantic.{h,c}` – **Análisis semántico** y tabla de símbolos.
    - `codegen/codegen.{h,c}` – **Generador de bytecode** (`codigo.txt`).
    - `Makefile` – Script de compilación del compilador.
  - **`tests/`**
    - `test.txt`, `testAuxiliares.txt` – Programas de ejemplo en el lenguaje.
  - **`build/`** (generado por `make`, no editar a mano)
    - `bin/mi_compilador` – Ejecutable del compilador.
    - `gen/` – Archivos generados por Bison/Flex:
      - `parser.tab.c`, `parser.tab.h`, `lexer.yy.c`.
    - `obj/` – Archivos objeto organizados por módulo.

- **`vm/`** – Máquina virtual y código del carro (PlatformIO + Arduino)
  - **`arduino/`**
    - `platformio.ini` – Configuración de PlatformIO para el ESP32.
    - `src/main.ino` – Implementación de la **VM** y lógica del carro:
      - Carga `/codigo.txt` desde la micro‑SD.
      - Interpreta el bytecode (pila, memoria, saltos).
      - Llama a funciones de movimiento y lectura de sensores.
    - `include/`, `lib/`, `test/`, `README.md` – Estructura estándar de
      PlatformIO.

---

## Flujo completo de compilación y ejecución

### 1. Escribir un programa en el lenguaje

Ejemplo (muy simplificado) de código fuente en un `.txt`:

```c
int velocidad = 100;

void setup() {
    mover();
    esperar(1000);
    parar();
}
```

- El archivo puede guardarse en `compiler/tests/` (por ejemplo `programa.txt`) o
  en cualquier ruta que se desee pasar al compilador.

### 2. Compilar el lenguaje en PC

Desde la carpeta `compiler/src`:

```bash
cd compiler/src
make          # compila el compilador y genera build/bin/mi_compilador
```

Para compilar un programa de ejemplo:

```bash
./../build/bin/mi_compilador ../tests/test.txt
```

o, usando la regla `run` del Makefile (que usa `tests/test.txt` por defecto):

```bash
make run
```

Al finalizar, el compilador generará un archivo **`codigo.txt`** (en la carpeta
`compiler/src`), que contiene el **bytecode** en formato texto, una instrucción
por línea:

```text
CONST 100
MOVER
DELAY
PARAR
HALT
```

- **Importante**: este `codigo.txt` es el archivo que debe copiarse a la
  **micro‑SD** del carro (normalmente en la raíz como `/codigo.txt`).

### 3. Copiar `codigo.txt` a la micro‑SD

- El archivo `compiler/src/codigo.txt` debe copiarse a la tarjeta micro‑SD que
  utilizará el ESP32.
- Es necesario que el nombre del archivo sea exactamente **`codigo.txt`** y que
  se encuentre en la **raíz** de la SD (el `main.ino` lo busca como
  `/codigo.txt`).

### 4. Cargar y ejecutar en el ESP32 (VM)

En la carpeta `vm/arduino`:

```bash
cd vm/arduino
platformio run                # compilar para ESP32
platformio run --target upload  # subir al ESP32
```

Una vez que el ESP32 arranca:

- La VM:
  - Inicializa la SD.
  - Lee `/codigo.txt` línea por línea.
  - Traduce cada instrucción textual a un opcode entero.
  - Ejecuta el programa usando una pila y memoria interna.
- Las instrucciones de bytecode mapean a acciones del carro:
  - `MOVER`, `PARAR`, `GIRAR_IZQ`, `GIRAR_DER`, `REVERSA`, `DELAY`,
    `LEER_SENSOR`, etc.

---

## Componentes del compilador (resumen técnico)

- **Análisis léxico (`lexer.l`)**

  - Reconoce:
    - Tipos: `int`, `boolean`, `float`, `char`, `string`.
    - Control: `if`, `else`, `for`, `while`, `do`, `void`, `setup`.
    - **Funciones reservadas del robot**:
      - `mover`, `girarIzq`, `girarDer`, `leerSensor`, `parar`, `reversa`,
        `esperar`.
  - Genera tokens para Bison (`parser.y`).

- **Análisis sintáctico (`parser.y`)**

  - Construye un **AST** usando las funciones de `ast/ast.c`.
  - Soporta:
    - Declaraciones y asignaciones.
    - `if/else`, `for`, `while`, `do/while`.
    - Funciones (`TipoRetorno id (params) { ... }`) y `void setup()`.
    - Expresiones aritméticas, lógicas y de comparación con precedencias
      correctas.

- **Análisis semántico (`semantic/semantic.c`)**

  - Tabla de símbolos con:
    - Nombre, tipo, ámbito, línea y **dirección de memoria virtual**.
  - Dos fases:
    - Recolección de símbolos globales y encabezados de funciones.
    - Análisis de cuerpos (tipos de expresiones, argumentos, etc.).
  - Valida el uso correcto de las funciones reservadas del robot:
    - Sin argumentos (`mover()`, `parar()`, `girarIzq()`, etc.).
    - `esperar(ms)` con 1 argumento numérico.
    - `leerSensor()` que devuelve `int`.

- **Generación de código (`codegen/codegen.c`)**
  - Recorre el AST y emite bytecode para una **máquina de pila**:
    - Operaciones aritméticas y lógicas.
    - Saltos condicionales y bucles.
    - Acceso a variables usando direcciones de la tabla de símbolos.
    - Llamadas a funciones reservadas → opcodes de la VM (mover, parar, etc.).
  - Escribe el bytecode legible en `codigo.txt`.

---

## Notas y buenas prácticas

- **No se debe editar nada dentro de `build/` a mano**:
  - Todo su contenido se genera y limpia usando `make clean`.
- Si se modifica la gramática (`parser.y`) o el lexer (`lexer.l`), basta con
  ejecutar:

```bash
cd compiler/src
make clean
make
```

- Los warnings del compilador C (por ejemplo, sobre extensiones C23) no afectan
  la lógica principal, pero pueden revisarse si se desea un código C más
  estricto.