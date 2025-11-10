# Nombre del compilador
CC = gcc
# Opciones del compilador (g = debug symbols)
CFLAGS = -g -Wall -Wno-unused-function

# Nombres de los ejecutables de Flex y Bison
FLEX = flex
BISON = bison

# Nombre del ejecutable final
TARGET = mi_compilador

# Archivos fuente .c
# parser.tab.c y lexer.yy.c son generados por Bison y Flex
SOURCES = main.c ast.c semantic.c parser.tab.c lexer.yy.c
# Archivos de cabecera .h
HEADERS = ast.h semantic.h parser.tab.h
# Archivos objeto .o
OBJECTS = $(SOURCES:.c=.o)


# Regla principal: construir el ejecutable
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lm

# Regla para generar el parser (de .y a .c y .h)
# -d: genera el archivo .h (parser.tab.h)
parser.tab.c parser.tab.h: parser.y ast.h
	$(BISON) -d -Wcounterexamples --report=all parser.y

# Regla para generar el lexer (de .l a .c)
lexer.yy.c: lexer.l parser.tab.h
	$(FLEX) -o lexer.yy.c lexer.l

# Regla genérica para compilar archivos .c a .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar los archivos generados
clean:
	rm -f $(TARGET) $(OBJECTS) parser.tab.c parser.tab.h lexer.yy.c

# Regla para ejecutar (requiere un archivo 'test.txt')
run: all
	./$(TARGET) test.txt