# Nombre del compilador
CC = gcc

# Carpeta de salida (todo lo generado va aquí)
BUILD_DIR = ../build
BIN_DIR   = $(BUILD_DIR)/bin
OBJ_DIR   = $(BUILD_DIR)/obj
GEN_DIR   = $(BUILD_DIR)/gen

# Opciones del compilador (agregamos -I para encontrar parser.tab.h en GEN_DIR
# y también para que pueda localizar headers en el árbol de src/, como ast/ast.h)
CFLAGS = -g -Wall -Wno-unused-function -I. -I$(GEN_DIR)

# Nombres de los ejecutables de Flex y Bison
FLEX  = flex
BISON = bison

# Nombre del ejecutable final (en la carpeta de salida)
TARGET = $(BIN_DIR)/mi_compilador

# Fuentes "manuales" (no generadas)
SRC_FILES = \
	main.c \
	ast/ast.c \
	semantic/semantic.c \
	codegen/codegen.c

# Objetos correspondientes en OBJ_DIR (se respeta la misma estructura de subcarpetas)
OBJECTS = \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/ast/ast.o \
	$(OBJ_DIR)/semantic/semantic.o \
	$(OBJ_DIR)/codegen/codegen.o \
	$(OBJ_DIR)/gen/parser.tab.o \
	$(OBJ_DIR)/gen/lexer.yy.o

# Regla principal: construir el ejecutable
all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm


########################################################
# Generación de archivos con Bison y Flex (en GEN_DIR) #
########################################################

# parser.tab.c y parser.tab.h generados por Bison en GEN_DIR
$(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h: parser.y ast/ast.h
	mkdir -p $(GEN_DIR)
	$(BISON) -d -o $(GEN_DIR)/parser.tab.c parser.y

# lexer.yy.c generado por Flex en GEN_DIR, depende de parser.tab.h
$(GEN_DIR)/lexer.yy.c: lexer.l $(GEN_DIR)/parser.tab.h
	mkdir -p $(GEN_DIR)
	$(FLEX) -o $(GEN_DIR)/lexer.yy.c lexer.l


#########################################
# Compilación de .c a .o en OBJ_DIR     #
#########################################

# Regla genérica para fuentes manuales (en src/, ast/, semantic/, codegen/)
$(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Reglas específicas para los archivos generados
$(OBJ_DIR)/gen/parser.tab.o: $(GEN_DIR)/parser.tab.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/gen/lexer.yy.o: $(GEN_DIR)/lexer.yy.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


###########################
# Limpieza y ejecución    #
###########################

clean:
	rm -rf $(BUILD_DIR)

# Regla para ejecutar (requiere un archivo 'tests/test.txt')
run: all
	$(TARGET) ../tests/test.txt