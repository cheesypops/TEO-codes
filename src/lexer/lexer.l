/*
 * Analizador Léxico (Lexer) - Tokenización del Código Fuente.
 * 
 * Este archivo contiene la especificación de patrones léxicos en formato Flex.
 * El lexer convierte el código fuente en una secuencia de tokens que el parser
 * utiliza para construir el Árbol Sintáctico Abstracto (AST).
 * 
 * GENERADO POR: Flex (lex) - este archivo se compila para generar lexer.yy.c
 * 
 * FUNCIONAMIENTO:
 *   - Flex busca el patrón más largo que coincida con la entrada
 *   - Si hay empate, se elige el patrón definido primero
 *   - Los tokens se retornan al parser mediante return
 *   - Los valores semánticos se asignan a yylval
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ast/ast.h"
#include "../parser/parser.tab.h"

void yyerror(const char *s);

/**
 * Configuración de ubicación (location) para reporte de errores.
 * 
 * Flex permite rastrear la posición exacta de cada token en el código fuente
 * mediante la estructura yylloc. Esta información es útil para reportar
 * errores con precisión (línea y columna).
 */
#ifndef YY_NULL
#define YY_NULL 0
#endif

int yycolumn = 1;  /* Contador de columna actual */

/* Macro ejecutada automáticamente para cada token reconocido */
#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

%}

/* Opciones de Flex */
/* Mantener contador de líneas automáticamente */
%option yylineno
/* No llamar a yywrap() al final del archivo */
%option noyywrap

/* ========== Definiciones de Patrones Regulares ========== */

DIGITO      [0-9]
LETRA       [a-zA-Z_]
ID          {LETRA}({LETRA}|{DIGITO})*
NUMERO      {DIGITO}+(\.{DIGITO}+)?
CADENA      \"(\\.|[^"\\])*\"

/* Patrones para Espacios y Comentarios */
ESPACIO     [ \t\r]+
NUEVA_LINEA \n
COMENT_LIN  "//".*
COMENT_BLOQ "/*"([^*]|(\*+[^*/]))*\*+"/"

%%
"int"       { return TIPO_TOKEN_INT; }
"boolean"   { return TIPO_TOKEN_BOOLEAN; }
"float"     { return TIPO_TOKEN_FLOAT; }
"char"      { return TIPO_TOKEN_CHAR; }
"string"    { return TIPO_TOKEN_STRING; }

"if"        { return IF_TOKEN; }
"else"      { return ELSE_TOKEN; }
"for"       { return FOR_TOKEN; }
"while"     { return WHILE_TOKEN; }
"do"        { return DO_TOKEN; }
"void"      { return VOID_TOKEN; }
"setup"     { return SETUP_TOKEN; }

"mover"      { return MOVER_TOKEN; }      
"girarIzq"   { return GIRARIZQ_TOKEN; }   
"girarDer"   { return GIRARDER_TOKEN; }   
"leerSensor" { return LEERSENSOR_TOKEN; } 
"parar"      { return PARAR_TOKEN; }      
"reversa"    { return REVERSA_TOKEN; }    

"true"      { yylval.booleano = 1; return TRUE_TOKEN; }
"false"     { yylval.booleano = 0; return FALSE_TOKEN; }
{NUMERO}    {
                /* Convertir string a número (soporta enteros y flotantes) */
                yylval.numero = atof(yytext);
                return NUMERO_TOKEN;
            }

{CADENA}    {
                /* 
                 * IMPORTANTE: Asignar memoria dinámicamente para el string.
                 * El lexer es responsable de asignar esta memoria, y el AST
                 * la liberará cuando se destruya el árbol.
                 * 
                 * Se eliminan las comillas dobles del inicio y fin.
                 */
                char *str = (char*)malloc(yyleng - 1);
                if (str) {
                    strncpy(str, yytext + 1, yyleng - 2);  /* Saltar primera comilla */
                    str[yyleng - 2] = '\0';                 /* Terminar string */
                    yylval.cadena = str;
                } else {
                    yyerror("Error: No hay memoria para string literal");
                    yylval.cadena = NULL;
                }
                return CADENA_TOKEN;
            }

{ID}        {
                /* 
                 * IMPORTANTE: strdup() asigna memoria dinámicamente.
                 * El AST será responsable de liberar esta memoria.
                 */
                yylval.cadena = strdup(yytext);
                return ID_TOKEN;
            }

"="         { return ASIGN_TOKEN; }
"||"        { return OR_TOKEN; }
"&&"        { return AND_TOKEN; }
"!"         { return NOT_TOKEN; }
"=="        { return IGUAL_TOKEN; }
"!="        { return NO_IGUAL_TOKEN; }
"<"         { return MENOR_TOKEN; }
"<="        { return MENOR_IGUAL_TOKEN; }
">"         { return MAYOR_TOKEN; }
">="        { return MAYOR_IGUAL_TOKEN; }
"+"         { return MAS_TOKEN; }
"-"         { return MENOS_TOKEN; }
"*"         { return MULT_TOKEN; }
"/"         { return DIV_TOKEN; }
"%"         { return MOD_TOKEN; }
"++"        { return INC_TOKEN; }
"--"        { return DEC_TOKEN; }

"{"         { return INICIO_TOKEN; }
"}"         { return FIN_TOKEN; }
"("         { return PAREN_IZQ_TOKEN; }
")"         { return PAREN_DER_TOKEN; }
"["         { return CORCH_IZQ_TOKEN; }
"]"         { return CORCH_DER_TOKEN; }
";"         { return PUNTOYCOMA_TOKEN; }
","         { return COMA_TOKEN; }

{ESPACIO}   { /* Ignorar espacios en blanco */ }
{NUEVA_LINEA} { yycolumn = 1; /* Reiniciar contador de columna al inicio de línea */ }
{COMENT_LIN}  { yycolumn = 1; /* Ignorar comentarios de línea */ }
{COMENT_BLOQ} { /* Ignorar comentarios de bloque (pueden ser multilínea) */ }

.           { 
                /* Error léxico: caracter no reconocido por ningún patrón */
                fprintf(stderr, "Error Léxico en línea %d: Caracter inesperado '%s'\n", yylineno, yytext);
                /* NOTA: El lexer continúa procesando después de reportar el error */
            }

%%