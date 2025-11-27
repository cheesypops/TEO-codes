# Makefile para el Compilador del Robot

# Nombre del ejecutable final
TARGET = compiler

# Compilador y Flags
CC = gcc
CFLAGS = -Wall -g

# Archivos fuente generados y estáticos
SOURCES = main.c ast.c semantic.c codegen.c y.tab.c lex.yy.c
HEADERS = ast.h semantic.h codegen.h y.tab.h

# Regla principal (por defecto)
all: $(TARGET)

# Paso 1: Generar el Parser con Bison (crea y.tab.c y y.tab.h)
y.tab.c y.tab.h: parser.y
	bison -d -y parser.y -Wcounterexamples

# Paso 2: Generar el Lexer con Flex (crea lex.yy.c)
lex.yy.c: lexer.l y.tab.h
	flex lexer.l

# Paso 3: Compilar todo el proyecto
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

# Regla para limpiar archivos generados
clean:
	rm -f $(TARGET) y.tab.c y.tab.h lex.yy.c codigo.txt

# Regla de prueba (opcional)
run: $(TARGET)
	./$(TARGET) test.txt