
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DOUBLETYPE = 258,
     INTTYPE = 259,
     STRINGTYPE = 260,
     BOOLTYPE = 261,
     TRUE_ = 262,
     FALSE_ = 263,
     NAME = 264,
     INTCONSTANT = 265,
     REALCONSTANT = 266,
     STRINGCONSTANT = 267,
     EQ_ = 268,
     NE_ = 269,
     GE_ = 270,
     LE_ = 271,
     AND_ = 272,
     OR_ = 273,
     XOR_ = 274,
     NOT_ = 275,
     BINAND_ = 276,
     BINOR_ = 277,
     BINXOR_ = 278,
     BINCOMPL_ = 279,
     SHIFTLEFT_ = 280,
     SHIFTRIGHT_ = 281,
     INVALID_CHAR = 282,
     UMIN_ = 283
   };
#endif
/* Tokens.  */
#define DOUBLETYPE 258
#define INTTYPE 259
#define STRINGTYPE 260
#define BOOLTYPE 261
#define TRUE_ 262
#define FALSE_ 263
#define NAME 264
#define INTCONSTANT 265
#define REALCONSTANT 266
#define STRINGCONSTANT 267
#define EQ_ 268
#define NE_ 269
#define GE_ 270
#define LE_ 271
#define AND_ 272
#define OR_ 273
#define XOR_ 274
#define NOT_ 275
#define BINAND_ 276
#define BINOR_ 277
#define BINXOR_ 278
#define BINCOMPL_ 279
#define SHIFTLEFT_ 280
#define SHIFTRIGHT_ 281
#define INVALID_CHAR 282
#define UMIN_ 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE expressionyylval;


