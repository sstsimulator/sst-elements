/* eval.h - expression evaluator interfaces */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

#ifndef EVAL_H
#define EVAL_H

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdio.h>
#include "host.h"
#include "misc.h"
#include "machine.h"

/* forward declarations */
struct eval_state_t;
struct eval_value_t;

/* an identifier evaluator, when an evaluator is instantiated, the user
   must supply a function of this type that returns the value of identifiers
   encountered in expressions */
typedef struct eval_value_t		  /* value of the identifier */
(*eval_ident_t)(struct eval_state_t *es); /* ident string in es->tok_buf */

/* expression tokens */
enum eval_token_t {
  tok_ident,		/* user-valued identifiers */
  tok_const,		/* numeric literals */
  tok_plus,		/* `+' */
  tok_minus,		/* `-' */
  tok_mult,		/* `*' */
  tok_div,		/* `/' */
  tok_exp,		/* `^' */
  tok_log,		/* `!' */
  tok_oparen,		/* `(' */
  tok_cparen,		/* `)' */
  tok_eof,		/* end of file */
  tok_whitespace,	/* ` ', `\t', `\n' */
  tok_invalid		/* unrecognized token */
};

/* an evaluator state record */
struct eval_state_t {
  const char *p;			/* ptr to next char to consume from expr */
  const char *lastp;			/* save space for token peeks */
  eval_ident_t f_eval_ident;	/* identifier evaluator */
  void *user_ptr;		/* user-supplied argument pointer */
  char tok_buf[512];		/* text of last token returned */
  enum eval_token_t peek_tok;	/* peek buffer, for one token look-ahead */
};

/* evaluation errors */
enum eval_err_t {
  ERR_NOERR,			/* no error */
  ERR_UPAREN,			/* unmatched parenthesis */
  ERR_NOTERM,			/* expression term is missing */
  ERR_DIV0,			/* divide by zero */
  ERR_BADCONST,			/* badly formed constant */
  ERR_BADEXPR,			/* badly formed constant */
  ERR_UNDEFVAR,			/* variable is undefined */
  ERR_EXTRA,			/* extra characters at end of expression */
  ERR_NUM
};

/* expression evaluation error, this must be a global */
extern enum eval_err_t eval_error /* = ERR_NOERR */;

/* enum eval_err_t -> error description string map */
extern char *eval_err_str[ERR_NUM];

/* expression value types */
enum eval_type_t {
  et_int,			/* signed integer result */
  et_uint,			/* unsigned integer result */
  et_addr,			/* address value */
  et_qword,			/* unsigned qword length integer result */
  et_sqword,			/* signed qword length integer result */
  et_float,			/* single-precision floating point value */
  et_double,			/* double-precision floating point value */
  et_symbol,			/* non-numeric result (!allowed in exprs)*/
  et_NUM
};

/* non-zero if type is an integral type */
#define EVAL_INTEGRAL(TYPE)						\
  ((TYPE) == et_int || (TYPE) == et_uint || (TYPE) == et_addr		\
   || (TYPE) == et_qword || (TYPE) == et_sqword)

/* enum eval_type_t -> expression type description string map */
extern char *eval_type_str[et_NUM];

/* an expression value */
struct eval_value_t {
  enum eval_type_t type;		/* type of expression value */
  union {
    int as_int;				/* value for type == et_int */
    unsigned int as_uint;		/* value for type == et_uint */
    md_addr_t as_addr;			/* value for type == et_addr */
    qword_t as_qword;			/* value for type == ec_qword */
    sqword_t as_sqword;			/* value for type == ec_sqword */
    float as_float;			/* value for type == et_float */
    double as_double;			/* value for type == et_double */
    const char *as_symbol;			/* value for type == et_symbol */
  } value;
};

/*
 * expression value arithmetic conversions
 */

/* eval_value_t (any numeric type) -> double */
double eval_as_double(struct eval_value_t val);

/* eval_value_t (any numeric type) -> float */
float eval_as_float(struct eval_value_t val);

/* eval_value_t (any numeric type) -> qword_t */
qword_t eval_as_qword(struct eval_value_t val);

/* eval_value_t (any numeric type) -> sqword_t */
sqword_t eval_as_sqword(struct eval_value_t val);

/* eval_value_t (any numeric type) -> md_addr_t */
md_addr_t eval_as_addr(struct eval_value_t val);

/* eval_value_t (any numeric type) -> unsigned int */
unsigned int eval_as_uint(struct eval_value_t val);

/* eval_value_t (any numeric type) -> int */
int eval_as_int(struct eval_value_t val);

/* create an evaluator */
struct eval_state_t *			/* expression evaluator */
eval_new(eval_ident_t f_eval_ident,	/* user ident evaluator */
	 void *user_ptr);		/* user ptr passed to ident fn */

/* delete an evaluator */
void
eval_delete(struct eval_state_t *es);	/* evaluator to delete */

/* evaluate an expression, if an error occurs during evaluation, the
   global variable eval_error will be set to a value other than ERR_NOERR
   NOTE: Order of evaluation is indeterminant, use parens for ordering
   of complex expressions. */
struct eval_value_t			/* value of the expression */
eval_expr(struct eval_state_t *es,	/* expression evaluator */
	  const char *p,			/* ptr to expression string */
	  const char **endp);			/* returns ptr to 1st unused char */

/* print an expression value */
void
eval_print(FILE *stream,		/* output stream */
	   struct eval_value_t val);	/* expression value to print */

//#ifdef __cplusplus
//}
//#endif

#endif /* EVAL_H */
