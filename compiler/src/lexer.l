%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Definiciones del AST compartidas con el parser */
#include "ast/ast.h"
/* Cabecera generada por Bison con la tabla de tokens */
#include "parser.tab.h"

/* Prototipo de la función de reporte de errores del parser. */
void yyerror(const char *s); 

/* Configuración de información de ubicación (línea y columna).
 * YY_USER_ACTION se ejecuta para cada token reconocido y actualiza
 * la estructura yylloc y la columna actual para mejorar los mensajes de error.
 */
#ifndef YY_NULL
#define YY_NULL 0
#endif

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

%}

/* Opciones de Flex.
 * yylineno permite llevar la cuenta de la línea actual
 * y noyywrap evita que se solicite más entrada al terminar el archivo.
 */
%option yylineno
%option noyywrap

/* Definiciones regulares básicas utilizadas en las reglas del lexer. */
DIGITO      [0-9]
LETRA       [a-zA-Z_]
ID          {LETRA}({LETRA}|{DIGITO})*
NUMERO      {DIGITO}+(\.{DIGITO}+)?
CADENA      \"(\\.|[^"\\])*\"

/* Patrones para espacios en blanco y comentarios, que no generan tokens. */
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
"esperar"    { return ESPERAR_TOKEN; }

"true"      { yylval.booleano = 1; return TRUE_TOKEN; }
"false"     { yylval.booleano = 0; return FALSE_TOKEN; }

{NUMERO}    {
                yylval.numero = atof(yytext);
                return NUMERO_TOKEN;
            }

{CADENA}    {
                /* Elimina las comillas dobles de inicio y fin del literal. */
                char *str = (char*)malloc(yyleng - 1);
                if (str) {
                    strncpy(str, yytext + 1, yyleng - 2);
                    str[yyleng - 2] = '\0';
                    yylval.cadena = str;
                } else {
                    yyerror("Error: no hay memoria disponible para el literal de cadena.");
                    yylval.cadena = NULL;
                }
                return CADENA_TOKEN;
            }

{ID}        {
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

{ESPACIO}     { /* Ignora espacios en blanco sin generar tokens. */ }
{NUEVA_LINEA} { yycolumn = 1; /* Reinicia la columna al comienzo de cada línea. */ }
{COMENT_LIN}  { yycolumn = 1; /* Ignora comentarios de línea completos. */ }
{COMENT_BLOQ} { /* Ignora comentarios de bloque sin generar tokens. */ }

.           { 
                fprintf(stderr, "Error léxico en la línea %d: carácter inesperado '%s'.\n", yylineno, yytext);
                /* A partir de este punto se puede decidir si el compilador continúa o se detiene. */
            }

%%
