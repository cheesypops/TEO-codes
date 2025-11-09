%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h" // Incluimos el AST para TipoDato (aunque no se usa aquí)

/* Incluimos el archivo de tokens generado por Bison */
#include "parser.tab.h" 

/* * Bison 3+ requiere esta declaración.
 * Le dice a Flex cómo y dónde almacenar el valor del token (yylval)
 * y la información de ubicación (yylloc).
 */
#ifndef YY_NULL
#define YY_NULL 0
#endif
#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1;

int yycolumn = 1;
#define YY_USER_ACTION yycolumn += yyleng;

%}

/* Opciones de Flex */
%option yylineno
%option noyywrap

/* Definiciones Regulares */
DIGITO      [0-9]
LETRA       [a-zA-Z_]
ID          {LETRA}({LETRA}|{DIGITO})*
NUMERO      {DIGITO}+(\.{DIGITO}+)?
CADENA      \"(\\.|[^"\\])*\"

/* Comentarios y Espacios */
ESPACIO     [ \t\r]+
NUEVA_LINEA \n
COMENT_LIN  "//".*
COMENT_BLOQ "/*"([^*]|(\*+[^*/]))*\*+"/"

%%

/* ------------------------------------ */
/* Palabras Clave (Keywords)            */
/* ------------------------------------ */
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

"true"      { yylval.booleano = 1; return TRUE_TOKEN; }
"false"     { yylval.booleano = 0; return FALSE_TOKEN; }

/* ------------------------------------ */
/* Literales (Números, Cadenas)         */
/* ------------------------------------ */
{NUMERO}    {
                yylval.numero = atof(yytext);
                return NUMERO_TOKEN;
            }

{CADENA}    {
                /* Quitar las comillas dobles de inicio y fin */
                char *str = (char*)malloc(yyleng - 1);
                if (str) {
                    strncpy(str, yytext + 1, yyleng - 2);
                    str[yyleng - 2] = '\0';
                    yylval.cadena = str;
                } else {
                    yyerror("Error: No hay memoria para string literal");
                    yylval.cadena = NULL;
                }
                return CADENA_TOKEN;
            }

/* ------------------------------------ */
/* Identificadores (debe ir después de Keywords) */
/* ------------------------------------ */
{ID}        {
                yylval.cadena = strdup(yytext);
                return ID_TOKEN;
            }

/* ------------------------------------ */
/* Operadores                           */
/* ------------------------------------ */
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

/* ------------------------------------ */
/* Puntuación y Delimitadores           */
/* ------------------------------------ */
"{"         { return INICIO_TOKEN; }
"}"         { return FIN_TOKEN; }
"("         { return PAREN_IZQ_TOKEN; }
")"         { return PAREN_DER_TOKEN; }
"["         { return CORCH_IZQ_TOKEN; }
"]"         { return CORCH_DER_TOKEN; }
";"         { return PUNTOYCOMA_TOKEN; }
","         { return COMA_TOKEN; }

/* ------------------------------------ */
/* Ignorar Espacios y Comentarios       */
/* ------------------------------------ */
{ESPACIO}   { /* Ignorar */ }
{NUEVA_LINEA} { yycolumn = 1; /* Reiniciar columna */ }
{COMENT_LIN}  { yycolumn = 1; /* Ignorar */ }
{COMENT_BLOQ} { /* Ignorar */ }

/* ------------------------------------ */
/* Error Léxico                         */
/* ------------------------------------ */
.           { 
                fprintf(stderr, "Error Léxico en línea %d: Caracter inesperado '%s'\n", yylineno, yytext);
                /* Podemos decidir parar o continuar */
            }

%%