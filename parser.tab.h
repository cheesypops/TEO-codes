/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ID_TOKEN = 258,                /* ID_TOKEN  */
    NUMERO_TOKEN = 259,            /* NUMERO_TOKEN  */
    CADENA_TOKEN = 260,            /* CADENA_TOKEN  */
    TRUE_TOKEN = 261,              /* TRUE_TOKEN  */
    FALSE_TOKEN = 262,             /* FALSE_TOKEN  */
    TIPO_TOKEN_INT = 263,          /* TIPO_TOKEN_INT  */
    TIPO_TOKEN_BOOLEAN = 264,      /* TIPO_TOKEN_BOOLEAN  */
    TIPO_TOKEN_FLOAT = 265,        /* TIPO_TOKEN_FLOAT  */
    TIPO_TOKEN_CHAR = 266,         /* TIPO_TOKEN_CHAR  */
    TIPO_TOKEN_STRING = 267,       /* TIPO_TOKEN_STRING  */
    IF_TOKEN = 268,                /* IF_TOKEN  */
    ELSE_TOKEN = 269,              /* ELSE_TOKEN  */
    FOR_TOKEN = 270,               /* FOR_TOKEN  */
    WHILE_TOKEN = 271,             /* WHILE_TOKEN  */
    DO_TOKEN = 272,                /* DO_TOKEN  */
    VOID_TOKEN = 273,              /* VOID_TOKEN  */
    SETUP_TOKEN = 274,             /* SETUP_TOKEN  */
    MOVER_TOKEN = 275,             /* MOVER_TOKEN  */
    GIRARIZQ_TOKEN = 276,          /* GIRARIZQ_TOKEN  */
    GIRARDER_TOKEN = 277,          /* GIRARDER_TOKEN  */
    LEERSENSOR_TOKEN = 278,        /* LEERSENSOR_TOKEN  */
    PARAR_TOKEN = 279,             /* PARAR_TOKEN  */
    REVERSA_TOKEN = 280,           /* REVERSA_TOKEN  */
    INICIO_TOKEN = 281,            /* INICIO_TOKEN  */
    FIN_TOKEN = 282,               /* FIN_TOKEN  */
    PAREN_IZQ_TOKEN = 283,         /* PAREN_IZQ_TOKEN  */
    PAREN_DER_TOKEN = 284,         /* PAREN_DER_TOKEN  */
    CORCH_IZQ_TOKEN = 285,         /* CORCH_IZQ_TOKEN  */
    CORCH_DER_TOKEN = 286,         /* CORCH_DER_TOKEN  */
    PUNTOYCOMA_TOKEN = 287,        /* PUNTOYCOMA_TOKEN  */
    COMA_TOKEN = 288,              /* COMA_TOKEN  */
    ASIGN_TOKEN = 289,             /* ASIGN_TOKEN  */
    OR_TOKEN = 290,                /* OR_TOKEN  */
    AND_TOKEN = 291,               /* AND_TOKEN  */
    IGUAL_TOKEN = 292,             /* IGUAL_TOKEN  */
    NO_IGUAL_TOKEN = 293,          /* NO_IGUAL_TOKEN  */
    MENOR_TOKEN = 294,             /* MENOR_TOKEN  */
    MENOR_IGUAL_TOKEN = 295,       /* MENOR_IGUAL_TOKEN  */
    MAYOR_TOKEN = 296,             /* MAYOR_TOKEN  */
    MAYOR_IGUAL_TOKEN = 297,       /* MAYOR_IGUAL_TOKEN  */
    MAS_TOKEN = 298,               /* MAS_TOKEN  */
    MENOS_TOKEN = 299,             /* MENOS_TOKEN  */
    MULT_TOKEN = 300,              /* MULT_TOKEN  */
    DIV_TOKEN = 301,               /* DIV_TOKEN  */
    MOD_TOKEN = 302,               /* MOD_TOKEN  */
    NOT_TOKEN = 303,               /* NOT_TOKEN  */
    INC_TOKEN = 304,               /* INC_TOKEN  */
    DEC_TOKEN = 305                /* DEC_TOKEN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 15 "parser.y"

    char* cadena;
    double  numero;
    int     booleano;
    TipoDato tipo_dato;
    ASTNode* nodo;

#line 122 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
