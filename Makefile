# Makefile para el Compilador
# Organiza la compilación de todos los módulos del compilador

# Nombre del compilador C
CC = gcc
# Opciones del compilador (g = símbolos de depuración)
CFLAGS = -g -Wall -Wno-unused-function -Isrc/ast -Isrc/parser -Isrc/lexer -Isrc/semantic -Isrc/codegen

# Nombres de los ejecutables de Flex y Bison
FLEX = flex
BISON = bison

# Nombre del ejecutable final
TARGET = mi_compilador

# Directorios
SRC_DIR = src
AST_DIR = $(SRC_DIR)/ast
PARSER_DIR = $(SRC_DIR)/parser
LEXER_DIR = $(SRC_DIR)/lexer
SEMANTIC_DIR = $(SRC_DIR)/semantic
CODEGEN_DIR = $(SRC_DIR)/codegen
BUILD_DIR = build

# Archivos fuente .c (sin los generados)
SOURCES_C = $(SRC_DIR)/main.c \
            $(AST_DIR)/ast.c \
            $(SEMANTIC_DIR)/semantic.c \
            $(CODEGEN_DIR)/codegen.c

# Archivos generados por Bison y Flex
PARSER_GEN = $(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.tab.h
LEXER_GEN = $(LEXER_DIR)/lexer.yy.c

# Todos los archivos fuente .c
SOURCES = $(SOURCES_C) $(PARSER_GEN:.h=.c) $(LEXER_GEN)

# Archivos de cabecera .h
HEADERS = $(AST_DIR)/ast.h \
          $(SEMANTIC_DIR)/semantic.h \
          $(CODEGEN_DIR)/codegen.h \
          $(PARSER_DIR)/parser.tab.h

# Archivos objeto .o (en el directorio build)
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/ast/ast.o \
          $(BUILD_DIR)/semantic/semantic.o \
          $(BUILD_DIR)/codegen/codegen.o \
          $(BUILD_DIR)/parser/parser.tab.o \
          $(BUILD_DIR)/lexer/lexer.yy.o

# Regla principal: construir el ejecutable
all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm

# Crear directorio build si no existe
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/ast $(BUILD_DIR)/parser $(BUILD_DIR)/lexer $(BUILD_DIR)/semantic $(BUILD_DIR)/codegen

# Regla para generar el parser (de .y a .c y .h)
# -d: genera el archivo .h (parser.tab.h)
$(PARSER_GEN): $(PARSER_DIR)/parser.y $(AST_DIR)/ast.h
	$(BISON) -d --report=all -o $(PARSER_DIR)/parser.tab.c $(PARSER_DIR)/parser.y

# Regla para generar el lexer (de .l a .c)
$(LEXER_GEN): $(LEXER_DIR)/lexer.l $(PARSER_DIR)/parser.tab.h
	$(FLEX) -o $(LEXER_GEN) $(LEXER_DIR)/lexer.l

# Regla específica para main.c
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla específica para ast.c
$(BUILD_DIR)/ast/ast.o: $(AST_DIR)/ast.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla específica para semantic.c
$(BUILD_DIR)/semantic/semantic.o: $(SEMANTIC_DIR)/semantic.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla específica para codegen.c
$(BUILD_DIR)/codegen/codegen.o: $(CODEGEN_DIR)/codegen.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla específica para parser.tab.c
$(BUILD_DIR)/parser/parser.tab.o: $(PARSER_DIR)/parser.tab.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla específica para lexer.yy.c
$(BUILD_DIR)/lexer/lexer.yy.o: $(LEXER_GEN) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar los archivos generados
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET) $(TARGET).exe
	rm -f $(PARSER_GEN) $(LEXER_GEN)
	rm -f $(PARSER_DIR)/parser.output

# Regla para ejecutar (requiere un archivo de prueba)
run: all
	./$(TARGET) tests/test.txt

# Regla para ejecutar con archivo auxiliar
run-aux: all
	./$(TARGET) tests/test_auxiliares.txt
