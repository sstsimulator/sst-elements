
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison GLR parsers in C
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IMPORT = 258,
     PACKAGE = 259,
     PROPERTY = 260,
     MODULE = 261,
     SIMPLE = 262,
     NETWORK = 263,
     CHANNEL = 264,
     MODULEINTERFACE = 265,
     CHANNELINTERFACE = 266,
     EXTENDS = 267,
     LIKE = 268,
     TYPES = 269,
     PARAMETERS = 270,
     GATES = 271,
     SUBMODULES = 272,
     CONNECTIONS = 273,
     ALLOWUNCONNECTED = 274,
     DOUBLETYPE = 275,
     INTTYPE = 276,
     STRINGTYPE = 277,
     BOOLTYPE = 278,
     XMLTYPE = 279,
     VOLATILE = 280,
     INPUT_ = 281,
     OUTPUT_ = 282,
     INOUT_ = 283,
     IF = 284,
     FOR = 285,
     RIGHTARROW = 286,
     LEFTARROW = 287,
     DBLARROW = 288,
     TO = 289,
     TRUE_ = 290,
     FALSE_ = 291,
     THIS_ = 292,
     DEFAULT = 293,
     ASK = 294,
     CONST_ = 295,
     SIZEOF = 296,
     INDEX_ = 297,
     XMLDOC = 298,
     NAME = 299,
     INTCONSTANT = 300,
     REALCONSTANT = 301,
     STRINGCONSTANT = 302,
     CHARCONSTANT = 303,
     PLUSPLUS = 304,
     DOUBLEASTERISK = 305,
     EQ = 306,
     NE = 307,
     GE = 308,
     LE = 309,
     AND = 310,
     OR = 311,
     XOR = 312,
     NOT = 313,
     BIN_AND = 314,
     BIN_OR = 315,
     BIN_XOR = 316,
     BIN_COMPL = 317,
     SHIFT_LEFT = 318,
     SHIFT_RIGHT = 319,
     EXPRESSION_SELECTOR = 320,
     INVALID_CHAR = 321,
     UMIN = 322
   };
#endif


#ifndef YYSTYPE
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{

  int first_line;
  int first_column;
  int last_line;
  int last_column;

} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



extern YYSTYPE ned2yylval;

extern YYLTYPE ned2yylloc;


