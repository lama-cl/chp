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

#ifndef YY_YY_GRAM_H_INCLUDED
# define YY_YY_GRAM_H_INCLUDED
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
    tSYMBOL = 258,                 /* tSYMBOL  */
    tREGEXP = 259,                 /* tREGEXP  */
    tSTRING = 260,                 /* tSTRING  */
    tINTEGER = 261,                /* tINTEGER  */
    tREAL = 262,                   /* tREAL  */
    tSUB = 263,                    /* tSUB  */
    tSTATE = 264,                  /* tSTATE  */
    tSTART = 265,                  /* tSTART  */
    tSTARTRULES = 266,             /* tSTARTRULES  */
    tNAMERULES = 267,              /* tNAMERULES  */
    tBEGIN = 268,                  /* tBEGIN  */
    tEND = 269,                    /* tEND  */
    tRETURN = 270,                 /* tRETURN  */
    tIF = 271,                     /* tIF  */
    tELSE = 272,                   /* tELSE  */
    tLOCAL = 273,                  /* tLOCAL  */
    tWHILE = 274,                  /* tWHILE  */
    tFOR = 275,                    /* tFOR  */
    tEXTENDS = 276,                /* tEXTENDS  */
    tADDASSIGN = 277,              /* tADDASSIGN  */
    tSUBASSIGN = 278,              /* tSUBASSIGN  */
    tMULASSIGN = 279,              /* tMULASSIGN  */
    tDIVASSIGN = 280,              /* tDIVASSIGN  */
    tOR = 281,                     /* tOR  */
    tAND = 282,                    /* tAND  */
    tEQ = 283,                     /* tEQ  */
    tNE = 284,                     /* tNE  */
    tGE = 285,                     /* tGE  */
    tLE = 286,                     /* tLE  */
    tDIV = 287,                    /* tDIV  */
    tPLUSPLUS = 288,               /* tPLUSPLUS  */
    tMINUSMINUS = 289              /* tMINUSMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define tSYMBOL 258
#define tREGEXP 259
#define tSTRING 260
#define tINTEGER 261
#define tREAL 262
#define tSUB 263
#define tSTATE 264
#define tSTART 265
#define tSTARTRULES 266
#define tNAMERULES 267
#define tBEGIN 268
#define tEND 269
#define tRETURN 270
#define tIF 271
#define tELSE 272
#define tLOCAL 273
#define tWHILE 274
#define tFOR 275
#define tEXTENDS 276
#define tADDASSIGN 277
#define tSUBASSIGN 278
#define tMULASSIGN 279
#define tDIVASSIGN 280
#define tOR 281
#define tAND 282
#define tEQ 283
#define tNE 284
#define tGE 285
#define tLE 286
#define tDIV 287
#define tPLUSPLUS 288
#define tMINUSMINUS 289

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 34 "gram.y"

  List *lst;
  Node *node;
  Cons *cons;
  Stmt *stmt;
  Expr *expr;

#line 143 "gram.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_GRAM_H_INCLUDED  */
