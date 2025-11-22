# Compilador TEO

Compilador para el lenguaje TEO que realiza análisis léxico, sintáctico y semántico del código fuente.

## Estructura del Proyecto

```
TEO-codes/
├── src/                    # Código fuente del compilador
│   ├── main.c             # Punto de entrada principal
│   ├── ast/               # Módulo del Árbol Sintáctico Abstracto
│   │   ├── ast.c
│   │   └── ast.h
│   ├── lexer/             # Analizador Léxico
│   │   └── lexer.l
│   ├── parser/            # Analizador Sintáctico
│   │   └── parser.y
│   └── semantic/          # Analizador Semántico
│       ├── semantic.c
│       └── semantic.h
├── tests/                 # Archivos de prueba
│   ├── test.txt
│   └── test_auxiliares.txt
├── build/                 # Archivos generados (creado automáticamente)
│   ├── *.o               # Archivos objeto
│   └── ...
└── Makefile              # Archivo de construcción

```

## Módulos

### Analizador Léxico (`src/lexer/`)
- **lexer.l**: Define las reglas para reconocer tokens del lenguaje fuente
- Generado por Flex

### Analizador Sintáctico (`src/parser/`)
- **parser.y**: Define la gramática del lenguaje y construye el AST
- Generado por Bison
- Genera: `parser.tab.c` y `parser.tab.h`

### Árbol Sintáctico Abstracto (`src/ast/`)
- **ast.h**: Definiciones de estructuras y tipos de nodos del AST
- **ast.c**: Implementación de funciones para crear y manipular el AST

### Analizador Semántico (`src/semantic/`)
- **semantic.h**: Definiciones de la tabla de símbolos y funciones de análisis
- **semantic.c**: Implementación del análisis semántico (verificación de tipos, declaraciones, etc.)

## Compilación

```bash
# Compilar el proyecto
make

# Limpiar archivos generados
make clean

# Compilar y ejecutar con archivo de prueba
make run

# Compilar y ejecutar con archivo auxiliar
make run-aux
```

## Uso

```bash
./mi_compilador <archivo_fuente>
```

El compilador realiza las siguientes fases:
1. **Análisis Léxico**: Convierte el código fuente en tokens
2. **Análisis Sintáctico**: Construye el Árbol Sintáctico Abstracto (AST)
3. **Análisis Semántico**: Verifica tipos, declaraciones y uso correcto de variables/funciones

## Requisitos

- GCC (compilador C)
- Flex (generador de analizadores léxicos)
- Bison (generador de analizadores sintácticos)

## Notas

- Los nombres de archivos y carpetas están en inglés
- Los comentarios en el código están en español
- Los archivos generados se crean en el directorio `build/`

