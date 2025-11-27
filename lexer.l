%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "y.tab.h"

void yyerror(const char *s);
%}

/* Opciones */
%option noyywrap
%option yylineno
%option nounput
%option noinput

/* Definiciones */
DIGITO      [0-9]
LETRA       [a-zA-Z_]
ID          {LETRA}({LETRA}|{DIGITO})*
NUMERO      {DIGITO}+
CADENA      \"[^\"]*\"
ESPACIO     [ \t\r]+

%%

    /* 1. COMENTARIOS (Solo linea //) */
"//".* { /* Ignorar todo hasta el salto de linea */ }

    /* 2. ESPACIOS */
\n          { }
{ESPACIO}   { }

    /* 3. PALABRAS CLAVE */
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"for"       { return FOR; }
"do"        { return DO; }
"return"    { return RETURN; }

    /* 4. TIPOS DE DATOS */
"int"       { return TYPE_INT; }
"float"     { return TYPE_FLOAT; }
"boolean"   { return TYPE_BOOL; }
"char"      { return TYPE_CHAR; }
"string"    { return TYPE_STRING; }
"void"      { return TYPE_VOID; }

    /* 5. LITERALES */
"true"      { yylval.ival = 1; return TRUE; }
"false"     { yylval.ival = 0; return FALSE; }

    /* 6. TOKENS ROBOT */
"token_mover"       { return TOKEN_MOVER; }
"token_retroceder"  { return TOKEN_RETROCEDER; }
"token_girarIzq"    { return TOKEN_GIRAR_IZQ; }
"token_girarDer"    { return TOKEN_GIRAR_DER; }
"token_esperar"     { return TOKEN_ESPERAR; }
"token_leerSensor"  { return TOKEN_LEER_SENSOR; }
"token_parar"       { return TOKEN_PARAR; }

    /* 7. OPERADORES */
"=="        { yylval.sval = strdup(yytext); return EQ; }
"!="        { yylval.sval = strdup(yytext); return NEQ; }
"<="        { yylval.sval = strdup(yytext); return LE; }
">="        { yylval.sval = strdup(yytext); return GE; }
"&&"        { yylval.sval = strdup(yytext); return AND; }
"||"        { yylval.sval = strdup(yytext); return OR; }
"++"        { yylval.sval = strdup(yytext); return INC; }
"--"        { yylval.sval = strdup(yytext); return DEC; }

"+"         { yylval.sval = strdup(yytext); return '+'; }
"-"         { yylval.sval = strdup(yytext); return '-'; }
"*"         { yylval.sval = strdup(yytext); return '*'; }
"/"         { yylval.sval = strdup(yytext); return '/'; }
"%"         { yylval.sval = strdup(yytext); return '%'; }
"="         { yylval.sval = strdup(yytext); return '='; }
"<"         { yylval.sval = strdup(yytext); return '<'; }
">"         { yylval.sval = strdup(yytext); return '>'; }
"!"         { yylval.sval = strdup(yytext); return '!'; }
";"         { return ';'; }
","         { return ','; }
"{"         { return '{'; }
"}"         { return '}'; }
"("         { return '('; }
")"         { return ')'; }
"["         { return '['; }
"]"         { return ']'; }

    /* 8. VALORES E IDs */
{NUMERO}    { yylval.ival = atoi(yytext); return NUMERO; }
{CADENA}    { yylval.sval = strdup(yytext); return CADENA; }
{ID}        { yylval.sval = strdup(yytext); return ID; }

.           { printf("Error lexico linea %d: Caracter invalido '%s'\n", yylineno, yytext); }

%%