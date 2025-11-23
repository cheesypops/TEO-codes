# Contexto para Cursor – Proyecto TEO: Lenguaje + VM en ESP32

Este archivo sirve como **contexto de diseño** para trabajar en el proyecto TEO con Cursor.

La idea es que, al modificar o generar código, siempre se respete lo siguiente:

- Qué ya existe en el repo.
- Qué queremos lograr (lenguaje propio + VM en ESP32).
- Qué formato exacto tendrá el archivo de instrucciones que va en la microSD.
- Qué piezas nuevas hay que crear y cómo se conectan.

---

## 1. Objetivo general

El proyecto define un **lenguaje propio**, especificado en:

- `src/lexer/lexer.l`
- `src/parser/parser.y`
- `gramatica.txt` (referencia teórica de la gramática)

La meta es poder:

1. Compilar programas escritos en este lenguaje en la PC.
2. Generar un **archivo de bytecode en texto plano**.
3. Copiar ese archivo a la **microSD** de un carro basado en **ESP32**.
4. Ejecutar ese bytecode con una **máquina virtual basada en pila** que corre en el ESP32 y controla el carro (motores, giros, sensores, etc.).

La prioridad actual es el **backend del compilador en la PC**: tomar el AST ya existente y generar el archivo de bytecode con el formato acordado.

La VM en el ESP32 se implementará después, siguiendo este contrato de formato de archivo e instrucciones.

---

## 2. Estructura actual del repo

Estructura real del proyecto:

```text
TEO-codes/
├── src/
│   ├── main.c
│   ├── ast/
│   │   ├── ast.c
│   │   └── ast.h
│   ├── lexer/
│   │   └── lexer.l
│   ├── parser/
│   │   └── parser.y
│   └── semantic/
│       ├── semantic.c
│       └── semantic.h
├── tests/
│   ├── test.txt
│   └── test_auxiliares.txt
├── build/
│   ├── ast/
│   ├── parser/
│   ├── lexer/
│   └── semantic/
├── Makefile
├── README.md
├── plan.md
└── gramatica.txt
```

Notas:

- `gramatica.txt` es la descripción de la gramática del lenguaje (BNF u otra notación).  
  `parser.y` debe ser coherente con este archivo.
- `build/` se genera automáticamente al compilar (objetos `.o`, archivos intermedios de Flex/Bison). No es lugar para lógica manual.
- Los archivos fuente “reales” viven en `src/`.

---

## 3. Qué ya existe (frontend del compilador)

### 3.1. Lexer – `src/lexer/lexer.l`

- Definido con Flex.
- Reconoce tokens: identificadores, números, palabras reservadas, operadores, símbolos, etc.
- Es la entrada del parser Bison.

### 3.2. Parser – `src/parser/parser.y`

- Definido con Bison.
- Implementa la gramática del lenguaje de acuerdo a `gramatica.txt`.
- Construye un **AST** usando las funciones de `ast.c`.
- Soporta, a grandes rasgos:
  - declaraciones de variables,
  - declaraciones de funciones,
  - expresiones aritméticas y lógicas,
  - control de flujo (`if`, `while`, etc.),
  - funciones reservadas pensadas para el carro:
    - `mover`, `girarIzq`, `girarDer`, `reversa`, `leerSensor`, `parar`, etc.

### 3.3. AST – `src/ast/ast.h`, `src/ast/ast.c`

- Define la estructura `ASTNode` y un enum con tipos de nodo
  (número, identificador, llamada a función, asignación, if, while, bloque, etc.).
- Cada nodo tiene típicamente:
  - `tipo` (enum),
  - punteros a `hijo1`, `hijo2` y `siguiente`,
  - algún campo de datos (número, cadena, operador, etc.).
- Proporciona funciones para:
  - crear nodos,
  - recorrer/imprimir el árbol (para depurar),
  - liberar memoria del AST.

Este AST es la estructura base que usará la nueva fase de **generación de bytecode**.

### 3.4. Semántico – `src/semantic/semantic.h`, `src/semantic/semantic.c`

- Tabla de símbolos con:
  - variables,
  - funciones definidas por el usuario,
  - funciones reservadas.
- Fases principales:
  1. **Recolección de símbolos** – registra declaraciones y construye la tabla de símbolos.
  2. **Chequeo de cuerpos** – verifica usos de variables, llamadas a funciones, tipos básicos, y el uso adecuado de funciones reservadas.

La tabla de símbolos y su información (por ejemplo, índices de variables) deben reutilizarse para la fase de **codegen**, especialmente para decidir qué índice numérico se asigna a cada variable en el bytecode.

### 3.5. `src/main.c`

- Configura y ejecuta el lexer y el parser.
- Construye el AST.
- Llama a la fase semántica.
- Imprime el árbol (para debug) y lo libera.
- Actualmente **no** genera archivo de bytecode ni invoca ninguna VM.

---

## 4. Arquitectura deseada (backend + VM)

### 4.1. Flujo general

1. En la PC (compilador):
   - `lexer` + `parser` → AST.
   - `semantic` → verifica el programa.
   - Nueva fase `codegen` → recorre el AST y genera un archivo de bytecode en texto plano, por ejemplo `program.vmcode`.

2. En el ESP32:
   - Se copia `program.vmcode` a la microSD.
   - Una VM basada en pila abre y carga ese archivo.
   - La VM ejecuta instrucción por instrucción y llama a primitivas para controlar el carro.

### 4.2. Arquitectura de la VM (conceptual)

La VM tiene:

- **Pila de operandos** (enteros al inicio).
- **Arreglo de variables** indexado por entero (`0, 1, 2, …`).
  - La asignación de índices se decide en el compilador usando la tabla de símbolos.
- **Program counter (PC)**: índice de la instrucción actual.
- Conjunto fijo de instrucciones (mnemonics) que:
  - manipulan la pila,
  - leen/escriben variables,
  - saltan condicional o incondicionalmente,
  - ejecutan primitivas del carro,
  - terminan el programa.

La VM del ESP32 todavía no está escrita, pero el diseño de sus instrucciones ya está definido aquí y es el contrato que el compilador debe respetar.

---

## 5. Formato del archivo de bytecode (`.vmcode`)

El compilador en la PC generará un archivo de **texto plano**, sin comentarios y sin índices de instrucción.

**Formato exacto de cada línea:**

```text
<MNEMONIC> <arg1> <arg2>
```

Donde:

- `<MNEMONIC>` es una palabra en mayúsculas sin espacios (por ejemplo `LOADI`, `ADD`, `CALL_MOVER`).
- `<arg1>` y `<arg2>` son enteros.  
  Si una instrucción no usa uno de los argumentos, se pone `0` en su lugar.
- Cada línea contiene exactamente **una** instrucción.
- El **orden de las líneas** es el orden de ejecución del programa.
- El program counter vive solo dentro de la VM (por ejemplo, `pc = 0` al inicio, se incrementa o salta según el opcode).

Ejemplo conceptual de programa muy simple:

```text
LOADI 10 0
CALL_GIRAR_IZQ 0 0
CALL_LEER_SENSOR 0 0
CALL_PARAR 0 0
HALT 0 0
```

Este formato es el contrato que debe respetar todo el código de generación en la PC y que la VM en el ESP32 debe interpretar.

No se escriben comentarios dentro de este archivo.

---

## 6. Conjunto de instrucciones (diseño)

La VM es de **arquitectura de pila**: casi todo entra y sale de una pila de operandos.

### 6.1. Pila y constantes

Instrucciones base para manejar constantes:

- `LOADI v 0`  
  Empuja a la pila el entero `v`.

Opcionales para etapas posteriores (no obligatorios al inicio, pero compatibles con el diseño):

- `POP 0 0` – descarta el tope de la pila.
- `DUP 0 0` – duplica el tope de la pila.

### 6.2. Variables

Las variables del lenguaje se representan como posiciones en un arreglo interno de la VM:

- Cada variable tiene un índice entero `i` asignado por el compilador (usando la tabla de símbolos).
- La VM tiene un arreglo `vars[i]` con el valor actual de cada variable.

Instrucciones:

- `LOAD i 0`  
  Empuja a la pila el valor de la variable con índice `i`.

- `STORE i 0`  
  Saca el tope de la pila y lo guarda en la variable `i`.

### 6.3. Aritmética y lógica

Operaciones binarias sobre la pila:

- `ADD 0 0`
- `SUB 0 0`
- `MUL 0 0`
- `DIV 0 0`

Comparaciones (resultado 0 o 1 en la pila):

- `EQ 0 0`
- `NEQ 0 0`
- `LT 0 0`
- `GT 0 0`
- `LE 0 0`
- `GE 0 0`

Operadores lógicos (0 = falso, 1 = verdadero):

- `AND 0 0`
- `OR 0 0`
- `NOT 0 0`

### 6.4. Control de flujo

Los saltos usan como argumento el **índice de instrucción** (número de línea, empezando desde 0) dentro del programa ya cargado.

- `JMP destino 0`  
  Salto incondicional a la instrucción con índice `destino`.

- `JZ destino 0`  
  Toma el tope de la pila; si es 0, salta a `destino`.  
  Si no, continúa con la siguiente instrucción.

Finalización:

- `HALT 0 0`  
  Termina la ejecución del programa.

El compilador es responsable de calcular los `destino` correctos según la posición de cada instrucción, haciendo el backpatching necesario para `if`, `while`, `for`, etc.

### 6.5. Primitivas del carro / ESP32

El lenguaje tiene funciones reservadas pensadas para el carro, por ejemplo:

- `mover(...)`
- `girarIzq(...)`
- `girarDer(...)`
- `reversa(...)`
- `leerSensor()`
- `parar()`

Estas se traducen a mnemonics específicos en el bytecode. Diseño actual:

- `CALL_MOVER 0 0`
- `CALL_GIRAR_IZQ 0 0`
- `CALL_GIRAR_DER 0 0`
- `CALL_REVERSA 0 0`
- `CALL_LEER_SENSOR 0 0`
- `CALL_PARAR 0 0`

La semántica esperada (lado VM) es:

- `CALL_MOVER` usa el tope de la pila como parámetro (distancia, tiempo, etc.).
- `CALL_GIRAR_IZQ` usa el tope de la pila como ángulo.
- `CALL_GIRAR_DER` y `CALL_REVERSA` se comportan de forma análoga con sus parámetros.
- `CALL_LEER_SENSOR` lee el sensor y empuja el valor a la pila.
- `CALL_PARAR` detiene el carro, sin tocar la pila.

La correspondencia con el lenguaje fuente es directa; por ejemplo:

- `girarIzq(10);`  
  → el compilador genera:
  - evaluación de `10` (usando `LOADI`), y luego
  - `CALL_GIRAR_IZQ 0 0`.

- `parar();`  
  → el compilador genera solo `CALL_PARAR 0 0`.

### 6.6. Funciones de usuario (para más adelante)

El lenguaje admite funciones definidas por el usuario.  
En la primera versión del sistema se puede asumir incluso un solo flujo lineal (como una “función main”), pero el diseño futuro prevé:

- Soporte de múltiples funciones en el bytecode (módulo con varias secciones o un encabezado).
- Instrucciones como:
  - `CALL_FUNC idx 0`
  - `RET 0 0`
- Pila de llamadas (frames) en la VM.

Por ahora, el foco está en generar un flujo lineal único con control de flujo (`if`, `while`, etc.) y primitivas del carro.

---

## 7. Nuevas piezas a implementar (PC)

### 7.1. Nuevo módulo: `codegen`

Se añadirá un módulo para la generación de bytecode a partir del AST, por ejemplo:

```text
src/
  codegen/
    codegen.c
    codegen.h
```

Responsabilidades de `codegen`:

- Recorrer el AST ya verificado por el semántico.
- Emitir líneas de bytecode en el formato:

  ```text
  MNEMONIC arg1 arg2
  ```

- Resolver:
  - expresiones (generando las instrucciones de pila),
  - asignaciones (usando índices de variables de la tabla de símbolos),
  - llamadas a funciones reservadas,
  - control de flujo con saltos (`JMP`, `JZ`),
  - y al final, añadir `HALT 0 0`.

Punto de entrada esperado:

- `generar_bytecode(ASTNode *raiz, FILE *out);`

Internamente se esperarán helpers del estilo:

- `gen_programa(...)`
- `gen_bloque(...)`
- `gen_instruccion(...)`
- `gen_expresion(...)`
- `gen_llamada(...)`

La forma de recorrer el árbol debe ser coherente con:
- cómo se imprime actualmente el AST, y
- cómo recorre el semántico las estructuras de control.

### 7.2. Integración en `src/main.c`

`main.c` deberá integrar la fase de generación de bytecode con este flujo general:

1. Ejecutar lexer/parser para construir el AST.
2. Ejecutar análisis semántico.
   - Si hay errores, **no** generar bytecode.
3. Si el análisis semántico fue exitoso:
   - abrir un archivo de salida, por ejemplo `program.vmcode`,
   - llamar a `generar_bytecode(raiz_ast, archivo_salida)`,
   - cerrar el archivo.
4. Liberar el AST y terminar.

El resultado es que al compilar un programa de prueba (`tests/*.txt`), aparezca en la raíz del proyecto (o en `build/`) un archivo `.vmcode` listo para copiar a la SD.

### 7.3. Uso de `gramatica.txt`

`gramatica.txt` debe seguir siendo la referencia principal de cómo se ve el lenguaje a nivel teórico.  
Cualquier cambio grande en el parser (`parser.y`) o en el `codegen` debe ser coherente con:

- las construcciones descritas en `gramatica.txt`,
- la forma en que el AST representa esas construcciones,
- y la semántica que deseamos en el bytecode.

---

## 8. Plan futuro para la VM en ESP32 (resumen)

Aunque la implementación de la VM se hará después, su diseño ya está marcado por este documento:

- Leer el archivo `.vmcode` desde la SD.
- Guardar cada instrucción (`MNEMONIC`, `arg1`, `arg2`) en memoria.
- Ejecutar en un bucle controlado por un program counter (PC).
- Mantener:
  - una pila de enteros,
  - un arreglo de variables,
  - las primitivas del carro (`CALL_*`) implementadas con funciones de C++/Arduino.

Mientras se trabaje en el compilador y el codegen en la PC, hay que asegurarse de que:

- el archivo que se genere respeta el formato acordado,
- las instrucciones utilizadas pertenecen al set descrito arriba,
- y el comportamiento esperado se puede implementar con una VM de pila razonable en el ESP32.

---

## 9. Resumen rápido para Cursor

Cuando Cursor genere o modifique código en este repo, debe:

- Respetar la estructura de directorios mostrada arriba.
- Tratar `gramatica.txt` como referencia conceptual del lenguaje.
- No cambiar el contrato del archivo de bytecode:
  - una instrucción por línea,
  - formato `MNEMONIC arg1 arg2`,
  - sin comentarios en el archivo,
  - sin índice al inicio de cada línea.
- Mantener el modelo de VM como **pila + arreglo de variables + saltos por índice de instrucción**.
- Colocar la lógica de generación de bytecode en un módulo dedicado (`codegen`) y conectar esa fase desde `main.c` después del análisis semántico.

Con esto, todo lo que se genere debería encajar bien en la arquitectura del proyecto TEO.
