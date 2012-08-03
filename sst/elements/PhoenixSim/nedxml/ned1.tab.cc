
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         ned1yyparse
#define yylex           ned1yylex
#define yyerror         ned1yyerror
#define yylval          ned1yylval
#define yychar          ned1yychar
#define yydebug         ned1yydebug
#define yynerrs         ned1yynerrs
#define yylloc          ned1yylloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 77 "ned1.y"


/*
 * Note:
 * This file contains about 5 shift-reduce conflicts, 3 of them around 'expression'.
 *
 * Plus one (real) ambiguity exists between submodule display string
 * and compound module display string if no connections are present.
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "nedyydefs.h"
#include "nedutil.h"
#include "nederror.h"
#include "nedexception.h"
#include "stringutil.h"
#include "commonutil.h"

#define YYDEBUG 1           /* allow debugging */
#define YYDEBUGGING_ON 0    /* turn on/off debugging */

#if YYDEBUG != 0
#define YYERROR_VERBOSE     /* more detailed error messages */
#include <string.h>         /* YYVERBOSE needs it */
#endif

#define yylloc ned1yylloc
#define yyin ned1yyin
#define yyout ned1yyout
#define yyrestart ned1yyrestart
#define yy_scan_string ned1yy_scan_string
#define yy_delete_buffer ned1yy_delete_buffer
extern FILE *yyin;
extern FILE *yyout;
struct yy_buffer_state;
struct yy_buffer_state *yy_scan_string(const char *str);
void yy_delete_buffer(struct yy_buffer_state *);
void yyrestart(FILE *);
int yylex();
void yyerror (const char *s);

#include "nedparser.h"
#include "nedfilebuffer.h"
#include "nedelements.h"
#include "nedutil.h"
#include "nedyylib.h"

USING_NAMESPACE

static struct NED1ParserState
{
    bool inLoop;
    bool inNetwork;
    bool inGroup;

    /* tmp flags, used with msg fields */
    bool isAbstract;
    bool isReadonly;

    /* NED-I: modules, channels, networks */
    NedFileElement *nedfile;
    CommentElement *comment;
    ImportElement *import;
    ExtendsElement *extends;
    ChannelElement *channel;
    NEDElement *module;  // in fact, CompoundModuleElement* or SimpleModule*
    ParametersElement *params;
    ParamElement *param;
    ParametersElement *substparams;
    ParamElement *substparam;
    PropertyElement *property;
    PropertyKeyElement *propkey;
    GatesElement *gates;
    GateElement *gate;
    GatesElement *gatesizes;
    GateElement *gatesize;
    SubmodulesElement *submods;
    SubmoduleElement *submod;
    ConnectionsElement *conns;
    ConnectionGroupElement *conngroup;
    ConnectionElement *conn;
    ChannelSpecElement *chanspec;
    LoopElement *loop;
    ConditionElement *condition;
} ps;

static void resetParserState()
{
    static NED1ParserState cleanps;
    ps = cleanps;
}

ChannelSpecElement *createChannelSpec(NEDElement *conn);
void removeRedundantChanSpecParams();
void createSubstparamsElementIfNotExists();
void createGatesizesElementIfNotExists();



/* Line 189 of yacc.c  */
#line 184 "ned1.tab.cc"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INCLUDE = 258,
     SIMPLE = 259,
     CHANNEL = 260,
     MODULE = 261,
     PARAMETERS = 262,
     GATES = 263,
     GATESIZES = 264,
     SUBMODULES = 265,
     CONNECTIONS = 266,
     DISPLAY = 267,
     IN = 268,
     OUT = 269,
     NOCHECK = 270,
     LEFT_ARROW = 271,
     RIGHT_ARROW = 272,
     FOR = 273,
     TO = 274,
     DO = 275,
     IF = 276,
     LIKE = 277,
     NETWORK = 278,
     ENDSIMPLE = 279,
     ENDMODULE = 280,
     ENDCHANNEL = 281,
     ENDNETWORK = 282,
     ENDFOR = 283,
     MACHINES = 284,
     ON = 285,
     CHANATTRNAME = 286,
     INTCONSTANT = 287,
     REALCONSTANT = 288,
     NAME = 289,
     STRINGCONSTANT = 290,
     CHARCONSTANT = 291,
     TRUE_ = 292,
     FALSE_ = 293,
     INPUT_ = 294,
     XMLDOC = 295,
     REF = 296,
     ANCESTOR = 297,
     CONSTDECL = 298,
     NUMERICTYPE = 299,
     STRINGTYPE = 300,
     BOOLTYPE = 301,
     XMLTYPE = 302,
     ANYTYPE = 303,
     CPLUSPLUS = 304,
     CPLUSPLUSBODY = 305,
     MESSAGE = 306,
     CLASS = 307,
     STRUCT = 308,
     ENUM = 309,
     NONCOBJECT = 310,
     EXTENDS = 311,
     FIELDS = 312,
     PROPERTIES = 313,
     ABSTRACT = 314,
     READONLY = 315,
     CHARTYPE = 316,
     SHORTTYPE = 317,
     INTTYPE = 318,
     LONGTYPE = 319,
     DOUBLETYPE = 320,
     UNSIGNED_ = 321,
     SIZEOF = 322,
     SUBMODINDEX = 323,
     PLUSPLUS = 324,
     EQ = 325,
     NE = 326,
     GT = 327,
     GE = 328,
     LS = 329,
     LE = 330,
     AND = 331,
     OR = 332,
     XOR = 333,
     NOT = 334,
     BIN_AND = 335,
     BIN_OR = 336,
     BIN_XOR = 337,
     BIN_COMPL = 338,
     SHIFT_LEFT = 339,
     SHIFT_RIGHT = 340,
     INVALID_CHAR = 341,
     UMIN = 342
   };
#endif
/* Tokens.  */
#define INCLUDE 258
#define SIMPLE 259
#define CHANNEL 260
#define MODULE 261
#define PARAMETERS 262
#define GATES 263
#define GATESIZES 264
#define SUBMODULES 265
#define CONNECTIONS 266
#define DISPLAY 267
#define IN 268
#define OUT 269
#define NOCHECK 270
#define LEFT_ARROW 271
#define RIGHT_ARROW 272
#define FOR 273
#define TO 274
#define DO 275
#define IF 276
#define LIKE 277
#define NETWORK 278
#define ENDSIMPLE 279
#define ENDMODULE 280
#define ENDCHANNEL 281
#define ENDNETWORK 282
#define ENDFOR 283
#define MACHINES 284
#define ON 285
#define CHANATTRNAME 286
#define INTCONSTANT 287
#define REALCONSTANT 288
#define NAME 289
#define STRINGCONSTANT 290
#define CHARCONSTANT 291
#define TRUE_ 292
#define FALSE_ 293
#define INPUT_ 294
#define XMLDOC 295
#define REF 296
#define ANCESTOR 297
#define CONSTDECL 298
#define NUMERICTYPE 299
#define STRINGTYPE 300
#define BOOLTYPE 301
#define XMLTYPE 302
#define ANYTYPE 303
#define CPLUSPLUS 304
#define CPLUSPLUSBODY 305
#define MESSAGE 306
#define CLASS 307
#define STRUCT 308
#define ENUM 309
#define NONCOBJECT 310
#define EXTENDS 311
#define FIELDS 312
#define PROPERTIES 313
#define ABSTRACT 314
#define READONLY 315
#define CHARTYPE 316
#define SHORTTYPE 317
#define INTTYPE 318
#define LONGTYPE 319
#define DOUBLETYPE 320
#define UNSIGNED_ 321
#define SIZEOF 322
#define SUBMODINDEX 323
#define PLUSPLUS 324
#define EQ 325
#define NE 326
#define GT 327
#define GE 328
#define LS 329
#define LE 330
#define AND 331
#define OR 332
#define XOR 333
#define NOT 334
#define BIN_AND 335
#define BIN_OR 336
#define BIN_XOR 337
#define BIN_COMPL 338
#define SHIFT_LEFT 339
#define SHIFT_RIGHT 340
#define INVALID_CHAR 341
#define UMIN 342




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 413 "ned1.tab.cc"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   781

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  104
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  108
/* YYNRULES -- Number of rules.  */
#define YYNRULES  241
/* YYNRULES -- Number of states.  */
#define YYNSTATES  419

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   342

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    93,     2,     2,
     101,   102,    91,    89,    97,    90,   103,    92,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    88,    96,
       2,   100,     2,    87,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    98,     2,    99,    94,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    95
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,     9,    11,    13,    15,    17,
      19,    23,    27,    29,    31,    35,    38,    39,    41,    46,
      50,    54,    57,    62,    65,    69,    72,    80,    83,    87,
      90,    92,    93,    98,   100,   101,   102,   107,   110,   111,
     115,   117,   119,   123,   126,   130,   135,   140,   144,   148,
     152,   156,   158,   159,   160,   165,   167,   168,   173,   177,
     182,   186,   190,   192,   196,   198,   202,   204,   208,   210,
     212,   213,   214,   219,   221,   222,   225,   227,   228,   235,
     236,   244,   245,   254,   255,   265,   269,   271,   272,   275,
     277,   278,   283,   284,   291,   294,   295,   299,   301,   305,
     306,   312,   318,   322,   325,   326,   328,   329,   332,   334,
     335,   340,   341,   348,   351,   352,   356,   358,   361,   366,
     367,   369,   370,   371,   377,   378,   383,   385,   386,   389,
     391,   393,   395,   396,   404,   408,   410,   416,   419,   420,
     423,   424,   427,   429,   436,   445,   452,   461,   465,   467,
     470,   472,   475,   477,   480,   483,   485,   488,   492,   494,
     497,   499,   502,   504,   507,   510,   512,   515,   517,   519,
     521,   524,   527,   531,   537,   540,   544,   546,   548,   555,
     560,   562,   566,   570,   574,   578,   582,   586,   590,   593,
     597,   601,   605,   609,   613,   617,   621,   625,   629,   632,
     636,   640,   644,   647,   651,   655,   661,   666,   671,   676,
     680,   685,   692,   701,   712,   714,   716,   718,   720,   723,
     727,   731,   734,   736,   740,   745,   747,   749,   751,   753,
     755,   757,   759,   761,   763,   767,   771,   774,   777,   779,
     780,   782
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     105,     0,    -1,   106,    -1,   106,   107,    -1,    -1,   108,
      -1,   111,    -1,   116,    -1,   119,    -1,   195,    -1,     3,
     109,    96,    -1,   109,    97,   110,    -1,   110,    -1,    35,
      -1,   112,   113,   115,    -1,     5,    34,    -1,    -1,   114,
      -1,   114,    31,   199,   210,    -1,    31,   199,   210,    -1,
      26,    34,   210,    -1,    26,   210,    -1,   117,   124,   130,
     118,    -1,     4,    34,    -1,    24,    34,   210,    -1,    24,
     210,    -1,   120,   124,   130,   139,   169,   122,   121,    -1,
       6,    34,    -1,    25,    34,   210,    -1,    25,   210,    -1,
     123,    -1,    -1,    12,    88,    35,    96,    -1,   125,    -1,
      -1,    -1,     7,    88,   126,   127,    -1,   128,    96,    -1,
      -1,   128,    97,   129,    -1,   129,    -1,    34,    -1,    34,
      88,    44,    -1,    43,    34,    -1,    34,    88,    43,    -1,
      34,    88,    43,    44,    -1,    34,    88,    44,    43,    -1,
      34,    88,    45,    -1,    34,    88,    46,    -1,    34,    88,
      47,    -1,    34,    88,    48,    -1,   131,    -1,    -1,    -1,
       8,    88,   132,   133,    -1,   134,    -1,    -1,   134,    13,
     135,    96,    -1,    13,   135,    96,    -1,   134,    14,   137,
      96,    -1,    14,   137,    96,    -1,   135,    97,   136,    -1,
     136,    -1,    34,    98,    99,    -1,    34,    -1,   137,    97,
     138,    -1,   138,    -1,    34,    98,    99,    -1,    34,    -1,
     140,    -1,    -1,    -1,    10,    88,   141,   142,    -1,   143,
      -1,    -1,   143,   144,    -1,   144,    -1,    -1,    34,    88,
      34,   210,   145,   149,    -1,    -1,    34,    88,    34,   198,
     210,   146,   149,    -1,    -1,    34,    88,    34,    22,    34,
     210,   147,   149,    -1,    -1,    34,    88,    34,   198,    22,
      34,   210,   148,   149,    -1,   150,   160,   168,    -1,   151,
      -1,    -1,   151,   152,    -1,   152,    -1,    -1,     7,    88,
     153,   155,    -1,    -1,     7,    21,   199,    88,   154,   155,
      -1,   156,    96,    -1,    -1,   156,    97,   157,    -1,   157,
      -1,    34,   100,   199,    -1,    -1,    34,   100,    39,   158,
     159,    -1,   101,   199,    97,    35,   102,    -1,   101,   199,
     102,    -1,   101,   102,    -1,    -1,   161,    -1,    -1,   161,
     162,    -1,   162,    -1,    -1,     9,    88,   163,   165,    -1,
      -1,     9,    21,   199,    88,   164,   165,    -1,   166,    96,
      -1,    -1,   166,    97,   167,    -1,   167,    -1,    34,   198,
      -1,    12,    88,    35,    96,    -1,    -1,   170,    -1,    -1,
      -1,    11,    15,    88,   171,   173,    -1,    -1,    11,    88,
     172,   173,    -1,   174,    -1,    -1,   174,   175,    -1,   175,
      -1,   176,    -1,   183,    -1,    -1,    18,   177,   178,    20,
     182,    28,   210,    -1,   179,    97,   178,    -1,   179,    -1,
      34,   100,   199,    19,   199,    -1,    21,   199,    -1,    -1,
      12,    35,    -1,    -1,   182,   183,    -1,   183,    -1,   184,
      17,   188,   180,   181,   211,    -1,   184,    17,   192,    17,
     188,   180,   181,   211,    -1,   184,    16,   188,   180,   181,
     211,    -1,   184,    16,   192,    16,   188,   180,   181,   211,
      -1,   185,   103,   186,    -1,   187,    -1,    34,   198,    -1,
      34,    -1,    34,   198,    -1,    34,    -1,    34,    69,    -1,
      34,   198,    -1,    34,    -1,    34,    69,    -1,   189,   103,
     190,    -1,   191,    -1,    34,   198,    -1,    34,    -1,    34,
     198,    -1,    34,    -1,    34,    69,    -1,    34,   198,    -1,
      34,    -1,    34,    69,    -1,   193,    -1,    34,    -1,   194,
      -1,   193,   194,    -1,    31,   199,    -1,   196,   150,   197,
      -1,    23,    34,    88,    34,   210,    -1,    27,   210,    -1,
      98,   199,    99,    -1,   201,    -1,   200,    -1,    40,   101,
     206,    97,   206,   102,    -1,    40,   101,   206,   102,    -1,
     202,    -1,   101,   201,   102,    -1,   201,    89,   201,    -1,
     201,    90,   201,    -1,   201,    91,   201,    -1,   201,    92,
     201,    -1,   201,    93,   201,    -1,   201,    94,   201,    -1,
      90,   201,    -1,   201,    70,   201,    -1,   201,    71,   201,
      -1,   201,    72,   201,    -1,   201,    73,   201,    -1,   201,
      74,   201,    -1,   201,    75,   201,    -1,   201,    76,   201,
      -1,   201,    77,   201,    -1,   201,    78,   201,    -1,    79,
     201,    -1,   201,    80,   201,    -1,   201,    81,   201,    -1,
     201,    82,   201,    -1,    83,   201,    -1,   201,    84,   201,
      -1,   201,    85,   201,    -1,   201,    87,   201,    88,   201,
      -1,    63,   101,   201,   102,    -1,    65,   101,   201,   102,
      -1,    45,   101,   201,   102,    -1,    34,   101,   102,    -1,
      34,   101,   201,   102,    -1,    34,   101,   201,    97,   201,
     102,    -1,    34,   101,   201,    97,   201,    97,   201,   102,
      -1,    34,   101,   201,    97,   201,    97,   201,    97,   201,
     102,    -1,   203,    -1,   204,    -1,   205,    -1,    34,    -1,
      41,    34,    -1,    41,    42,    34,    -1,    42,    41,    34,
      -1,    42,    34,    -1,    68,    -1,    68,   101,   102,    -1,
      67,   101,    34,   102,    -1,   206,    -1,   207,    -1,   208,
      -1,    35,    -1,    37,    -1,    38,    -1,    32,    -1,    33,
      -1,   209,    -1,   209,    32,    34,    -1,   209,    33,    34,
      -1,    32,    34,    -1,    33,    34,    -1,    96,    -1,    -1,
      97,    -1,    96,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   185,   185,   189,   190,   194,   196,   198,   200,   202,
     210,   215,   216,   220,   233,   241,   253,   255,   260,   270,
     283,   284,   291,   302,   311,   312,   319,   333,   342,   343,
     350,   351,   355,   382,   383,   388,   387,   399,   400,   404,
     409,   420,   426,   432,   437,   442,   447,   452,   457,   462,
     467,   478,   479,   484,   483,   495,   496,   500,   501,   502,
     503,   507,   508,   512,   520,   530,   531,   535,   543,   556,
     557,   562,   561,   573,   574,   578,   579,   584,   583,   595,
     594,   607,   606,   619,   618,   634,   643,   645,   649,   650,
     655,   654,   661,   660,   673,   674,   678,   679,   683,   691,
     690,   703,   711,   715,   716,   723,   725,   729,   730,   735,
     734,   741,   740,   753,   754,   758,   759,   763,   777,   798,
     805,   806,   811,   810,   821,   820,   833,   834,   838,   839,
     843,   844,   849,   848,   863,   864,   868,   879,   886,   890,
     914,   918,   919,   923,   929,   936,   943,   954,   955,   959,
     966,   975,   980,   984,   992,  1000,  1007,  1018,  1019,  1023,
    1028,  1035,  1040,  1044,  1052,  1057,  1061,  1070,  1079,  1085,
    1086,  1090,  1104,  1114,  1128,  1138,  1144,  1148,  1159,  1161,
    1166,  1167,  1170,  1172,  1174,  1176,  1178,  1180,  1183,  1187,
    1189,  1191,  1193,  1195,  1197,  1200,  1202,  1204,  1207,  1211,
    1213,  1215,  1218,  1221,  1223,  1225,  1228,  1230,  1232,  1235,
    1237,  1239,  1241,  1243,  1248,  1249,  1250,  1254,  1259,  1264,
    1269,  1274,  1282,  1284,  1286,  1291,  1292,  1293,  1297,  1302,
    1304,  1309,  1311,  1313,  1318,  1319,  1320,  1321,  1324,  1324,
    1326,  1326
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INCLUDE", "SIMPLE", "CHANNEL", "MODULE",
  "PARAMETERS", "GATES", "GATESIZES", "SUBMODULES", "CONNECTIONS",
  "DISPLAY", "IN", "OUT", "NOCHECK", "LEFT_ARROW", "RIGHT_ARROW", "FOR",
  "TO", "DO", "IF", "LIKE", "NETWORK", "ENDSIMPLE", "ENDMODULE",
  "ENDCHANNEL", "ENDNETWORK", "ENDFOR", "MACHINES", "ON", "CHANATTRNAME",
  "INTCONSTANT", "REALCONSTANT", "NAME", "STRINGCONSTANT", "CHARCONSTANT",
  "TRUE_", "FALSE_", "INPUT_", "XMLDOC", "REF", "ANCESTOR", "CONSTDECL",
  "NUMERICTYPE", "STRINGTYPE", "BOOLTYPE", "XMLTYPE", "ANYTYPE",
  "CPLUSPLUS", "CPLUSPLUSBODY", "MESSAGE", "CLASS", "STRUCT", "ENUM",
  "NONCOBJECT", "EXTENDS", "FIELDS", "PROPERTIES", "ABSTRACT", "READONLY",
  "CHARTYPE", "SHORTTYPE", "INTTYPE", "LONGTYPE", "DOUBLETYPE",
  "UNSIGNED_", "SIZEOF", "SUBMODINDEX", "PLUSPLUS", "EQ", "NE", "GT", "GE",
  "LS", "LE", "AND", "OR", "XOR", "NOT", "BIN_AND", "BIN_OR", "BIN_XOR",
  "BIN_COMPL", "SHIFT_LEFT", "SHIFT_RIGHT", "INVALID_CHAR", "'?'", "':'",
  "'+'", "'-'", "'*'", "'/'", "'%'", "'^'", "UMIN", "';'", "','", "'['",
  "']'", "'='", "'('", "')'", "'.'", "$accept", "networkdescription",
  "somedefinitions", "definition", "import", "filenames", "filename",
  "channeldefinition", "channelheader", "opt_channelattrblock",
  "channelattrblock", "endchannel", "simpledefinition", "simpleheader",
  "endsimple", "moduledefinition", "moduleheader", "endmodule",
  "opt_displayblock", "displayblock", "opt_paramblock", "paramblock",
  "$@1", "opt_parameters", "parameters", "parameter", "opt_gateblock",
  "gateblock", "$@2", "opt_gates", "gates", "gatesI", "gateI", "gatesO",
  "gateO", "opt_submodblock", "submodblock", "$@3", "opt_submodules",
  "submodules", "submodule", "$@4", "$@5", "$@6", "$@7", "submodule_body",
  "opt_substparamblocks", "substparamblocks", "substparamblock", "$@8",
  "$@9", "opt_substparameters", "substparameters", "substparameter",
  "$@10", "inputvalue", "opt_gatesizeblocks", "gatesizeblocks",
  "gatesizeblock", "$@11", "$@12", "opt_gatesizes", "gatesizes",
  "gatesize", "opt_submod_displayblock", "opt_connblock", "connblock",
  "$@13", "$@14", "opt_connections", "connections", "connection",
  "loopconnection", "$@15", "loopvarlist", "loopvar", "opt_conncondition",
  "opt_conn_displaystr", "notloopconnections", "notloopconnection",
  "leftgatespec", "leftmod", "leftgate", "parentleftgate", "rightgatespec",
  "rightmod", "rightgate", "parentrightgate", "channeldescr",
  "channelattrs", "chanattr", "networkdefinition", "networkheader",
  "endnetwork", "vector", "expression", "xmldocvalue", "expr",
  "simple_expr", "parameter_expr", "special_expr", "literal",
  "stringliteral", "boolliteral", "numliteral", "quantity",
  "opt_semicolon", "comma_or_semicolon", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,    63,    58,    43,
      45,    42,    47,    37,    94,   342,    59,    44,    91,    93,
      61,    40,    41,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   104,   105,   106,   106,   107,   107,   107,   107,   107,
     108,   109,   109,   110,   111,   112,   113,   113,   114,   114,
     115,   115,   116,   117,   118,   118,   119,   120,   121,   121,
     122,   122,   123,   124,   124,   126,   125,   127,   127,   128,
     128,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   130,   130,   132,   131,   133,   133,   134,   134,   134,
     134,   135,   135,   136,   136,   137,   137,   138,   138,   139,
     139,   141,   140,   142,   142,   143,   143,   145,   144,   146,
     144,   147,   144,   148,   144,   149,   150,   150,   151,   151,
     153,   152,   154,   152,   155,   155,   156,   156,   157,   158,
     157,   159,   159,   159,   159,   160,   160,   161,   161,   163,
     162,   164,   162,   165,   165,   166,   166,   167,   168,   168,
     169,   169,   171,   170,   172,   170,   173,   173,   174,   174,
     175,   175,   177,   176,   178,   178,   179,   180,   180,   181,
     181,   182,   182,   183,   183,   183,   183,   184,   184,   185,
     185,   186,   186,   186,   187,   187,   187,   188,   188,   189,
     189,   190,   190,   190,   191,   191,   191,   192,   193,   193,
     193,   194,   195,   196,   197,   198,   199,   199,   200,   200,
     201,   201,   201,   201,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   201,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   201,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   201,   202,   202,   202,   203,   203,   203,
     203,   203,   204,   204,   204,   205,   205,   205,   206,   207,
     207,   208,   208,   208,   209,   209,   209,   209,   210,   210,
     211,   211
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     0,     1,     1,     1,     1,     1,
       3,     3,     1,     1,     3,     2,     0,     1,     4,     3,
       3,     2,     4,     2,     3,     2,     7,     2,     3,     2,
       1,     0,     4,     1,     0,     0,     4,     2,     0,     3,
       1,     1,     3,     2,     3,     4,     4,     3,     3,     3,
       3,     1,     0,     0,     4,     1,     0,     4,     3,     4,
       3,     3,     1,     3,     1,     3,     1,     3,     1,     1,
       0,     0,     4,     1,     0,     2,     1,     0,     6,     0,
       7,     0,     8,     0,     9,     3,     1,     0,     2,     1,
       0,     4,     0,     6,     2,     0,     3,     1,     3,     0,
       5,     5,     3,     2,     0,     1,     0,     2,     1,     0,
       4,     0,     6,     2,     0,     3,     1,     2,     4,     0,
       1,     0,     0,     5,     0,     4,     1,     0,     2,     1,
       1,     1,     0,     7,     3,     1,     5,     2,     0,     2,
       0,     2,     1,     6,     8,     6,     8,     3,     1,     2,
       1,     2,     1,     2,     2,     1,     2,     3,     1,     2,
       1,     2,     1,     2,     2,     1,     2,     1,     1,     1,
       2,     2,     3,     5,     2,     3,     1,     1,     6,     4,
       1,     3,     3,     3,     3,     3,     3,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     3,
       3,     3,     2,     3,     3,     5,     4,     4,     4,     3,
       4,     6,     8,    10,     1,     1,     1,     1,     2,     3,
       3,     2,     1,     3,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     2,     2,     1,     0,
       1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,     2,     1,     0,     0,     0,     0,     0,     3,
       5,     6,    16,     7,    34,     8,    34,     9,    87,    13,
       0,    12,    23,    15,    27,     0,     0,     0,    17,     0,
      52,    33,    52,     0,     0,    86,    89,    10,     0,     0,
     231,   232,   217,   228,   229,   230,     0,     0,     0,     0,
       0,     0,     0,   222,     0,     0,     0,     0,   239,   177,
     176,   180,   214,   215,   216,   225,   226,   227,   233,   239,
      14,     0,    35,     0,     0,    51,    70,     0,    90,   239,
     172,    88,    11,   239,   236,   237,     0,     0,   218,     0,
     221,     0,     0,     0,     0,     0,     0,   198,   202,   188,
       0,   238,    19,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   239,    21,   239,    38,
      53,   239,    22,     0,   121,    69,     0,    95,   174,   173,
     209,     0,     0,   219,   220,     0,     0,     0,     0,   223,
     181,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     199,   200,   201,   203,   204,     0,   182,   183,   184,   185,
     186,   187,   234,   235,    20,    18,    41,     0,    36,     0,
      40,    56,   239,    25,    71,     0,    31,   120,    92,     0,
      91,     0,    97,     0,   210,     0,   179,   208,   206,   207,
     224,     0,     0,    43,    37,     0,     0,     0,    54,    55,
      24,    74,     0,   124,     0,     0,    30,    95,     0,    94,
       0,     0,     0,   205,    44,    42,    47,    48,    49,    50,
      39,    64,     0,    62,    68,     0,    66,     0,     0,     0,
      72,    73,    76,   122,   127,     0,   239,    26,    93,    99,
      98,    96,     0,   211,   178,    45,    46,     0,    58,     0,
       0,    60,     0,     0,     0,     0,    75,   127,   132,   155,
     125,   126,   129,   130,   131,     0,     0,   148,     0,   239,
      29,   104,     0,    63,    61,    67,    65,    57,    59,   239,
     123,     0,   156,     0,   154,   128,     0,     0,     0,    32,
      28,     0,   100,     0,   212,     0,   239,    77,     0,     0,
     135,     0,     0,   165,   138,     0,   158,     0,   167,   169,
     138,     0,   152,   147,   103,     0,     0,   239,     0,    79,
      87,     0,     0,     0,   175,   171,   166,   164,     0,   140,
       0,     0,   170,   140,     0,   153,   151,     0,   102,   213,
      81,   239,    87,    78,   106,     0,     0,   142,   134,   137,
       0,     0,   162,   157,   165,   138,     0,   138,     0,    87,
      83,    80,     0,   119,   105,   108,     0,   239,   141,   139,
     241,   240,   145,   163,   161,   140,   143,   140,   101,    82,
      87,     0,   109,     0,    85,   107,   136,   133,     0,     0,
      84,     0,   114,     0,   146,   144,   111,     0,   110,     0,
     116,     0,   114,   117,   113,     0,   118,   112,   115
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,     9,    10,    20,    21,    11,    12,    27,
      28,    70,    13,    14,   132,    15,    16,   247,   215,   216,
      30,    31,   129,   178,   179,   180,    74,    75,   181,   208,
     209,   232,   233,   235,   236,   134,   135,   211,   240,   241,
     242,   330,   352,   369,   390,   353,   354,    35,    36,   137,
     217,   190,   191,   192,   281,   302,   373,   374,   375,   402,
     412,   408,   409,   410,   394,   186,   187,   267,   244,   270,
     271,   272,   273,   291,   309,   310,   339,   361,   356,   274,
     275,   276,   323,   277,   314,   315,   363,   316,   317,   318,
     319,    17,    18,    80,   337,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,   102,   382
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -341
static const yytype_int16 yypact[] =
{
    -341,    31,   166,  -341,    14,     6,    30,    68,   102,  -341,
    -341,  -341,   123,  -341,   134,  -341,   134,  -341,   154,  -341,
      26,  -341,  -341,  -341,  -341,    86,   236,   157,   146,   100,
     202,  -341,   202,     1,   169,   154,  -341,  -341,    14,   190,
     192,   195,   136,  -341,  -341,  -341,   141,    85,    12,   150,
     161,   162,   163,   165,   290,   290,   290,   290,   144,  -341,
     601,  -341,  -341,  -341,  -341,  -341,  -341,  -341,   107,    -5,
    -341,   236,  -341,   159,   228,  -341,   262,   236,  -341,   144,
    -341,  -341,  -341,   144,  -341,  -341,    83,   240,  -341,   245,
    -341,   252,   290,   290,   290,   253,   186,  -341,  -341,  -341,
     411,  -341,  -341,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   261,   263,   144,  -341,   144,    -6,
    -341,    -2,  -341,   208,   287,  -341,   212,   268,  -341,  -341,
    -341,   312,   -67,  -341,  -341,   444,   477,   510,   204,  -341,
    -341,   164,   164,   164,   164,   164,   164,   676,   626,   651,
     687,   200,   249,   126,   126,   576,    88,    88,   213,   213,
     213,   213,  -341,  -341,  -341,  -341,   220,   276,  -341,    47,
    -341,   139,   144,  -341,  -341,     5,   299,  -341,  -341,   214,
    -341,    62,  -341,   290,  -341,   240,  -341,  -341,  -341,  -341,
    -341,   290,   188,  -341,  -341,    -6,   278,   279,  -341,   151,
    -341,   282,   229,  -341,   230,   296,  -341,   268,   160,  -341,
     268,   345,   234,   601,   286,   301,  -341,  -341,  -341,  -341,
    -341,   247,    79,  -341,   248,    90,  -341,   278,   279,   259,
    -341,   282,  -341,  -341,     8,   313,     0,  -341,  -341,  -341,
    -341,  -341,   290,  -341,  -341,  -341,  -341,   250,  -341,   278,
     251,  -341,   279,    94,   110,   317,  -341,     8,  -341,   -62,
    -341,     8,  -341,  -341,  -341,   187,   256,  -341,   258,   144,
    -341,   255,   378,  -341,  -341,  -341,  -341,  -341,  -341,   -10,
    -341,   318,  -341,   236,   257,  -341,    78,    78,   327,  -341,
    -341,    66,  -341,   290,  -341,   328,    -9,  -341,   264,   343,
     269,   266,   236,    -8,   346,   265,  -341,   354,   340,  -341,
     346,   355,   -55,  -341,  -341,    33,   543,   144,   341,  -341,
     154,   236,   342,   318,  -341,  -341,  -341,   271,   236,   365,
     344,   347,  -341,   365,   347,  -341,  -341,   360,  -341,  -341,
    -341,   144,   154,  -341,   370,   379,    98,  -341,  -341,  -341,
     372,   112,   -36,  -341,   -59,   346,   112,   346,   298,   154,
    -341,  -341,     4,   396,   370,  -341,   236,   144,  -341,  -341,
    -341,  -341,  -341,  -341,  -341,   365,  -341,   365,  -341,  -341,
     154,   236,  -341,   322,  -341,  -341,  -341,  -341,   112,   112,
    -341,   323,   390,   377,  -341,  -341,  -341,   315,  -341,   116,
    -341,   332,   390,  -341,  -341,   390,  -341,  -341,  -341
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -341,  -341,  -341,  -341,  -341,  -341,   393,  -341,  -341,  -341,
    -341,  -341,  -341,  -341,  -341,  -341,  -341,  -341,  -341,  -341,
     417,  -341,  -341,  -341,  -341,   235,   409,  -341,  -341,  -341,
    -341,   206,   185,   207,   184,  -341,  -341,  -341,  -341,  -341,
     216,  -341,  -341,  -341,  -341,  -331,   443,  -341,   429,  -341,
    -341,   260,  -341,   246,  -341,  -341,  -341,  -341,    99,  -341,
    -341,    64,  -341,    59,  -341,  -341,  -341,  -341,  -341,   211,
    -341,   219,  -341,  -341,   173,  -341,  -315,  -340,  -341,  -305,
    -341,  -341,  -341,  -341,  -184,  -341,  -341,  -341,   182,  -341,
     176,  -341,  -341,  -341,  -265,   -71,  -341,   -38,  -341,  -341,
    -341,  -341,   -85,  -341,  -341,  -341,   -68,  -261
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -169
static const yytype_int16 yytable[] =
{
     128,   127,   142,   366,   294,   343,   136,   292,  -168,  -168,
     336,   138,   305,   328,   345,   139,    97,    98,    99,   100,
     212,   371,    77,  -168,   306,   391,   268,   357,   176,   126,
     195,     3,   182,   383,   279,   196,   293,   177,   389,   293,
      22,  -150,   269,   293,  -160,   398,    90,   399,   141,    19,
     385,   378,   387,    91,   145,   146,   147,   346,   174,   400,
     175,   336,   293,   183,    23,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   101,   101,   293,    78,
     293,   101,   392,   213,   101,  -160,   101,   384,    40,    41,
      42,    43,    24,    44,    45,   386,    46,    47,    48,   312,
     222,    49,   313,   320,   210,    40,    41,    42,    43,    88,
      44,    45,    37,    38,    47,    48,   377,    89,    49,    50,
     347,    51,   269,    52,    53,   348,    25,   404,   405,   124,
     125,    29,   413,   204,   205,    54,    50,   250,    51,    55,
      52,    53,   206,   207,    26,   221,    56,   365,   219,   220,
     367,    33,    54,   223,   237,   238,    55,    57,   324,     4,
       5,     6,     7,    56,    39,   258,   259,    71,   280,   120,
     121,   122,   123,    69,    57,   140,   261,   262,    72,     8,
     287,   259,    40,    41,    42,    43,    79,    44,    45,   249,
      46,    47,    48,   296,   297,    49,   288,   262,   380,   381,
      73,   300,   414,   415,   282,   118,   119,   120,   121,   122,
     123,   307,   311,    50,    83,    51,    84,    52,    53,    85,
     325,   224,   225,   226,   227,   228,   229,    86,   329,    54,
     101,   335,    87,    55,   112,   113,   114,   130,   115,   116,
      56,    92,   131,   118,   119,   120,   121,   122,   123,   350,
     355,    57,    93,    94,    95,   326,    96,   359,    40,    41,
      42,    43,   133,    44,    45,    43,    46,    47,    48,   143,
     112,    49,   114,   370,   115,   116,   144,   148,   149,   118,
     119,   120,   121,   122,   123,   172,   184,   173,   185,    50,
     188,    51,   189,    52,    53,   396,   200,   123,   202,   397,
     203,   214,   231,   234,   218,    54,   239,   243,   245,    55,
     401,   246,    40,    41,    42,    43,    56,    44,    45,   112,
     255,    47,    48,   115,   116,    49,   254,    57,   118,   119,
     120,   121,   122,   123,   256,   257,   260,   265,   278,   283,
     285,   289,   308,    50,   299,    51,   301,    52,    53,   298,
    -149,   322,   327,   332,   331,   334,   333,   338,   340,    54,
     341,   312,   344,    55,  -159,   351,   269,   360,   362,   372,
      56,   364,   103,   104,   105,   106,   107,   108,   109,   110,
     111,    57,   112,   113,   114,   368,   115,   116,   376,   117,
     388,   118,   119,   120,   121,   122,   123,   379,   393,   193,
     403,   406,   411,   293,   194,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   407,   112,   113,   114,   416,   115,
     116,    82,   117,    32,   118,   119,   120,   121,   122,   123,
     230,    76,   252,   263,   284,   264,   286,   253,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   266,   112,   113,
     114,    34,   115,   116,    81,   117,   251,   118,   119,   120,
     121,   122,   123,   395,   418,   303,   417,   248,   290,   321,
     304,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     295,   112,   113,   114,   342,   115,   116,     0,   117,     0,
     118,   119,   120,   121,   122,   123,   358,     0,     0,     0,
       0,     0,     0,   150,   103,   104,   105,   106,   107,   108,
     109,   110,   111,     0,   112,   113,   114,     0,   115,   116,
       0,   117,     0,   118,   119,   120,   121,   122,   123,     0,
       0,     0,     0,     0,     0,     0,   197,   103,   104,   105,
     106,   107,   108,   109,   110,   111,     0,   112,   113,   114,
       0,   115,   116,     0,   117,     0,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,     0,     0,   198,
     103,   104,   105,   106,   107,   108,   109,   110,   111,     0,
     112,   113,   114,     0,   115,   116,     0,   117,     0,   118,
     119,   120,   121,   122,   123,     0,     0,     0,     0,     0,
       0,     0,   199,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,   112,   113,   114,     0,   115,   116,     0,
     117,     0,   118,   119,   120,   121,   122,   123,     0,     0,
       0,     0,     0,     0,     0,   349,   103,   104,   105,   106,
     107,   108,   109,   110,   111,     0,   112,   113,   114,     0,
     115,   116,     0,   117,   201,   118,   119,   120,   121,   122,
     123,   103,   104,   105,   106,   107,   108,   109,   110,   111,
       0,   112,   113,   114,     0,   115,   116,     0,   117,     0,
     118,   119,   120,   121,   122,   123,   103,   104,   105,   106,
     107,   108,   109,     0,   111,     0,   112,   113,   114,     0,
     115,   116,     0,     0,     0,   118,   119,   120,   121,   122,
     123,   103,   104,   105,   106,   107,   108,   109,     0,     0,
       0,   112,   113,   114,     0,   115,   116,     0,     0,     0,
     118,   119,   120,   121,   122,   123,   103,   104,   105,   106,
     107,   108,     0,     0,     0,     0,   112,   113,   114,     0,
     115,   116,     0,     0,     0,   118,   119,   120,   121,   122,
     123,   115,   116,     0,     0,     0,   118,   119,   120,   121,
     122,   123
};

static const yytype_int16 yycheck[] =
{
      71,    69,    87,   343,   269,   320,    77,    69,    16,    17,
      69,    79,    22,    22,    69,    83,    54,    55,    56,    57,
      15,   352,    21,    31,   289,    21,    18,   332,    34,    34,
      97,     0,    34,    69,    34,   102,    98,    43,   369,    98,
      34,   103,    34,    98,   103,   385,    34,   387,    86,    35,
     365,   356,   367,    41,    92,    93,    94,   322,   126,   390,
     128,    69,    98,   131,    34,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,    96,    96,    98,    88,
      98,    96,    88,    88,    96,   103,    96,   362,    32,    33,
      34,    35,    34,    37,    38,   366,    40,    41,    42,    31,
     195,    45,    34,   297,   182,    32,    33,    34,    35,    34,
      37,    38,    96,    97,    41,    42,    28,    42,    45,    63,
      97,    65,    34,    67,    68,   102,    34,   398,   399,    32,
      33,     7,   407,    96,    97,    79,    63,   218,    65,    83,
      67,    68,    13,    14,    31,   193,    90,   341,    96,    97,
     344,     7,    79,   201,    13,    14,    83,   101,   102,     3,
       4,     5,     6,    90,    88,    96,    97,    31,   246,    91,
      92,    93,    94,    26,   101,   102,    96,    97,    88,    23,
      96,    97,    32,    33,    34,    35,    27,    37,    38,    39,
      40,    41,    42,    16,    17,    45,    96,    97,    96,    97,
       8,   279,    96,    97,   252,    89,    90,    91,    92,    93,
      94,   289,   293,    63,    34,    65,    34,    67,    68,    34,
     301,    43,    44,    45,    46,    47,    48,   101,   306,    79,
      96,   312,   101,    83,    80,    81,    82,    88,    84,    85,
      90,   101,    24,    89,    90,    91,    92,    93,    94,   327,
     331,   101,   101,   101,   101,   303,   101,   338,    32,    33,
      34,    35,    10,    37,    38,    35,    40,    41,    42,    34,
      80,    45,    82,   351,    84,    85,    34,    34,   102,    89,
      90,    91,    92,    93,    94,    34,    88,    34,    11,    63,
      88,    65,    34,    67,    68,   376,   102,    94,    88,   377,
      34,    12,    34,    34,   100,    79,    34,    88,    88,    83,
     391,    25,    32,    33,    34,    35,    90,    37,    38,    80,
      44,    41,    42,    84,    85,    45,   102,   101,    89,    90,
      91,    92,    93,    94,    43,    98,    98,    88,    35,    99,
      99,    34,    34,    63,    96,    65,   101,    67,    68,   103,
     103,    34,    34,    20,   100,    99,    97,    21,   103,    79,
      16,    31,    17,    83,   103,    34,    34,    12,    34,     9,
      90,    34,    70,    71,    72,    73,    74,    75,    76,    77,
      78,   101,    80,    81,    82,    35,    84,    85,    19,    87,
     102,    89,    90,    91,    92,    93,    94,    35,    12,    97,
      88,    88,    35,    98,   102,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    34,    80,    81,    82,    96,    84,
      85,    38,    87,    16,    89,    90,    91,    92,    93,    94,
     205,    32,    97,   237,   259,   238,   262,   102,    70,    71,
      72,    73,    74,    75,    76,    77,    78,   241,    80,    81,
      82,    18,    84,    85,    35,    87,   220,    89,    90,    91,
      92,    93,    94,   374,   415,    97,   412,   217,   267,   297,
     102,    70,    71,    72,    73,    74,    75,    76,    77,    78,
     271,    80,    81,    82,   318,    84,    85,    -1,    87,    -1,
      89,    90,    91,    92,    93,    94,   333,    -1,    -1,    -1,
      -1,    -1,    -1,   102,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    -1,    80,    81,    82,    -1,    84,    85,
      -1,    87,    -1,    89,    90,    91,    92,    93,    94,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   102,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    -1,    80,    81,    82,
      -1,    84,    85,    -1,    87,    -1,    89,    90,    91,    92,
      93,    94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   102,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    -1,
      80,    81,    82,    -1,    84,    85,    -1,    87,    -1,    89,
      90,    91,    92,    93,    94,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   102,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    -1,    80,    81,    82,    -1,    84,    85,    -1,
      87,    -1,    89,    90,    91,    92,    93,    94,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   102,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    -1,    80,    81,    82,    -1,
      84,    85,    -1,    87,    88,    89,    90,    91,    92,    93,
      94,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      -1,    80,    81,    82,    -1,    84,    85,    -1,    87,    -1,
      89,    90,    91,    92,    93,    94,    70,    71,    72,    73,
      74,    75,    76,    -1,    78,    -1,    80,    81,    82,    -1,
      84,    85,    -1,    -1,    -1,    89,    90,    91,    92,    93,
      94,    70,    71,    72,    73,    74,    75,    76,    -1,    -1,
      -1,    80,    81,    82,    -1,    84,    85,    -1,    -1,    -1,
      89,    90,    91,    92,    93,    94,    70,    71,    72,    73,
      74,    75,    -1,    -1,    -1,    -1,    80,    81,    82,    -1,
      84,    85,    -1,    -1,    -1,    89,    90,    91,    92,    93,
      94,    84,    85,    -1,    -1,    -1,    89,    90,    91,    92,
      93,    94
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   105,   106,     0,     3,     4,     5,     6,    23,   107,
     108,   111,   112,   116,   117,   119,   120,   195,   196,    35,
     109,   110,    34,    34,    34,    34,    31,   113,   114,     7,
     124,   125,   124,     7,   150,   151,   152,    96,    97,    88,
      32,    33,    34,    35,    37,    38,    40,    41,    42,    45,
      63,    65,    67,    68,    79,    83,    90,   101,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,    26,
     115,    31,    88,     8,   130,   131,   130,    21,    88,    27,
     197,   152,   110,    34,    34,    34,   101,   101,    34,    42,
      34,    41,   101,   101,   101,   101,   101,   201,   201,   201,
     201,    96,   210,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    80,    81,    82,    84,    85,    87,    89,    90,
      91,    92,    93,    94,    32,    33,    34,   210,   199,   126,
      88,    24,   118,    10,   139,   140,   199,   153,   210,   210,
     102,   201,   206,    34,    34,   201,   201,   201,    34,   102,
     102,   201,   201,   201,   201,   201,   201,   201,   201,   201,
     201,   201,   201,   201,   201,   201,   201,   201,   201,   201,
     201,   201,    34,    34,   210,   210,    34,    43,   127,   128,
     129,   132,    34,   210,    88,    11,   169,   170,    88,    34,
     155,   156,   157,    97,   102,    97,   102,   102,   102,   102,
     102,    88,    88,    34,    96,    97,    13,    14,   133,   134,
     210,   141,    15,    88,    12,   122,   123,   154,   100,    96,
      97,   201,   206,   201,    43,    44,    45,    46,    47,    48,
     129,    34,   135,   136,    34,   137,   138,    13,    14,    34,
     142,   143,   144,    88,   172,    88,    25,   121,   155,    39,
     199,   157,    97,   102,   102,    44,    43,    98,    96,    97,
      98,    96,    97,   135,   137,    88,   144,   171,    18,    34,
     173,   174,   175,   176,   183,   184,   185,   187,    35,    34,
     210,   158,   201,    99,   136,    99,   138,    96,    96,    34,
     173,   177,    69,    98,   198,   175,    16,    17,   103,    96,
     210,   101,   159,    97,   102,    22,   198,   210,    34,   178,
     179,   199,    31,    34,   188,   189,   191,   192,   193,   194,
     188,   192,    34,   186,   102,   199,   201,    34,    22,   210,
     145,   100,    20,    97,    99,   199,    69,   198,    21,   180,
     103,    16,   194,   180,    17,    69,   198,    97,   102,   102,
     210,    34,   146,   149,   150,   199,   182,   183,   178,   199,
      12,   181,    34,   190,    34,   188,   181,   188,    35,   147,
     210,   149,     9,   160,   161,   162,    19,    28,   183,    35,
      96,    97,   211,    69,   198,   180,   211,   180,   102,   149,
     148,    21,    88,    12,   168,   162,   199,   210,   181,   181,
     149,   199,   163,    88,   211,   211,    88,    34,   165,   166,
     167,    35,   164,   198,    96,    97,    96,   165,   167
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:

/* Line 1455 of yacc.c  */
#line 197 "ned1.y"
    { if (np->getStoreSourceFlag()) storeComponentSourceCode(ps.channel, (yylsp[(1) - (1)])); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 199 "ned1.y"
    { if (np->getStoreSourceFlag()) storeComponentSourceCode(ps.module, (yylsp[(1) - (1)])); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 201 "ned1.y"
    { if (np->getStoreSourceFlag()) storeComponentSourceCode(ps.module, (yylsp[(1) - (1)])); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 203 "ned1.y"
    { if (np->getStoreSourceFlag()) storeComponentSourceCode(ps.module, (yylsp[(1) - (1)])); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 221 "ned1.y"
    {
                  ps.import = (ImportElement *)createElementWithTag(NED_IMPORT, ps.nedfile );
                  ps.import->setImportSpec(toString(trimQuotes((yylsp[(1) - (1)]))));
                  storeBannerAndRightComments(ps.import,(yylsp[(1) - (1)]));
                  storePos(ps.import, (yyloc));
                }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 234 "ned1.y"
    {
                  storePos(ps.channel, (yyloc));
                  storeTrailingComment(ps.channel,(yyloc));
                }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 242 "ned1.y"
    {
                  ps.channel = (ChannelElement *)createElementWithTag(NED_CHANNEL, ps.nedfile);
                  ps.channel->setName(toString((yylsp[(2) - (2)])));
                  ps.extends = (ExtendsElement *)createElementWithTag(NED_EXTENDS, ps.channel);
                  ps.extends->setName("ned.DatarateChannel");  // NED-1 channels are implicitly DatarateChannels
                  ps.params = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.channel);
                  ps.params->setIsImplicit(true);
                  storeBannerAndRightComments(ps.channel,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 256 "ned1.y"
    { storePos(ps.params, (yyloc)); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 261 "ned1.y"
    {
                  const char *name = toString((yylsp[(2) - (4)]));
                  if (strcmp(name,"error")==0) name = "ber";  // "error" got renamed to "ber" in 4.0
                  ps.params->setIsImplicit(false);
                  ps.param = addParameter(ps.params, name, (yylsp[(2) - (4)]));
                  addExpression(ps.param, "value",(yylsp[(3) - (4)]),(yyvsp[(3) - (4)]));
                  storeBannerAndRightComments(ps.param,(yylsp[(2) - (4)]),(yylsp[(3) - (4)]));
                  storePos(ps.param, (yylsp[(2) - (4)]),(yylsp[(4) - (4)]));
                }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 271 "ned1.y"
    {
                  const char *name = toString((yylsp[(1) - (3)]));
                  if (strcmp(name,"error")==0) name = "ber"; // "error" got renamed to "ber" in 4.0
                  ps.params->setIsImplicit(false);
                  ps.param = addParameter(ps.params, name, (yylsp[(1) - (3)]));
                  addExpression(ps.param, "value",(yylsp[(2) - (3)]),(yyvsp[(2) - (3)]));
                  storeBannerAndRightComments(ps.param,(yylsp[(1) - (3)]),(yylsp[(2) - (3)]));
                  storePos(ps.param, (yyloc));
                }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 295 "ned1.y"
    {
                  storePos(ps.module, (yyloc));
                  storeTrailingComment(ps.module,(yyloc));
                }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 303 "ned1.y"
    {
                  ps.module = (SimpleModuleElement *)createElementWithTag(NED_SIMPLE_MODULE, ps.nedfile );
                  ((SimpleModuleElement *)ps.module)->setName(toString((yylsp[(2) - (2)])));
                  storeBannerAndRightComments(ps.module,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 326 "ned1.y"
    {
                  storePos(ps.module, (yyloc));
                  storeTrailingComment(ps.module,(yyloc));
                }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 334 "ned1.y"
    {
                  ps.module = (CompoundModuleElement *)createElementWithTag(NED_COMPOUND_MODULE, ps.nedfile );
                  ((CompoundModuleElement *)ps.module)->setName(toString((yylsp[(2) - (2)])));
                  storeBannerAndRightComments(ps.module,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 356 "ned1.y"
    {
                  ps.property = addComponentProperty(ps.module, "display");
                  ps.params = (ParametersElement *)ps.module->getFirstChildWithTag(NED_PARAMETERS); // previous line doesn't set it
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.property);
                  LiteralElement *literal = (LiteralElement *)createElementWithTag(NED_LITERAL);
                  literal->setType(NED_CONST_STRING);
                  try {
                      std::string displaystring = DisplayStringUtil::upgradeBackgroundDisplayString(opp_parsequotedstr(toString((yylsp[(3) - (4)]))).c_str());
                      literal->setValue(displaystring.c_str());
                      // NOTE: no setText(): it would cause the OLD form to be exported into NED2 too
                  }
                  catch (std::exception& e) {
                      np->getErrors()->addError(ps.property, e.what());
                  }
                  ps.propkey->appendChild(literal);
                  storePos(ps.propkey, (yyloc));
                  storePos(literal, (yylsp[(3) - (4)]));
                  storePos(ps.property, (yyloc));
                  storeBannerAndRightComments(ps.property,(yyloc));
                }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 388 "ned1.y"
    {
                  ps.params = (ParametersElement *)getOrCreateElementWithTag(NED_PARAMETERS, ps.module); // network header may have created it for @isNetwork
                  storeBannerAndRightComments(ps.params,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 393 "ned1.y"
    {
                  storePos(ps.params, (yyloc));
                }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 405 "ned1.y"
    {
                  storePos(ps.param, (yylsp[(3) - (3)]));
                  storeBannerAndRightComments(ps.param,(yylsp[(3) - (3)]));
                }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 410 "ned1.y"
    {
                  storePos(ps.param, (yylsp[(1) - (1)]));
                  storeBannerAndRightComments(ps.param,(yylsp[(1) - (1)]));
                }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 421 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (1)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                  ps.param->setIsVolatile(true); // because CONST is missing
                }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 427 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                  ps.param->setIsVolatile(true); // because CONST is missing
                }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 433 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(2) - (2)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 438 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 443 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (4)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 448 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (4)]));
                  ps.param->setType(NED_PARTYPE_DOUBLE);
                }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 453 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)]));
                  ps.param->setType(NED_PARTYPE_STRING);
                }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 458 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)]));
                  ps.param->setType(NED_PARTYPE_BOOL);
                }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 463 "ned1.y"
    {
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)]));
                  ps.param->setType(NED_PARTYPE_XML);
                }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 468 "ned1.y"
    {
                  np->getErrors()->addError(ps.params,"type 'anytype' no longer supported");
                  ps.param = addParameter(ps.params, (yylsp[(1) - (3)])); // add anyway to prevent crash
                }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 484 "ned1.y"
    {
                  ps.gates = (GatesElement *)createElementWithTag(NED_GATES, ps.module );
                  storeBannerAndRightComments(ps.gates,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 489 "ned1.y"
    {
                  storePos(ps.gates, (yyloc));
                }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 513 "ned1.y"
    {
                  ps.gate = addGate(ps.gates, (yylsp[(1) - (3)]));
                  ps.gate->setType(NED_GATETYPE_INPUT);
                  ps.gate->setIsVector(true);
                  storeBannerAndRightComments(ps.gate,(yylsp[(1) - (3)]),(yylsp[(3) - (3)]));
                  storePos(ps.gate, (yyloc));
                }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 521 "ned1.y"
    {
                  ps.gate = addGate(ps.gates, (yylsp[(1) - (1)]));
                  ps.gate->setType(NED_GATETYPE_INPUT);
                  storeBannerAndRightComments(ps.gate,(yylsp[(1) - (1)]));
                  storePos(ps.gate, (yyloc));
                }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 536 "ned1.y"
    {
                  ps.gate = addGate(ps.gates, (yylsp[(1) - (3)]));
                  ps.gate->setType(NED_GATETYPE_OUTPUT);
                  ps.gate->setIsVector(true);
                  storeBannerAndRightComments(ps.gate,(yylsp[(1) - (3)]),(yylsp[(3) - (3)]));
                  storePos(ps.gate, (yyloc));
                }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 544 "ned1.y"
    {
                  ps.gate = addGate(ps.gates, (yylsp[(1) - (1)]));
                  ps.gate->setType(NED_GATETYPE_OUTPUT);
                  storeBannerAndRightComments(ps.gate,(yylsp[(1) - (1)]),(yylsp[(1) - (1)]));
                  storePos(ps.gate, (yyloc));
                }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 562 "ned1.y"
    {
                  ps.submods = (SubmodulesElement *)createElementWithTag(NED_SUBMODULES, ps.module );
                  storeBannerAndRightComments(ps.submods,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 567 "ned1.y"
    {
                  storePos(ps.submods, (yyloc));
                }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 584 "ned1.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((yylsp[(1) - (4)])));
                  ps.submod->setType(toString((yylsp[(3) - (4)])));
                  storeBannerAndRightComments(ps.submod,(yylsp[(1) - (4)]),(yylsp[(4) - (4)]));
                }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 591 "ned1.y"
    {
                  storePos(ps.submod, (yyloc));
                }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 595 "ned1.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((yylsp[(1) - (5)])));
                  ps.submod->setType(toString((yylsp[(3) - (5)])));
                  addVector(ps.submod, "vector-size",(yylsp[(4) - (5)]),(yyvsp[(4) - (5)]));
                  storeBannerAndRightComments(ps.submod,(yylsp[(1) - (5)]),(yylsp[(5) - (5)]));
                }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 603 "ned1.y"
    {
                  storePos(ps.submod, (yyloc));
                }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 607 "ned1.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((yylsp[(1) - (6)])));
                  ps.submod->setLikeType(toString((yylsp[(5) - (6)])));
                  ps.submod->setLikeParam(toString((yylsp[(3) - (6)]))); //FIXME store as expression!!!
                  storeBannerAndRightComments(ps.submod,(yylsp[(1) - (6)]),(yylsp[(6) - (6)]));
                }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 615 "ned1.y"
    {
                  storePos(ps.submod, (yyloc));
                }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 619 "ned1.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((yylsp[(1) - (7)])));
                  ps.submod->setLikeType(toString((yylsp[(6) - (7)])));
                  ps.submod->setLikeParam(toString((yylsp[(3) - (7)]))); //FIXME store as expression!!!
                  addVector(ps.submod, "vector-size",(yylsp[(4) - (7)]),(yyvsp[(4) - (7)]));
                  storeBannerAndRightComments(ps.submod,(yylsp[(1) - (7)]),(yylsp[(7) - (7)]));
                }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 628 "ned1.y"
    {
                  storePos(ps.submod, (yyloc));
                }
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 644 "ned1.y"
    { storePos(ps.substparams, (yyloc)); /*must do it here because there might be multiple (conditional) gatesizes/parameters sections */ }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 655 "ned1.y"
    {
                  createSubstparamsElementIfNotExists();
                  storeBannerAndRightComments(ps.substparams,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 661 "ned1.y"
    {
                  createSubstparamsElementIfNotExists();
                  delete (yyvsp[(3) - (4)]);
                  np->getErrors()->addError(ps.substparams,
                                    "conditional parameters no longer supported -- "
                                    "please rewrite parameter assignments to use "
                                    "conditional expression syntax (cond ? a : b)");
                }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 684 "ned1.y"
    {
                  ps.substparam = addParameter(ps.substparams,(yylsp[(1) - (3)]));
                  addExpression(ps.substparam, "value",(yylsp[(3) - (3)]),(yyvsp[(3) - (3)]));
                  storeBannerAndRightComments(ps.substparam,(yylsp[(1) - (3)]),(yylsp[(3) - (3)]));
                  storePos(ps.substparam, (yyloc));
                }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 691 "ned1.y"
    {
                  ps.substparam = addParameter(ps.substparams,(yylsp[(1) - (3)]));
                  ps.substparam->setIsDefault(true);
                }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 696 "ned1.y"
    {
                  storeBannerAndRightComments(ps.substparam,(yylsp[(1) - (5)]),(yylsp[(5) - (5)]));
                  storePos(ps.substparam, (yyloc));
                }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 704 "ned1.y"
    {
                  addExpression(ps.substparam, "value",(yylsp[(2) - (5)]),(yyvsp[(2) - (5)]));

                  PropertyElement *prop = addProperty(ps.substparam, "prompt");
                  PropertyKeyElement *propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, prop);
                  propkey->appendChild(createStringLiteral((yylsp[(4) - (5)])));
                }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 712 "ned1.y"
    {
                  addExpression(ps.substparam, "value",(yylsp[(2) - (3)]),(yyvsp[(2) - (3)]));
                }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 724 "ned1.y"
    { storePos(ps.gatesizes, (yyloc)); /*must do it here because there might be multiple (conditional) gatesizes/parameters sections */ }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 735 "ned1.y"
    {
                  createGatesizesElementIfNotExists();
                  storeBannerAndRightComments(ps.gatesizes,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 741 "ned1.y"
    {
                  createSubstparamsElementIfNotExists();
                  delete (yyvsp[(3) - (4)]);
                  np->getErrors()->addError(ps.substparams,
                                    "conditional gatesizes no longer supported -- "
                                    "please rewrite gatesize assignments to use "
                                    "conditional expression syntax (cond ? a : b)");
                }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 764 "ned1.y"
    {
                  ps.gatesize = addGate(ps.gatesizes,(yylsp[(1) - (2)]));
                  ps.gatesize->setIsVector(true);
                  addVector(ps.gatesize, "vector-size",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                  storeBannerAndRightComments(ps.gatesize,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                  storePos(ps.gatesize, (yyloc));
                }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 778 "ned1.y"
    {
                  ps.property = addComponentProperty(ps.submod, "display");
                  ps.substparams = (ParametersElement *)ps.submod->getFirstChildWithTag(NED_PARAMETERS); // previous line doesn't set it
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.property);
                  LiteralElement *literal = (LiteralElement *)createElementWithTag(NED_LITERAL);
                  literal->setType(NED_CONST_STRING);
                  try {
                      std::string displaystring = DisplayStringUtil::upgradeSubmoduleDisplayString(opp_parsequotedstr(toString((yylsp[(3) - (4)]))).c_str());
                      literal->setValue(displaystring.c_str());
                      // NOTE: no setText(): it would cause the OLD form to be exported into NED2 too
                  }
                  catch (std::exception& e) {
                      np->getErrors()->addError(ps.property, e.what());
                  }
                  ps.propkey->appendChild(literal);
                  storePos(ps.propkey, (yyloc));
                  storePos(literal, (yylsp[(3) - (4)]));
                  storePos(ps.property, (yyloc));
                  storeBannerAndRightComments(ps.property,(yyloc));
                }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 811 "ned1.y"
    {
                  ps.conns = (ConnectionsElement *)createElementWithTag(NED_CONNECTIONS, ps.module );
                  ps.conns->setAllowUnconnected(true);
                  storeBannerAndRightComments(ps.conns,(yylsp[(1) - (3)]),(yylsp[(3) - (3)]));
                }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 817 "ned1.y"
    {
                  storePos(ps.conns, (yyloc));
                }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 821 "ned1.y"
    {
                  ps.conns = (ConnectionsElement *)createElementWithTag(NED_CONNECTIONS, ps.module );
                  ps.conns->setAllowUnconnected(false);
                  storeBannerAndRightComments(ps.conns,(yylsp[(1) - (2)]),(yylsp[(2) - (2)]));
                }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 827 "ned1.y"
    {
                  storePos(ps.conns, (yyloc));
                }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 849 "ned1.y"
    {
                  ps.conngroup = (ConnectionGroupElement *)createElementWithTag(NED_CONNECTION_GROUP, ps.conns);
                  ps.inLoop=1;
                }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 854 "ned1.y"
    {
                  ps.inLoop=0;
                  storePos(ps.conngroup, (yyloc));
                  storeBannerAndRightComments(ps.conngroup,(yylsp[(1) - (7)]),(yylsp[(4) - (7)])); // "for..do"
                  storeTrailingComment(ps.conngroup,(yyloc));
                }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 869 "ned1.y"
    {
                  ps.loop = (LoopElement *)createElementWithTag(NED_LOOP, ps.conngroup);
                  ps.loop->setParamName( toString((yylsp[(1) - (5)])) );
                  addExpression(ps.loop, "from-value",(yylsp[(3) - (5)]),(yyvsp[(3) - (5)]));
                  addExpression(ps.loop, "to-value",(yylsp[(5) - (5)]),(yyvsp[(5) - (5)]));
                  storePos(ps.loop, (yyloc));
                }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 880 "ned1.y"
    {
                  // add condition to conn
                  ps.condition = (ConditionElement *)createElementWithTag(NED_CONDITION, ps.conn);
                  addExpression(ps.condition, "condition",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                  storePos(ps.condition, (yyloc));
                }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 891 "ned1.y"
    {
                  bool hadChanSpec = ps.chanspec!=NULL;
                  if (!ps.chanspec)
                      ps.chanspec = createChannelSpec(ps.conn);
                  ps.property = addComponentProperty(ps.chanspec, "display");
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.property);
                  LiteralElement *literal = (LiteralElement *)createElementWithTag(NED_LITERAL);
                  literal->setType(NED_CONST_STRING);
                  try {
                      std::string displaystring = DisplayStringUtil::upgradeConnectionDisplayString(opp_parsequotedstr(toString((yylsp[(2) - (2)]))).c_str());
                      literal->setValue(displaystring.c_str());
                  }
                  catch (std::exception& e) {
                      np->getErrors()->addError(ps.property, e.what());
                  }
                  // NOTE: no setText(): it would cause the OLD form to be exported into NED2 too
                  ps.propkey->appendChild(literal);
                  storePos(ps.propkey, (yyloc));
                  storePos(literal, (yylsp[(2) - (2)]));
                  storePos(ps.property, (yyloc));
                  if (!hadChanSpec)
                      storePos(ps.chanspec, (yyloc));
                }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 924 "ned1.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_L2R);
                  storeBannerAndRightComments(ps.conn,(yyloc));
                  storePos(ps.conn, (yyloc));
                }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 930 "ned1.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_L2R);
                  removeRedundantChanSpecParams();
                  storeBannerAndRightComments(ps.conn,(yyloc));
                  storePos(ps.conn, (yyloc));
                }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 937 "ned1.y"
    {
                  swapConnection(ps.conn);
                  ps.conn->setArrowDirection(NED_ARROWDIR_R2L);
                  storeBannerAndRightComments(ps.conn,(yyloc));
                  storePos(ps.conn, (yyloc));
                }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 944 "ned1.y"
    {
                  swapConnection(ps.conn);
                  ps.conn->setArrowDirection(NED_ARROWDIR_R2L);
                  removeRedundantChanSpecParams();
                  storeBannerAndRightComments(ps.conn,(yyloc));
                  storePos(ps.conn, (yyloc));
                }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 960 "ned1.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inLoop ? (NEDElement *)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule( toString((yylsp[(1) - (2)])) );
                  addVector(ps.conn, "src-module-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                  ps.chanspec = NULL;   // signal that there's no chanspec for this conn yet
                }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 967 "ned1.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inLoop ? (NEDElement *)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule( toString((yylsp[(1) - (1)])) );
                  ps.chanspec = NULL;   // signal that there's no chanspec for this conn yet
                }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 976 "ned1.y"
    {
                  ps.conn->setSrcGate( toString( (yylsp[(1) - (2)])) );
                  addVector(ps.conn, "src-gate-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 981 "ned1.y"
    {
                  ps.conn->setSrcGate( toString( (yylsp[(1) - (1)])) );
                }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 985 "ned1.y"
    {
                  ps.conn->setSrcGate( toString( (yylsp[(1) - (2)])) );
                  ps.conn->setSrcGatePlusplus(true);
                }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 993 "ned1.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inLoop ? (NEDElement *)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((yylsp[(1) - (2)])));
                  addVector(ps.conn, "src-gate-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                  ps.chanspec = NULL;   // signal that there's no chanspec for this conn yet
                }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1001 "ned1.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inLoop ? (NEDElement *)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((yylsp[(1) - (1)])));
                  ps.chanspec = NULL;   // signal that there's no chanspec for this conn yet
                }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1008 "ned1.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inLoop ? (NEDElement *)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((yylsp[(1) - (2)])));
                  ps.conn->setSrcGatePlusplus(true);
                  ps.chanspec = NULL;   // signal that there's no chanspec for this conn yet
                }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1024 "ned1.y"
    {
                  ps.conn->setDestModule( toString((yylsp[(1) - (2)])) );
                  addVector(ps.conn, "dest-module-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1029 "ned1.y"
    {
                  ps.conn->setDestModule( toString((yylsp[(1) - (1)])) );
                }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1036 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (2)])) );
                  addVector(ps.conn, "dest-gate-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1041 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (1)])) );
                }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1045 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (2)])) );
                  ps.conn->setDestGatePlusplus(true);
                }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1053 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (2)])) );
                  addVector(ps.conn, "dest-gate-index",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1058 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (1)])) );
                }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1062 "ned1.y"
    {
                  ps.conn->setDestGate( toString( (yylsp[(1) - (2)])) );
                  ps.conn->setDestGatePlusplus(true);
                }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1071 "ned1.y"
    {
                  storePos(ps.chanspec, (yyloc));
                  if (ps.chanspec->getFirstChildWithTag(NED_PARAMETERS)!=NULL)
                      storePos(ps.params, (yyloc));
                }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1080 "ned1.y"
    {
                  if (!ps.chanspec)
                      ps.chanspec = createChannelSpec(ps.conn);
                  ps.chanspec->setType(toString((yylsp[(1) - (1)])));
                }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1091 "ned1.y"
    {
                  if (!ps.chanspec)
                      ps.chanspec = createChannelSpec(ps.conn);
                  ps.param = addParameter(ps.params, (yylsp[(1) - (2)]));
                  addExpression(ps.param, "value",(yylsp[(2) - (2)]),(yyvsp[(2) - (2)]));
                  storePos(ps.param, (yyloc));
                }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1107 "ned1.y"
    {
                  storePos(ps.module, (yyloc));
                  storeTrailingComment(ps.module,(yyloc));
                }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1115 "ned1.y"
    {
                  ps.module = (CompoundModuleElement *)createElementWithTag(NED_COMPOUND_MODULE, ps.nedfile );
                  ((CompoundModuleElement *)ps.module)->setName(toString((yylsp[(2) - (5)])));
                  ps.extends = (ExtendsElement *)createElementWithTag(NED_EXTENDS, ps.module);
                  ps.extends->setName(toString((yylsp[(4) - (5)])));
                  storeBannerAndRightComments(ps.module,(yylsp[(1) - (5)]),(yylsp[(5) - (5)]));
                  storePos(ps.extends, (yylsp[(4) - (5)]));
                  setIsNetworkProperty(ps.module);
                  ps.inNetwork=1;
                }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1129 "ned1.y"
    {
                  ps.inNetwork=0;
                }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1139 "ned1.y"
    { (yyval) = (yyvsp[(2) - (3)]); }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1145 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createExpression((yyvsp[(1) - (1)]));
                }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1149 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createExpression((yyvsp[(1) - (1)]));
                }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1160 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction("xmldoc", (yyvsp[(3) - (6)]), (yyvsp[(5) - (6)])); }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1162 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction("xmldoc", (yyvsp[(3) - (4)])); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1168 "ned1.y"
    { (yyval) = (yyvsp[(2) - (3)]); }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1171 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("+", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1173 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("-", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1175 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("*", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1177 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("/", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1179 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("%", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1181 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("^", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1185 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = unaryMinus((yyvsp[(2) - (2)])); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1188 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("==", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1190 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("!=", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1192 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator(">", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1194 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator(">=", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1196 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("<", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1198 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("<=", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1201 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("&&", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1203 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("||", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1205 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("##", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1209 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("!", (yyvsp[(2) - (2)])); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1212 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("&", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1214 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("|", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1216 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("#", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1220 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("~", (yyvsp[(2) - (2)])); }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1222 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("<<", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1224 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator(">>", (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1226 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createOperator("?:", (yyvsp[(1) - (5)]), (yyvsp[(3) - (5)]), (yyvsp[(5) - (5)])); }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1229 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (4)])), (yyvsp[(3) - (4)])); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1231 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (4)])), (yyvsp[(3) - (4)])); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1233 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (4)])), (yyvsp[(3) - (4)])); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1236 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (3)]))); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1238 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (4)])), (yyvsp[(3) - (4)])); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1240 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (6)])), (yyvsp[(3) - (6)]), (yyvsp[(5) - (6)])); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1242 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (8)])), (yyvsp[(3) - (8)]), (yyvsp[(5) - (8)]), (yyvsp[(7) - (8)])); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1244 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction(toString((yylsp[(1) - (10)])), (yyvsp[(3) - (10)]), (yyvsp[(5) - (10)]), (yyvsp[(7) - (10)]), (yyvsp[(9) - (10)])); }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1255 "ned1.y"
    {
                  // if there's no modifier, might be a loop variable too
                  if (np->getParseExpressionsFlag()) (yyval) = createIdent((yylsp[(1) - (1)]));
                }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1260 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createIdent((yylsp[(2) - (2)]));
                  np->getErrors()->addError(ps.substparams,"`ref' modifier no longer supported (add `volatile' modifier to destination parameter instead)");
                }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1265 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createIdent((yylsp[(3) - (3)]));
                  np->getErrors()->addError(ps.substparams,"`ancestor' and `ref' modifiers no longer supported");
                }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1270 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createIdent((yylsp[(3) - (3)]));
                  np->getErrors()->addError(ps.substparams,"`ancestor' and `ref' modifiers no longer supported");
                }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1275 "ned1.y"
    {
                  if (np->getParseExpressionsFlag()) (yyval) = createIdent((yylsp[(2) - (2)]));
                  np->getErrors()->addError(ps.substparams,"`ancestor' modifier no longer supported");
                }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1283 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction("index"); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1285 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction("index"); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1287 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createFunction("sizeof", createIdent((yylsp[(3) - (4)]))); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1298 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createStringLiteral((yylsp[(1) - (1)])); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1303 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createLiteral(NED_CONST_BOOL, (yylsp[(1) - (1)]), (yylsp[(1) - (1)])); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1305 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createLiteral(NED_CONST_BOOL, (yylsp[(1) - (1)]), (yylsp[(1) - (1)])); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1310 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createLiteral(NED_CONST_INT, (yylsp[(1) - (1)]), (yylsp[(1) - (1)])); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1312 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createLiteral(NED_CONST_DOUBLE, (yylsp[(1) - (1)]), (yylsp[(1) - (1)])); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1314 "ned1.y"
    { if (np->getParseExpressionsFlag()) (yyval) = createQuantityLiteral((yylsp[(1) - (1)])); }
    break;



/* Line 1455 of yacc.c  */
#line 3583 "ned1.tab.cc"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

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
		      yytoken, &yylval, &yylloc);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1328 "ned1.y"


//----------------------------------------------------------------------
// general bison/flex stuff:
//

NEDElement *doParseNED1(NEDParser *p, const char *nedtext)
{
#if YYDEBUG != 0      /* #if added --VA */
    yydebug = YYDEBUGGING_ON;
#endif

    NONREENTRANT_NED_PARSER(p);

    // reset the lexer
    pos.co = 0;
    pos.li = 1;
    prevpos = pos;

    yyin = NULL;
    yyout = stderr; // not used anyway

    // alloc buffer
    struct yy_buffer_state *handle = yy_scan_string(nedtext);
    if (!handle)
        {np->getErrors()->addError("", "unable to allocate work memory"); return false;}

    // create parser state and NEDFileElement
    resetParserState();
    ps.nedfile = new NedFileElement();

    // store file name with slashes always, even on Windows -- neddoc relies on that
    ps.nedfile->setFilename(slashifyFilename(np->getFileName()).c_str());
    ps.nedfile->setVersion("1");

    // store file comment
    storeFileComment(ps.nedfile);

    if (np->getStoreSourceFlag())
        storeSourceCode(ps.nedfile, np->getSource()->getFullTextPos());

    // parse
    int ret;
    try
    {
        ret = yyparse();
    }
    catch (NEDException& e)
    {
        yyerror((std::string("error during parsing: ")+e.what()).c_str());
        yy_delete_buffer(handle);
        return 0;
    }

    yy_delete_buffer(handle);

    //FIXME TODO: fill in @documentation properties from comments
    return ps.nedfile;
}

void yyerror(const char *s)
{
    // chop newline
    char buf[250];
    strcpy(buf, s);
    if (buf[strlen(buf)-1] == '\n')
        buf[strlen(buf)-1] = '\0';

    np->error(buf, pos.li);
}

// this function depends too much on ps, cannot be put into nedyylib.cc
ChannelSpecElement *createChannelSpec(NEDElement *conn)
{
   ChannelSpecElement *chanspec = (ChannelSpecElement *)createElementWithTag(NED_CHANNEL_SPEC, ps.conn);
   ps.params = (ParametersElement *)createElementWithTag(NED_PARAMETERS, chanspec);
   ps.params->setIsImplicit(true);
   return chanspec;
}

void createSubstparamsElementIfNotExists()
{
   // check if already exists (multiple blocks must be merged)
   NEDElement *parent = ps.inNetwork ? (NEDElement *)ps.module : (NEDElement *)ps.submod;
   ps.substparams = (ParametersElement *)parent->getFirstChildWithTag(NED_PARAMETERS);
   if (!ps.substparams)
       ps.substparams = (ParametersElement *)createElementWithTag(NED_PARAMETERS, parent);
}

void createGatesizesElementIfNotExists()
{
   // check if already exists (multiple blocks must be merged)
   ps.gatesizes = (GatesElement *)ps.submod->getFirstChildWithTag(NED_GATES);
   if (!ps.gatesizes)
       ps.gatesizes = (GatesElement *)createElementWithTag(NED_GATES, ps.submod);
}

void removeRedundantChanSpecParams()
{
    if (ps.chanspec && !ps.params->getFirstChild())
        delete ps.chanspec->removeChild(ps.params);
}


