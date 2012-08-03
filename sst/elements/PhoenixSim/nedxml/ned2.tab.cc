
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison GLR parsers in C
   
      Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
   
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

/* C GLR parser skeleton written by Paul Hilfinger.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1


/* Substitute the variable and function names.  */
#define yyparse ned2yyparse
#define yylex   ned2yylex
#define yyerror ned2yyerror
#define yylval  ned2yylval
#define yychar  ned2yychar
#define yydebug ned2yydebug
#define yynerrs ned2yynerrs
#define yylloc  ned2yylloc

/* Copy the first part of user declarations.  */

/* Line 172 of glr.c  */
#line 74 "ned2.y"


#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include "nedyydefs.h"
#include "nederror.h"
#include "nedexception.h"
#include "commonutil.h"

#define YYDEBUG 1           /* allow debugging */
#define YYDEBUGGING_ON 0    /* turn on/off debugging */

#if YYDEBUG != 0
#define YYERROR_VERBOSE     /* more detailed error messages */
#include <string.h>         /* YYVERBOSE needs it */
#endif

/* increase GLR stack -- with the default 200 some NED files have reportedly caused a "memory exhausted" error */
#define YYINITDEPTH 500

#define yylloc ned2yylloc
#define yyin ned2yyin
#define yyout ned2yyout
#define yyrestart ned2yyrestart
#define yy_scan_string ned2yy_scan_string
#define yy_delete_buffer ned2yy_delete_buffer
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

static struct NED2ParserState
{
    bool inTypes;
    bool inConnGroup;
    std::stack<NEDElement *> propertyscope; // top(): where to insert properties as we parse them
    std::stack<NEDElement *> blockscope;    // top(): where to insert parameters, gates, etc
    std::stack<NEDElement *> typescope;     // top(): as blockscope, but ignore submodules and connection channels

    /* tmp flags, used with param, gate and conn */
    int paramType;
    int gateType;
    bool isVolatile;
    bool isDefault;
    YYLTYPE exprPos;
    int subgate;
    std::vector<NEDElement *> propvals; // temporarily collects property values

    /* tmp flags, used with msg fields */
    bool isAbstract;
    bool isReadonly;

    /* NED-II: modules, channels */
    NedFileElement *nedfile;
    CommentElement *comment;
    PackageElement *package;
    ImportElement *import;
    PropertyDeclElement *propertydecl;
    ExtendsElement *extends;
    InterfaceNameElement *interfacename;
    NEDElement *component;  // compound/simple module, module interface, channel or channel interface
    ParametersElement *parameters;
    ParamElement *param;
    PropertyElement *property;
    PropertyKeyElement *propkey;
    TypesElement *types;
    GatesElement *gates;
    GateElement *gate;
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
    static NED2ParserState cleanps;
    ps = cleanps;
}

static NED2ParserState globalps;  // for error recovery

static void restoreGlobalParserState()  // for error recovery
{
    ps = globalps;
}

static void assertNonEmpty(std::stack<NEDElement *>& somescope)
{
    // for error recovery: STL stack::top() crashes if stack is empty
    if (somescope.empty())
    {
        INTERNAL_ERROR0(NULL, "error during parsing: scope stack empty");
        somescope.push(NULL);
    }
}



/* Line 172 of glr.c  */
#line 185 "ned2.tab.cc"



#include "ned2.tab.hh"

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

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template,
   here we set the default value of $$ to a zeroed-out value.
   Since the default value is undefined, this behavior is
   technically correct.  */
static YYSTYPE yyval_default;

/* Copy the second part of user declarations.  */


/* Line 243 of glr.c  */
#line 220 "ned2.tab.cc"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#ifndef YYFREE
# define YYFREE free
#endif
#ifndef YYMALLOC
# define YYMALLOC malloc
#endif
#ifndef YYREALLOC
# define YYREALLOC realloc
#endif

#define YYSIZEMAX ((size_t) -1)

#ifdef __cplusplus
   typedef bool yybool;
#else
   typedef unsigned char yybool;
#endif
#define yytrue 1
#define yyfalse 0

#ifndef YYSETJMP
# include <setjmp.h>
# define YYJMP_BUF jmp_buf
# define YYSETJMP(env) setjmp (env)
# define YYLONGJMP(env, val) longjmp (env, val)
#endif

/*-----------------.
| GCC extensions.  |
`-----------------*/

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__)
#  define __attribute__(Spec) /* empty */
# endif
#endif

#define YYOPTIONAL_LOC(Name) Name

#ifndef YYASSERT
# define YYASSERT(condition) ((void) ((condition) || (abort (), 0)))
#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  86
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1478

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  90
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  140
/* YYNRULES -- Number of rules.  */
#define YYNRULES  367
/* YYNRULES -- Number of states.  */
#define YYNSTATES  613
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 22
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   322

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    89,    75,     2,     2,
      81,    82,    73,    71,    87,    72,    80,    74,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    68,    78,
      70,    88,    69,    67,    83,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    84,     2,    85,    76,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    86,     2,    79,     2,     2,     2,     2,
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
      65,    66,    77
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     6,     8,    10,    11,    14,    16,    18,
      20,    22,    24,    26,    28,    30,    32,    34,    36,    38,
      42,    46,    50,    54,    58,    62,    66,    70,    74,    78,
      82,    86,    90,    94,    96,   100,   104,   106,   109,   112,
     115,   117,   119,   121,   125,   132,   136,   142,   144,   145,
     149,   151,   153,   156,   157,   163,   164,   169,   170,   173,
     176,   181,   183,   187,   189,   191,   192,   198,   199,   204,
     207,   208,   212,   214,   215,   222,   223,   228,   229,   239,
     240,   245,   246,   256,   257,   262,   263,   270,   271,   276,
     278,   279,   284,   286,   287,   290,   292,   294,   296,   298,
     300,   304,   311,   315,   317,   322,   324,   326,   328,   330,
     332,   334,   335,   337,   342,   344,   346,   348,   349,   352,
     354,   358,   362,   364,   366,   371,   376,   378,   380,   384,
     386,   390,   392,   395,   400,   403,   405,   409,   412,   415,
     418,   420,   425,   428,   434,   436,   440,   442,   446,   448,
     452,   454,   456,   458,   459,   462,   464,   466,   468,   470,
     472,   474,   476,   478,   480,   482,   484,   486,   488,   490,
     492,   494,   496,   498,   500,   502,   504,   506,   508,   510,
     512,   514,   516,   518,   520,   522,   524,   526,   528,   530,
     532,   533,   534,   539,   541,   542,   545,   547,   548,   553,
     556,   561,   565,   567,   571,   574,   576,   578,   580,   582,
     583,   584,   589,   591,   592,   595,   597,   599,   601,   603,
     605,   607,   609,   611,   613,   615,   616,   617,   622,   624,
     625,   628,   630,   633,   634,   642,   646,   652,   654,   657,
     660,   664,   666,   667,   668,   674,   675,   680,   682,   683,
     686,   688,   690,   694,   695,   702,   704,   705,   709,   711,
     713,   715,   722,   726,   732,   736,   742,   746,   752,   756,
     758,   761,   763,   766,   770,   774,   777,   781,   785,   789,
     791,   793,   796,   799,   803,   807,   810,   814,   818,   821,
     822,   824,   825,   831,   832,   834,   838,   841,   845,   847,
     849,   856,   861,   863,   867,   872,   876,   880,   884,   888,
     892,   896,   899,   903,   907,   911,   915,   919,   923,   927,
     931,   935,   938,   942,   946,   950,   953,   957,   961,   967,
     972,   977,   982,   986,   991,   998,  1007,  1018,  1031,  1046,
    1063,  1082,  1103,  1126,  1128,  1130,  1132,  1134,  1138,  1142,
    1149,  1151,  1155,  1160,  1162,  1164,  1166,  1168,  1170,  1172,
    1174,  1176,  1178,  1182,  1186,  1189,  1192,  1194
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const short int yyrhs[] =
{
      91,     0,    -1,    65,   218,    -1,    92,    -1,    93,    -1,
      -1,    93,    94,    -1,    94,    -1,    95,    -1,    97,    -1,
     100,    -1,   105,    -1,   106,    -1,   114,    -1,   120,    -1,
     124,    -1,   128,    -1,   132,    -1,    78,    -1,   116,     1,
      79,    -1,    11,     1,    79,    -1,   122,     1,    79,    -1,
       7,     1,    79,    -1,   126,     1,    79,    -1,     6,     1,
      79,    -1,   130,     1,    79,    -1,     8,     1,    79,    -1,
     134,     1,    79,    -1,    10,     1,    79,    -1,   108,     1,
      79,    -1,     9,     1,    79,    -1,     4,    96,    78,    -1,
      96,    80,    44,    -1,    44,    -1,     3,    98,    78,    -1,
      98,    80,    99,    -1,    99,    -1,    99,    44,    -1,    99,
      73,    -1,    99,    50,    -1,    44,    -1,    73,    -1,    50,
      -1,   101,   148,    78,    -1,   101,    81,   102,    82,   148,
      78,    -1,     5,    83,    44,    -1,     5,    83,    44,    84,
      85,    -1,   103,    -1,    -1,   103,    78,   104,    -1,   104,
      -1,    44,    -1,   156,    78,    -1,    -1,   108,    86,   107,
     136,    79,    -1,    -1,     9,    44,   109,   110,    -1,    -1,
      12,   111,    -1,    13,   112,    -1,    12,   111,    13,   112,
      -1,    96,    -1,   112,    87,   113,    -1,   113,    -1,    96,
      -1,    -1,   116,    86,   115,   136,    79,    -1,    -1,    11,
      44,   117,   118,    -1,    12,   119,    -1,    -1,   119,    87,
     111,    -1,   111,    -1,    -1,   122,    86,   121,   136,   165,
      79,    -1,    -1,     7,    44,   123,   110,    -1,    -1,   126,
      86,   125,   136,   165,   174,   180,   190,    79,    -1,    -1,
       6,    44,   127,   110,    -1,    -1,   130,    86,   129,   136,
     165,   174,   180,   190,    79,    -1,    -1,     8,    44,   131,
     110,    -1,    -1,   134,    86,   133,   136,   165,    79,    -1,
      -1,    10,    44,   135,   118,    -1,   138,    -1,    -1,    15,
      68,   137,   138,    -1,   139,    -1,    -1,   139,   140,    -1,
     140,    -1,   141,    -1,   155,    -1,   142,    -1,   144,    -1,
     143,   148,    78,    -1,   143,   148,    88,   147,   148,    78,
      -1,   146,   145,    44,    -1,    44,    -1,   150,    88,   147,
      78,    -1,    20,    -1,    21,    -1,    22,    -1,    23,    -1,
      24,    -1,    25,    -1,    -1,   218,    -1,    38,    81,   218,
      82,    -1,    38,    -1,    39,    -1,   149,    -1,    -1,   149,
     156,    -1,   156,    -1,   151,    80,   152,    -1,   151,    80,
     152,    -1,   152,    -1,   153,    -1,   153,    84,   154,    85,
      -1,   153,    84,    73,    85,    -1,    50,    -1,    44,    -1,
      44,    89,    44,    -1,     9,    -1,    86,   154,    79,    -1,
      73,    -1,   153,    44,    -1,   153,    86,   154,    79,    -1,
     153,    73,    -1,    45,    -1,    45,    34,    45,    -1,    34,
      45,    -1,    45,    34,    -1,   156,    78,    -1,   157,    -1,
     157,    81,   158,    82,    -1,    83,    44,    -1,    83,    44,
      84,    44,    85,    -1,   159,    -1,   159,    78,   160,    -1,
     160,    -1,    44,    88,   161,    -1,   161,    -1,   161,    87,
     162,    -1,   162,    -1,   163,    -1,    47,    -1,    -1,   163,
     164,    -1,   164,    -1,    44,    -1,    45,    -1,    46,    -1,
      35,    -1,    36,    -1,    89,    -1,    83,    -1,    68,    -1,
      84,    -1,    85,    -1,    86,    -1,    79,    -1,    80,    -1,
      67,    -1,    76,    -1,    71,    -1,    72,    -1,    73,    -1,
      74,    -1,    75,    -1,    70,    -1,    69,    -1,    51,    -1,
      52,    -1,    54,    -1,    53,    -1,    50,    -1,    34,    -1,
      49,    -1,    56,    -1,    55,    -1,    57,    -1,    58,    -1,
     166,    -1,    -1,    -1,    16,    68,   167,   168,    -1,   169,
      -1,    -1,   169,   170,    -1,   170,    -1,    -1,   172,   171,
     148,    78,    -1,   173,    44,    -1,   173,    44,    84,    85,
      -1,   173,    44,   217,    -1,    44,    -1,    44,    84,    85,
      -1,    44,   217,    -1,    26,    -1,    27,    -1,    28,    -1,
     175,    -1,    -1,    -1,    14,    68,   176,   177,    -1,   178,
      -1,    -1,   178,   179,    -1,   179,    -1,   100,    -1,   106,
      -1,   114,    -1,   120,    -1,   124,    -1,   128,    -1,   132,
      -1,    78,    -1,   181,    -1,    -1,    -1,    17,    68,   182,
     183,    -1,   184,    -1,    -1,   184,   185,    -1,   185,    -1,
     187,    78,    -1,    -1,   187,    86,   186,   136,   165,    79,
     229,    -1,   188,    68,    96,    -1,   188,    68,   189,    13,
      96,    -1,    44,    -1,    44,   217,    -1,    70,    69,    -1,
      70,   218,    69,    -1,   191,    -1,    -1,    -1,    18,    19,
      68,   192,   194,    -1,    -1,    18,    68,   193,   194,    -1,
     195,    -1,    -1,   195,   196,    -1,   196,    -1,   197,    -1,
     203,   199,    78,    -1,    -1,   199,    86,   198,   195,    79,
     229,    -1,   200,    -1,    -1,   200,    87,   201,    -1,   201,
      -1,   202,    -1,   216,    -1,    30,    44,    88,   218,    34,
     218,    -1,   204,    31,   208,    -1,   204,    31,   213,    31,
     208,    -1,   204,    32,   208,    -1,   204,    32,   213,    32,
     208,    -1,   204,    33,   208,    -1,   204,    33,   213,    33,
     208,    -1,   205,    80,   206,    -1,   207,    -1,    44,   217,
      -1,    44,    -1,    44,   212,    -1,    44,   212,   217,    -1,
      44,   212,    49,    -1,    44,   212,    -1,    44,   212,   217,
      -1,    44,   212,    49,    -1,   209,    80,   210,    -1,   211,
      -1,    44,    -1,    44,   217,    -1,    44,   212,    -1,    44,
     212,   217,    -1,    44,   212,    49,    -1,    44,   212,    -1,
      44,   212,   217,    -1,    44,   212,    49,    -1,    89,    44,
      -1,    -1,   215,    -1,    -1,   215,    86,   214,   136,    79,
      -1,    -1,    96,    -1,   189,    13,    96,    -1,    29,   218,
      -1,    84,   218,    85,    -1,   220,    -1,   219,    -1,    43,
      81,   225,    87,   225,    82,    -1,    43,    81,   225,    82,
      -1,   221,    -1,    81,   220,    82,    -1,    40,    81,   220,
      82,    -1,   220,    71,   220,    -1,   220,    72,   220,    -1,
     220,    73,   220,    -1,   220,    74,   220,    -1,   220,    75,
     220,    -1,   220,    76,   220,    -1,    72,   220,    -1,   220,
      51,   220,    -1,   220,    52,   220,    -1,   220,    69,   220,
      -1,   220,    53,   220,    -1,   220,    70,   220,    -1,   220,
      54,   220,    -1,   220,    55,   220,    -1,   220,    56,   220,
      -1,   220,    57,   220,    -1,    58,   220,    -1,   220,    59,
     220,    -1,   220,    60,   220,    -1,   220,    61,   220,    -1,
      62,   220,    -1,   220,    63,   220,    -1,   220,    64,   220,
      -1,   220,    67,   220,    68,   220,    -1,    21,    81,   220,
      82,    -1,    20,    81,   220,    82,    -1,    22,    81,   220,
      82,    -1,    44,    81,    82,    -1,    44,    81,   220,    82,
      -1,    44,    81,   220,    87,   220,    82,    -1,    44,    81,
     220,    87,   220,    87,   220,    82,    -1,    44,    81,   220,
      87,   220,    87,   220,    87,   220,    82,    -1,    44,    81,
     220,    87,   220,    87,   220,    87,   220,    87,   220,    82,
      -1,    44,    81,   220,    87,   220,    87,   220,    87,   220,
      87,   220,    87,   220,    82,    -1,    44,    81,   220,    87,
     220,    87,   220,    87,   220,    87,   220,    87,   220,    87,
     220,    82,    -1,    44,    81,   220,    87,   220,    87,   220,
      87,   220,    87,   220,    87,   220,    87,   220,    87,   220,
      82,    -1,    44,    81,   220,    87,   220,    87,   220,    87,
     220,    87,   220,    87,   220,    87,   220,    87,   220,    87,
     220,    82,    -1,    44,    81,   220,    87,   220,    87,   220,
      87,   220,    87,   220,    87,   220,    87,   220,    87,   220,
      87,   220,    87,   220,    82,    -1,   222,    -1,   223,    -1,
     224,    -1,    44,    -1,    37,    80,    44,    -1,    44,    80,
      44,    -1,    44,    84,   220,    85,    80,    44,    -1,    42,
      -1,    42,    81,    82,    -1,    41,    81,   222,    82,    -1,
     225,    -1,   226,    -1,   227,    -1,    47,    -1,    35,    -1,
      36,    -1,    45,    -1,    46,    -1,   228,    -1,   228,    45,
      44,    -1,   228,    46,    44,    -1,    45,    44,    -1,    46,
      44,    -1,    78,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   197,   197,   198,   205,   206,   210,   211,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   227,
     229,   231,   233,   235,   237,   239,   241,   243,   245,   247,
     249,   254,   264,   265,   272,   282,   283,   287,   288,   289,
     290,   291,   292,   299,   304,   312,   317,   326,   327,   331,
     332,   336,   348,   360,   359,   383,   382,   391,   393,   394,
     395,   399,   408,   409,   413,   426,   425,   449,   448,   458,
     459,   463,   464,   472,   471,   496,   495,   509,   508,   536,
     535,   549,   548,   576,   575,   592,   591,   616,   615,   628,
     637,   636,   646,   647,   651,   652,   656,   657,   661,   662,
     669,   675,   693,   700,   708,   730,   732,   734,   736,   738,
     743,   746,   750,   752,   754,   758,   766,   767,   771,   772,
     776,   780,   781,   785,   786,   787,   788,   792,   793,   794,
     795,   796,   797,   798,   799,   803,   804,   805,   806,   813,
     821,   822,   826,   832,   842,   846,   847,   851,   860,   872,
     874,   879,   881,   884,   892,   893,   897,   897,   897,   897,
     897,   898,   898,   898,   898,   898,   898,   898,   898,   898,
     899,   899,   899,   899,   899,   899,   899,   899,   899,   899,
     899,   899,   900,   900,   900,   900,   900,   900,   900,   907,
     908,   913,   912,   925,   926,   930,   934,   945,   944,   956,
     961,   967,   974,   978,   983,   992,   994,   996,  1004,  1005,
    1010,  1009,  1026,  1027,  1031,  1032,  1036,  1037,  1038,  1039,
    1040,  1041,  1042,  1043,  1050,  1051,  1056,  1055,  1068,  1069,
    1073,  1074,  1078,  1084,  1083,  1103,  1107,  1115,  1120,  1129,
    1131,  1139,  1140,  1145,  1144,  1156,  1155,  1168,  1169,  1173,
    1174,  1178,  1179,  1195,  1194,  1216,  1219,  1223,  1228,  1236,
    1237,  1241,  1256,  1260,  1264,  1269,  1274,  1278,  1285,  1286,
    1290,  1296,  1304,  1309,  1315,  1324,  1331,  1339,  1350,  1351,
    1355,  1359,  1367,  1372,  1378,  1387,  1392,  1398,  1407,  1418,
    1422,  1425,  1424,  1441,  1444,  1449,  1461,  1474,  1480,  1484,
    1495,  1497,  1502,  1503,  1505,  1508,  1510,  1512,  1514,  1516,
    1518,  1521,  1525,  1527,  1529,  1531,  1533,  1535,  1538,  1540,
    1542,  1545,  1549,  1551,  1553,  1556,  1559,  1561,  1563,  1566,
    1568,  1570,  1573,  1575,  1577,  1579,  1581,  1583,  1585,  1587,
    1589,  1591,  1593,  1599,  1600,  1601,  1605,  1607,  1609,  1611,
    1616,  1618,  1620,  1625,  1626,  1627,  1631,  1636,  1638,  1643,
    1645,  1647,  1652,  1653,  1654,  1655,  1659,  1660
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IMPORT", "PACKAGE", "PROPERTY",
  "MODULE", "SIMPLE", "NETWORK", "CHANNEL", "MODULEINTERFACE",
  "CHANNELINTERFACE", "EXTENDS", "LIKE", "TYPES", "PARAMETERS", "GATES",
  "SUBMODULES", "CONNECTIONS", "ALLOWUNCONNECTED", "DOUBLETYPE", "INTTYPE",
  "STRINGTYPE", "BOOLTYPE", "XMLTYPE", "VOLATILE", "INPUT_", "OUTPUT_",
  "INOUT_", "IF", "FOR", "RIGHTARROW", "LEFTARROW", "DBLARROW", "TO",
  "TRUE_", "FALSE_", "THIS_", "DEFAULT", "ASK", "CONST_", "SIZEOF",
  "INDEX_", "XMLDOC", "NAME", "INTCONSTANT", "REALCONSTANT",
  "STRINGCONSTANT", "CHARCONSTANT", "PLUSPLUS", "DOUBLEASTERISK", "EQ",
  "NE", "GE", "LE", "AND", "OR", "XOR", "NOT", "BIN_AND", "BIN_OR",
  "BIN_XOR", "BIN_COMPL", "SHIFT_LEFT", "SHIFT_RIGHT",
  "EXPRESSION_SELECTOR", "INVALID_CHAR", "'?'", "':'", "'>'", "'<'", "'+'",
  "'-'", "'*'", "'/'", "'%'", "'^'", "UMIN", "';'", "'}'", "'.'", "'('",
  "')'", "'@'", "'['", "']'", "'{'", "','", "'='", "'$'", "$accept",
  "startsymbol", "nedfile", "definitions", "definition",
  "packagedeclaration", "dottedname", "import", "importspec", "importname",
  "propertydecl", "propertydecl_header", "opt_propertydecl_keys",
  "propertydecl_keys", "propertydecl_key", "fileproperty",
  "channeldefinition", "$@1", "channelheader", "$@2", "opt_inheritance",
  "extendsname", "likenames", "likename", "channelinterfacedefinition",
  "$@3", "channelinterfaceheader", "$@4", "opt_interfaceinheritance",
  "extendsnames", "simplemoduledefinition", "$@5", "simplemoduleheader",
  "$@6", "compoundmoduledefinition", "$@7", "compoundmoduleheader", "$@8",
  "networkdefinition", "$@9", "networkheader", "$@10",
  "moduleinterfacedefinition", "$@11", "moduleinterfaceheader", "$@12",
  "opt_paramblock", "$@13", "opt_params", "params", "paramsitem", "param",
  "param_typenamevalue", "param_typename", "pattern_value", "paramtype",
  "opt_volatile", "paramvalue", "opt_inline_properties",
  "inline_properties", "pattern", "pattern2", "pattern_elem",
  "pattern_name", "pattern_index", "property", "property_namevalue",
  "property_name", "opt_property_keys", "property_keys", "property_key",
  "property_values", "property_value", "property_value_tokens",
  "property_value_token", "opt_gateblock", "gateblock", "$@14",
  "opt_gates", "gates", "gate", "$@15", "gate_typenamesize", "gatetype",
  "opt_typeblock", "typeblock", "$@16", "opt_localtypes", "localtypes",
  "localtype", "opt_submodblock", "submodblock", "$@17", "opt_submodules",
  "submodules", "submodule", "$@18", "submoduleheader", "submodulename",
  "likeparam", "opt_connblock", "connblock", "$@19", "$@20",
  "opt_connections", "connections", "connectionsitem", "connectiongroup",
  "$@21", "opt_loops_and_conditions", "loops_and_conditions",
  "loop_or_condition", "loop", "connection", "leftgatespec", "leftmod",
  "leftgate", "parentleftgate", "rightgatespec", "rightmod", "rightgate",
  "parentrightgate", "opt_subgate", "channelspec", "$@22",
  "channelspec_header", "condition", "vector", "expression", "xmldocvalue",
  "expr", "simple_expr", "identifier", "special_expr", "literal",
  "stringliteral", "boolliteral", "numliteral", "quantity",
  "opt_semicolon", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    90,    91,    91,    92,    92,    93,    93,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    95,    96,    96,    97,    98,    98,    99,    99,    99,
      99,    99,    99,   100,   100,   101,   101,   102,   102,   103,
     103,   104,   105,   107,   106,   109,   108,   110,   110,   110,
     110,   111,   112,   112,   113,   115,   114,   117,   116,   118,
     118,   119,   119,   121,   120,   123,   122,   125,   124,   127,
     126,   129,   128,   131,   130,   133,   132,   135,   134,   136,
     137,   136,   138,   138,   139,   139,   140,   140,   141,   141,
     142,   142,   143,   143,   144,   145,   145,   145,   145,   145,
     146,   146,   147,   147,   147,   147,   148,   148,   149,   149,
     150,   151,   151,   152,   152,   152,   152,   153,   153,   153,
     153,   153,   153,   153,   153,   154,   154,   154,   154,   155,
     156,   156,   157,   157,   158,   159,   159,   160,   160,   161,
     161,   162,   162,   162,   163,   163,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   165,
     165,   167,   166,   168,   168,   169,   169,   171,   170,   172,
     172,   172,   172,   172,   172,   173,   173,   173,   174,   174,
     176,   175,   177,   177,   178,   178,   179,   179,   179,   179,
     179,   179,   179,   179,   180,   180,   182,   181,   183,   183,
     184,   184,   185,   186,   185,   187,   187,   188,   188,   189,
     189,   190,   190,   192,   191,   193,   191,   194,   194,   195,
     195,   196,   196,   198,   197,   199,   199,   200,   200,   201,
     201,   202,   203,   203,   203,   203,   203,   203,   204,   204,
     205,   205,   206,   206,   206,   207,   207,   207,   208,   208,
     209,   209,   210,   210,   210,   211,   211,   211,   212,   212,
     213,   214,   213,   215,   215,   215,   216,   217,   218,   218,
     219,   219,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   221,   221,   221,   222,   222,   222,   222,
     223,   223,   223,   224,   224,   224,   225,   226,   226,   227,
     227,   227,   228,   228,   228,   228,   229,   229
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     1,     1,     0,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     3,     3,     1,     2,     2,     2,
       1,     1,     1,     3,     6,     3,     5,     1,     0,     3,
       1,     1,     2,     0,     5,     0,     4,     0,     2,     2,
       4,     1,     3,     1,     1,     0,     5,     0,     4,     2,
       0,     3,     1,     0,     6,     0,     4,     0,     9,     0,
       4,     0,     9,     0,     4,     0,     6,     0,     4,     1,
       0,     4,     1,     0,     2,     1,     1,     1,     1,     1,
       3,     6,     3,     1,     4,     1,     1,     1,     1,     1,
       1,     0,     1,     4,     1,     1,     1,     0,     2,     1,
       3,     3,     1,     1,     4,     4,     1,     1,     3,     1,
       3,     1,     2,     4,     2,     1,     3,     2,     2,     2,
       1,     4,     2,     5,     1,     3,     1,     3,     1,     3,
       1,     1,     1,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     0,     4,     1,     0,     2,     1,     0,     4,     2,
       4,     3,     1,     3,     2,     1,     1,     1,     1,     0,
       0,     4,     1,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     0,     4,     1,     0,
       2,     1,     2,     0,     7,     3,     5,     1,     2,     2,
       3,     1,     0,     0,     5,     0,     4,     1,     0,     2,
       1,     1,     3,     0,     6,     1,     0,     3,     1,     1,
       1,     6,     3,     5,     3,     5,     3,     5,     3,     1,
       2,     1,     2,     3,     3,     2,     3,     3,     3,     1,
       1,     2,     2,     3,     3,     2,     3,     3,     2,     0,
       1,     0,     5,     0,     1,     3,     2,     3,     1,     1,
       6,     4,     1,     3,     4,     3,     3,     3,     3,     3,
       3,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     3,     3,     3,     2,     3,     3,     5,     4,
       4,     4,     3,     4,     6,     8,    10,    12,    14,    16,
      18,    20,    22,     1,     1,     1,     1,     3,     3,     6,
       1,     3,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     2,     2,     1,     0
};

/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const unsigned char yydprec[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0
};

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const unsigned char yymerger[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error.  */
static const unsigned short int yydefact[] =
{
       5,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    18,     0,     0,     3,     4,     7,     8,     9,    10,
     117,    11,    12,     0,    13,     0,    14,     0,    15,     0,
      16,     0,    17,     0,     0,   140,    40,    42,    41,     0,
      36,    33,     0,     0,     0,    79,     0,    75,     0,    83,
       0,    55,     0,    87,     0,    67,     0,     0,     0,   357,
     358,     0,     0,     0,   350,     0,   346,   359,   360,   356,
       0,     0,     0,     0,     2,   299,   298,   302,   343,   344,
     345,   353,   354,   355,   361,   142,     1,     6,    48,     0,
     116,   119,     0,    53,     0,    65,     0,    73,     0,    77,
       0,    81,     0,    85,    52,   153,    34,     0,    37,    39,
      38,    31,     0,    45,    24,    57,    22,    57,    26,    57,
      30,    57,    28,    70,    20,    70,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   364,   365,   321,
     325,   311,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    51,     0,    47,
      50,    43,   118,    29,   111,    19,   111,    21,   111,    23,
      93,    25,    93,    27,   111,   183,   159,   160,   156,   157,
     158,   152,   184,   182,   178,   179,   181,   180,   186,   185,
     187,   188,   169,   163,   177,   176,   171,   172,   173,   174,
     175,   170,   167,   168,   162,   164,   165,   166,   161,     0,
     144,   146,   148,   150,   151,   155,    35,    32,     0,     0,
       0,    80,    76,    84,    56,     0,    88,    68,     0,     0,
       0,   347,     0,   346,     0,   351,     0,   348,   332,     0,
       0,   303,   312,   313,   315,   317,   318,   319,   320,   322,
     323,   324,   326,   327,     0,   314,   316,   305,   306,   307,
     308,   309,   310,   362,   363,     0,   117,     0,   129,     0,
     110,   127,   126,   131,     0,     0,    89,    92,    95,    96,
      98,   117,    99,     0,     0,     0,   122,   123,    97,     0,
       0,   190,   190,   190,   190,   153,   141,   153,   153,   156,
     154,    46,    61,    58,    64,    59,    63,    72,    69,   330,
     329,   331,   304,   352,   301,     0,   333,     0,     0,     0,
     143,     0,    49,    90,     0,     0,   135,     0,    54,    94,
       0,   105,   106,   107,   108,   109,     0,     0,     0,   132,
     134,     0,     0,   139,    66,     0,     0,   189,   209,   209,
       0,   147,   145,   149,     0,     0,     0,     0,     0,     0,
     328,    44,    93,   128,   137,   138,   130,   100,     0,   102,
     114,   115,     0,   112,   127,   120,     0,     0,     0,   191,
      74,     0,   225,   208,   225,    86,    60,    62,    71,   300,
     334,     0,   349,    91,   136,   117,     0,   104,   125,   124,
     133,   194,   210,     0,   242,   224,   242,     0,     0,     0,
     205,   206,   207,   202,   192,   193,   196,   197,     0,   213,
     226,     0,     0,   241,     0,   335,     0,   101,   113,     0,
     204,   195,   117,   199,     0,     0,     0,     0,     0,     0,
     223,   216,   217,     0,   218,     0,   219,     0,   220,     0,
     221,     0,   222,     0,   211,   212,   215,   229,     0,   245,
      78,    82,     0,   203,     0,     0,     0,   201,   214,   237,
     227,   228,   231,     0,     0,   243,   248,   336,     0,   297,
     198,   200,     0,   238,   230,   232,   233,     0,   248,     0,
       0,   289,   246,   247,   250,   251,     0,   255,   258,   259,
     256,     0,     0,   269,   260,     0,   111,     0,   235,     0,
     244,   296,     0,     0,   275,   270,   249,   253,     0,     0,
     293,   293,   293,     0,   337,     0,   190,   239,     0,     0,
       0,   288,   277,   276,   256,   257,   252,    33,   294,     0,
     262,     0,   279,     0,   290,   264,     0,   266,     0,   289,
     268,     0,     0,   240,   236,     0,   256,   285,   281,     0,
       0,     0,   291,     0,     0,   272,   338,     0,   367,     0,
     367,   287,   286,   295,   289,   278,   289,   263,   111,   265,
     267,   274,   273,     0,   366,   234,   261,   254,   282,     0,
     339,     0,   284,   283,   292,     0,   340,     0,     0,   341,
       0,     0,   342
};

/* YYPDEFGOTO[NTERM-NUM].  */
static const short int yydefgoto[] =
{
      -1,    13,    14,    15,    16,    17,   312,    18,    39,    40,
      19,    20,   168,   169,   170,    21,    22,   174,    23,   121,
     231,   313,   315,   316,    24,   176,    25,   125,   236,   318,
      26,   178,    27,   117,    28,   180,    29,   115,    30,   182,
      31,   119,    32,   184,    33,   123,   285,   372,   286,   287,
     288,   289,   290,   291,   292,   346,   293,   382,    89,    90,
     294,   295,   296,   297,   337,   298,   299,    35,   219,   220,
     221,   222,   223,   224,   225,   356,   357,   411,   424,   425,
     426,   442,   427,   428,   392,   393,   429,   464,   465,   466,
     414,   415,   467,   480,   481,   482,   516,   483,   484,   549,
     432,   433,   498,   486,   502,   503,   504,   505,   544,   506,
     507,   508,   509,   510,   511,   512,   560,   513,   550,   551,
     585,   552,   567,   553,   588,   554,   514,   568,   474,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,   595
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -501
static const short int yypact[] =
{
      45,   210,   -30,   -24,    75,   117,   119,   123,   128,   130,
     480,  -501,    60,   135,  -501,    30,  -501,  -501,  -501,  -501,
      55,  -501,  -501,    15,  -501,    20,  -501,    21,  -501,    23,
    -501,    25,  -501,    26,   104,   109,  -501,  -501,  -501,   129,
     212,  -501,   179,   166,   202,  -501,   213,  -501,   226,  -501,
     245,  -501,   251,  -501,   254,  -501,   149,   221,   255,  -501,
    -501,   271,   272,   275,   286,   288,   291,   329,   337,  -501,
     536,   536,   536,   536,  -501,  -501,  1298,  -501,  -501,  -501,
    -501,  -501,  -501,  -501,    81,   322,  -501,  -501,   365,   367,
     360,  -501,   368,  -501,   370,  -501,   374,  -501,   375,  -501,
     381,  -501,   382,  -501,  -501,   569,  -501,   210,  -501,  -501,
    -501,  -501,   400,   362,  -501,   282,  -501,   282,  -501,   282,
    -501,   282,  -501,   458,  -501,   458,   536,   536,   536,   431,
     536,   242,   394,   438,   443,   342,   536,  -501,  -501,  -501,
    -501,  -501,  1080,   536,   536,   536,   536,   536,   536,   536,
     536,   536,   536,   536,   536,   536,   536,   536,   536,   536,
     536,   536,   536,   536,   444,   446,   448,  -501,   411,   416,
    -501,  -501,  -501,  -501,   153,  -501,   153,  -501,   155,  -501,
     125,  -501,   125,  -501,   155,  -501,  -501,  -501,   415,  -501,
    -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,
    -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,
    -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,  -501,   422,
     427,  -501,   421,  -501,   681,  -501,   212,  -501,   425,   -30,
     -30,  -501,  -501,  -501,  -501,   -30,  -501,  -501,  1112,  1144,
    1176,  -501,  1208,   274,   430,  -501,   265,  -501,  -501,   720,
    1053,  -501,   268,   268,   268,   268,  1402,  1350,  1376,   491,
     344,   392,   319,   319,  1272,   268,   268,   -29,   -29,   452,
     452,   452,   452,  -501,  -501,   428,   360,   365,  -501,   451,
    -501,   408,  -501,  -501,   168,   435,  -501,   133,  -501,  -501,
    -501,   360,  -501,   418,   441,   454,  -501,   108,  -501,   459,
     457,   523,   523,   523,   523,   625,  -501,   569,   625,  -501,
    -501,  -501,   463,   532,   463,   460,  -501,  -501,   461,  -501,
    -501,  -501,  -501,  -501,  -501,   438,  -501,   536,   466,   536,
    -501,   471,  -501,  -501,   506,   508,   517,   481,  -501,  -501,
      44,  -501,  -501,  -501,  -501,  -501,   515,   390,    93,  -501,
    -501,   139,   168,  -501,  -501,   500,   495,  -501,   561,   561,
     505,   421,  -501,  -501,   -30,   -30,   -30,   497,   757,   541,
    1324,  -501,   133,  -501,  -501,   542,  -501,  -501,   390,  -501,
     507,  -501,   511,  -501,   501,   512,   510,   514,   518,  -501,
    -501,   525,   574,  -501,   574,  -501,   460,  -501,  -501,  -501,
    -501,   536,  -501,  -501,  -501,   360,   480,  -501,  -501,  -501,
    -501,   262,  -501,   528,   568,  -501,   568,   794,   522,   519,
    -501,  -501,  -501,   526,  -501,   262,  -501,  -501,   558,   237,
    -501,    49,   527,  -501,   530,  -501,   536,  -501,  -501,   229,
    -501,  -501,   360,   544,   563,   567,   585,   586,   587,   588,
    -501,  -501,  -501,   547,  -501,   548,  -501,   549,  -501,   560,
    -501,   564,  -501,   565,  -501,   237,  -501,   603,   589,  -501,
    -501,  -501,   831,  -501,   571,   534,   276,  -501,  -501,   578,
    -501,   603,  -501,    73,   595,  -501,    28,  -501,   536,  -501,
    -501,  -501,   480,  -501,  -501,  -501,  -501,   223,    28,   480,
     620,   220,  -501,    28,  -501,  -501,   579,   580,  -501,  -501,
     278,   366,   593,  -501,  -501,   868,   155,   437,   463,   653,
    -501,  -501,   596,   624,   151,  -501,  -501,  -501,   278,   607,
     233,   233,   233,   642,  -501,   536,   523,  -501,   618,   -30,
     480,  -501,  -501,  -501,   185,  -501,  -501,   296,   463,   675,
    -501,   609,  -501,   659,   605,  -501,   670,  -501,   673,   614,
    -501,   905,   628,  -501,   463,   678,    86,   171,  -501,   -30,
     669,   674,  -501,   674,   674,   174,  -501,   536,   641,   480,
     641,  -501,  -501,   463,   614,  -501,   266,  -501,   153,  -501,
    -501,  -501,  -501,   942,  -501,  -501,  -501,  -501,   184,   643,
    -501,   536,  -501,  -501,  -501,   979,  -501,   536,  1016,  -501,
     536,  1240,  -501
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -501,  -501,  -501,  -501,   705,  -501,     1,  -501,  -501,   616,
    -400,  -501,  -501,  -501,   447,  -501,  -399,  -501,  -362,  -501,
     161,  -233,   357,   363,  -324,  -501,  -269,  -501,   604,  -501,
    -244,  -501,  -243,  -501,  -241,  -501,  -240,  -501,  -238,  -501,
    -228,  -501,  -225,  -501,  -212,  -501,  -167,  -501,   369,  -501,
     453,  -501,  -501,  -501,  -501,  -501,  -501,   364,  -266,  -501,
    -501,  -501,   395,  -501,    50,  -501,     8,  -501,  -501,  -501,
     439,   440,   436,  -501,   535,  -284,  -501,  -501,  -501,  -501,
     333,  -501,  -501,  -501,   388,  -501,  -501,  -501,  -501,   297,
     384,  -501,  -501,  -501,  -501,   287,  -501,  -501,  -501,   285,
     347,  -501,  -501,  -501,   290,   225,  -491,  -501,  -501,   289,
    -501,   257,  -501,  -501,  -501,  -501,  -501,  -501,  -500,  -501,
    -501,  -501,  -459,  -172,  -501,  -501,  -501,  -380,   -10,  -501,
     -66,  -501,   655,  -501,  -501,  -132,  -501,  -501,  -501,   217
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -290
static const short int yytable[] =
{
      74,   246,   317,    42,   139,   140,   141,   142,    34,   300,
     331,   301,   526,   302,    41,   303,    92,   304,   358,   359,
     360,    94,    96,    34,    98,   340,   100,   102,    91,   451,
     452,   555,   557,     1,     2,     3,     4,     5,     6,     7,
       8,     9,   524,   440,   160,   161,   162,   163,     1,     2,
       3,     4,     5,     6,     7,     8,     9,   499,   500,    43,
     238,   239,   240,   477,   242,   451,   452,   453,   468,   249,
     250,   587,   501,   589,   590,   526,    44,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   172,   493,
     575,    93,   278,   453,    85,   454,    95,    97,    11,    99,
      10,   101,   103,    12,  -256,   499,   500,   469,    46,    45,
      48,   525,   377,    11,    50,   598,   164,   165,    12,    52,
     501,    54,   378,   398,   278,    86,    88,   384,    12,   418,
     279,   454,   278,   282,   543,  -111,  -111,  -111,  -111,  -111,
     280,   495,   349,  -111,  -111,  -111,  -111,  -111,   280,   496,
     455,    47,   278,    49,   278,   580,   283,    51,   279,   281,
     279,   -93,    53,   335,    55,   282,   475,   281,   280,   284,
     280,   350,   104,   282,   336,   456,   457,   582,   458,   459,
     105,   460,   351,   367,   352,   592,   455,   281,   283,   281,
     542,   461,   335,   282,   462,   282,   283,   106,    12,   107,
     113,   284,   386,   336,   499,   500,    12,   463,   603,   284,
     581,   456,   457,   591,   458,   459,   283,   460,   283,   501,
     126,   314,   -93,   602,   -93,   492,    12,   461,    12,   284,
     462,   284,     3,   444,   445,   446,   447,   448,   449,    56,
      57,    58,   562,   463,    36,   492,   108,   111,   492,   112,
      37,   368,   109,   370,    59,    60,    61,    41,   492,    62,
      63,    64,    65,    66,    67,    68,    69,   547,   232,    61,
     233,   114,   234,    38,    91,   110,   243,    70,   420,   421,
     422,    71,   116,   517,   229,   230,    56,    57,    58,    91,
    -271,    72,   127,   517,   492,   118,   423,   499,   500,   523,
      73,    59,    60,    61,   473,   450,    62,    63,    64,    65,
      66,    67,    68,    69,   120,  -289,  -289,   150,   151,   152,
     122,   153,   154,   124,    70,   417,   128,   383,    71,   158,
     159,   160,   161,   162,   163,  -289,  -280,   324,    72,   536,
     492,   129,   325,   130,   134,   523,   131,    73,   136,   556,
     558,   491,    56,    57,    58,   314,   314,   132,   383,   133,
     472,   134,   135,   137,  -289,   136,   -33,    59,    60,    61,
     492,   138,    62,    63,    64,   523,    66,    67,    68,    69,
     158,   159,   160,   161,   162,   163,   419,   530,   531,   532,
      70,   387,   388,   150,    71,   152,   166,   153,   154,   167,
      56,    57,    58,    91,    72,   158,   159,   160,   161,   162,
     163,   599,   515,    73,   248,    59,    60,    61,   380,   381,
      62,    63,    64,    65,    66,    67,    68,    69,   341,   342,
     343,   344,   345,    12,   227,   171,   228,   173,    70,   175,
      91,   150,    71,   177,   179,   153,   154,    56,    57,    58,
     181,   183,    72,   158,   159,   160,   161,   162,   163,   561,
     235,    73,    59,    60,    61,   241,   245,    62,    63,    64,
      65,    66,    67,    68,    69,    69,  -103,   247,   273,   521,
     274,  -103,   275,   276,   277,    70,  -103,   334,   518,    71,
      56,    57,    58,   305,   306,   307,   537,   538,   308,    72,
     311,   593,   323,   330,   338,    59,    60,    61,    73,   333,
      62,    63,    64,    65,    66,    67,    68,    69,   163,   347,
     565,   548,   548,   548,   348,   605,   354,   353,    70,   355,
     564,   608,    71,   112,   611,   364,   369,   365,   366,   371,
     373,   375,    72,   374,   153,   154,    56,    57,    58,   379,
     376,    73,   158,   159,   160,   161,   162,   163,   389,   596,
     583,    59,    60,    61,   390,   391,    62,    63,    64,   399,
      66,    67,    68,    69,   395,   402,   431,   404,   406,   407,
     334,   413,  -121,   412,    70,   408,   430,   410,    71,   409,
     437,   438,   443,   185,   186,   187,   470,    45,    72,   471,
     439,    47,   490,   188,   189,   190,   191,    73,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   201,   476,    49,
      51,    53,    55,    93,    95,    97,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,    99,   479,   212,   213,
     101,   103,   214,   215,   216,   217,   489,   485,   218,   185,
     186,   187,   492,   497,   522,   527,   539,   528,   541,   309,
     189,   190,   191,   533,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   540,   546,   559,   563,   569,   570,
     571,   572,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   573,   523,   212,   213,   574,   578,   214,   215,
     216,   217,   579,   584,   218,   185,   186,   187,   586,   594,
      87,   396,   604,   226,   332,   309,   189,   190,   397,   237,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     339,   403,   405,   385,   363,   361,   362,   394,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   441,   310,
     212,   213,   478,   434,   214,   215,   216,   217,   494,   566,
     218,   143,   144,   145,   146,   147,   148,   149,   416,   150,
     151,   152,   519,   153,   154,   545,   244,   155,   520,   156,
     157,   158,   159,   160,   161,   162,   163,   597,     0,   529,
       0,     0,   326,     0,     0,     0,     0,   327,   143,   144,
     145,   146,   147,   148,   149,     0,   150,   151,   152,     0,
     153,   154,     0,     0,   155,     0,   156,   157,   158,   159,
     160,   161,   162,   163,     0,     0,     0,     0,     0,   400,
       0,     0,     0,     0,   401,   143,   144,   145,   146,   147,
     148,   149,     0,   150,   151,   152,     0,   153,   154,     0,
       0,   155,     0,   156,   157,   158,   159,   160,   161,   162,
     163,     0,     0,     0,     0,     0,   435,     0,     0,     0,
       0,   436,   143,   144,   145,   146,   147,   148,   149,     0,
     150,   151,   152,     0,   153,   154,     0,     0,   155,     0,
     156,   157,   158,   159,   160,   161,   162,   163,     0,     0,
       0,     0,     0,   487,     0,     0,     0,     0,   488,   143,
     144,   145,   146,   147,   148,   149,     0,   150,   151,   152,
       0,   153,   154,     0,     0,   155,     0,   156,   157,   158,
     159,   160,   161,   162,   163,     0,     0,     0,     0,     0,
     534,     0,     0,     0,     0,   535,   143,   144,   145,   146,
     147,   148,   149,     0,   150,   151,   152,     0,   153,   154,
       0,     0,   155,     0,   156,   157,   158,   159,   160,   161,
     162,   163,     0,     0,     0,     0,     0,   576,     0,     0,
       0,     0,   577,   143,   144,   145,   146,   147,   148,   149,
       0,   150,   151,   152,     0,   153,   154,     0,     0,   155,
       0,   156,   157,   158,   159,   160,   161,   162,   163,     0,
       0,     0,     0,     0,   600,     0,     0,     0,     0,   601,
     143,   144,   145,   146,   147,   148,   149,     0,   150,   151,
     152,     0,   153,   154,     0,     0,   155,     0,   156,   157,
     158,   159,   160,   161,   162,   163,     0,     0,     0,     0,
       0,   606,     0,     0,     0,     0,   607,   143,   144,   145,
     146,   147,   148,   149,     0,   150,   151,   152,     0,   153,
     154,     0,     0,   155,     0,   156,   157,   158,   159,   160,
     161,   162,   163,     0,     0,     0,     0,     0,   609,     0,
       0,     0,     0,   610,   143,   144,   145,   146,   147,   148,
     149,     0,   150,   151,   152,     0,   153,   154,     0,     0,
     155,     0,   156,   157,   158,   159,   160,   161,   162,   163,
       0,   143,   144,   145,   146,   147,   148,   149,   328,   150,
     151,   152,     0,   153,   154,     0,     0,   155,     0,   156,
     157,   158,   159,   160,   161,   162,   163,     0,     0,     0,
       0,     0,   251,   143,   144,   145,   146,   147,   148,   149,
       0,   150,   151,   152,     0,   153,   154,     0,     0,   155,
       0,   156,   157,   158,   159,   160,   161,   162,   163,     0,
       0,     0,     0,     0,   319,   143,   144,   145,   146,   147,
     148,   149,     0,   150,   151,   152,     0,   153,   154,     0,
       0,   155,     0,   156,   157,   158,   159,   160,   161,   162,
     163,     0,     0,     0,     0,     0,   320,   143,   144,   145,
     146,   147,   148,   149,     0,   150,   151,   152,     0,   153,
     154,     0,     0,   155,     0,   156,   157,   158,   159,   160,
     161,   162,   163,     0,     0,     0,     0,     0,   321,   143,
     144,   145,   146,   147,   148,   149,     0,   150,   151,   152,
       0,   153,   154,     0,     0,   155,     0,   156,   157,   158,
     159,   160,   161,   162,   163,     0,     0,     0,     0,     0,
     322,   143,   144,   145,   146,   147,   148,   149,     0,   150,
     151,   152,     0,   153,   154,     0,     0,   155,     0,   156,
     157,   158,   159,   160,   161,   162,   163,     0,     0,     0,
       0,     0,   612,   143,   144,   145,   146,   147,   148,   149,
       0,   150,   151,   152,     0,   153,   154,     0,     0,   155,
     329,   156,   157,   158,   159,   160,   161,   162,   163,   143,
     144,   145,   146,   147,   148,   149,     0,   150,   151,   152,
       0,   153,   154,     0,     0,   155,     0,   156,   157,   158,
     159,   160,   161,   162,   163,   143,   144,   145,   146,   147,
     148,   149,     0,   150,   151,   152,     0,   153,   154,     0,
       0,   155,     0,   156,   157,   158,   159,   160,   161,   162,
     163,   143,   144,   145,   146,   147,     0,   149,     0,   150,
     151,   152,     0,   153,   154,     0,     0,     0,     0,   156,
     157,   158,   159,   160,   161,   162,   163,   143,   144,   145,
     146,   147,     0,     0,     0,   150,   151,   152,     0,   153,
     154,     0,     0,     0,     0,   156,   157,   158,   159,   160,
     161,   162,   163,   143,   144,   145,   146,     0,     0,     0,
       0,   150,   151,   152,     0,   153,   154,     0,     0,     0,
       0,   156,   157,   158,   159,   160,   161,   162,   163
};

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const unsigned char yyconflp[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     3,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       9,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     5,     0,     0,     0,
       7,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0
};

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short int yyconfl[] =
{
       0,   298,     0,   289,     0,   280,     0,   289,     0,   289,
       0
};

static const short int yycheck[] =
{
      10,   133,   235,     2,    70,    71,    72,    73,     0,   176,
     276,   178,   503,   180,    44,   182,     1,   184,   302,   303,
     304,     1,     1,    15,     1,   291,     1,     1,    20,   429,
     429,   531,   532,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   501,   423,    73,    74,    75,    76,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    29,    30,    83,
     126,   127,   128,   443,   130,   465,   465,   429,    19,   135,
     136,   571,    44,   573,   574,   566,     1,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    90,   479,
     559,    86,     9,   465,    44,   429,    86,    86,    78,    86,
      65,    86,    86,    83,    86,    29,    30,    68,     1,    44,
       1,   501,    78,    78,     1,   584,    45,    46,    83,     1,
      44,     1,    88,   366,     9,     0,    81,    44,    83,   405,
      15,   465,     9,    50,   524,    20,    21,    22,    23,    24,
      25,    78,    44,    20,    21,    22,    23,    24,    25,    86,
     429,    44,     9,    44,     9,    79,    73,    44,    15,    44,
      15,    16,    44,    34,    44,    50,   442,    44,    25,    86,
      25,    73,    78,    50,    45,   429,   429,   567,   429,   429,
      81,   429,    84,   325,    86,   575,   465,    44,    73,    44,
      49,   429,    34,    50,   429,    50,    73,    78,    83,    80,
      44,    86,    73,    45,    29,    30,    83,   429,   598,    86,
      49,   465,   465,    49,   465,   465,    73,   465,    73,    44,
      81,   230,    79,    49,    79,    84,    83,   465,    83,    86,
     465,    86,     5,     6,     7,     8,     9,    10,    11,    20,
      21,    22,   536,   465,    44,    84,    44,    78,    84,    80,
      50,   327,    50,   329,    35,    36,    37,    44,    84,    40,
      41,    42,    43,    44,    45,    46,    47,    44,   117,    37,
     119,    79,   121,    73,   276,    73,    44,    58,    26,    27,
      28,    62,    79,    70,    12,    13,    20,    21,    22,   291,
      80,    72,    81,    70,    84,    79,    44,    29,    30,    89,
      81,    35,    36,    37,    85,    78,    40,    41,    42,    43,
      44,    45,    46,    47,    79,    29,    30,    59,    60,    61,
      79,    63,    64,    79,    58,   401,    81,   347,    62,    71,
      72,    73,    74,    75,    76,    49,    80,    82,    72,   516,
      84,    80,    87,    81,    80,    89,    81,    81,    84,   531,
     532,    85,    20,    21,    22,   364,   365,    81,   378,    81,
     436,    80,    81,    44,    78,    84,    80,    35,    36,    37,
      84,    44,    40,    41,    42,    89,    44,    45,    46,    47,
      71,    72,    73,    74,    75,    76,   406,    31,    32,    33,
      58,   351,   352,    59,    62,    61,    84,    63,    64,    44,
      20,    21,    22,   405,    72,    71,    72,    73,    74,    75,
      76,   588,   488,    81,    82,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    20,    21,
      22,    23,    24,    83,    44,    78,    84,    79,    58,    79,
     442,    59,    62,    79,    79,    63,    64,    20,    21,    22,
      79,    79,    72,    71,    72,    73,    74,    75,    76,   535,
      12,    81,    35,    36,    37,    44,    82,    40,    41,    42,
      43,    44,    45,    46,    47,    47,    78,    44,    44,   499,
      44,    83,    44,    82,    78,    58,    88,    89,   497,    62,
      20,    21,    22,    88,    82,    78,    69,   517,    87,    72,
      85,   577,    82,    85,    79,    35,    36,    37,    81,    68,
      40,    41,    42,    43,    44,    45,    46,    47,    76,    88,
     540,   530,   531,   532,    80,   601,    79,    78,    58,    16,
     539,   607,    62,    80,   610,    13,    80,    87,    87,    78,
      44,    34,    72,    45,    63,    64,    20,    21,    22,    44,
      79,    81,    71,    72,    73,    74,    75,    76,    68,   579,
     569,    35,    36,    37,    79,    14,    40,    41,    42,    82,
      44,    45,    46,    47,    79,    44,    18,    45,    81,    78,
      89,    17,    80,    68,    58,    85,    68,    79,    62,    85,
      78,    82,    44,    34,    35,    36,    79,    44,    72,    79,
      84,    44,    78,    44,    45,    46,    47,    81,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    84,    44,
      44,    44,    44,    86,    86,    86,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    86,    44,    79,    80,
      86,    86,    83,    84,    85,    86,    85,    68,    89,    34,
      35,    36,    84,    68,    44,    86,    13,    87,    44,    44,
      45,    46,    47,    80,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    88,    78,    44,    69,    13,    80,
      31,    86,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    32,    89,    79,    80,    33,    79,    83,    84,
      85,    86,    34,    44,    89,    34,    35,    36,    44,    78,
      15,   364,    79,   107,   277,    44,    45,    46,   365,   125,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
     287,   372,   378,   348,   308,   305,   307,   359,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,   425,   224,
      79,    80,   465,   416,    83,    84,    85,    86,   481,   544,
      89,    51,    52,    53,    54,    55,    56,    57,   394,    59,
      60,    61,   497,    63,    64,   528,   131,    67,   498,    69,
      70,    71,    72,    73,    74,    75,    76,   580,    -1,   510,
      -1,    -1,    82,    -1,    -1,    -1,    -1,    87,    51,    52,
      53,    54,    55,    56,    57,    -1,    59,    60,    61,    -1,
      63,    64,    -1,    -1,    67,    -1,    69,    70,    71,    72,
      73,    74,    75,    76,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    -1,    87,    51,    52,    53,    54,    55,
      56,    57,    -1,    59,    60,    61,    -1,    63,    64,    -1,
      -1,    67,    -1,    69,    70,    71,    72,    73,    74,    75,
      76,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      -1,    87,    51,    52,    53,    54,    55,    56,    57,    -1,
      59,    60,    61,    -1,    63,    64,    -1,    -1,    67,    -1,
      69,    70,    71,    72,    73,    74,    75,    76,    -1,    -1,
      -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,    87,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    61,
      -1,    63,    64,    -1,    -1,    67,    -1,    69,    70,    71,
      72,    73,    74,    75,    76,    -1,    -1,    -1,    -1,    -1,
      82,    -1,    -1,    -1,    -1,    87,    51,    52,    53,    54,
      55,    56,    57,    -1,    59,    60,    61,    -1,    63,    64,
      -1,    -1,    67,    -1,    69,    70,    71,    72,    73,    74,
      75,    76,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,
      -1,    -1,    87,    51,    52,    53,    54,    55,    56,    57,
      -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,    67,
      -1,    69,    70,    71,    72,    73,    74,    75,    76,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    -1,    87,
      51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
      61,    -1,    63,    64,    -1,    -1,    67,    -1,    69,    70,
      71,    72,    73,    74,    75,    76,    -1,    -1,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    -1,    87,    51,    52,    53,
      54,    55,    56,    57,    -1,    59,    60,    61,    -1,    63,
      64,    -1,    -1,    67,    -1,    69,    70,    71,    72,    73,
      74,    75,    76,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    -1,    87,    51,    52,    53,    54,    55,    56,
      57,    -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,
      67,    -1,    69,    70,    71,    72,    73,    74,    75,    76,
      -1,    51,    52,    53,    54,    55,    56,    57,    85,    59,
      60,    61,    -1,    63,    64,    -1,    -1,    67,    -1,    69,
      70,    71,    72,    73,    74,    75,    76,    -1,    -1,    -1,
      -1,    -1,    82,    51,    52,    53,    54,    55,    56,    57,
      -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,    67,
      -1,    69,    70,    71,    72,    73,    74,    75,    76,    -1,
      -1,    -1,    -1,    -1,    82,    51,    52,    53,    54,    55,
      56,    57,    -1,    59,    60,    61,    -1,    63,    64,    -1,
      -1,    67,    -1,    69,    70,    71,    72,    73,    74,    75,
      76,    -1,    -1,    -1,    -1,    -1,    82,    51,    52,    53,
      54,    55,    56,    57,    -1,    59,    60,    61,    -1,    63,
      64,    -1,    -1,    67,    -1,    69,    70,    71,    72,    73,
      74,    75,    76,    -1,    -1,    -1,    -1,    -1,    82,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    61,
      -1,    63,    64,    -1,    -1,    67,    -1,    69,    70,    71,
      72,    73,    74,    75,    76,    -1,    -1,    -1,    -1,    -1,
      82,    51,    52,    53,    54,    55,    56,    57,    -1,    59,
      60,    61,    -1,    63,    64,    -1,    -1,    67,    -1,    69,
      70,    71,    72,    73,    74,    75,    76,    -1,    -1,    -1,
      -1,    -1,    82,    51,    52,    53,    54,    55,    56,    57,
      -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    61,
      -1,    63,    64,    -1,    -1,    67,    -1,    69,    70,    71,
      72,    73,    74,    75,    76,    51,    52,    53,    54,    55,
      56,    57,    -1,    59,    60,    61,    -1,    63,    64,    -1,
      -1,    67,    -1,    69,    70,    71,    72,    73,    74,    75,
      76,    51,    52,    53,    54,    55,    -1,    57,    -1,    59,
      60,    61,    -1,    63,    64,    -1,    -1,    -1,    -1,    69,
      70,    71,    72,    73,    74,    75,    76,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    59,    60,    61,    -1,    63,
      64,    -1,    -1,    -1,    -1,    69,    70,    71,    72,    73,
      74,    75,    76,    51,    52,    53,    54,    -1,    -1,    -1,
      -1,    59,    60,    61,    -1,    63,    64,    -1,    -1,    -1,
      -1,    69,    70,    71,    72,    73,    74,    75,    76
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      65,    78,    83,    91,    92,    93,    94,    95,    97,   100,
     101,   105,   106,   108,   114,   116,   120,   122,   124,   126,
     128,   130,   132,   134,   156,   157,    44,    50,    73,    98,
      99,    44,    96,    83,     1,    44,     1,    44,     1,    44,
       1,    44,     1,    44,     1,    44,    20,    21,    22,    35,
      36,    37,    40,    41,    42,    43,    44,    45,    46,    47,
      58,    62,    72,    81,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    44,     0,    94,    81,   148,
     149,   156,     1,    86,     1,    86,     1,    86,     1,    86,
       1,    86,     1,    86,    78,    81,    78,    80,    44,    50,
      73,    78,    80,    44,    79,   127,    79,   123,    79,   131,
      79,   109,    79,   135,    79,   117,    81,    81,    81,    80,
      81,    81,    81,    81,    80,    81,    84,    44,    44,   220,
     220,   220,   220,    51,    52,    53,    54,    55,    56,    57,
      59,    60,    61,    63,    64,    67,    69,    70,    71,    72,
      73,    74,    75,    76,    45,    46,    84,    44,   102,   103,
     104,    78,   156,    79,   107,    79,   115,    79,   121,    79,
     125,    79,   129,    79,   133,    34,    35,    36,    44,    45,
      46,    47,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    79,    80,    83,    84,    85,    86,    89,   158,
     159,   160,   161,   162,   163,   164,    99,    44,    84,    12,
      13,   110,   110,   110,   110,    12,   118,   118,   220,   220,
     220,    44,   220,    44,   222,    82,   225,    44,    82,   220,
     220,    82,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,    44,    44,    44,    82,    78,     9,    15,
      25,    44,    50,    73,    86,   136,   138,   139,   140,   141,
     142,   143,   144,   146,   150,   151,   152,   153,   155,   156,
     136,   136,   136,   136,   136,    88,    82,    78,    87,    44,
     164,    85,    96,   111,    96,   112,   113,   111,   119,    82,
      82,    82,    82,    82,    82,    87,    82,    87,    85,    68,
      85,   148,   104,    68,    89,    34,    45,   154,    79,   140,
     148,    20,    21,    22,    23,    24,   145,    88,    80,    44,
      73,    84,    86,    78,    79,    16,   165,   166,   165,   165,
     165,   161,   160,   162,    13,    87,    87,   225,   220,    80,
     220,    78,   137,    44,    45,    34,    79,    78,    88,    44,
      38,    39,   147,   218,    44,   152,    73,   154,   154,    68,
      79,    14,   174,   175,   174,    79,   112,   113,   111,    82,
      82,    87,    44,   138,    45,   147,    81,    78,    85,    85,
      79,   167,    68,    17,   180,   181,   180,   220,   148,   218,
      26,    27,    28,    44,   168,   169,   170,   172,   173,   176,
      68,    18,   190,   191,   190,    82,    87,    78,    82,    84,
     217,   170,   171,    44,     6,     7,     8,     9,    10,    11,
      78,   100,   106,   108,   114,   116,   120,   122,   124,   126,
     128,   130,   132,   134,   177,   178,   179,   182,    19,    68,
      79,    79,   220,    85,   218,   148,    84,   217,   179,    44,
     183,   184,   185,   187,   188,    68,   193,    82,    87,    85,
      78,    85,    84,   217,   185,    78,    86,    68,   192,    29,
      30,    44,   194,   195,   196,   197,   199,   200,   201,   202,
     203,   204,   205,   207,   216,   220,   186,    70,    96,   189,
     194,   218,    44,    89,   212,   217,   196,    86,    87,   199,
      31,    32,    33,    80,    82,    87,   136,    69,   218,    13,
      88,    44,    49,   217,   198,   201,    78,    44,    96,   189,
     208,   209,   211,   213,   215,   208,   213,   208,   213,    44,
     206,   220,   165,    69,    96,   218,   195,   212,   217,    13,
      80,    31,    86,    32,    33,   212,    82,    87,    79,    34,
      79,    49,   217,    96,    44,   210,    44,   208,   214,   208,
     208,    49,   217,   220,    78,   229,   218,   229,   212,   136,
      82,    87,    49,   217,    79,   220,    82,    87,   220,    82,
      87,   220,    82
};


/* Prevent warning if -Wmissing-prototypes.  */
int yyparse (void);

/* Error token number */
#define YYTERROR 1

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */


#define YYRHSLOC(Rhs, K) ((Rhs)[K].yystate.yyloc)
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))							\
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

/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# define YY_LOCATION_PRINT(File, Loc)			\
    fprintf (File, "%d.%d-%d.%d",			\
	     (Loc).first_line, (Loc).first_column,	\
	     (Loc).last_line,  (Loc).last_column)
#endif


#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#define YYLEX yylex ()

YYSTYPE yylval;

YYLTYPE yylloc;

int yynerrs;
int yychar;

static const int YYEOF = 0;
static const int YYEMPTY = -2;

typedef enum { yyok, yyaccept, yyabort, yyerr } YYRESULTTAG;

#define YYCHK(YYE)							     \
   do { YYRESULTTAG yyflag = YYE; if (yyflag != yyok) return yyflag; }	     \
   while (YYID (0))

#if YYDEBUG

# ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
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

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
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

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			    \
do {									    \
  if (yydebug)								    \
    {									    \
      YYFPRINTF (stderr, "%s ", Title);					    \
      yy_symbol_print (stderr, Type,					    \
		       Value, Location);  \
      YYFPRINTF (stderr, "\n");						    \
    }									    \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

#else /* !YYDEBUG */

# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)

#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
# if (! defined __cplusplus \
      || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	  && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL))
#  define YYSTACKEXPANDABLE 1
# else
#  define YYSTACKEXPANDABLE 0
# endif
#endif

#if YYSTACKEXPANDABLE
# define YY_RESERVE_GLRSTACK(Yystack)			\
  do {							\
    if (Yystack->yyspaceLeft < YYHEADROOM)		\
      yyexpandGLRStack (Yystack);			\
  } while (YYID (0))
#else
# define YY_RESERVE_GLRSTACK(Yystack)			\
  do {							\
    if (Yystack->yyspaceLeft < YYHEADROOM)		\
      yyMemoryExhausted (Yystack);			\
  } while (YYID (0))
#endif


#if YYERROR_VERBOSE

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
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
static size_t
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
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
    return strlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* !YYERROR_VERBOSE */

/** State numbers, as in LALR(1) machine */
typedef int yyStateNum;

/** Rule numbers, as in LALR(1) machine */
typedef int yyRuleNum;

/** Grammar symbol */
typedef short int yySymbol;

/** Item references, as in LALR(1) machine */
typedef short int yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState {
  /** Type tag: always true.  */
  yybool yyisState;
  /** Type tag for yysemantics.  If true, yysval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yyStateNum yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the first token produced by my symbol */
  size_t yyposn;
  union {
    /** First in a chain of alternative reductions producing the
     *  non-terminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yysval;
  } yysemantics;
  /** Source location for this state.  */
  YYLTYPE yyloc;
};

struct yyGLRStateSet {
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != YYEMPTY.  */
  yybool* yylookaheadNeeds;
  size_t yysize, yycapacity;
};

struct yySemanticOption {
  /** Type tag: always false.  */
  yybool yyisState;
  /** Rule number for this reduction */
  yyRuleNum yyrule;
  /** The last RHS state in the list of states to be reduced.  */
  yyGLRState* yystate;
  /** The lookahead for this reduction.  */
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;
  /** Next sibling in chain of options.  To facilitate merging,
   *  options are chained in decreasing order by address.  */
  yySemanticOption* yynext;
};

/** Type of the items in the GLR stack.  The yyisState field
 *  indicates which item of the union is valid.  */
union yyGLRStackItem {
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack {
  int yyerrState;
  /* To compute the location of the error token.  */
  yyGLRStackItem yyerror_range[3];

  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  size_t yyspaceLeft;
  yyGLRState* yysplitPoint;
  yyGLRState* yylastDeleted;
  yyGLRStateSet yytops;
};

#if YYSTACKEXPANDABLE
static void yyexpandGLRStack (yyGLRStack* yystackp);
#endif

static void yyFail (yyGLRStack* yystackp, const char* yymsg)
  __attribute__ ((__noreturn__));
static void
yyFail (yyGLRStack* yystackp, const char* yymsg)
{
  if (yymsg != NULL)
    yyerror (yymsg);
  YYLONGJMP (yystackp->yyexception_buffer, 1);
}

static void yyMemoryExhausted (yyGLRStack* yystackp)
  __attribute__ ((__noreturn__));
static void
yyMemoryExhausted (yyGLRStack* yystackp)
{
  YYLONGJMP (yystackp->yyexception_buffer, 2);
}

#if YYDEBUG || YYERROR_VERBOSE
/** A printable representation of TOKEN.  */
static inline const char*
yytokenName (yySymbol yytoken)
{
  if (yytoken == YYEMPTY)
    return "";

  return yytname[yytoken];
}
#endif

/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin (yyGLRStackItem *, int, int) __attribute__ ((__unused__));
static void
yyfillin (yyGLRStackItem *yyvsp, int yylow0, int yylow1)
{
  yyGLRState* s;
  int i;
  s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0-1; i >= yylow1; i -= 1)
    {
      YYASSERT (s->yyresolved);
      yyvsp[i].yystate.yyresolved = yytrue;
      yyvsp[i].yystate.yysemantics.yysval = s->yysemantics.yysval;
      yyvsp[i].yystate.yyloc = s->yyloc;
      s = yyvsp[i].yystate.yypred = s->yypred;
    }
}

/* Do nothing if YYNORMAL or if *YYLOW <= YYLOW1.  Otherwise, fill in
 * YYVSP[YYLOW1 .. *YYLOW-1] as in yyfillin and set *YYLOW = YYLOW1.
 * For convenience, always return YYLOW1.  */
static inline int yyfill (yyGLRStackItem *, int *, int, yybool)
     __attribute__ ((__unused__));
static inline int
yyfill (yyGLRStackItem *yyvsp, int *yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
    {
      yyfillin (yyvsp, *yylow, yylow1);
      *yylow = yylow1;
    }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT.  */
/*ARGSUSED*/ static YYRESULTTAG
yyuserAction (yyRuleNum yyn, int yyrhslen, yyGLRStackItem* yyvsp,
	      YYSTYPE* yyvalp,
	      YYLTYPE* YYOPTIONAL_LOC (yylocp),
	      yyGLRStack* yystackp
	      )
{
  yybool yynormal __attribute__ ((__unused__)) =
    (yystackp->yysplitPoint == NULL);
  int yylow;
# undef yyerrok
# define yyerrok (yystackp->yyerrState = 0)
# undef YYACCEPT
# define YYACCEPT return yyaccept
# undef YYABORT
# define YYABORT return yyabort
# undef YYERROR
# define YYERROR return yyerrok, yyerr
# undef YYRECOVERING
# define YYRECOVERING() (yystackp->yyerrState != 0)
# undef yyclearin
# define yyclearin (yychar = YYEMPTY)
# undef YYFILL
# define YYFILL(N) yyfill (yyvsp, &yylow, N, yynormal)
# undef YYBACKUP
# define YYBACKUP(Token, Value)						     \
  return yyerror (YY_("syntax error: cannot back up")),     \
	 yyerrok, yyerr

  yylow = 1;
  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL (1-yyrhslen)].yystate.yysemantics.yysval;
  YYLLOC_DEFAULT ((*yylocp), (yyvsp - yyrhslen), yyrhslen);
  yystackp->yyerror_range[1].yystate.yyloc = *yylocp;

  switch (yyn)
    {
        case 19:

/* Line 936 of glr.c  */
#line 228 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 20:

/* Line 936 of glr.c  */
#line 230 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 21:

/* Line 936 of glr.c  */
#line 232 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 22:

/* Line 936 of glr.c  */
#line 234 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 23:

/* Line 936 of glr.c  */
#line 236 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 24:

/* Line 936 of glr.c  */
#line 238 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 25:

/* Line 936 of glr.c  */
#line 240 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 26:

/* Line 936 of glr.c  */
#line 242 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 27:

/* Line 936 of glr.c  */
#line 244 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 28:

/* Line 936 of glr.c  */
#line 246 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 29:

/* Line 936 of glr.c  */
#line 248 "ned2.y"
    { storePos(ps.component, (*yylocp)); restoreGlobalParserState(); }
    break;

  case 30:

/* Line 936 of glr.c  */
#line 250 "ned2.y"
    { restoreGlobalParserState(); }
    break;

  case 31:

/* Line 936 of glr.c  */
#line 255 "ned2.y"
    {
                  ps.package = (PackageElement *)createElementWithTag(NED_PACKAGE, ps.nedfile);
                  ps.package->setName(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yyloc)).c_str());
                  storePos(ps.package,(*yylocp));
                  storeBannerAndRightComments(ps.package,(*yylocp));
                }
    break;

  case 34:

/* Line 936 of glr.c  */
#line 273 "ned2.y"
    {
                  ps.import = (ImportElement *)createElementWithTag(NED_IMPORT, ps.nedfile);
                  ps.import->setImportSpec(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yyloc)).c_str());
                  storePos(ps.import,(*yylocp));
                  storeBannerAndRightComments(ps.import,(*yylocp));
                }
    break;

  case 43:

/* Line 936 of glr.c  */
#line 300 "ned2.y"
    {
                    storePos(ps.propertydecl, (*yylocp));
                    storeBannerAndRightComments(ps.propertydecl,(*yylocp));
                }
    break;

  case 44:

/* Line 936 of glr.c  */
#line 305 "ned2.y"
    {
                    storePos(ps.propertydecl, (*yylocp));
                    storeBannerAndRightComments(ps.propertydecl,(*yylocp));
                }
    break;

  case 45:

/* Line 936 of glr.c  */
#line 313 "ned2.y"
    {
                  ps.propertydecl = (PropertyDeclElement *)createElementWithTag(NED_PROPERTY_DECL, ps.nedfile);
                  ps.propertydecl->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc)));
                }
    break;

  case 46:

/* Line 936 of glr.c  */
#line 318 "ned2.y"
    {
                  ps.propertydecl = (PropertyDeclElement *)createElementWithTag(NED_PROPERTY_DECL, ps.nedfile);
                  ps.propertydecl->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (5))].yystate.yyloc)));
                  ps.propertydecl->setIsArray(true);
                }
    break;

  case 51:

/* Line 936 of glr.c  */
#line 337 "ned2.y"
    {
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.propertydecl);
                  ps.propkey->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)));
                  storePos(ps.propkey, (*yylocp));
                }
    break;

  case 52:

/* Line 936 of glr.c  */
#line 349 "ned2.y"
    {
                  storePos(ps.property, (*yylocp));
                  storeBannerAndRightComments(ps.property,(*yylocp));
                }
    break;

  case 53:

/* Line 936 of glr.c  */
#line 360 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.component);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 54:

/* Line 936 of glr.c  */
#line 369 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 55:

/* Line 936 of glr.c  */
#line 383 "ned2.y"
    {
                  ps.component = (ChannelElement *)createElementWithTag(NED_CHANNEL, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile);
                  ((ChannelElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 56:

/* Line 936 of glr.c  */
#line 388 "ned2.y"
    { storeBannerAndRightComments(ps.component,(*yylocp)); }
    break;

  case 61:

/* Line 936 of glr.c  */
#line 400 "ned2.y"
    {
                  ps.extends = (ExtendsElement *)createElementWithTag(NED_EXTENDS, ps.component);
                  ps.extends->setName(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)).c_str());
                  storePos(ps.extends, (*yylocp));
                }
    break;

  case 64:

/* Line 936 of glr.c  */
#line 414 "ned2.y"
    {
                  ps.interfacename = (InterfaceNameElement *)createElementWithTag(NED_INTERFACE_NAME, ps.component);
                  ps.interfacename->setName(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)).c_str());
                  storePos(ps.interfacename, (*yylocp));
                }
    break;

  case 65:

/* Line 936 of glr.c  */
#line 426 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.component);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 66:

/* Line 936 of glr.c  */
#line 435 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 67:

/* Line 936 of glr.c  */
#line 449 "ned2.y"
    {
                  ps.component = (ChannelInterfaceElement *)createElementWithTag(NED_CHANNEL_INTERFACE, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile);
                  ((ChannelInterfaceElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 68:

/* Line 936 of glr.c  */
#line 454 "ned2.y"
    { storeBannerAndRightComments(ps.component,(*yylocp)); }
    break;

  case 73:

/* Line 936 of glr.c  */
#line 472 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.component);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 74:

/* Line 936 of glr.c  */
#line 482 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 75:

/* Line 936 of glr.c  */
#line 496 "ned2.y"
    {
                  ps.component = (SimpleModuleElement *)createElementWithTag(NED_SIMPLE_MODULE, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile );
                  ((SimpleModuleElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 76:

/* Line 936 of glr.c  */
#line 501 "ned2.y"
    { storeBannerAndRightComments(ps.component,(*yylocp)); }
    break;

  case 77:

/* Line 936 of glr.c  */
#line 509 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.component);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 78:

/* Line 936 of glr.c  */
#line 522 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 79:

/* Line 936 of glr.c  */
#line 536 "ned2.y"
    {
                  ps.component = (CompoundModuleElement *)createElementWithTag(NED_COMPOUND_MODULE, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile );
                  ((CompoundModuleElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 80:

/* Line 936 of glr.c  */
#line 541 "ned2.y"
    { storeBannerAndRightComments(ps.component,(*yylocp)); }
    break;

  case 81:

/* Line 936 of glr.c  */
#line 549 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)ps.component->getFirstChildWithTag(NED_PARAMETERS); // networkheader already created it for @isNetwork
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 82:

/* Line 936 of glr.c  */
#line 562 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 83:

/* Line 936 of glr.c  */
#line 576 "ned2.y"
    {
                  ps.component = (CompoundModuleElement *)createElementWithTag(NED_COMPOUND_MODULE, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile );
                  ((CompoundModuleElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 84:

/* Line 936 of glr.c  */
#line 581 "ned2.y"
    {
                  setIsNetworkProperty(ps.component);
                  storeBannerAndRightComments(ps.component,(*yylocp));
                }
    break;

  case 85:

/* Line 936 of glr.c  */
#line 592 "ned2.y"
    {
                  ps.typescope.push(ps.component);
                  ps.blockscope.push(ps.component);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.component);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 86:

/* Line 936 of glr.c  */
#line 602 "ned2.y"
    {
                  ps.propertyscope.pop();
                  ps.blockscope.pop();
                  ps.component = ps.typescope.top();
                  ps.typescope.pop();
                  if (np->getStoreSourceFlag())
                      storeComponentSourceCode(ps.component, (*yylocp));
                  storePos(ps.component, (*yylocp));
                  storeTrailingComment(ps.component,(*yylocp));
                }
    break;

  case 87:

/* Line 936 of glr.c  */
#line 616 "ned2.y"
    {
                  ps.component = (ModuleInterfaceElement *)createElementWithTag(NED_MODULE_INTERFACE, ps.inTypes ? (NEDElement *)ps.types : (NEDElement *)ps.nedfile);
                  ((ModuleInterfaceElement *)ps.component)->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 88:

/* Line 936 of glr.c  */
#line 621 "ned2.y"
    { storeBannerAndRightComments(ps.component,(*yylocp)); }
    break;

  case 89:

/* Line 936 of glr.c  */
#line 629 "ned2.y"
    {
                  storePos(ps.parameters, (*yylocp));
                  if (!ps.parameters->getFirstChild()) { // delete "parameters" element if empty
                      ps.parameters->getParent()->removeChild(ps.parameters);
                      delete ps.parameters;
                  }
                }
    break;

  case 90:

/* Line 936 of glr.c  */
#line 637 "ned2.y"
    {
                  ps.parameters->setIsImplicit(false);
                  storeBannerAndRightComments(ps.parameters,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 91:

/* Line 936 of glr.c  */
#line 642 "ned2.y"
    { storePos(ps.parameters, (*yylocp)); }
    break;

  case 100:

/* Line 936 of glr.c  */
#line 670 "ned2.y"
    {
                  ps.propertyscope.pop();
                  storePos(ps.param, (*yylocp));
                  storeBannerAndRightComments(ps.param,(*yylocp));
                }
    break;

  case 101:

/* Line 936 of glr.c  */
#line 676 "ned2.y"
    {
                  ps.propertyscope.pop();
                  if (!isEmpty(ps.exprPos))  // note: $4 cannot be checked, as it's always NULL when expression parsing is off
                      addExpression(ps.param, "value",ps.exprPos,(((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (6))].yystate.yysemantics.yysval));
                  else {
                      // Note: "=default" is currently not accepted in NED files, because
                      // it would be complicated to support in the Inifile Editor.
                      if (ps.isDefault)
                          np->getErrors()->addError(ps.param,"applying the default value (\"=default\" syntax) is not supported in NED files");
                  }
                  ps.param->setIsDefault(ps.isDefault);
                  storePos(ps.param, (*yylocp));
                  storeBannerAndRightComments(ps.param,(*yylocp));
                }
    break;

  case 102:

/* Line 936 of glr.c  */
#line 694 "ned2.y"
    {
                  ps.param = addParameter(ps.parameters, (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc));
                  ps.param->setType(ps.paramType);
                  ps.param->setIsVolatile(ps.isVolatile);
                  ps.propertyscope.push(ps.param);
                }
    break;

  case 103:

/* Line 936 of glr.c  */
#line 701 "ned2.y"
    {
                  ps.param = addParameter(ps.parameters, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc));
                  ps.propertyscope.push(ps.param);
                }
    break;

  case 104:

/* Line 936 of glr.c  */
#line 709 "ned2.y"
    {
                  ps.param = addParameter(ps.parameters, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yyloc));
                  ps.param->setIsPattern(true);
                  const char *patt = ps.param->getName();
                  if (strchr(patt,' ') || strchr(patt,'\t') || strchr(patt,'\n'))
                      np->getErrors()->addError(ps.param,"parameter name patterns may not contain whitespace");
                  if (!isEmpty(ps.exprPos))  // note: $3 cannot be checked, as it's always NULL when expression parsing is off
                      addExpression(ps.param, "value",ps.exprPos,(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval));
                  else {
                      // Note: "=default" is currently not accepted in NED files, because
                      // it would be complicated to support in the Inifile Editor.
                      if (ps.isDefault)
                          np->getErrors()->addError(ps.param,"applying the default value (\"=default\" syntax) is not supported in NED files");
                  }
                  ps.param->setIsDefault(ps.isDefault);
                  storePos(ps.param, (*yylocp));
                  storeBannerAndRightComments(ps.param,(*yylocp));
                }
    break;

  case 105:

/* Line 936 of glr.c  */
#line 731 "ned2.y"
    { ps.paramType = NED_PARTYPE_DOUBLE; }
    break;

  case 106:

/* Line 936 of glr.c  */
#line 733 "ned2.y"
    { ps.paramType = NED_PARTYPE_INT; }
    break;

  case 107:

/* Line 936 of glr.c  */
#line 735 "ned2.y"
    { ps.paramType = NED_PARTYPE_STRING; }
    break;

  case 108:

/* Line 936 of glr.c  */
#line 737 "ned2.y"
    { ps.paramType = NED_PARTYPE_BOOL; }
    break;

  case 109:

/* Line 936 of glr.c  */
#line 739 "ned2.y"
    { ps.paramType = NED_PARTYPE_XML; }
    break;

  case 110:

/* Line 936 of glr.c  */
#line 744 "ned2.y"
    { ps.isVolatile = true; }
    break;

  case 111:

/* Line 936 of glr.c  */
#line 746 "ned2.y"
    { ps.isVolatile = false; }
    break;

  case 112:

/* Line 936 of glr.c  */
#line 751 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval); ps.exprPos = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc); ps.isDefault = false; }
    break;

  case 113:

/* Line 936 of glr.c  */
#line 753 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval); ps.exprPos = (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yyloc); ps.isDefault = true; }
    break;

  case 114:

/* Line 936 of glr.c  */
#line 755 "ned2.y"
    {
                  ((*yyvalp)) = NULL; ps.exprPos = makeEmptyYYLTYPE(); ps.isDefault = true;
                }
    break;

  case 115:

/* Line 936 of glr.c  */
#line 759 "ned2.y"
    {
                  np->getErrors()->addError(ps.parameters,"interactive prompting (\"=ask\" syntax) is not supported in NED files");
                  ((*yyvalp)) = NULL; ps.exprPos = makeEmptyYYLTYPE(); ps.isDefault = false;
                }
    break;

  case 139:

/* Line 936 of glr.c  */
#line 814 "ned2.y"
    {
                  storePos(ps.property, (*yylocp));
                  storeBannerAndRightComments(ps.property,(*yylocp));
                }
    break;

  case 142:

/* Line 936 of glr.c  */
#line 827 "ned2.y"
    {
                  assertNonEmpty(ps.propertyscope);
                  ps.property = addProperty(ps.propertyscope.top(), toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                  ps.propvals.clear(); // just to be safe
                }
    break;

  case 143:

/* Line 936 of glr.c  */
#line 833 "ned2.y"
    {
                  assertNonEmpty(ps.propertyscope);
                  ps.property = addProperty(ps.propertyscope.top(), toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (5))].yystate.yyloc)));
                  ps.property->setIndex(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (5))].yystate.yyloc)));
                  ps.propvals.clear(); // just to be safe
                }
    break;

  case 147:

/* Line 936 of glr.c  */
#line 852 "ned2.y"
    {
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.property);
                  ps.propkey->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)));
                  for (int i=0; i<(int)ps.propvals.size(); i++)
                      ps.propkey->appendChild(ps.propvals[i]);
                  ps.propvals.clear();
                  storePos(ps.propkey, (*yylocp));
                }
    break;

  case 148:

/* Line 936 of glr.c  */
#line 861 "ned2.y"
    {
                  ps.propkey = (PropertyKeyElement *)createElementWithTag(NED_PROPERTY_KEY, ps.property);
                  ps.propkey->appendChild((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval));
                  for (int i=0; i<(int)ps.propvals.size(); i++)
                      ps.propkey->appendChild(ps.propvals[i]);
                  ps.propvals.clear();
                  storePos(ps.propkey, (*yylocp));
                }
    break;

  case 149:

/* Line 936 of glr.c  */
#line 873 "ned2.y"
    { ps.propvals.push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 150:

/* Line 936 of glr.c  */
#line 875 "ned2.y"
    { ps.propvals.push_back((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval)); }
    break;

  case 151:

/* Line 936 of glr.c  */
#line 880 "ned2.y"
    { ((*yyvalp)) = createLiteral(NED_CONST_SPEC, (*yylocp), (*yylocp)); }
    break;

  case 152:

/* Line 936 of glr.c  */
#line 882 "ned2.y"
    { ((*yyvalp)) = createStringLiteral((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 153:

/* Line 936 of glr.c  */
#line 884 "ned2.y"
    {
                  LiteralElement *node = (LiteralElement *)createElementWithTag(NED_LITERAL);
                  node->setType(NED_CONST_SPEC); // and leave both value and text at ""
                  ((*yyvalp)) = node;
                }
    break;

  case 191:

/* Line 936 of glr.c  */
#line 913 "ned2.y"
    {
                  assertNonEmpty(ps.blockscope);
                  ps.gates = (GatesElement *)createElementWithTag(NED_GATES, ps.blockscope.top());
                  storeBannerAndRightComments(ps.gates,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 192:

/* Line 936 of glr.c  */
#line 919 "ned2.y"
    {
                  storePos(ps.gates, (*yylocp));
                }
    break;

  case 195:

/* Line 936 of glr.c  */
#line 931 "ned2.y"
    {
                  storeBannerAndRightComments(ps.gate,(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 196:

/* Line 936 of glr.c  */
#line 935 "ned2.y"
    {
                  storeBannerAndRightComments(ps.gate,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc));
                }
    break;

  case 197:

/* Line 936 of glr.c  */
#line 945 "ned2.y"
    {
                  ps.propertyscope.push(ps.gate);
                }
    break;

  case 198:

/* Line 936 of glr.c  */
#line 949 "ned2.y"
    {
                  ps.propertyscope.pop();
                  storePos(ps.gate, (*yylocp));
                }
    break;

  case 199:

/* Line 936 of glr.c  */
#line 957 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                  ps.gate->setType(ps.gateType);
                }
    break;

  case 200:

/* Line 936 of glr.c  */
#line 962 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (4))].yystate.yyloc));
                  ps.gate->setType(ps.gateType);
                  ps.gate->setIsVector(true);
                }
    break;

  case 201:

/* Line 936 of glr.c  */
#line 968 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yyloc));
                  ps.gate->setType(ps.gateType);
                  ps.gate->setIsVector(true);
                  addVector(ps.gate, "vector-size",(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                }
    break;

  case 202:

/* Line 936 of glr.c  */
#line 975 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc));
                }
    break;

  case 203:

/* Line 936 of glr.c  */
#line 979 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc));
                  ps.gate->setIsVector(true);
                }
    break;

  case 204:

/* Line 936 of glr.c  */
#line 984 "ned2.y"
    {
                  ps.gate = addGate(ps.gates, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc));
                  ps.gate->setIsVector(true);
                  addVector(ps.gate, "vector-size",(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval));
                }
    break;

  case 205:

/* Line 936 of glr.c  */
#line 993 "ned2.y"
    { ps.gateType = NED_GATETYPE_INPUT; }
    break;

  case 206:

/* Line 936 of glr.c  */
#line 995 "ned2.y"
    { ps.gateType = NED_GATETYPE_OUTPUT; }
    break;

  case 207:

/* Line 936 of glr.c  */
#line 997 "ned2.y"
    { ps.gateType = NED_GATETYPE_INOUT; }
    break;

  case 210:

/* Line 936 of glr.c  */
#line 1010 "ned2.y"
    {
                  assertNonEmpty(ps.blockscope);
                  ps.types = (TypesElement *)createElementWithTag(NED_TYPES, ps.blockscope.top());
                  storeBannerAndRightComments(ps.types,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                  if (ps.inTypes)
                     np->getErrors()->addError(ps.types,"more than one level of type nesting is not allowed");
                  ps.inTypes = true;
                }
    break;

  case 211:

/* Line 936 of glr.c  */
#line 1019 "ned2.y"
    {
                  ps.inTypes = false;
                  storePos(ps.types, (*yylocp));
                }
    break;

  case 226:

/* Line 936 of glr.c  */
#line 1056 "ned2.y"
    {
                  assertNonEmpty(ps.blockscope);
                  ps.submods = (SubmodulesElement *)createElementWithTag(NED_SUBMODULES, ps.blockscope.top());
                  storeBannerAndRightComments(ps.submods,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 227:

/* Line 936 of glr.c  */
#line 1062 "ned2.y"
    {
                  storePos(ps.submods, (*yylocp));
                }
    break;

  case 232:

/* Line 936 of glr.c  */
#line 1079 "ned2.y"
    {
                  storeBannerAndRightComments(ps.submod,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                  storePos(ps.submod, (*yylocp));
                }
    break;

  case 233:

/* Line 936 of glr.c  */
#line 1084 "ned2.y"
    {
                  ps.blockscope.push(ps.submod);
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.submod);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                  storeBannerAndRightComments(ps.submod,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 234:

/* Line 936 of glr.c  */
#line 1094 "ned2.y"
    {
                  ps.blockscope.pop();
                  ps.propertyscope.pop();
                  storePos(ps.submod, (*yylocp));
                  storeTrailingComment(ps.submod,(*yylocp));
                }
    break;

  case 235:

/* Line 936 of glr.c  */
#line 1104 "ned2.y"
    {
                  ps.submod->setType(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc)).c_str());
                }
    break;

  case 236:

/* Line 936 of glr.c  */
#line 1108 "ned2.y"
    {
                  addLikeParam(ps.submod, "like-param", (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (5))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (5))].yystate.yysemantics.yysval));
                  ps.submod->setLikeType(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (5))].yystate.yyloc)).c_str());
                }
    break;

  case 237:

/* Line 936 of glr.c  */
#line 1116 "ned2.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)));
                }
    break;

  case 238:

/* Line 936 of glr.c  */
#line 1121 "ned2.y"
    {
                  ps.submod = (SubmoduleElement *)createElementWithTag(NED_SUBMODULE, ps.submods);
                  ps.submod->setName(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)));
                  addVector(ps.submod, "vector-size",(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval));
                }
    break;

  case 239:

/* Line 936 of glr.c  */
#line 1130 "ned2.y"
    { ((*yyvalp)) = NULL; }
    break;

  case 240:

/* Line 936 of glr.c  */
#line 1132 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval); }
    break;

  case 243:

/* Line 936 of glr.c  */
#line 1145 "ned2.y"
    {
                  assertNonEmpty(ps.blockscope);
                  ps.conns = (ConnectionsElement *)createElementWithTag(NED_CONNECTIONS, ps.blockscope.top());
                  ps.conns->setAllowUnconnected(true);
                  storeBannerAndRightComments(ps.conns,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc));
                }
    break;

  case 244:

/* Line 936 of glr.c  */
#line 1152 "ned2.y"
    {
                  storePos(ps.conns, (*yylocp));
                }
    break;

  case 245:

/* Line 936 of glr.c  */
#line 1156 "ned2.y"
    {
                  assertNonEmpty(ps.blockscope);
                  ps.conns = (ConnectionsElement *)createElementWithTag(NED_CONNECTIONS, ps.blockscope.top());
                  storeBannerAndRightComments(ps.conns,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                }
    break;

  case 246:

/* Line 936 of glr.c  */
#line 1162 "ned2.y"
    {
                  storePos(ps.conns, (*yylocp));
                }
    break;

  case 252:

/* Line 936 of glr.c  */
#line 1180 "ned2.y"
    {
                  ps.chanspec = (ChannelSpecElement *)ps.conn->getFirstChildWithTag(NED_CHANNEL_SPEC);
                  if (ps.chanspec)
                      ps.conn->appendChild(ps.conn->removeChild(ps.chanspec)); // move channelspec to conform DTD
                  if ((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval)) {
                      transferChildren((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval), ps.conn);
                      delete (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval);
                  }
                  storePos(ps.conn, (*yylocp));
                  storeBannerAndRightComments(ps.conn,(*yylocp));
                }
    break;

  case 253:

/* Line 936 of glr.c  */
#line 1195 "ned2.y"
    {
                  if (ps.inConnGroup)
                      np->getErrors()->addError(ps.conngroup,"nested connection groups are not allowed");
                  ps.conngroup = (ConnectionGroupElement *)createElementWithTag(NED_CONNECTION_GROUP, ps.conns);
                  if ((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval)) {
                      // for's and if's were collected in a temporary UnknownElement, put them under conngroup now
                      transferChildren((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval), ps.conngroup);
                      delete (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yysemantics.yysval);
                  }
                  ps.inConnGroup = true;
                  storeBannerAndRightComments(ps.conngroup,(((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc));
                }
    break;

  case 254:

/* Line 936 of glr.c  */
#line 1208 "ned2.y"
    {
                  ps.inConnGroup = false;
                  storePos(ps.conngroup,(*yylocp));
                  storeTrailingComment(ps.conngroup,(*yylocp));
                }
    break;

  case 255:

/* Line 936 of glr.c  */
#line 1217 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval); }
    break;

  case 256:

/* Line 936 of glr.c  */
#line 1219 "ned2.y"
    { ((*yyvalp)) = NULL; }
    break;

  case 257:

/* Line 936 of glr.c  */
#line 1224 "ned2.y"
    {
                  (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval)->appendChild((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                  ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval);
                }
    break;

  case 258:

/* Line 936 of glr.c  */
#line 1229 "ned2.y"
    {
                  ((*yyvalp)) = new UnknownElement();
                  ((*yyvalp))->appendChild((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval));
                }
    break;

  case 261:

/* Line 936 of glr.c  */
#line 1242 "ned2.y"
    {
                  ps.loop = (LoopElement *)createElementWithTag(NED_LOOP);
                  ps.loop->setParamName( toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (6))].yystate.yyloc)) );
                  addExpression(ps.loop, "from-value",(((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (6))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((4) - (6))].yystate.yysemantics.yysval));
                  addExpression(ps.loop, "to-value",(((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (6))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (6))].yystate.yysemantics.yysval));
                  storePos(ps.loop, (*yylocp));
                  ((*yyvalp)) = ps.loop;
                }
    break;

  case 262:

/* Line 936 of glr.c  */
#line 1257 "ned2.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_L2R);
                }
    break;

  case 263:

/* Line 936 of glr.c  */
#line 1261 "ned2.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_L2R);
                }
    break;

  case 264:

/* Line 936 of glr.c  */
#line 1265 "ned2.y"
    {
                  swapConnection(ps.conn);
                  ps.conn->setArrowDirection(NED_ARROWDIR_R2L);
                }
    break;

  case 265:

/* Line 936 of glr.c  */
#line 1270 "ned2.y"
    {
                  swapConnection(ps.conn);
                  ps.conn->setArrowDirection(NED_ARROWDIR_R2L);
                }
    break;

  case 266:

/* Line 936 of glr.c  */
#line 1275 "ned2.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_BIDIR);
                }
    break;

  case 267:

/* Line 936 of glr.c  */
#line 1279 "ned2.y"
    {
                  ps.conn->setArrowDirection(NED_ARROWDIR_BIDIR);
                }
    break;

  case 270:

/* Line 936 of glr.c  */
#line 1291 "ned2.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inConnGroup ? (NEDElement*)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule( toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)) );
                  addVector(ps.conn, "src-module-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval));
                }
    break;

  case 271:

/* Line 936 of glr.c  */
#line 1297 "ned2.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inConnGroup ? (NEDElement*)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule( toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)) );
                }
    break;

  case 272:

/* Line 936 of glr.c  */
#line 1305 "ned2.y"
    {
                  ps.conn->setSrcGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)) );
                  ps.conn->setSrcGateSubg(ps.subgate);
                }
    break;

  case 273:

/* Line 936 of glr.c  */
#line 1310 "ned2.y"
    {
                  ps.conn->setSrcGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setSrcGateSubg(ps.subgate);
                  addVector(ps.conn, "src-gate-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                }
    break;

  case 274:

/* Line 936 of glr.c  */
#line 1316 "ned2.y"
    {
                  ps.conn->setSrcGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setSrcGateSubg(ps.subgate);
                  ps.conn->setSrcGatePlusplus(true);
                }
    break;

  case 275:

/* Line 936 of glr.c  */
#line 1325 "ned2.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inConnGroup ? (NEDElement*)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)));
                  ps.conn->setSrcGateSubg(ps.subgate);
                }
    break;

  case 276:

/* Line 936 of glr.c  */
#line 1332 "ned2.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inConnGroup ? (NEDElement*)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)));
                  ps.conn->setSrcGateSubg(ps.subgate);
                  addVector(ps.conn, "src-gate-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                }
    break;

  case 277:

/* Line 936 of glr.c  */
#line 1340 "ned2.y"
    {
                  ps.conn = (ConnectionElement *)createElementWithTag(NED_CONNECTION, ps.inConnGroup ? (NEDElement*)ps.conngroup : (NEDElement*)ps.conns );
                  ps.conn->setSrcModule("");
                  ps.conn->setSrcGate(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)));
                  ps.conn->setSrcGateSubg(ps.subgate);
                  ps.conn->setSrcGatePlusplus(true);
                }
    break;

  case 280:

/* Line 936 of glr.c  */
#line 1356 "ned2.y"
    {
                  ps.conn->setDestModule( toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)) );
                }
    break;

  case 281:

/* Line 936 of glr.c  */
#line 1360 "ned2.y"
    {
                  ps.conn->setDestModule( toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)) );
                  addVector(ps.conn, "dest-module-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval));
                }
    break;

  case 282:

/* Line 936 of glr.c  */
#line 1368 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                }
    break;

  case 283:

/* Line 936 of glr.c  */
#line 1373 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                  addVector(ps.conn, "dest-gate-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                }
    break;

  case 284:

/* Line 936 of glr.c  */
#line 1379 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                  ps.conn->setDestGatePlusplus(true);
                }
    break;

  case 285:

/* Line 936 of glr.c  */
#line 1388 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (2))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                }
    break;

  case 286:

/* Line 936 of glr.c  */
#line 1393 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                  addVector(ps.conn, "dest-gate-index",(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval));
                }
    break;

  case 287:

/* Line 936 of glr.c  */
#line 1399 "ned2.y"
    {
                  ps.conn->setDestGate( toString( (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)) );
                  ps.conn->setDestGateSubg(ps.subgate);
                  ps.conn->setDestGatePlusplus(true);
                }
    break;

  case 288:

/* Line 936 of glr.c  */
#line 1408 "ned2.y"
    {
                  const char *s = toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc));
                  if (!strcmp(s,"i"))
                      ps.subgate = NED_SUBGATE_I;
                  else if (!strcmp(s,"o"))
                      ps.subgate = NED_SUBGATE_O;
                  else
                       np->getErrors()->addError(currentLocation(), "invalid subgate spec `%s', must be `i' or `o'", toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc)));
                }
    break;

  case 289:

/* Line 936 of glr.c  */
#line 1418 "ned2.y"
    { ps.subgate = NED_SUBGATE_NONE; }
    break;

  case 290:

/* Line 936 of glr.c  */
#line 1423 "ned2.y"
    { storePos(ps.chanspec, (*yylocp)); }
    break;

  case 291:

/* Line 936 of glr.c  */
#line 1425 "ned2.y"
    {
                  ps.parameters = (ParametersElement *)createElementWithTag(NED_PARAMETERS, ps.chanspec);
                  ps.parameters->setIsImplicit(true);
                  ps.propertyscope.push(ps.parameters);
                }
    break;

  case 292:

/* Line 936 of glr.c  */
#line 1432 "ned2.y"
    {
                  ps.propertyscope.pop();
                  storePos(ps.chanspec, (*yylocp));
                }
    break;

  case 293:

/* Line 936 of glr.c  */
#line 1441 "ned2.y"
    {
                  ps.chanspec = (ChannelSpecElement *)createElementWithTag(NED_CHANNEL_SPEC, ps.conn);
                }
    break;

  case 294:

/* Line 936 of glr.c  */
#line 1445 "ned2.y"
    {
                  ps.chanspec = (ChannelSpecElement *)createElementWithTag(NED_CHANNEL_SPEC, ps.conn);
                  ps.chanspec->setType(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)).c_str());
                }
    break;

  case 295:

/* Line 936 of glr.c  */
#line 1450 "ned2.y"
    {
                  ps.chanspec = (ChannelSpecElement *)createElementWithTag(NED_CHANNEL_SPEC, ps.conn);
                  addLikeParam(ps.chanspec, "like-param", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval));
                  ps.chanspec->setLikeType(removeSpaces((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc)).c_str());
                }
    break;

  case 296:

/* Line 936 of glr.c  */
#line 1462 "ned2.y"
    {
                  ps.condition = (ConditionElement *)createElementWithTag(NED_CONDITION);
                  addExpression(ps.condition, "condition",(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yyloc),(((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval));
                  storePos(ps.condition, (*yylocp));
                  ((*yyvalp)) = ps.condition;
                }
    break;

  case 297:

/* Line 936 of glr.c  */
#line 1475 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval); }
    break;

  case 298:

/* Line 936 of glr.c  */
#line 1481 "ned2.y"
    {
                  if (np->getParseExpressionsFlag()) ((*yyvalp)) = createExpression((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval));
                }
    break;

  case 299:

/* Line 936 of glr.c  */
#line 1485 "ned2.y"
    {
                  if (np->getParseExpressionsFlag()) ((*yyvalp)) = createExpression((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yysemantics.yysval));
                }
    break;

  case 300:

/* Line 936 of glr.c  */
#line 1496 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("xmldoc", (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (6))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (6))].yystate.yysemantics.yysval)); }
    break;

  case 301:

/* Line 936 of glr.c  */
#line 1498 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("xmldoc", (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 303:

/* Line 936 of glr.c  */
#line 1504 "ned2.y"
    { ((*yyvalp)) = (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (3))].yystate.yysemantics.yysval); }
    break;

  case 304:

/* Line 936 of glr.c  */
#line 1506 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("const", (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 305:

/* Line 936 of glr.c  */
#line 1509 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("+", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 306:

/* Line 936 of glr.c  */
#line 1511 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("-", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 307:

/* Line 936 of glr.c  */
#line 1513 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("*", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 308:

/* Line 936 of glr.c  */
#line 1515 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("/", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 309:

/* Line 936 of glr.c  */
#line 1517 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("%", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 310:

/* Line 936 of glr.c  */
#line 1519 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("^", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 311:

/* Line 936 of glr.c  */
#line 1523 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = unaryMinus((((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval)); }
    break;

  case 312:

/* Line 936 of glr.c  */
#line 1526 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("==", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 313:

/* Line 936 of glr.c  */
#line 1528 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("!=", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 314:

/* Line 936 of glr.c  */
#line 1530 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator(">", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 315:

/* Line 936 of glr.c  */
#line 1532 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator(">=", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 316:

/* Line 936 of glr.c  */
#line 1534 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("<", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 317:

/* Line 936 of glr.c  */
#line 1536 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("<=", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 318:

/* Line 936 of glr.c  */
#line 1539 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("&&", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 319:

/* Line 936 of glr.c  */
#line 1541 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("||", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 320:

/* Line 936 of glr.c  */
#line 1543 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("##", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 321:

/* Line 936 of glr.c  */
#line 1547 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("!", (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval)); }
    break;

  case 322:

/* Line 936 of glr.c  */
#line 1550 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("&", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 323:

/* Line 936 of glr.c  */
#line 1552 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("|", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 324:

/* Line 936 of glr.c  */
#line 1554 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("#", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 325:

/* Line 936 of glr.c  */
#line 1558 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("~", (((yyGLRStackItem const *)yyvsp)[YYFILL ((2) - (2))].yystate.yysemantics.yysval)); }
    break;

  case 326:

/* Line 936 of glr.c  */
#line 1560 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("<<", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 327:

/* Line 936 of glr.c  */
#line 1562 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator(">>", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yysemantics.yysval)); }
    break;

  case 328:

/* Line 936 of glr.c  */
#line 1564 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createOperator("?:", (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (5))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (5))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (5))].yystate.yysemantics.yysval)); }
    break;

  case 329:

/* Line 936 of glr.c  */
#line 1567 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 330:

/* Line 936 of glr.c  */
#line 1569 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 331:

/* Line 936 of glr.c  */
#line 1571 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 332:

/* Line 936 of glr.c  */
#line 1574 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc))); }
    break;

  case 333:

/* Line 936 of glr.c  */
#line 1576 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (4))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 334:

/* Line 936 of glr.c  */
#line 1578 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (6))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (6))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (6))].yystate.yysemantics.yysval)); }
    break;

  case 335:

/* Line 936 of glr.c  */
#line 1580 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (8))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (8))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (8))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (8))].yystate.yysemantics.yysval)); }
    break;

  case 336:

/* Line 936 of glr.c  */
#line 1582 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (10))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (10))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (10))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (10))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (10))].yystate.yysemantics.yysval)); }
    break;

  case 337:

/* Line 936 of glr.c  */
#line 1584 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (12))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (12))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (12))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (12))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (12))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (12))].yystate.yysemantics.yysval)); }
    break;

  case 338:

/* Line 936 of glr.c  */
#line 1586 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (14))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (14))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (14))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (14))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (14))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (14))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((13) - (14))].yystate.yysemantics.yysval)); }
    break;

  case 339:

/* Line 936 of glr.c  */
#line 1588 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (16))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((13) - (16))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((15) - (16))].yystate.yysemantics.yysval)); }
    break;

  case 340:

/* Line 936 of glr.c  */
#line 1590 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (18))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((13) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((15) - (18))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((17) - (18))].yystate.yysemantics.yysval)); }
    break;

  case 341:

/* Line 936 of glr.c  */
#line 1592 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (20))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((13) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((15) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((17) - (20))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((19) - (20))].yystate.yysemantics.yysval)); }
    break;

  case 342:

/* Line 936 of glr.c  */
#line 1594 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction(toString((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (22))].yystate.yyloc)), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((5) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((7) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((9) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((11) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((13) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((15) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((17) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((19) - (22))].yystate.yysemantics.yysval), (((yyGLRStackItem const *)yyvsp)[YYFILL ((21) - (22))].yystate.yysemantics.yysval)); }
    break;

  case 346:

/* Line 936 of glr.c  */
#line 1606 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createIdent((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 347:

/* Line 936 of glr.c  */
#line 1608 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createIdent((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)); }
    break;

  case 348:

/* Line 936 of glr.c  */
#line 1610 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createIdent((((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (3))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (3))].yystate.yyloc)); }
    break;

  case 349:

/* Line 936 of glr.c  */
#line 1612 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createIdent((((yyGLRStackItem const *)yyvsp)[YYFILL ((6) - (6))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (6))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (6))].yystate.yysemantics.yysval)); }
    break;

  case 350:

/* Line 936 of glr.c  */
#line 1617 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("index"); }
    break;

  case 351:

/* Line 936 of glr.c  */
#line 1619 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("index"); }
    break;

  case 352:

/* Line 936 of glr.c  */
#line 1621 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createFunction("sizeof", (((yyGLRStackItem const *)yyvsp)[YYFILL ((3) - (4))].yystate.yysemantics.yysval)); }
    break;

  case 356:

/* Line 936 of glr.c  */
#line 1632 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createStringLiteral((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 357:

/* Line 936 of glr.c  */
#line 1637 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createLiteral(NED_CONST_BOOL, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 358:

/* Line 936 of glr.c  */
#line 1639 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createLiteral(NED_CONST_BOOL, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 359:

/* Line 936 of glr.c  */
#line 1644 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createLiteral(NED_CONST_INT, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 360:

/* Line 936 of glr.c  */
#line 1646 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createLiteral(NED_CONST_DOUBLE, (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc), (((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;

  case 361:

/* Line 936 of glr.c  */
#line 1648 "ned2.y"
    { if (np->getParseExpressionsFlag()) ((*yyvalp)) = createQuantityLiteral((((yyGLRStackItem const *)yyvsp)[YYFILL ((1) - (1))].yystate.yyloc)); }
    break;



/* Line 936 of glr.c  */
#line 3896 "ned2.tab.cc"
      default: break;
    }

  return yyok;
# undef yyerrok
# undef YYABORT
# undef YYACCEPT
# undef YYERROR
# undef YYBACKUP
# undef yyclearin
# undef YYRECOVERING
}


/*ARGSUSED*/ static void
yyuserMerge (int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YYUSE (yy0);
  YYUSE (yy1);

  switch (yyn)
    {
      
      default: break;
    }
}

			      /* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
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

/** Number of symbols composing the right hand side of rule #RULE.  */
static inline int
yyrhsLength (yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void
yydestroyGLRState (char const *yymsg, yyGLRState *yys)
{
  if (yys->yyresolved)
    yydestruct (yymsg, yystos[yys->yylrState],
		&yys->yysemantics.yysval, &yys->yyloc);
  else
    {
#if YYDEBUG
      if (yydebug)
	{
	  if (yys->yysemantics.yyfirstVal)
	    YYFPRINTF (stderr, "%s unresolved ", yymsg);
	  else
	    YYFPRINTF (stderr, "%s incomplete ", yymsg);
	  yy_symbol_print (stderr, yystos[yys->yylrState],
			   NULL, &yys->yyloc);
	  YYFPRINTF (stderr, "\n");
	}
#endif

      if (yys->yysemantics.yyfirstVal)
	{
	  yySemanticOption *yyoption = yys->yysemantics.yyfirstVal;
	  yyGLRState *yyrh;
	  int yyn;
	  for (yyrh = yyoption->yystate, yyn = yyrhsLength (yyoption->yyrule);
	       yyn > 0;
	       yyrh = yyrh->yypred, yyn -= 1)
	    yydestroyGLRState (yymsg, yyrh);
	}
    }
}

/** Left-hand-side symbol for rule #RULE.  */
static inline yySymbol
yylhsNonterm (yyRuleNum yyrule)
{
  return yyr1[yyrule];
}

#define yyis_pact_ninf(yystate) \
  ((yystate) == YYPACT_NINF)

/** True iff LR state STATE has only a default reduction (regardless
 *  of token).  */
static inline yybool
yyisDefaultedState (yyStateNum yystate)
{
  return yyis_pact_ninf (yypact[yystate]);
}

/** The default reduction for STATE, assuming it has one.  */
static inline yyRuleNum
yydefaultAction (yyStateNum yystate)
{
  return yydefact[yystate];
}

#define yyis_table_ninf(yytable_value) \
  YYID (0)

/** Set *YYACTION to the action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *CONFLICTS to a pointer into yyconfl to 0-terminated list of
 *  conflicting reductions.
 */
static inline void
yygetLRActions (yyStateNum yystate, int yytoken,
		int* yyaction, const short int** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
    {
      *yyaction = -yydefact[yystate];
      *yyconflicts = yyconfl;
    }
  else if (! yyis_table_ninf (yytable[yyindex]))
    {
      *yyaction = yytable[yyindex];
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
  else
    {
      *yyaction = 0;
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
}

static inline yyStateNum
yyLRgotoState (yyStateNum yystate, yySymbol yylhs)
{
  int yyr;
  yyr = yypgoto[yylhs - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yylhs - YYNTOKENS];
}

static inline yybool
yyisShiftAction (int yyaction)
{
  return 0 < yyaction;
}

static inline yybool
yyisErrorAction (int yyaction)
{
  return yyaction == 0;
}

				/* GLRStates */

/** Return a fresh GLRStackItem.  Callers should call
 * YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 * headroom.  */

static inline yyGLRStackItem*
yynewGLRStackItem (yyGLRStack* yystackp, yybool yyisState)
{
  yyGLRStackItem* yynewItem = yystackp->yynextFree;
  yystackp->yyspaceLeft -= 1;
  yystackp->yynextFree += 1;
  yynewItem->yystate.yyisState = yyisState;
  return yynewItem;
}

/** Add a new semantic action that will execute the action for rule
 *  RULENUM on the semantic values in RHS to the list of
 *  alternative actions for STATE.  Assumes that RHS comes from
 *  stack #K of *STACKP. */
static void
yyaddDeferredAction (yyGLRStack* yystackp, size_t yyk, yyGLRState* yystate,
		     yyGLRState* rhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption =
    &yynewGLRStackItem (yystackp, yyfalse)->yyoption;
  yynewOption->yystate = rhs;
  yynewOption->yyrule = yyrule;
  if (yystackp->yytops.yylookaheadNeeds[yyk])
    {
      yynewOption->yyrawchar = yychar;
      yynewOption->yyval = yylval;
      yynewOption->yyloc = yylloc;
    }
  else
    yynewOption->yyrawchar = YYEMPTY;
  yynewOption->yynext = yystate->yysemantics.yyfirstVal;
  yystate->yysemantics.yyfirstVal = yynewOption;

  YY_RESERVE_GLRSTACK (yystackp);
}

				/* GLRStacks */

/** Initialize SET to a singleton set containing an empty stack.  */
static yybool
yyinitStateSet (yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates = (yyGLRState**) YYMALLOC (16 * sizeof yyset->yystates[0]);
  if (! yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = NULL;
  yyset->yylookaheadNeeds =
    (yybool*) YYMALLOC (16 * sizeof yyset->yylookaheadNeeds[0]);
  if (! yyset->yylookaheadNeeds)
    {
      YYFREE (yyset->yystates);
      return yyfalse;
    }
  return yytrue;
}

static void yyfreeStateSet (yyGLRStateSet* yyset)
{
  YYFREE (yyset->yystates);
  YYFREE (yyset->yylookaheadNeeds);
}

/** Initialize STACK to a single empty stack, with total maximum
 *  capacity for all stacks of SIZE.  */
static yybool
yyinitGLRStack (yyGLRStack* yystackp, size_t yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems =
    (yyGLRStackItem*) YYMALLOC (yysize * sizeof yystackp->yynextFree[0]);
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = NULL;
  yystackp->yylastDeleted = NULL;
  return yyinitStateSet (&yystackp->yytops);
}


#if YYSTACKEXPANDABLE
# define YYRELOC(YYFROMITEMS,YYTOITEMS,YYX,YYTYPE) \
  &((YYTOITEMS) - ((YYFROMITEMS) - (yyGLRStackItem*) (YYX)))->YYTYPE

/** If STACK is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void
yyexpandGLRStack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem* yyp0, *yyp1;
  size_t yysize, yynewSize;
  size_t yyn;
  yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted (yystackp);
  yynewSize = 2*yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems = (yyGLRStackItem*) YYMALLOC (yynewSize * sizeof yynewItems[0]);
  if (! yynewItems)
    yyMemoryExhausted (yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize;
       0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
    {
      *yyp1 = *yyp0;
      if (*(yybool *) yyp0)
	{
	  yyGLRState* yys0 = &yyp0->yystate;
	  yyGLRState* yys1 = &yyp1->yystate;
	  if (yys0->yypred != NULL)
	    yys1->yypred =
	      YYRELOC (yyp0, yyp1, yys0->yypred, yystate);
	  if (! yys0->yyresolved && yys0->yysemantics.yyfirstVal != NULL)
	    yys1->yysemantics.yyfirstVal =
	      YYRELOC(yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
	}
      else
	{
	  yySemanticOption* yyv0 = &yyp0->yyoption;
	  yySemanticOption* yyv1 = &yyp1->yyoption;
	  if (yyv0->yystate != NULL)
	    yyv1->yystate = YYRELOC (yyp0, yyp1, yyv0->yystate, yystate);
	  if (yyv0->yynext != NULL)
	    yyv1->yynext = YYRELOC (yyp0, yyp1, yyv0->yynext, yyoption);
	}
    }
  if (yystackp->yysplitPoint != NULL)
    yystackp->yysplitPoint = YYRELOC (yystackp->yyitems, yynewItems,
				 yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != NULL)
      yystackp->yytops.yystates[yyn] =
	YYRELOC (yystackp->yyitems, yynewItems,
		 yystackp->yytops.yystates[yyn], yystate);
  YYFREE (yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void
yyfreeGLRStack (yyGLRStack* yystackp)
{
  YYFREE (yystackp->yyitems);
  yyfreeStateSet (&yystackp->yytops);
}

/** Assuming that S is a GLRState somewhere on STACK, update the
 *  splitpoint of STACK, if needed, so that it is at least as deep as
 *  S.  */
static inline void
yyupdateSplit (yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != NULL && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #K in STACK.  */
static inline void
yymarkStackDeleted (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yytops.yystates[yyk] != NULL)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = NULL;
}

/** Undelete the last stack that was marked as deleted.  Can only be
    done once after a deletion, and only when all other stacks have
    been deleted.  */
static void
yyundeleteLastStack (yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == NULL || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YYDPRINTF ((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = NULL;
}

static inline void
yyremoveDeletes (yyGLRStack* yystackp)
{
  size_t yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
    {
      if (yystackp->yytops.yystates[yyi] == NULL)
	{
	  if (yyi == yyj)
	    {
	      YYDPRINTF ((stderr, "Removing dead stacks.\n"));
	    }
	  yystackp->yytops.yysize -= 1;
	}
      else
	{
	  yystackp->yytops.yystates[yyj] = yystackp->yytops.yystates[yyi];
	  /* In the current implementation, it's unnecessary to copy
	     yystackp->yytops.yylookaheadNeeds[yyi] since, after
	     yyremoveDeletes returns, the parser immediately either enters
	     deterministic operation or shifts a token.  However, it doesn't
	     hurt, and the code might evolve to need it.  */
	  yystackp->yytops.yylookaheadNeeds[yyj] =
	    yystackp->yytops.yylookaheadNeeds[yyi];
	  if (yyj != yyi)
	    {
	      YYDPRINTF ((stderr, "Rename stack %lu -> %lu.\n",
			  (unsigned long int) yyi, (unsigned long int) yyj));
	    }
	  yyj += 1;
	}
      yyi += 1;
    }
}

/** Shift to a new state on stack #K of STACK, corresponding to LR state
 * LRSTATE, at input position POSN, with (resolved) semantic value SVAL.  */
static inline void
yyglrShift (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
	    size_t yyposn,
	    YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yysval = *yyvalp;
  yynewState->yyloc = *yylocp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK (yystackp);
}

/** Shift stack #K of YYSTACK, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static inline void
yyglrShiftDefer (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
		 size_t yyposn, yyGLRState* rhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = NULL;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction (yystackp, yyk, yynewState, rhs, yyrule);
}

/** Pop the symbols consumed by reduction #RULE from the top of stack
 *  #K of STACK, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *VALP to the resulting value,
 *  and *LOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static inline YYRESULTTAG
yydoAction (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
	    YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  int yynrhs = yyrhsLength (yyrule);

  if (yystackp->yysplitPoint == NULL)
    {
      /* Standard special case: single stack.  */
      yyGLRStackItem* rhs = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
      YYASSERT (yyk == 0);
      yystackp->yynextFree -= yynrhs;
      yystackp->yyspaceLeft += yynrhs;
      yystackp->yytops.yystates[0] = & yystackp->yynextFree[-1].yystate;
      return yyuserAction (yyrule, yynrhs, rhs,
			   yyvalp, yylocp, yystackp);
    }
  else
    {
      /* At present, doAction is never called in nondeterministic
       * mode, so this branch is never taken.  It is here in
       * anticipation of a future feature that will allow immediate
       * evaluation of selected actions in nondeterministic mode.  */
      int yyi;
      yyGLRState* yys;
      yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
      yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred
	= yystackp->yytops.yystates[yyk];
      if (yynrhs == 0)
	/* Set default location.  */
	yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yys->yyloc;
      for (yyi = 0; yyi < yynrhs; yyi += 1)
	{
	  yys = yys->yypred;
	  YYASSERT (yys);
	}
      yyupdateSplit (yystackp, yys);
      yystackp->yytops.yystates[yyk] = yys;
      return yyuserAction (yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
			   yyvalp, yylocp, yystackp);
    }
}

#if !YYDEBUG
# define YY_REDUCE_PRINT(Args)
#else
# define YY_REDUCE_PRINT(Args)		\
do {					\
  if (yydebug)				\
    yy_reduce_print Args;		\
} while (YYID (0))

/*----------------------------------------------------------.
| Report that the RULE is going to be reduced on stack #K.  |
`----------------------------------------------------------*/

/*ARGSUSED*/ static inline void
yy_reduce_print (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
		 YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  int yynrhs = yyrhsLength (yyrule);
  yybool yynormal __attribute__ ((__unused__)) =
    (yystackp->yysplitPoint == NULL);
  yyGLRStackItem* yyvsp = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
  int yylow = 1;
  int yyi;
  YYUSE (yyvalp);
  YYUSE (yylocp);
  YYFPRINTF (stderr, "Reducing stack %lu by rule %d (line %lu):\n",
	     (unsigned long int) yyk, yyrule - 1,
	     (unsigned long int) yyrline[yyrule]);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(((yyGLRStackItem const *)yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yysemantics.yysval)
		       , &(((yyGLRStackItem const *)yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yyloc)		       );
      YYFPRINTF (stderr, "\n");
    }
}
#endif

/** Pop items off stack #K of STACK according to grammar rule RULE,
 *  and push back on the resulting nonterminal symbol.  Perform the
 *  semantic action associated with RULE and store its value with the
 *  newly pushed state, if FORCEEVAL or if STACK is currently
 *  unambiguous.  Otherwise, store the deferred semantic action with
 *  the new state.  If the new state would have an identical input
 *  position, LR state, and predecessor to an existing state on the stack,
 *  it is identified with that existing state, eliminating stack #K from
 *  the STACK.  In this case, the (necessarily deferred) semantic value is
 *  added to the options for the existing state's semantic value.
 */
static inline YYRESULTTAG
yyglrReduce (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
	     yybool yyforceEval)
{
  size_t yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == NULL)
    {
      YYSTYPE yysval;
      YYLTYPE yyloc;

      YY_REDUCE_PRINT ((yystackp, yyk, yyrule, &yysval, &yyloc));
      YYCHK (yydoAction (yystackp, yyk, yyrule, &yysval,
			 &yyloc));
      YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyrule], &yysval, &yyloc);
      yyglrShift (yystackp, yyk,
		  yyLRgotoState (yystackp->yytops.yystates[yyk]->yylrState,
				 yylhsNonterm (yyrule)),
		  yyposn, &yysval, &yyloc);
    }
  else
    {
      size_t yyi;
      int yyn;
      yyGLRState* yys, *yys0 = yystackp->yytops.yystates[yyk];
      yyStateNum yynewLRState;

      for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength (yyrule);
	   0 < yyn; yyn -= 1)
	{
	  yys = yys->yypred;
	  YYASSERT (yys);
	}
      yyupdateSplit (yystackp, yys);
      yynewLRState = yyLRgotoState (yys->yylrState, yylhsNonterm (yyrule));
      YYDPRINTF ((stderr,
		  "Reduced stack %lu by rule #%d; action deferred.  Now in state %d.\n",
		  (unsigned long int) yyk, yyrule - 1, yynewLRState));
      for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
	if (yyi != yyk && yystackp->yytops.yystates[yyi] != NULL)
	  {
	    yyGLRState* yyp, *yysplit = yystackp->yysplitPoint;
	    yyp = yystackp->yytops.yystates[yyi];
	    while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
	      {
		if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
		  {
		    yyaddDeferredAction (yystackp, yyk, yyp, yys0, yyrule);
		    yymarkStackDeleted (yystackp, yyk);
		    YYDPRINTF ((stderr, "Merging stack %lu into stack %lu.\n",
				(unsigned long int) yyk,
				(unsigned long int) yyi));
		    return yyok;
		  }
		yyp = yyp->yypred;
	      }
	  }
      yystackp->yytops.yystates[yyk] = yys;
      yyglrShiftDefer (yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
    }
  return yyok;
}

static size_t
yysplitStack (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yysplitPoint == NULL)
    {
      YYASSERT (yyk == 0);
      yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
    }
  if (yystackp->yytops.yysize >= yystackp->yytops.yycapacity)
    {
      yyGLRState** yynewStates;
      yybool* yynewLookaheadNeeds;

      yynewStates = NULL;

      if (yystackp->yytops.yycapacity
	  > (YYSIZEMAX / (2 * sizeof yynewStates[0])))
	yyMemoryExhausted (yystackp);
      yystackp->yytops.yycapacity *= 2;

      yynewStates =
	(yyGLRState**) YYREALLOC (yystackp->yytops.yystates,
				  (yystackp->yytops.yycapacity
				   * sizeof yynewStates[0]));
      if (yynewStates == NULL)
	yyMemoryExhausted (yystackp);
      yystackp->yytops.yystates = yynewStates;

      yynewLookaheadNeeds =
	(yybool*) YYREALLOC (yystackp->yytops.yylookaheadNeeds,
			     (yystackp->yytops.yycapacity
			      * sizeof yynewLookaheadNeeds[0]));
      if (yynewLookaheadNeeds == NULL)
	yyMemoryExhausted (yystackp);
      yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
    }
  yystackp->yytops.yystates[yystackp->yytops.yysize]
    = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize]
    = yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize-1;
}

/** True iff Y0 and Y1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool
yyidenticalOptions (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
    {
      yyGLRState *yys0, *yys1;
      int yyn;
      for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
	   yyn = yyrhsLength (yyy0->yyrule);
	   yyn > 0;
	   yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
	if (yys0->yyposn != yys1->yyposn)
	  return yyfalse;
      return yytrue;
    }
  else
    return yyfalse;
}

/** Assuming identicalOptions (Y0,Y1), destructively merge the
 *  alternative semantic values for the RHS-symbols of Y1 and Y0.  */
static void
yymergeOptionSets (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
       yyn = yyrhsLength (yyy0->yyrule);
       yyn > 0;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
    {
      if (yys0 == yys1)
	break;
      else if (yys0->yyresolved)
	{
	  yys1->yyresolved = yytrue;
	  yys1->yysemantics.yysval = yys0->yysemantics.yysval;
	}
      else if (yys1->yyresolved)
	{
	  yys0->yyresolved = yytrue;
	  yys0->yysemantics.yysval = yys1->yysemantics.yysval;
	}
      else
	{
	  yySemanticOption** yyz0p;
	  yySemanticOption* yyz1;
	  yyz0p = &yys0->yysemantics.yyfirstVal;
	  yyz1 = yys1->yysemantics.yyfirstVal;
	  while (YYID (yytrue))
	    {
	      if (yyz1 == *yyz0p || yyz1 == NULL)
		break;
	      else if (*yyz0p == NULL)
		{
		  *yyz0p = yyz1;
		  break;
		}
	      else if (*yyz0p < yyz1)
		{
		  yySemanticOption* yyz = *yyz0p;
		  *yyz0p = yyz1;
		  yyz1 = yyz1->yynext;
		  (*yyz0p)->yynext = yyz;
		}
	      yyz0p = &(*yyz0p)->yynext;
	    }
	  yys1->yysemantics.yyfirstVal = yys0->yysemantics.yyfirstVal;
	}
    }
}

/** Y0 and Y1 represent two possible actions to take in a given
 *  parsing state; return 0 if no combination is possible,
 *  1 if user-mergeable, 2 if Y0 is preferred, 3 if Y1 is preferred.  */
static int
yypreference (yySemanticOption* y0, yySemanticOption* y1)
{
  yyRuleNum r0 = y0->yyrule, r1 = y1->yyrule;
  int p0 = yydprec[r0], p1 = yydprec[r1];

  if (p0 == p1)
    {
      if (yymerger[r0] == 0 || yymerger[r0] != yymerger[r1])
	return 0;
      else
	return 1;
    }
  if (p0 == 0 || p1 == 0)
    return 0;
  if (p0 < p1)
    return 3;
  if (p1 < p0)
    return 2;
  return 0;
}

static YYRESULTTAG yyresolveValue (yyGLRState* yys,
				   yyGLRStack* yystackp);


/** Resolve the previous N states starting at and including state S.  If result
 *  != yyok, some states may have been left unresolved possibly with empty
 *  semantic option chains.  Regardless of whether result = yyok, each state
 *  has been left with consistent data so that yydestroyGLRState can be invoked
 *  if necessary.  */
static YYRESULTTAG
yyresolveStates (yyGLRState* yys, int yyn,
		 yyGLRStack* yystackp)
{
  if (0 < yyn)
    {
      YYASSERT (yys->yypred);
      YYCHK (yyresolveStates (yys->yypred, yyn-1, yystackp));
      if (! yys->yyresolved)
	YYCHK (yyresolveValue (yys, yystackp));
    }
  return yyok;
}

/** Resolve the states for the RHS of OPT, perform its user action, and return
 *  the semantic value and location.  Regardless of whether result = yyok, all
 *  RHS states have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG
yyresolveAction (yySemanticOption* yyopt, yyGLRStack* yystackp,
		 YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs;
  int yychar_current;
  YYSTYPE yylval_current;
  YYLTYPE yylloc_current;
  YYRESULTTAG yyflag;

  yynrhs = yyrhsLength (yyopt->yyrule);
  yyflag = yyresolveStates (yyopt->yystate, yynrhs, yystackp);
  if (yyflag != yyok)
    {
      yyGLRState *yys;
      for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
	yydestroyGLRState ("Cleanup: popping", yys);
      return yyflag;
    }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  if (yynrhs == 0)
    /* Set default location.  */
    yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yyopt->yystate->yyloc;
  yychar_current = yychar;
  yylval_current = yylval;
  yylloc_current = yylloc;
  yychar = yyopt->yyrawchar;
  yylval = yyopt->yyval;
  yylloc = yyopt->yyloc;
  yyflag = yyuserAction (yyopt->yyrule, yynrhs,
			   yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
			   yyvalp, yylocp, yystackp);
  yychar = yychar_current;
  yylval = yylval_current;
  yylloc = yylloc_current;
  return yyflag;
}

#if YYDEBUG
static void
yyreportTree (yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength (yyx->yyrule);
  int yyi;
  yyGLRState* yys;
  yyGLRState* yystates[1 + YYMAXRHS];
  yyGLRState yyleftmost_state;

  for (yyi = yynrhs, yys = yyx->yystate; 0 < yyi; yyi -= 1, yys = yys->yypred)
    yystates[yyi] = yys;
  if (yys == NULL)
    {
      yyleftmost_state.yyposn = 0;
      yystates[0] = &yyleftmost_state;
    }
  else
    yystates[0] = yys;

  if (yyx->yystate->yyposn < yys->yyposn + 1)
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, empty>\n",
	       yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
	       yyx->yyrule - 1);
  else
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, tokens %lu .. %lu>\n",
	       yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
	       yyx->yyrule - 1, (unsigned long int) (yys->yyposn + 1),
	       (unsigned long int) yyx->yystate->yyposn);
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
    {
      if (yystates[yyi]->yyresolved)
	{
	  if (yystates[yyi-1]->yyposn+1 > yystates[yyi]->yyposn)
	    YYFPRINTF (stderr, "%*s%s <empty>\n", yyindent+2, "",
		       yytokenName (yyrhs[yyprhs[yyx->yyrule]+yyi-1]));
	  else
	    YYFPRINTF (stderr, "%*s%s <tokens %lu .. %lu>\n", yyindent+2, "",
		       yytokenName (yyrhs[yyprhs[yyx->yyrule]+yyi-1]),
		       (unsigned long int) (yystates[yyi - 1]->yyposn + 1),
		       (unsigned long int) yystates[yyi]->yyposn);
	}
      else
	yyreportTree (yystates[yyi]->yysemantics.yyfirstVal, yyindent+2);
    }
}
#endif

/*ARGSUSED*/ static YYRESULTTAG
yyreportAmbiguity (yySemanticOption* yyx0,
		   yySemanticOption* yyx1)
{
  YYUSE (yyx0);
  YYUSE (yyx1);

#if YYDEBUG
  YYFPRINTF (stderr, "Ambiguity detected.\n");
  YYFPRINTF (stderr, "Option 1,\n");
  yyreportTree (yyx0, 2);
  YYFPRINTF (stderr, "\nOption 2,\n");
  yyreportTree (yyx1, 2);
  YYFPRINTF (stderr, "\n");
#endif

  yyerror (YY_("syntax is ambiguous"));
  return yyabort;
}

/** Starting at and including state S1, resolve the location for each of the
 *  previous N1 states that is unresolved.  The first semantic option of a state
 *  is always chosen.  */
static void
yyresolveLocations (yyGLRState* yys1, int yyn1,
		    yyGLRStack *yystackp)
{
  if (0 < yyn1)
    {
      yyresolveLocations (yys1->yypred, yyn1 - 1, yystackp);
      if (!yys1->yyresolved)
	{
	  yySemanticOption *yyoption;
	  yyGLRStackItem yyrhsloc[1 + YYMAXRHS];
	  int yynrhs;
	  int yychar_current;
	  YYSTYPE yylval_current;
	  YYLTYPE yylloc_current;
	  yyoption = yys1->yysemantics.yyfirstVal;
	  YYASSERT (yyoption != NULL);
	  yynrhs = yyrhsLength (yyoption->yyrule);
	  if (yynrhs > 0)
	    {
	      yyGLRState *yys;
	      int yyn;
	      yyresolveLocations (yyoption->yystate, yynrhs,
				  yystackp);
	      for (yys = yyoption->yystate, yyn = yynrhs;
		   yyn > 0;
		   yys = yys->yypred, yyn -= 1)
		yyrhsloc[yyn].yystate.yyloc = yys->yyloc;
	    }
	  else
	    {
	      /* Both yyresolveAction and yyresolveLocations traverse the GSS
		 in reverse rightmost order.  It is only necessary to invoke
		 yyresolveLocations on a subforest for which yyresolveAction
		 would have been invoked next had an ambiguity not been
		 detected.  Thus the location of the previous state (but not
		 necessarily the previous state itself) is guaranteed to be
		 resolved already.  */
	      yyGLRState *yyprevious = yyoption->yystate;
	      yyrhsloc[0].yystate.yyloc = yyprevious->yyloc;
	    }
	  yychar_current = yychar;
	  yylval_current = yylval;
	  yylloc_current = yylloc;
	  yychar = yyoption->yyrawchar;
	  yylval = yyoption->yyval;
	  yylloc = yyoption->yyloc;
	  YYLLOC_DEFAULT ((yys1->yyloc), yyrhsloc, yynrhs);
	  yychar = yychar_current;
	  yylval = yylval_current;
	  yylloc = yylloc_current;
	}
    }
}

/** Resolve the ambiguity represented in state S, perform the indicated
 *  actions, and set the semantic value of S.  If result != yyok, the chain of
 *  semantic options in S has been cleared instead or it has been left
 *  unmodified except that redundant options may have been removed.  Regardless
 *  of whether result = yyok, S has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest;
  yySemanticOption** yypp;
  yybool yymerge;
  YYSTYPE yysval;
  YYRESULTTAG yyflag;
  YYLTYPE *yylocp = &yys->yyloc;

  yybest = yyoptionList;
  yymerge = yyfalse;
  for (yypp = &yyoptionList->yynext; *yypp != NULL; )
    {
      yySemanticOption* yyp = *yypp;

      if (yyidenticalOptions (yybest, yyp))
	{
	  yymergeOptionSets (yybest, yyp);
	  *yypp = yyp->yynext;
	}
      else
	{
	  switch (yypreference (yybest, yyp))
	    {
	    case 0:
	      yyresolveLocations (yys, 1, yystackp);
	      return yyreportAmbiguity (yybest, yyp);
	      break;
	    case 1:
	      yymerge = yytrue;
	      break;
	    case 2:
	      break;
	    case 3:
	      yybest = yyp;
	      yymerge = yyfalse;
	      break;
	    default:
	      /* This cannot happen so it is not worth a YYASSERT (yyfalse),
		 but some compilers complain if the default case is
		 omitted.  */
	      break;
	    }
	  yypp = &yyp->yynext;
	}
    }

  if (yymerge)
    {
      yySemanticOption* yyp;
      int yyprec = yydprec[yybest->yyrule];
      yyflag = yyresolveAction (yybest, yystackp, &yysval,
				yylocp);
      if (yyflag == yyok)
	for (yyp = yybest->yynext; yyp != NULL; yyp = yyp->yynext)
	  {
	    if (yyprec == yydprec[yyp->yyrule])
	      {
		YYSTYPE yysval_other;
		YYLTYPE yydummy;
		yyflag = yyresolveAction (yyp, yystackp, &yysval_other,
					  &yydummy);
		if (yyflag != yyok)
		  {
		    yydestruct ("Cleanup: discarding incompletely merged value for",
				yystos[yys->yylrState],
				&yysval, yylocp);
		    break;
		  }
		yyuserMerge (yymerger[yyp->yyrule], &yysval, &yysval_other);
	      }
	  }
    }
  else
    yyflag = yyresolveAction (yybest, yystackp, &yysval, yylocp);

  if (yyflag == yyok)
    {
      yys->yyresolved = yytrue;
      yys->yysemantics.yysval = yysval;
    }
  else
    yys->yysemantics.yyfirstVal = NULL;
  return yyflag;
}

static YYRESULTTAG
yyresolveStack (yyGLRStack* yystackp)
{
  if (yystackp->yysplitPoint != NULL)
    {
      yyGLRState* yys;
      int yyn;

      for (yyn = 0, yys = yystackp->yytops.yystates[0];
	   yys != yystackp->yysplitPoint;
	   yys = yys->yypred, yyn += 1)
	continue;
      YYCHK (yyresolveStates (yystackp->yytops.yystates[0], yyn, yystackp
			     ));
    }
  return yyok;
}

static void
yycompressStack (yyGLRStack* yystackp)
{
  yyGLRState* yyp, *yyq, *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == NULL)
    return;

  for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = NULL;
       yyp != yystackp->yysplitPoint;
       yyr = yyp, yyp = yyq, yyq = yyp->yypred)
    yyp->yypred = yyr;

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = ((yyGLRStackItem*) yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= yystackp->yynextFree - yystackp->yyitems;
  yystackp->yysplitPoint = NULL;
  yystackp->yylastDeleted = NULL;

  while (yyr != NULL)
    {
      yystackp->yynextFree->yystate = *yyr;
      yyr = yyr->yypred;
      yystackp->yynextFree->yystate.yypred = &yystackp->yynextFree[-1].yystate;
      yystackp->yytops.yystates[0] = &yystackp->yynextFree->yystate;
      yystackp->yynextFree += 1;
      yystackp->yyspaceLeft -= 1;
    }
}

static YYRESULTTAG
yyprocessOneStack (yyGLRStack* yystackp, size_t yyk,
		   size_t yyposn)
{
  int yyaction;
  const short int* yyconflicts;
  yyRuleNum yyrule;

  while (yystackp->yytops.yystates[yyk] != NULL)
    {
      yyStateNum yystate = yystackp->yytops.yystates[yyk]->yylrState;
      YYDPRINTF ((stderr, "Stack %lu Entering state %d\n",
		  (unsigned long int) yyk, yystate));

      YYASSERT (yystate != YYFINAL);

      if (yyisDefaultedState (yystate))
	{
	  yyrule = yydefaultAction (yystate);
	  if (yyrule == 0)
	    {
	      YYDPRINTF ((stderr, "Stack %lu dies.\n",
			  (unsigned long int) yyk));
	      yymarkStackDeleted (yystackp, yyk);
	      return yyok;
	    }
	  YYCHK (yyglrReduce (yystackp, yyk, yyrule, yyfalse));
	}
      else
	{
	  yySymbol yytoken;
	  yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;
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

	  yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);

	  while (*yyconflicts != 0)
	    {
	      size_t yynewStack = yysplitStack (yystackp, yyk);
	      YYDPRINTF ((stderr, "Splitting off stack %lu from %lu.\n",
			  (unsigned long int) yynewStack,
			  (unsigned long int) yyk));
	      YYCHK (yyglrReduce (yystackp, yynewStack,
				  *yyconflicts, yyfalse));
	      YYCHK (yyprocessOneStack (yystackp, yynewStack,
					yyposn));
	      yyconflicts += 1;
	    }

	  if (yyisShiftAction (yyaction))
	    break;
	  else if (yyisErrorAction (yyaction))
	    {
	      YYDPRINTF ((stderr, "Stack %lu dies.\n",
			  (unsigned long int) yyk));
	      yymarkStackDeleted (yystackp, yyk);
	      break;
	    }
	  else
	    YYCHK (yyglrReduce (yystackp, yyk, -yyaction,
				yyfalse));
	}
    }
  return yyok;
}

/*ARGSUSED*/ static void
yyreportSyntaxError (yyGLRStack* yystackp)
{
  if (yystackp->yyerrState == 0)
    {
#if YYERROR_VERBOSE
      int yyn;
      yyn = yypact[yystackp->yytops.yystates[0]->yylrState];
      if (YYPACT_NINF < yyn && yyn <= YYLAST)
	{
	  yySymbol yytoken = YYTRANSLATE (yychar);
	  size_t yysize0 = yytnamerr (NULL, yytokenName (yytoken));
	  size_t yysize = yysize0;
	  size_t yysize1;
	  yybool yysize_overflow = yyfalse;
	  char* yymsg = NULL;
	  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;
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

	  yyarg[0] = yytokenName (yytoken);
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
		yyarg[yycount++] = yytokenName (yyx);
		yysize1 = yysize + yytnamerr (NULL, yytokenName (yyx));
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + strlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow)
	    yymsg = (char *) YYMALLOC (yysize);

	  if (yymsg)
	    {
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
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
	      yyerror (yymsg);
	      YYFREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      yyMemoryExhausted (yystackp);
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
      yynerrs += 1;
    }
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
/*ARGSUSED*/ static void
yyrecoverSyntaxError (yyGLRStack* yystackp)
{
  size_t yyk;
  int yyj;

  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (YYID (yytrue))
      {
	yySymbol yytoken;
	if (yychar == YYEOF)
	  yyFail (yystackp, NULL);
	if (yychar != YYEMPTY)
	  {
	    /* We throw away the lookahead, but the error range
	       of the shifted error token must take it into account.  */
	    yyGLRState *yys = yystackp->yytops.yystates[0];
	    yyGLRStackItem yyerror_range[3];
	    yyerror_range[1].yystate.yyloc = yys->yyloc;
	    yyerror_range[2].yystate.yyloc = yylloc;
	    YYLLOC_DEFAULT ((yys->yyloc), yyerror_range, 2);
	    yytoken = YYTRANSLATE (yychar);
	    yydestruct ("Error: discarding",
			yytoken, &yylval, &yylloc);
	  }
	YYDPRINTF ((stderr, "Reading a token: "));
	yychar = YYLEX;
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
	yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
	if (yyis_pact_ninf (yyj))
	  return;
	yyj += yytoken;
	if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
	  {
	    if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
	      return;
	  }
	else if (yytable[yyj] != 0 && ! yyis_table_ninf (yytable[yyj]))
	  return;
      }

  /* Reduce to one stack.  */
  for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
    if (yystackp->yytops.yystates[yyk] != NULL)
      break;
  if (yyk >= yystackp->yytops.yysize)
    yyFail (yystackp, NULL);
  for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
    yymarkStackDeleted (yystackp, yyk);
  yyremoveDeletes (yystackp);
  yycompressStack (yystackp);

  /* Now pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != NULL)
    {
      yyGLRState *yys = yystackp->yytops.yystates[0];
      yyj = yypact[yys->yylrState];
      if (! yyis_pact_ninf (yyj))
	{
	  yyj += YYTERROR;
	  if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYTERROR
	      && yyisShiftAction (yytable[yyj]))
	    {
	      /* Shift the error token having adjusted its location.  */
	      YYLTYPE yyerrloc;
	      yystackp->yyerror_range[2].yystate.yyloc = yylloc;
	      YYLLOC_DEFAULT (yyerrloc, (yystackp->yyerror_range), 2);
	      YY_SYMBOL_PRINT ("Shifting", yystos[yytable[yyj]],
			       &yylval, &yyerrloc);
	      yyglrShift (yystackp, 0, yytable[yyj],
			  yys->yyposn, &yylval, &yyerrloc);
	      yys = yystackp->yytops.yystates[0];
	      break;
	    }
	}
      yystackp->yyerror_range[1].yystate.yyloc = yys->yyloc;
      if (yys->yypred != NULL)
	yydestroyGLRState ("Error: popping", yys);
      yystackp->yytops.yystates[0] = yys->yypred;
      yystackp->yynextFree -= 1;
      yystackp->yyspaceLeft += 1;
    }
  if (yystackp->yytops.yystates[0] == NULL)
    yyFail (yystackp, NULL);
}

#define YYCHK1(YYE)							     \
  do {									     \
    switch (YYE) {							     \
    case yyok:								     \
      break;								     \
    case yyabort:							     \
      goto yyabortlab;							     \
    case yyaccept:							     \
      goto yyacceptlab;							     \
    case yyerr:								     \
      goto yyuser_error;						     \
    default:								     \
      goto yybuglab;							     \
    }									     \
  } while (YYID (0))


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  size_t yyposn;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY;
  yylval = yyval_default;

#if YYLTYPE_IS_TRIVIAL
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif


  if (! yyinitGLRStack (yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP (yystack.yyexception_buffer))
    {
    case 0: break;
    case 1: goto yyabortlab;
    case 2: goto yyexhaustedlab;
    default: goto yybuglab;
    }
  yyglrShift (&yystack, 0, 0, 0, &yylval, &yylloc);
  yyposn = 0;

  while (YYID (yytrue))
    {
      /* For efficiency, we have two loops, the first of which is
	 specialized to deterministic operation (single stack, no
	 potential ambiguity).  */
      /* Standard mode */
      while (YYID (yytrue))
	{
	  yyRuleNum yyrule;
	  int yyaction;
	  const short int* yyconflicts;

	  yyStateNum yystate = yystack.yytops.yystates[0]->yylrState;
	  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
	  if (yystate == YYFINAL)
	    goto yyacceptlab;
	  if (yyisDefaultedState (yystate))
	    {
	      yyrule = yydefaultAction (yystate);
	      if (yyrule == 0)
		{
		  yystack.yyerror_range[1].yystate.yyloc = yylloc;
		  yyreportSyntaxError (&yystack);
		  goto yyuser_error;
		}
	      YYCHK1 (yyglrReduce (&yystack, 0, yyrule, yytrue));
	    }
	  else
	    {
	      yySymbol yytoken;
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

	      yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);
	      if (*yyconflicts != 0)
		break;
	      if (yyisShiftAction (yyaction))
		{
		  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
		  yychar = YYEMPTY;
		  yyposn += 1;
		  yyglrShift (&yystack, 0, yyaction, yyposn, &yylval, &yylloc);
		  if (0 < yystack.yyerrState)
		    yystack.yyerrState -= 1;
		}
	      else if (yyisErrorAction (yyaction))
		{
		  yystack.yyerror_range[1].yystate.yyloc = yylloc;
		  yyreportSyntaxError (&yystack);
		  goto yyuser_error;
		}
	      else
		YYCHK1 (yyglrReduce (&yystack, 0, -yyaction, yytrue));
	    }
	}

      while (YYID (yytrue))
	{
	  yySymbol yytoken_to_shift;
	  size_t yys;

	  for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
	    yystackp->yytops.yylookaheadNeeds[yys] = yychar != YYEMPTY;

	  /* yyprocessOneStack returns one of three things:

	      - An error flag.  If the caller is yyprocessOneStack, it
		immediately returns as well.  When the caller is finally
		yyparse, it jumps to an error label via YYCHK1.

	      - yyok, but yyprocessOneStack has invoked yymarkStackDeleted
		(&yystack, yys), which sets the top state of yys to NULL.  Thus,
		yyparse's following invocation of yyremoveDeletes will remove
		the stack.

	      - yyok, when ready to shift a token.

	     Except in the first case, yyparse will invoke yyremoveDeletes and
	     then shift the next token onto all remaining stacks.  This
	     synchronization of the shift (that is, after all preceding
	     reductions on all stacks) helps prevent double destructor calls
	     on yylval in the event of memory exhaustion.  */

	  for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
	    YYCHK1 (yyprocessOneStack (&yystack, yys, yyposn));
	  yyremoveDeletes (&yystack);
	  if (yystack.yytops.yysize == 0)
	    {
	      yyundeleteLastStack (&yystack);
	      if (yystack.yytops.yysize == 0)
		yyFail (&yystack, YY_("syntax error"));
	      YYCHK1 (yyresolveStack (&yystack));
	      YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
	      yystack.yyerror_range[1].yystate.yyloc = yylloc;
	      yyreportSyntaxError (&yystack);
	      goto yyuser_error;
	    }

	  /* If any yyglrShift call fails, it will fail after shifting.  Thus,
	     a copy of yylval will already be on stack 0 in the event of a
	     failure in the following loop.  Thus, yychar is set to YYEMPTY
	     before the loop to make sure the user destructor for yylval isn't
	     called twice.  */
	  yytoken_to_shift = YYTRANSLATE (yychar);
	  yychar = YYEMPTY;
	  yyposn += 1;
	  for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
	    {
	      int yyaction;
	      const short int* yyconflicts;
	      yyStateNum yystate = yystack.yytops.yystates[yys]->yylrState;
	      yygetLRActions (yystate, yytoken_to_shift, &yyaction,
			      &yyconflicts);
	      /* Note that yyconflicts were handled by yyprocessOneStack.  */
	      YYDPRINTF ((stderr, "On stack %lu, ", (unsigned long int) yys));
	      YY_SYMBOL_PRINT ("shifting", yytoken_to_shift, &yylval, &yylloc);
	      yyglrShift (&yystack, yys, yyaction, yyposn,
			  &yylval, &yylloc);
	      YYDPRINTF ((stderr, "Stack %lu now in state #%d\n",
			  (unsigned long int) yys,
			  yystack.yytops.yystates[yys]->yylrState));
	    }

	  if (yystack.yytops.yysize == 1)
	    {
	      YYCHK1 (yyresolveStack (&yystack));
	      YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
	      yycompressStack (&yystack);
	      break;
	    }
	}
      continue;
    yyuser_error:
      yyrecoverSyntaxError (&yystack);
      yyposn = yystack.yytops.yystates[0]->yyposn;
    }

 yyacceptlab:
  yyresult = 0;
  goto yyreturn;

 yybuglab:
  YYASSERT (yyfalse);
  goto yyabortlab;

 yyabortlab:
  yyresult = 1;
  goto yyreturn;

 yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;

 yyreturn:
  if (yychar != YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead",
		YYTRANSLATE (yychar),
		&yylval, &yylloc);

  /* If the stack is well-formed, pop the stack until it is empty,
     destroying its entries as we go.  But free the stack regardless
     of whether it is well-formed.  */
  if (yystack.yyitems)
    {
      yyGLRState** yystates = yystack.yytops.yystates;
      if (yystates)
	{
	  size_t yysize = yystack.yytops.yysize;
	  size_t yyk;
	  for (yyk = 0; yyk < yysize; yyk += 1)
	    if (yystates[yyk])
	      {
		while (yystates[yyk])
		  {
		    yyGLRState *yys = yystates[yyk];
		    yystack.yyerror_range[1].yystate.yyloc = yys->yyloc;
		    if (yys->yypred != NULL)
		      yydestroyGLRState ("Cleanup: popping", yys);
		    yystates[yyk] = yys->yypred;
		    yystack.yynextFree -= 1;
		    yystack.yyspaceLeft += 1;
		  }
		break;
	      }
	}
      yyfreeGLRStack (&yystack);
    }

  /* Make sure YYID is used.  */
  return YYID (yyresult);
}

/* DEBUGGING ONLY */
#if YYDEBUG
static void yypstack (yyGLRStack* yystackp, size_t yyk)
  __attribute__ ((__unused__));
static void yypdumpstack (yyGLRStack* yystackp) __attribute__ ((__unused__));

static void
yy_yypstack (yyGLRState* yys)
{
  if (yys->yypred)
    {
      yy_yypstack (yys->yypred);
      YYFPRINTF (stderr, " -> ");
    }
  YYFPRINTF (stderr, "%d@%lu", yys->yylrState,
             (unsigned long int) yys->yyposn);
}

static void
yypstates (yyGLRState* yyst)
{
  if (yyst == NULL)
    YYFPRINTF (stderr, "<null>");
  else
    yy_yypstack (yyst);
  YYFPRINTF (stderr, "\n");
}

static void
yypstack (yyGLRStack* yystackp, size_t yyk)
{
  yypstates (yystackp->yytops.yystates[yyk]);
}

#define YYINDEX(YYX)							     \
    ((YYX) == NULL ? -1 : (yyGLRStackItem*) (YYX) - yystackp->yyitems)


static void
yypdumpstack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yyp;
  size_t yyi;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
    {
      YYFPRINTF (stderr, "%3lu. ",
                 (unsigned long int) (yyp - yystackp->yyitems));
      if (*(yybool *) yyp)
	{
	  YYFPRINTF (stderr, "Res: %d, LR State: %d, posn: %lu, pred: %ld",
		     yyp->yystate.yyresolved, yyp->yystate.yylrState,
		     (unsigned long int) yyp->yystate.yyposn,
		     (long int) YYINDEX (yyp->yystate.yypred));
	  if (! yyp->yystate.yyresolved)
	    YYFPRINTF (stderr, ", firstVal: %ld",
		       (long int) YYINDEX (yyp->yystate
                                             .yysemantics.yyfirstVal));
	}
      else
	{
	  YYFPRINTF (stderr, "Option. rule: %d, state: %ld, next: %ld",
		     yyp->yyoption.yyrule - 1,
		     (long int) YYINDEX (yyp->yyoption.yystate),
		     (long int) YYINDEX (yyp->yyoption.yynext));
	}
      YYFPRINTF (stderr, "\n");
    }
  YYFPRINTF (stderr, "Tops:");
  for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
    YYFPRINTF (stderr, "%lu: %ld; ", (unsigned long int) yyi,
	       (long int) YYINDEX (yystackp->yytops.yystates[yyi]));
  YYFPRINTF (stderr, "\n");
}
#endif



/* Line 2634 of glr.c  */
#line 1663 "ned2.y"


//----------------------------------------------------------------------
// general bison/flex stuff:
//

NEDElement *doParseNED2(NEDParser *p, const char *nedtext)
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
    ps.nedfile->setVersion("2");

    // store file comment
    storeFileComment(ps.nedfile);

    ps.propertyscope.push(ps.nedfile);

    globalps = ps; // remember this for error recovery

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

    if (np->getErrors()->empty())
    {
        // more sanity checks
        if (ps.propertyscope.size()!=1 || ps.propertyscope.top()!=ps.nedfile)
            INTERNAL_ERROR0(NULL, "error during parsing: imbalanced propertyscope");
        if (!ps.blockscope.empty() || !ps.typescope.empty())
            INTERNAL_ERROR0(NULL, "error during parsing: imbalanced blockscope or typescope");
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


