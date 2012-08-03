
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
     NAMESPACE = 258,
     CPLUSPLUS = 259,
     CPLUSPLUSBODY = 260,
     MESSAGE = 261,
     PACKET = 262,
     CLASS = 263,
     STRUCT = 264,
     ENUM = 265,
     NONCOBJECT = 266,
     EXTENDS = 267,
     FIELDS = 268,
     PROPERTIES = 269,
     ABSTRACT = 270,
     READONLY = 271,
     NAME = 272,
     DOUBLECOLON = 273,
     INTCONSTANT = 274,
     REALCONSTANT = 275,
     STRINGCONSTANT = 276,
     CHARCONSTANT = 277,
     TRUE_ = 278,
     FALSE_ = 279,
     BOOLTYPE = 280,
     CHARTYPE = 281,
     SHORTTYPE = 282,
     INTTYPE = 283,
     LONGTYPE = 284,
     DOUBLETYPE = 285,
     UNSIGNED_ = 286,
     STRINGTYPE = 287,
     EQ = 288,
     NE = 289,
     GE = 290,
     LE = 291,
     AND = 292,
     OR = 293,
     XOR = 294,
     NOT = 295,
     BIN_AND = 296,
     BIN_OR = 297,
     BIN_XOR = 298,
     BIN_COMPL = 299,
     SHIFT_LEFT = 300,
     SHIFT_RIGHT = 301,
     INVALID_CHAR = 302,
     UMIN = 303
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



extern YYSTYPE msg2yylval;

extern YYLTYPE msg2yylloc;


