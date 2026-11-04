/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "gram.y"

/* 								-*- c -*-
 * Grammar for states.
 * Copyright (c) 1997-1998 Markku Rossi.
 *
 * Author: Markku Rossi <mtr@iki.fi>
 */

/*
 * This file is part of GNU Enscript.
 *
 * Enscript is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Enscript is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enscript.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * $Id: gram.y,v 1.1.1.1 2003/03/05 07:25:52 mtr Exp $
 */

#include "defs.h"

#line 103 "gram.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
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

#line 232 "gram.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_tSYMBOL = 3,                    /* tSYMBOL  */
  YYSYMBOL_tREGEXP = 4,                    /* tREGEXP  */
  YYSYMBOL_tSTRING = 5,                    /* tSTRING  */
  YYSYMBOL_tINTEGER = 6,                   /* tINTEGER  */
  YYSYMBOL_tREAL = 7,                      /* tREAL  */
  YYSYMBOL_tSUB = 8,                       /* tSUB  */
  YYSYMBOL_tSTATE = 9,                     /* tSTATE  */
  YYSYMBOL_tSTART = 10,                    /* tSTART  */
  YYSYMBOL_tSTARTRULES = 11,               /* tSTARTRULES  */
  YYSYMBOL_tNAMERULES = 12,                /* tNAMERULES  */
  YYSYMBOL_tBEGIN = 13,                    /* tBEGIN  */
  YYSYMBOL_tEND = 14,                      /* tEND  */
  YYSYMBOL_tRETURN = 15,                   /* tRETURN  */
  YYSYMBOL_tIF = 16,                       /* tIF  */
  YYSYMBOL_tELSE = 17,                     /* tELSE  */
  YYSYMBOL_tLOCAL = 18,                    /* tLOCAL  */
  YYSYMBOL_tWHILE = 19,                    /* tWHILE  */
  YYSYMBOL_tFOR = 20,                      /* tFOR  */
  YYSYMBOL_tEXTENDS = 21,                  /* tEXTENDS  */
  YYSYMBOL_22_ = 22,                       /* '='  */
  YYSYMBOL_tADDASSIGN = 23,                /* tADDASSIGN  */
  YYSYMBOL_tSUBASSIGN = 24,                /* tSUBASSIGN  */
  YYSYMBOL_tMULASSIGN = 25,                /* tMULASSIGN  */
  YYSYMBOL_tDIVASSIGN = 26,                /* tDIVASSIGN  */
  YYSYMBOL_27_ = 27,                       /* '?'  */
  YYSYMBOL_28_ = 28,                       /* ':'  */
  YYSYMBOL_tOR = 29,                       /* tOR  */
  YYSYMBOL_tAND = 30,                      /* tAND  */
  YYSYMBOL_tEQ = 31,                       /* tEQ  */
  YYSYMBOL_tNE = 32,                       /* tNE  */
  YYSYMBOL_33_ = 33,                       /* '<'  */
  YYSYMBOL_34_ = 34,                       /* '>'  */
  YYSYMBOL_tGE = 35,                       /* tGE  */
  YYSYMBOL_tLE = 36,                       /* tLE  */
  YYSYMBOL_37_ = 37,                       /* '+'  */
  YYSYMBOL_38_ = 38,                       /* '-'  */
  YYSYMBOL_39_ = 39,                       /* '*'  */
  YYSYMBOL_tDIV = 40,                      /* tDIV  */
  YYSYMBOL_41_ = 41,                       /* '!'  */
  YYSYMBOL_tPLUSPLUS = 42,                 /* tPLUSPLUS  */
  YYSYMBOL_tMINUSMINUS = 43,               /* tMINUSMINUS  */
  YYSYMBOL_44_ = 44,                       /* '['  */
  YYSYMBOL_45_ = 45,                       /* ']'  */
  YYSYMBOL_46_ = 46,                       /* '{'  */
  YYSYMBOL_47_ = 47,                       /* '}'  */
  YYSYMBOL_48_ = 48,                       /* ';'  */
  YYSYMBOL_49_ = 49,                       /* ','  */
  YYSYMBOL_50_ = 50,                       /* '('  */
  YYSYMBOL_51_ = 51,                       /* ')'  */
  YYSYMBOL_YYACCEPT = 52,                  /* $accept  */
  YYSYMBOL_file = 53,                      /* file  */
  YYSYMBOL_toplevel = 54,                  /* toplevel  */
  YYSYMBOL_regexp_sym_list = 55,           /* regexp_sym_list  */
  YYSYMBOL_staterules = 56,                /* staterules  */
  YYSYMBOL_staterule = 57,                 /* staterule  */
  YYSYMBOL_symbol_list = 58,               /* symbol_list  */
  YYSYMBOL_rest_symbol_list = 59,          /* rest_symbol_list  */
  YYSYMBOL_locals = 60,                    /* locals  */
  YYSYMBOL_locals_rest = 61,               /* locals_rest  */
  YYSYMBOL_local_def = 62,                 /* local_def  */
  YYSYMBOL_stmt_list = 63,                 /* stmt_list  */
  YYSYMBOL_stmt = 64,                      /* stmt  */
  YYSYMBOL_expr = 65,                      /* expr  */
  YYSYMBOL_cond_expr = 66,                 /* cond_expr  */
  YYSYMBOL_expr_list = 67,                 /* expr_list  */
  YYSYMBOL_rest_expr_list = 68             /* rest_expr_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   571

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  52
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  17
/* YYNRULES -- Number of rules.  */
#define YYNRULES  76
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  167

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   289


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    41,     2,     2,     2,     2,     2,     2,
      50,    51,    39,    37,    49,    38,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    28,    48,
      33,    22,    34,    27,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    44,     2,    45,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,     2,    47,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    23,    24,    25,
      26,    29,    30,    31,    32,    35,    36,    40,    42,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    65,    65,    66,    69,    70,    72,    74,    76,    78,
      81,    82,    86,    87,    89,    90,    91,    92,    95,    96,
      99,   100,   103,   104,   107,   108,   111,   112,   115,   116,
     119,   121,   123,   128,   130,   132,   135,   137,   140,   144,
     146,   148,   150,   152,   154,   156,   157,   158,   160,   162,
     164,   166,   168,   170,   172,   174,   176,   178,   180,   181,
     183,   185,   186,   187,   188,   190,   191,   192,   193,   194,
     195,   198,   199,   202,   203,   206,   207
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "tSYMBOL", "tREGEXP",
  "tSTRING", "tINTEGER", "tREAL", "tSUB", "tSTATE", "tSTART",
  "tSTARTRULES", "tNAMERULES", "tBEGIN", "tEND", "tRETURN", "tIF", "tELSE",
  "tLOCAL", "tWHILE", "tFOR", "tEXTENDS", "'='", "tADDASSIGN",
  "tSUBASSIGN", "tMULASSIGN", "tDIVASSIGN", "'?'", "':'", "tOR", "tAND",
  "tEQ", "tNE", "'<'", "'>'", "tGE", "tLE", "'+'", "'-'", "'*'", "tDIV",
  "'!'", "tPLUSPLUS", "tMINUSMINUS", "'['", "']'", "'{'", "'}'", "';'",
  "','", "'('", "')'", "$accept", "file", "toplevel", "regexp_sym_list",
  "staterules", "staterule", "symbol_list", "rest_symbol_list", "locals",
  "locals_rest", "local_def", "stmt_list", "stmt", "expr", "cond_expr",
  "expr_list", "rest_expr_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-41)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -41,    71,   -41,   300,   -41,   -41,   -41,   -41,     1,     9,
     -40,   -28,   -27,    18,   -24,   -18,   -17,   296,    28,    31,
     -41,   296,   -41,   -41,   391,   296,   296,   296,   296,   296,
     -41,   -41,   296,   -15,   -19,   -41,   -41,   -41,   -41,   411,
     296,   296,   296,    12,   -41,   -41,   126,   322,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   -41,   486,   486,   486,   486,   486,   486,     6,
      20,    64,    67,   -41,   146,     3,     4,   -41,   345,   368,
     486,    10,   -41,   -41,   -41,   470,   501,   515,   527,   527,
      55,    55,    55,    55,    45,    45,    12,    12,   451,   -41,
     296,   -41,    21,    24,    60,    49,   -41,    94,   -41,   -41,
     290,   290,   296,   296,    85,   486,    62,   112,   -41,    70,
      73,    74,    76,   -41,   -41,    75,   107,   -41,   431,   486,
     296,   108,   -41,    51,   -41,   -41,   -41,   -41,   -41,   290,
     296,   486,   122,   -41,   -41,   175,   194,   223,   242,   -41,
      84,   105,   -38,   -41,   271,   -41,   -41,   -41,   -41,   290,
     296,   -41,   122,   -41,   -41,   486,   -41
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     1,    43,    40,    39,    41,    42,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,     0,     3,     9,     0,     0,     0,     0,     0,     0,
      53,    54,    73,     0,     0,    28,    10,    10,    30,     0,
       0,     0,    71,    44,    55,    56,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    38,    48,    49,    50,    51,    52,    75,     0,
      74,    18,     0,    12,     0,     0,     0,    31,     0,     0,
      72,     0,    33,    29,    58,     0,    46,    45,    67,    68,
      65,    66,    69,    70,    63,    64,    61,    62,     0,    47,
       0,    20,     0,    19,     0,     0,     4,     0,     5,     6,
       0,     0,     0,     0,    59,    76,     0,     0,    12,     0,
       0,     0,     0,     7,    13,     0,    34,    36,     0,    60,
       0,    22,    21,     0,    28,    28,    28,    28,    11,     0,
      71,    57,     0,    28,     8,     0,     0,     0,     0,    35,
       0,    26,     0,    24,     0,    17,    16,    14,    15,     0,
       0,    23,     0,    32,    37,    27,    25
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -41,   -41,   -41,    99,    19,   -41,   -41,   -41,   -41,   -41,
     -23,   -32,    -1,   -12,     0,   -41,   -41
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,    22,    75,   105,   124,   102,   103,   143,   152,
     153,    46,    83,    24,    81,    69,    70
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      23,    39,    72,    74,    33,    43,    35,   107,   107,    47,
     161,   162,    34,    63,    64,    65,    66,    67,    36,    37,
      68,     3,     4,     5,     6,     7,    40,    73,    78,    79,
      80,    44,    41,    42,    45,    71,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
     108,   109,   119,   120,   119,   120,    61,    99,   112,    17,
      18,    19,   121,   122,   121,   122,    38,   101,    21,   100,
     104,     2,   116,   117,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    59,    60,    13,    14,   115,    61,
      15,    16,    57,    58,    59,    60,   123,   125,   144,    61,
     128,   129,   145,   146,   147,   148,   118,   130,   131,   126,
     127,   154,    17,    18,    19,   132,   134,    20,   141,   135,
     136,    21,   137,   138,   139,   151,   142,   160,    80,     3,
       4,     5,     6,     7,     8,   159,    76,   133,   149,   166,
     150,    13,    14,     0,     0,    15,    16,     0,   165,     3,
       4,     5,     6,     7,     8,     0,     0,     0,   164,     0,
       0,    13,    14,     0,     0,    15,    16,    17,    18,    19,
       0,     0,    20,    82,     0,     0,    21,     0,     3,     4,
       5,     6,     7,     8,     0,     0,     0,    17,    18,    19,
      13,    14,    20,   106,    15,    16,    21,     3,     4,     5,
       6,     7,     8,     0,     0,     0,     0,     0,     0,    13,
      14,     0,     0,    15,    16,     0,    17,    18,    19,     0,
       0,    20,   155,     0,     0,    21,     3,     4,     5,     6,
       7,     8,     0,     0,     0,    17,    18,    19,    13,    14,
      20,   156,    15,    16,    21,     3,     4,     5,     6,     7,
       8,     0,     0,     0,     0,     0,     0,    13,    14,     0,
       0,    15,    16,     0,    17,    18,    19,     0,     0,    20,
     157,     0,     0,    21,     3,     4,     5,     6,     7,     8,
       0,     0,     0,    17,    18,    19,    13,    14,    20,   158,
      15,    16,    21,     3,     4,     5,     6,     7,     8,     3,
       4,     5,     6,     7,     0,    13,    14,     0,     0,    15,
      16,     0,    17,    18,    19,     0,     0,    20,   163,     0,
       0,    21,    25,    26,    27,    28,    29,     0,     0,     0,
       0,    17,    18,    19,     0,     0,    20,    17,    18,    19,
      21,     0,    30,    31,     0,     0,    21,     0,     0,    48,
      32,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,     0,     0,     0,    61,     0,     0,     0,
       0,     0,    48,    84,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,     0,     0,     0,    61,
       0,     0,     0,     0,     0,    48,   110,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,     0,
       0,     0,    61,     0,     0,     0,     0,     0,    48,   111,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,     0,     0,     0,    61,     0,     0,    48,    62,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,     0,     0,     0,    61,     0,     0,    48,    77,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,     0,     0,     0,    61,     0,     0,    48,   140,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,     0,     0,     0,    61,   114,    48,   113,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,     0,     0,    48,    61,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,     0,     0,     0,
      61,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,     0,     0,     0,    61,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,     0,     0,     0,    61,
      53,    54,    55,    56,    57,    58,    59,    60,     0,     0,
       0,    61
};

static const yytype_int16 yycheck[] =
{
       1,    13,    21,    35,     3,    17,    46,     4,     4,    21,
      48,    49,     3,    25,    26,    27,    28,    29,    46,    46,
      32,     3,     4,     5,     6,     7,    50,    46,    40,    41,
      42,     3,    50,    50,     3,    50,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      47,    47,     3,     4,     3,     4,    44,    51,    48,    41,
      42,    43,    13,    14,    13,    14,    48,     3,    50,    49,
       3,     0,    51,    49,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    39,    40,    15,    16,   100,    44,
      19,    20,    37,    38,    39,    40,    47,     3,    47,    44,
     112,   113,   134,   135,   136,   137,    46,    22,    46,   110,
     111,   143,    41,    42,    43,     3,    46,    46,   130,    46,
      46,    50,    46,    48,    17,     3,    18,    22,   140,     3,
       4,     5,     6,     7,     8,    51,    37,   118,   139,   162,
     140,    15,    16,    -1,    -1,    19,    20,    -1,   160,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,   159,    -1,
      -1,    15,    16,    -1,    -1,    19,    20,    41,    42,    43,
      -1,    -1,    46,    47,    -1,    -1,    50,    -1,     3,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    41,    42,    43,
      15,    16,    46,    47,    19,    20,    50,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,    -1,    15,
      16,    -1,    -1,    19,    20,    -1,    41,    42,    43,    -1,
      -1,    46,    47,    -1,    -1,    50,     3,     4,     5,     6,
       7,     8,    -1,    -1,    -1,    41,    42,    43,    15,    16,
      46,    47,    19,    20,    50,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,    -1,
      -1,    19,    20,    -1,    41,    42,    43,    -1,    -1,    46,
      47,    -1,    -1,    50,     3,     4,     5,     6,     7,     8,
      -1,    -1,    -1,    41,    42,    43,    15,    16,    46,    47,
      19,    20,    50,     3,     4,     5,     6,     7,     8,     3,
       4,     5,     6,     7,    -1,    15,    16,    -1,    -1,    19,
      20,    -1,    41,    42,    43,    -1,    -1,    46,    47,    -1,
      -1,    50,    22,    23,    24,    25,    26,    -1,    -1,    -1,
      -1,    41,    42,    43,    -1,    -1,    46,    41,    42,    43,
      50,    -1,    42,    43,    -1,    -1,    50,    -1,    -1,    27,
      50,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    -1,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    -1,    27,    51,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    -1,    27,    51,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    27,    51,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    44,    -1,    -1,    27,    48,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    44,    -1,    -1,    27,    48,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    44,    -1,    -1,    27,    48,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    44,    45,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    -1,    -1,    27,    44,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      44,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    44,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    -1,    -1,    -1,    44,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      -1,    44
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    53,     0,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    15,    16,    19,    20,    41,    42,    43,
      46,    50,    54,    64,    65,    22,    23,    24,    25,    26,
      42,    43,    50,     3,     3,    46,    46,    46,    48,    65,
      50,    50,    50,    65,     3,     3,    63,    65,    27,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    44,    48,    65,    65,    65,    65,    65,    65,    67,
      68,    50,    21,    46,    63,    55,    55,    48,    65,    65,
      65,    66,    47,    64,    51,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    51,
      49,     3,    58,    59,     3,    56,    47,     4,    47,    47,
      51,    51,    48,    28,    45,    65,    51,    49,    46,     3,
       4,    13,    14,    47,    57,     3,    64,    64,    65,    65,
      22,    46,     3,    56,    46,    46,    46,    46,    48,    17,
      48,    65,    18,    60,    47,    63,    63,    63,    63,    64,
      66,     3,    61,    62,    63,    47,    47,    47,    47,    51,
      22,    48,    49,    47,    64,    65,    62
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    52,    53,    53,    54,    54,    54,    54,    54,    54,
      55,    55,    56,    56,    57,    57,    57,    57,    58,    58,
      59,    59,    60,    60,    61,    61,    62,    62,    63,    63,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    66,    66,    67,    67,    68,    68
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     4,     4,     4,     5,     7,     1,
       0,     4,     0,     2,     4,     4,     4,     4,     0,     1,
       1,     3,     0,     3,     1,     3,     1,     3,     0,     2,
       2,     3,     9,     3,     5,     7,     5,     9,     2,     1,
       1,     1,     1,     1,     2,     3,     3,     4,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     6,     3,     4,
       5,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     0,     1,     0,     1,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4: /* toplevel: tSTART '{' stmt_list '}'  */
#line 69 "gram.y"
                                        { start_stmts = (yyvsp[-1].lst); }
#line 1462 "gram.c"
    break;

  case 5: /* toplevel: tSTARTRULES '{' regexp_sym_list '}'  */
#line 71 "gram.y"
                                        { startrules = (yyvsp[-1].lst); }
#line 1468 "gram.c"
    break;

  case 6: /* toplevel: tNAMERULES '{' regexp_sym_list '}'  */
#line 73 "gram.y"
                                        { namerules = (yyvsp[-1].lst); }
#line 1474 "gram.c"
    break;

  case 7: /* toplevel: tSTATE tSYMBOL '{' staterules '}'  */
#line 75 "gram.y"
                                        { define_state ((yyvsp[-3].node), NULL, (yyvsp[-1].lst)); }
#line 1480 "gram.c"
    break;

  case 8: /* toplevel: tSTATE tSYMBOL tEXTENDS tSYMBOL '{' staterules '}'  */
#line 77 "gram.y"
                                        { define_state ((yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-1].lst)); }
#line 1486 "gram.c"
    break;

  case 9: /* toplevel: stmt  */
#line 78 "gram.y"
                                        { list_append (global_stmts, (yyvsp[0].stmt)); }
#line 1492 "gram.c"
    break;

  case 10: /* regexp_sym_list: %empty  */
#line 81 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1498 "gram.c"
    break;

  case 11: /* regexp_sym_list: regexp_sym_list tREGEXP tSYMBOL ';'  */
#line 83 "gram.y"
                                        { list_append ((yyvsp[-3].lst), cons ((yyvsp[-2].node), (yyvsp[-1].node))); }
#line 1504 "gram.c"
    break;

  case 12: /* staterules: %empty  */
#line 86 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1510 "gram.c"
    break;

  case 13: /* staterules: staterules staterule  */
#line 87 "gram.y"
                                        { list_append ((yyvsp[-1].lst), (yyvsp[0].cons)); }
#line 1516 "gram.c"
    break;

  case 14: /* staterule: tBEGIN '{' stmt_list '}'  */
#line 89 "gram.y"
                                        { (yyval.cons) = cons (RULE_BEGIN, (yyvsp[-1].lst)); }
#line 1522 "gram.c"
    break;

  case 15: /* staterule: tEND '{' stmt_list '}'  */
#line 90 "gram.y"
                                        { (yyval.cons) = cons (RULE_END, (yyvsp[-1].lst)); }
#line 1528 "gram.c"
    break;

  case 16: /* staterule: tREGEXP '{' stmt_list '}'  */
#line 91 "gram.y"
                                        { (yyval.cons) = cons ((yyvsp[-3].node), (yyvsp[-1].lst)); }
#line 1534 "gram.c"
    break;

  case 17: /* staterule: tSYMBOL '{' stmt_list '}'  */
#line 92 "gram.y"
                                        { (yyval.cons) = cons ((yyvsp[-3].node), (yyvsp[-1].lst)); }
#line 1540 "gram.c"
    break;

  case 18: /* symbol_list: %empty  */
#line 95 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1546 "gram.c"
    break;

  case 19: /* symbol_list: rest_symbol_list  */
#line 96 "gram.y"
                                        { (yyval.lst) = (yyvsp[0].lst); }
#line 1552 "gram.c"
    break;

  case 20: /* rest_symbol_list: tSYMBOL  */
#line 99 "gram.y"
                                        { (yyval.lst) = list (); list_append ((yyval.lst), (yyvsp[0].node)); }
#line 1558 "gram.c"
    break;

  case 21: /* rest_symbol_list: rest_symbol_list ',' tSYMBOL  */
#line 100 "gram.y"
                                        { list_append ((yyvsp[-2].lst), (yyvsp[0].node)); }
#line 1564 "gram.c"
    break;

  case 22: /* locals: %empty  */
#line 103 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1570 "gram.c"
    break;

  case 23: /* locals: tLOCAL locals_rest ';'  */
#line 104 "gram.y"
                                        { (yyval.lst) = (yyvsp[-1].lst); }
#line 1576 "gram.c"
    break;

  case 24: /* locals_rest: local_def  */
#line 107 "gram.y"
                                        { (yyval.lst) = list (); list_append ((yyval.lst), (yyvsp[0].cons)); }
#line 1582 "gram.c"
    break;

  case 25: /* locals_rest: locals_rest ',' local_def  */
#line 108 "gram.y"
                                        { list_append ((yyvsp[-2].lst), (yyvsp[0].cons)); }
#line 1588 "gram.c"
    break;

  case 26: /* local_def: tSYMBOL  */
#line 111 "gram.y"
                                        { (yyval.cons) = cons ((yyvsp[0].node), NULL); }
#line 1594 "gram.c"
    break;

  case 27: /* local_def: tSYMBOL '=' expr  */
#line 112 "gram.y"
                                        { (yyval.cons) = cons ((yyvsp[-2].node), (yyvsp[0].expr)); }
#line 1600 "gram.c"
    break;

  case 28: /* stmt_list: %empty  */
#line 115 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1606 "gram.c"
    break;

  case 29: /* stmt_list: stmt_list stmt  */
#line 116 "gram.y"
                                        { list_append ((yyvsp[-1].lst), (yyvsp[0].stmt)); }
#line 1612 "gram.c"
    break;

  case 30: /* stmt: tRETURN ';'  */
#line 119 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sRETURN, NULL, NULL,
							NULL, NULL); }
#line 1619 "gram.c"
    break;

  case 31: /* stmt: tRETURN expr ';'  */
#line 121 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sRETURN, (yyvsp[-1].expr), NULL,
							NULL, NULL); }
#line 1626 "gram.c"
    break;

  case 32: /* stmt: tSUB tSYMBOL '(' symbol_list ')' '{' locals stmt_list '}'  */
#line 124 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sDEFSUB, (yyvsp[-7].node),
							cons (cons ((yyvsp[-5].lst), (yyvsp[-2].lst)),
							      (yyvsp[-1].lst)),
							NULL, NULL); }
#line 1635 "gram.c"
    break;

  case 33: /* stmt: '{' stmt_list '}'  */
#line 128 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sBLOCK, (yyvsp[-1].lst), NULL,
							NULL, NULL); }
#line 1642 "gram.c"
    break;

  case 34: /* stmt: tIF '(' expr ')' stmt  */
#line 130 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sIF, (yyvsp[-2].expr), (yyvsp[0].stmt), NULL,
							NULL); }
#line 1649 "gram.c"
    break;

  case 35: /* stmt: tIF '(' expr ')' stmt tELSE stmt  */
#line 133 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sIF, (yyvsp[-4].expr), (yyvsp[-2].stmt), (yyvsp[0].stmt),
							NULL); }
#line 1656 "gram.c"
    break;

  case 36: /* stmt: tWHILE '(' expr ')' stmt  */
#line 135 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sWHILE, (yyvsp[-2].expr), (yyvsp[0].stmt),
							NULL, NULL); }
#line 1663 "gram.c"
    break;

  case 37: /* stmt: tFOR '(' cond_expr ';' expr ';' cond_expr ')' stmt  */
#line 138 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sFOR, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr),
							(yyvsp[0].stmt)); }
#line 1670 "gram.c"
    break;

  case 38: /* stmt: expr ';'  */
#line 140 "gram.y"
                                        { (yyval.stmt) = mk_stmt (sEXPR, (yyvsp[-1].expr), NULL,
							NULL, NULL); }
#line 1677 "gram.c"
    break;

  case 39: /* expr: tSTRING  */
#line 144 "gram.y"
                                        { (yyval.expr) = mk_expr (eSTRING, (yyvsp[0].node), NULL,
							NULL); }
#line 1684 "gram.c"
    break;

  case 40: /* expr: tREGEXP  */
#line 146 "gram.y"
                                        { (yyval.expr) = mk_expr (eREGEXP, (yyvsp[0].node), NULL,
							NULL); }
#line 1691 "gram.c"
    break;

  case 41: /* expr: tINTEGER  */
#line 148 "gram.y"
                                        { (yyval.expr) = mk_expr (eINTEGER, (yyvsp[0].node), NULL,
							NULL); }
#line 1698 "gram.c"
    break;

  case 42: /* expr: tREAL  */
#line 150 "gram.y"
                                        { (yyval.expr) = mk_expr (eREAL, (yyvsp[0].node), NULL,
							NULL); }
#line 1705 "gram.c"
    break;

  case 43: /* expr: tSYMBOL  */
#line 152 "gram.y"
                                        { (yyval.expr) = mk_expr (eSYMBOL, (yyvsp[0].node), NULL,
							NULL); }
#line 1712 "gram.c"
    break;

  case 44: /* expr: '!' expr  */
#line 154 "gram.y"
                                        { (yyval.expr) = mk_expr (eNOT, (yyvsp[0].expr), NULL,
							NULL); }
#line 1719 "gram.c"
    break;

  case 45: /* expr: expr tAND expr  */
#line 156 "gram.y"
                                        { (yyval.expr) = mk_expr (eAND, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1725 "gram.c"
    break;

  case 46: /* expr: expr tOR expr  */
#line 157 "gram.y"
                                        { (yyval.expr) = mk_expr (eOR, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1731 "gram.c"
    break;

  case 47: /* expr: tSYMBOL '(' expr_list ')'  */
#line 158 "gram.y"
                                        { (yyval.expr) = mk_expr (eFCALL, (yyvsp[-3].node), (yyvsp[-1].lst),
							NULL); }
#line 1738 "gram.c"
    break;

  case 48: /* expr: tSYMBOL '=' expr  */
#line 160 "gram.y"
                                        { (yyval.expr) = mk_expr (eASSIGN, (yyvsp[-2].node), (yyvsp[0].expr),
							NULL); }
#line 1745 "gram.c"
    break;

  case 49: /* expr: tSYMBOL tADDASSIGN expr  */
#line 162 "gram.y"
                                        { (yyval.expr) = mk_expr (eADDASSIGN, (yyvsp[-2].node), (yyvsp[0].expr),
							NULL); }
#line 1752 "gram.c"
    break;

  case 50: /* expr: tSYMBOL tSUBASSIGN expr  */
#line 164 "gram.y"
                                        { (yyval.expr) = mk_expr (eSUBASSIGN, (yyvsp[-2].node), (yyvsp[0].expr),
							NULL); }
#line 1759 "gram.c"
    break;

  case 51: /* expr: tSYMBOL tMULASSIGN expr  */
#line 166 "gram.y"
                                        { (yyval.expr) = mk_expr (eMULASSIGN, (yyvsp[-2].node), (yyvsp[0].expr),
							NULL); }
#line 1766 "gram.c"
    break;

  case 52: /* expr: tSYMBOL tDIVASSIGN expr  */
#line 168 "gram.y"
                                        { (yyval.expr) = mk_expr (eDIVASSIGN, (yyvsp[-2].node), (yyvsp[0].expr),
							NULL); }
#line 1773 "gram.c"
    break;

  case 53: /* expr: tSYMBOL tPLUSPLUS  */
#line 170 "gram.y"
                                        { (yyval.expr) = mk_expr (ePOSTFIXADD, (yyvsp[-1].node), NULL,
							NULL); }
#line 1780 "gram.c"
    break;

  case 54: /* expr: tSYMBOL tMINUSMINUS  */
#line 172 "gram.y"
                                        { (yyval.expr) = mk_expr (ePOSTFIXSUB, (yyvsp[-1].node), NULL,
							NULL); }
#line 1787 "gram.c"
    break;

  case 55: /* expr: tPLUSPLUS tSYMBOL  */
#line 174 "gram.y"
                                        { (yyval.expr) = mk_expr (ePREFIXADD, (yyvsp[0].node), NULL,
							NULL); }
#line 1794 "gram.c"
    break;

  case 56: /* expr: tMINUSMINUS tSYMBOL  */
#line 176 "gram.y"
                                        { (yyval.expr) = mk_expr (ePREFIXSUB, (yyvsp[0].node), NULL,
							NULL); }
#line 1801 "gram.c"
    break;

  case 57: /* expr: expr '[' expr ']' '=' expr  */
#line 178 "gram.y"
                                        { (yyval.expr) = mk_expr (eARRAYASSIGN, (yyvsp[-5].expr), (yyvsp[-3].expr),
							(yyvsp[0].expr)); }
#line 1808 "gram.c"
    break;

  case 58: /* expr: '(' expr ')'  */
#line 180 "gram.y"
                                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 1814 "gram.c"
    break;

  case 59: /* expr: expr '[' expr ']'  */
#line 181 "gram.y"
                                        { (yyval.expr) = mk_expr (eARRAYREF, (yyvsp[-3].expr), (yyvsp[-1].expr),
							NULL); }
#line 1821 "gram.c"
    break;

  case 60: /* expr: expr '?' expr ':' expr  */
#line 183 "gram.y"
                                        { (yyval.expr) = mk_expr (eQUESTCOLON, (yyvsp[-4].expr), (yyvsp[-2].expr),
							(yyvsp[0].expr)); }
#line 1828 "gram.c"
    break;

  case 61: /* expr: expr '*' expr  */
#line 185 "gram.y"
                                        { (yyval.expr) = mk_expr (eMULT, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1834 "gram.c"
    break;

  case 62: /* expr: expr tDIV expr  */
#line 186 "gram.y"
                                        { (yyval.expr) = mk_expr (eDIV, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1840 "gram.c"
    break;

  case 63: /* expr: expr '+' expr  */
#line 187 "gram.y"
                                        { (yyval.expr) = mk_expr (ePLUS, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1846 "gram.c"
    break;

  case 64: /* expr: expr '-' expr  */
#line 188 "gram.y"
                                        { (yyval.expr) = mk_expr (eMINUS, (yyvsp[-2].expr), (yyvsp[0].expr),
							NULL); }
#line 1853 "gram.c"
    break;

  case 65: /* expr: expr '<' expr  */
#line 190 "gram.y"
                                        { (yyval.expr) = mk_expr (eLT, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1859 "gram.c"
    break;

  case 66: /* expr: expr '>' expr  */
#line 191 "gram.y"
                                        { (yyval.expr) = mk_expr (eGT, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1865 "gram.c"
    break;

  case 67: /* expr: expr tEQ expr  */
#line 192 "gram.y"
                                        { (yyval.expr) = mk_expr (eEQ, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1871 "gram.c"
    break;

  case 68: /* expr: expr tNE expr  */
#line 193 "gram.y"
                                        { (yyval.expr) = mk_expr (eNE, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1877 "gram.c"
    break;

  case 69: /* expr: expr tGE expr  */
#line 194 "gram.y"
                                        { (yyval.expr) = mk_expr (eGE, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1883 "gram.c"
    break;

  case 70: /* expr: expr tLE expr  */
#line 195 "gram.y"
                                        { (yyval.expr) = mk_expr (eLE, (yyvsp[-2].expr), (yyvsp[0].expr), NULL); }
#line 1889 "gram.c"
    break;

  case 71: /* cond_expr: %empty  */
#line 198 "gram.y"
                                        { (yyval.expr) = NULL; }
#line 1895 "gram.c"
    break;

  case 72: /* cond_expr: expr  */
#line 199 "gram.y"
                                        { (yyval.expr) = (yyvsp[0].expr); }
#line 1901 "gram.c"
    break;

  case 73: /* expr_list: %empty  */
#line 202 "gram.y"
                                        { (yyval.lst) = list (); }
#line 1907 "gram.c"
    break;

  case 74: /* expr_list: rest_expr_list  */
#line 203 "gram.y"
                                        { (yyval.lst) = (yyvsp[0].lst); }
#line 1913 "gram.c"
    break;

  case 75: /* rest_expr_list: expr  */
#line 206 "gram.y"
                                        { (yyval.lst) = list (); list_append ((yyval.lst), (yyvsp[0].expr)); }
#line 1919 "gram.c"
    break;

  case 76: /* rest_expr_list: rest_expr_list ',' expr  */
#line 207 "gram.y"
                                        { list_append ((yyvsp[-2].lst), (yyvsp[0].expr)); }
#line 1925 "gram.c"
    break;


#line 1929 "gram.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 210 "gram.y"


void
yyerror (msg)
     char *msg;
{
  fprintf (stderr, "%s:%d: %s\n", yyin_name, linenum, msg);
}
