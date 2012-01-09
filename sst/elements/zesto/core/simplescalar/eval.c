/* eval.c - expression evaluator routines */
/*
 * SimpleScalar Ô Tool Suite
 * © 1994-2003 Todd M. Austin, Ph.D. and SimpleScalar, LLC
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING SIMPLESCALAR, YOU ARE AGREEING TO
 * THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted as
 * described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express or
 * implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged.  SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship purposes
 * provided that this notice in its entirety accompanies all copies. Copies of
 * the modified software can be delivered to persons who use it solely for
 * nonprofit, educational, noncommercial research, and noncommercial
 * scholarship purposes provided that this notice in its entirety accompanies
 * all copies.
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
 * someone who received only the executable form, and is accompanied by a copy
 * of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright © 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "host.h"
#include "misc.h"
#include "eval.h"

#if defined(sparc) && !defined(__svr4__)
#define strtoul strtol
#endif

/* expression evaluation error, this must be a global */
enum eval_err_t eval_error = ERR_NOERR;

/* enum eval_err_t -> error description string map */
char *eval_err_str[ERR_NUM] = {
  /* ERR_NOERR */        (char*)"!! no error!!",
  /* ERR_UPAREN */        (char*)"unmatched parenthesis",
  /* ERR_NOTERM */        (char*)"expression term is missing",
  /* ERR_DIV0 */        (char*)"DBZ",
  /* ERR_BADCONST */        (char*)"badly formed constant",
  /* ERR_BADEXPR */        (char*)"badly formed expression",
  /* ERR_UNDEFVAR */        (char*)"variable is undefined",
  /* ERR_EXTRA */        (char*)"extra characters at end of expression"
};

/* *first* token character -> enum eval_token_t map */
static enum eval_token_t tok_map[256];
static int tok_map_initialized = FALSE;

/* builds the first token map */
static void
init_tok_map(void)
{
  int i;

  for (i=0; i<256; i++)
    tok_map[i] = tok_invalid;

  /* identifier characters */
  for (i='a'; i<='z'; i++)
    tok_map[i] = tok_ident;
  for (i='A'; i<='Z'; i++)
    tok_map[i] = tok_ident;
  tok_map[(int)'_'] = tok_ident;
  tok_map[(int)'$'] = tok_ident;

  /* numeric characters */
  for (i='0'; i<='9'; i++)
    tok_map[i] = tok_const;
  tok_map[(int)'.'] = tok_const;

  /* operator characters */
  tok_map[(int)'+'] = tok_plus;
  tok_map[(int)'-'] = tok_minus;
  tok_map[(int)'*'] = tok_mult;
  tok_map[(int)'/'] = tok_div;
  tok_map[(int)'('] = tok_oparen;
  tok_map[(int)')'] = tok_cparen;
  tok_map[(int)'^'] = tok_exp; /* exponentiate (natural base) - GL */
  tok_map[(int)'!'] = tok_log; /* natural log - GL */

  /* whitespace characers */
  tok_map[(int)' '] = tok_whitespace;
  tok_map[(int)'\t'] = tok_whitespace;
}

/* get next token from the expression string */
static enum eval_token_t                /* token parsed */
get_next_token(struct eval_state_t *es)        /* expression evaluator */
{
  int allow_hex;
  enum eval_token_t tok;
  char *ptok_buf, last_char;

  /* initialize the token map, if needed */
  if (!tok_map_initialized)
    {
      init_tok_map();
      tok_map_initialized = TRUE;
    }

  /* use the peek'ed token, if available, tok_buf should still be valid */
  if (es->peek_tok != tok_invalid)
    {
      tok = es->peek_tok;
      es->peek_tok = tok_invalid;
      return tok;
    }

  /* set up the token string space */
  ptok_buf = es->tok_buf;
  *ptok_buf = '\0';

  /* skip whitespace */
  while (*es->p && tok_map[(int)*es->p] == tok_whitespace)
    es->p++;

  /* end of token stream? */
  if (*es->p == '\0')
    return tok_eof;

  *ptok_buf++ = *es->p;
  tok = tok_map[(int)*es->p++];
  switch (tok)
    {
    case tok_ident:
      /* parse off next identifier */
      while (*es->p
             && (tok_map[(int)*es->p] == tok_ident
                 || tok_map[(int)*es->p] == tok_const))
        {
          *ptok_buf++ = *es->p++;
        }
      break;
    case tok_const:
      /* parse off next numeric literal */
      last_char = '\0';
      allow_hex = FALSE;
      while (*es->p &&
             (tok_map[(int)*es->p] == tok_const
              || (*es->p == '-' && last_char == 'e')
              || (*es->p == '+' && last_char == 'e')
              || mytolower(*es->p) == 'e'
              || mytolower(*es->p) == 'x'
              || (mytolower(*es->p) == 'a' && allow_hex)
              || (mytolower(*es->p) == 'b' && allow_hex)
              || (mytolower(*es->p) == 'c' && allow_hex)
              || (mytolower(*es->p) == 'd' && allow_hex)
              || (mytolower(*es->p) == 'e' && allow_hex)
              || (mytolower(*es->p) == 'f' && allow_hex)))
        {
          last_char = mytolower(*es->p);
          if (*es->p == 'x' || *es->p == 'X')
            allow_hex = TRUE;
          *ptok_buf++ = *es->p++;
        }
      break;
    case tok_plus:
    case tok_minus:
    case tok_mult:
    case tok_div:
    case tok_exp:
    case tok_log:
    case tok_oparen:
    case tok_cparen:
      /* just pass on the token */
      break;
    default:
      tok = tok_invalid;
      break;
    }

  /* terminate the token string buffer */
  *ptok_buf = '\0';

  return tok;
}

/* peek ahead at the next token from the expression stream, currently
   only the next token can be peek'ed at */
static enum eval_token_t                 /* next token in expression */
peek_next_token(struct eval_state_t *es) /* expression evalutor */
{
  /* if there is no peek ahead token, get one */
  if (es->peek_tok == tok_invalid)
    {
      es->lastp = es->p;
      es->peek_tok = get_next_token(es);
    }

  /* return peek ahead token */
  return es->peek_tok;
}

/* forward declaration */
static struct eval_value_t expr(struct eval_state_t *es);

/* default expression error value, eval_err is also set */
static struct eval_value_t err_value = { et_int, { 0 } };

/* expression type strings */
char *eval_type_str[et_NUM] = {
  /* et_int */                (char*)"int",
  /* et_uint */                (char*)"unsigned int",
  /* et_addr */                (char*)"md_addr_t",
  /* et_qword */        (char*)"qword_t",
  /* et_sqword */        (char*)"sqword_t",
  /* et_float */        (char*)"float",
  /* et_double */        (char*)"double",
  /* et_symbol */        (char*)"symbol"
};

/* determine necessary arithmetic conversion on T1 <op> T2 */
static enum eval_type_t                        /* type of expression result */
result_type(enum eval_type_t t1,        /* left operand type */
            enum eval_type_t t2)        /* right operand type */
{
  /* sanity check, symbols should not show up in arithmetic exprs */
  if (t1 == et_symbol || t2 == et_symbol)
    panic("symbol used in expression");

  /* using C rules, i.e., A6.5 */
  if (t1 == et_double || t2 == et_double)
    return et_double;
  else if (t1 == et_float || t2 == et_float)
    return et_float;
  else if (t1 == et_qword || t2 == et_qword)
    return et_qword;
  else if (t1 == et_sqword || t2 == et_sqword)
    return et_sqword;
  else if (t1 == et_addr || t2 == et_addr)
    return et_addr;
  else if (t1 == et_uint || t2 == et_uint)
    return et_uint;
  else
    return et_int;
}

/*
 * expression value arithmetic conversions
 */

/* eval_value_t (any numeric type) -> double */
double
eval_as_double(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return val.value.as_double;
    case et_float:
      return (double)val.value.as_float;
    case et_qword:
      return (double)val.value.as_qword;
    case et_sqword:
      return (double)val.value.as_sqword;
    case et_addr:
      return (double)val.value.as_addr;
    case et_uint:
      return (double)val.value.as_uint;
    case et_int:
      return (double)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> float */
float
eval_as_float(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (float)val.value.as_double;
    case et_float:
      return val.value.as_float;
    case et_qword:
      return (float)val.value.as_qword;
    case et_sqword:
      return (float)val.value.as_sqword;
    case et_addr:
      return (float)val.value.as_addr;
    case et_uint:
      return (float)val.value.as_uint;
    case et_int:
      return (float)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> qword_t */
qword_t
eval_as_qword(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (qword_t)val.value.as_double;
    case et_float:
      return (qword_t)val.value.as_float;
    case et_qword:
      return val.value.as_qword;
    case et_sqword:
      return (qword_t)val.value.as_sqword;
    case et_addr:
      return (qword_t)val.value.as_addr;
    case et_uint:
      return (qword_t)val.value.as_uint;
    case et_int:
      return (qword_t)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> sqword_t */
sqword_t
eval_as_sqword(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (sqword_t)val.value.as_double;
    case et_float:
      return (sqword_t)val.value.as_float;
    case et_qword:
      return (sqword_t)val.value.as_qword;
    case et_sqword:
      return val.value.as_sqword;
    case et_addr:
      return (sqword_t)val.value.as_addr;
    case et_uint:
      return (sqword_t)val.value.as_uint;
    case et_int:
      return (sqword_t)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> unsigned int */
md_addr_t
eval_as_addr(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (md_addr_t)val.value.as_double;
    case et_float:
      return (md_addr_t)val.value.as_float;
    case et_qword:
      return (md_addr_t)val.value.as_qword;
    case et_sqword:
      return (md_addr_t)val.value.as_sqword;
    case et_addr:
      return val.value.as_addr;
    case et_uint:
      return (md_addr_t)val.value.as_uint;
    case et_int:
      return (md_addr_t)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> unsigned int */
unsigned int
eval_as_uint(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (unsigned int)val.value.as_double;
    case et_float:
      return (unsigned int)val.value.as_float;
    case et_qword:
      return (unsigned int)val.value.as_qword;
    case et_sqword:
      return (unsigned int)val.value.as_sqword;
    case et_addr:
      return (unsigned int)val.value.as_addr;
    case et_uint:
      return val.value.as_uint;
    case et_int:
      return (unsigned int)val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/* eval_value_t (any numeric type) -> int */
int
eval_as_int(struct eval_value_t val)
{
  switch (val.type)
    {
    case et_double:
      return (int)val.value.as_double;
    case et_float:
      return (int)val.value.as_float;
    case et_qword:
      return (int)val.value.as_qword;
    case et_sqword:
      return (int)val.value.as_sqword;
    case et_addr:
      return (int)val.value.as_addr;
    case et_uint:
      return (int)val.value.as_uint;
    case et_int:
      return val.value.as_int;
    case et_symbol:
      panic("symbol used in expression");
    default:
      panic("illegal arithmetic expression conversion");
    }
}

/*
 * arithmetic intrinsics operations, used during expression evaluation
 */

/* compute <val1> + <val2> */
static struct eval_value_t
f_add(struct eval_value_t val1, struct eval_value_t val2)
{
  enum eval_type_t et;
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol || val2.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* get result type, and perform operation in that type */
  et = result_type(val1.type, val2.type);
  switch (et)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = eval_as_double(val1) + eval_as_double(val2);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = eval_as_float(val1) + eval_as_float(val2);
      break;
    case et_qword:
      val.type = et_qword;
      val.value.as_qword = eval_as_qword(val1) + eval_as_qword(val2);
      break;
    case et_sqword:
      val.type = et_sqword;
      val.value.as_sqword = eval_as_sqword(val1) + eval_as_sqword(val2);
      break;
    case et_addr:
      val.type = et_addr;
      val.value.as_addr = eval_as_addr(val1) + eval_as_addr(val2);
      break;
    case et_uint:
      val.type = et_uint;
      val.value.as_uint = eval_as_uint(val1) + eval_as_uint(val2);
      break;
    case et_int:
      val.type = et_int;
      val.value.as_int = eval_as_int(val1) + eval_as_int(val2);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute <val1> - <val2> */
static struct eval_value_t
f_sub(struct eval_value_t val1, struct eval_value_t val2)
{
  enum eval_type_t et;
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol || val2.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* get result type, and perform operation in that type */
  et = result_type(val1.type, val2.type);
  switch (et)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = eval_as_double(val1) - eval_as_double(val2);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = eval_as_float(val1) - eval_as_float(val2);
      break;
    case et_qword:
      val.type = et_qword;
      val.value.as_qword = eval_as_qword(val1) - eval_as_qword(val2);
      break;
    case et_sqword:
      val.type = et_sqword;
      val.value.as_sqword = eval_as_sqword(val1) - eval_as_sqword(val2);
      break;
    case et_addr:
      val.type = et_addr;
      val.value.as_addr = eval_as_addr(val1) - eval_as_addr(val2);
      break;
    case et_uint:
      val.type = et_uint;
      val.value.as_uint = eval_as_uint(val1) - eval_as_uint(val2);
      break;
    case et_int:
      val.type = et_int;
      val.value.as_int = eval_as_int(val1) - eval_as_int(val2);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute <val1> * <val2> */
static struct eval_value_t
f_mult(struct eval_value_t val1, struct eval_value_t val2)
{
  enum eval_type_t et;
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol || val2.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* get result type, and perform operation in that type */
  et = result_type(val1.type, val2.type);
  switch (et)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = eval_as_double(val1) * eval_as_double(val2);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = eval_as_float(val1) * eval_as_float(val2);
      break;
    case et_qword:
      val.type = et_qword;
      val.value.as_qword = eval_as_qword(val1) * eval_as_qword(val2);
      break;
    case et_sqword:
      val.type = et_sqword;
      val.value.as_sqword = eval_as_sqword(val1) * eval_as_sqword(val2);
      break;
    case et_addr:
      val.type = et_addr;
      val.value.as_addr = eval_as_addr(val1) * eval_as_addr(val2);
      break;
    case et_uint:
      val.type = et_uint;
      val.value.as_uint = eval_as_uint(val1) * eval_as_uint(val2);
      break;
    case et_int:
      val.type = et_int;
      val.value.as_int = eval_as_int(val1) * eval_as_int(val2);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute <val1> / <val2> */
static struct eval_value_t
f_div(struct eval_value_t val1, struct eval_value_t val2)
{
  enum eval_type_t et;
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol || val2.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* get result type, and perform operation in that type */
  et = result_type(val1.type, val2.type);
  switch (et)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = eval_as_double(val1) / eval_as_double(val2);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = eval_as_float(val1) / eval_as_float(val2);
      break;
    case et_qword:
      val.type = et_qword;
      val.value.as_qword = eval_as_qword(val1) / eval_as_qword(val2);
      break;
    case et_sqword:
      val.type = et_sqword;
      val.value.as_sqword = eval_as_sqword(val1) / eval_as_sqword(val2);
      break;
    case et_addr:
      val.type = et_addr;
      val.value.as_addr = eval_as_addr(val1) / eval_as_addr(val2);
      break;
    case et_uint:
      val.type = et_uint;
      val.value.as_uint = eval_as_uint(val1) / eval_as_uint(val2);
      break;
    case et_int:
      val.type = et_int;
      val.value.as_int = eval_as_int(val1) / eval_as_int(val2);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute - <val1> */
static struct eval_value_t
f_neg(struct eval_value_t val1)
{
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* result type is the same as the operand type */
  switch (val1.type)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = - val1.value.as_double;
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = - val1.value.as_float;
      break;
    case et_qword:
      val.type = et_sqword;
      val.value.as_qword = - (sqword_t)val1.value.as_qword;
      break;
    case et_sqword:
      val.type = et_sqword;
      val.value.as_sqword = - val1.value.as_sqword;
      break;
    case et_addr:
      val.type = et_addr;
      val.value.as_addr = - val1.value.as_addr;
      break;
    case et_uint:
      if ((unsigned int)val1.value.as_uint > 2147483648U)
        {
          /* promote type */
          val.type = et_sqword;
          val.value.as_sqword = - ((sqword_t)val1.value.as_uint);
        }
      else
        {
          /* don't promote type */
          val.type = et_int;
          val.value.as_int = - ((int)val1.value.as_uint);
        }
      break;
    case et_int:
      if ((unsigned int)val1.value.as_int == 0x80000000U)
        {
          /* promote type */
          val.type = et_uint;
          val.value.as_uint = 2147483648U;
        }
      else
        {
          /* don't promote type */
          val.type = et_int;
          val.value.as_int = - val1.value.as_int;
        }
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute exp(<val1>) */
static struct eval_value_t
f_exp(struct eval_value_t val1)
{
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* except for float, result type is double */
  switch (val1.type)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = exp(val1.value.as_double);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = (float)exp((double)val1.value.as_float);
      break;
    case et_qword:
      val.type = et_double;
      val.value.as_double = exp((double)val1.value.as_qword);
      break;
    case et_sqword:
      val.type = et_double;
      val.value.as_double = exp((double)val1.value.as_sqword);
      break;
    case et_addr:
      val.type = et_double;
      val.value.as_double = exp((double)val1.value.as_addr);
      break;
    case et_int:
      val.type = et_double;
      val.value.as_double = exp((double)val1.value.as_int);
      break;
    case et_uint:
      val.type = et_double;
      val.value.as_double = exp((double)val1.value.as_uint);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute log(<val1>) */
static struct eval_value_t
f_log(struct eval_value_t val1)
{
  struct eval_value_t val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return err_value;
    }

  /* except for float, result type is double */
  switch (val1.type)
    {
    case et_double:
      val.type = et_double;
      val.value.as_double = log(val1.value.as_double);
      break;
    case et_float:
      val.type = et_float;
      val.value.as_float = (float)log((double)val1.value.as_float);
      break;
    case et_qword:
      val.type = et_double;
      val.value.as_double = log((double)val1.value.as_qword);
      break;
    case et_sqword:
      val.type = et_double;
      val.value.as_double = log((double)val1.value.as_sqword);
      break;
    case et_addr:
      val.type = et_double;
      val.value.as_double = log((double)val1.value.as_addr);
      break;
    case et_int:
      val.type = et_double;
      val.value.as_double = log((double)val1.value.as_int);
      break;
    case et_uint:
      val.type = et_double;
      val.value.as_double = log((double)val1.value.as_uint);
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* compute val1 == 0 */
static int
f_eq_zero(struct eval_value_t val1)
{
  int val;

  /* symbols are not allowed in arithmetic expressions */
  if (val1.type == et_symbol)
    {
      eval_error = ERR_BADEXPR;
      return FALSE;
    }

  switch (val1.type)
    {
    case et_double:
      val = val1.value.as_double == 0.0;
      break;
    case et_float:
      val = val1.value.as_float == 0.0;
      break;
    case et_qword:
      val = val1.value.as_qword == 0;
      break;
    case et_sqword:
      val = val1.value.as_sqword == 0;
      break;
    case et_addr:
      val = val1.value.as_addr == 0;
      break;
    case et_uint:
      val = val1.value.as_uint == 0;
      break;
    case et_int:
      val = val1.value.as_int == 0;
      break;
    default:
      panic("bogus expression type");
    }

  return val;
}

/* evaluate the value of the numeric literal constant in ES->TOK_BUF,
   eval_err is set to a value other than ERR_NOERR if the constant cannot
   be parsed and converted to an expression value */
static struct eval_value_t                /* value of the literal constant */
constant(struct eval_state_t *es)        /* expression evaluator */
{
  struct eval_value_t val;
  int int_val;
  unsigned int uint_val;
  double double_val;
  char *endp;
  sqword_t sqword_val;
  qword_t qword_val;

#if !defined(__CYGWIN32__)
  extern int errno;
#endif

  /*
   * attempt multiple conversions, from least to most precise, using
   * the value returned when the conversion is successful
   */

  /* attempt integer conversion */
  errno = 0;
  int_val = strtol(es->tok_buf, &endp, /* parse base */0);
  if (!errno && !*endp)
    {
      /* good conversion */
      val.type = et_int;
      val.value.as_int = int_val;
      return val;
    }

  /* else, not an integer, attempt unsigned int conversion */
  errno = 0;
  uint_val = strtoul(es->tok_buf, &endp, /* parse base */0);
  if (!errno && !*endp)
    {
      /* good conversion */
      val.type = et_uint;
      val.value.as_uint = uint_val;
      return val;
    }

  /* else, not an int/uint, attempt sqword_t conversion */
  errno = 0;
  sqword_val = myatosq(es->tok_buf, &endp, /* parse base */0);
  if (!errno && !*endp)
    {
      /* good conversion */
      val.type = et_sqword;
      val.value.as_sqword = sqword_val;
      return val;
    }

  /* else, not an sqword_t, attempt qword_t conversion */
  errno = 0;
  qword_val = myatoq(es->tok_buf, &endp, /* parse base */0);
  if (!errno && !*endp)
    {
      /* good conversion */
      val.type = et_qword;
      val.value.as_qword = qword_val;
      return val;
    }

  /* else, not any type of integer, attempt double conversion (NOTE: no
     reliable float conversion is available on all machines) */
  errno = 0;
  double_val = strtod(es->tok_buf, &endp);
  if (!errno && !*endp)
    {
      /* good conversion */
      val.type = et_double;
      val.value.as_double = double_val;
      return val;
    }

  /* else, not a double value, therefore, could not convert constant,
     declare an error */
  eval_error = ERR_BADCONST;
  return err_value;
}

/* evaluate an expression factor, eval_err will indicate it any
   expression evaluation occurs */
static struct eval_value_t                /* value of factor */
factor(struct eval_state_t *es)                /* expression evaluator */
{
  enum eval_token_t tok;
  struct eval_value_t val;

  tok = peek_next_token(es);
  switch (tok)
    {
    case tok_oparen:
      (void)get_next_token(es);
      val = expr(es);
      if (eval_error)
        return err_value;

      tok = peek_next_token(es);
      if (tok != tok_cparen)
        {
          eval_error = ERR_UPAREN;
          return err_value;
        }
      (void)get_next_token(es);
      break;

    case tok_minus:
      /* negation operator */
      (void)get_next_token(es);
      val = factor(es);
      if (eval_error)
        return err_value;
      val = f_neg(val);
      break;

    case tok_exp:
      /* exponentiation operator */
      (void)get_next_token(es);
      val = factor(es);
      if (eval_error)
        return err_value;
      val = f_exp(val);
      break;

    case tok_log:
      /* logarithm operator */
      (void)get_next_token(es);
      val = factor(es);
      if (eval_error)
        return err_value;
      val = f_log(val);
      break;

    case tok_ident:
      (void)get_next_token(es);
      /* evaluate the identifier in TOK_BUF */
      val = es->f_eval_ident(es);
      if (eval_error)
        return err_value;
      break;

    case tok_const:
      (void)get_next_token(es);
      val = constant(es);
      if (eval_error)
        return err_value;
      break;

    default:
      eval_error = ERR_NOTERM;
      return err_value;
    }

  return val;
}

/* evaluate an expression term, eval_err will indicate it any
   expression evaluation occurs */
static struct eval_value_t                /* value to expression term */
term(struct eval_state_t *es)                /* expression evaluator */
{
  enum eval_token_t tok;
  struct eval_value_t val, val1;

  val = factor(es);
  if (eval_error)
    return err_value;

  tok = peek_next_token(es);
  switch (tok)
    {
    case tok_mult:
      (void)get_next_token(es);
      val = f_mult(val, term(es));
      if (eval_error)
        return err_value;
      break;

    case tok_div:
      (void)get_next_token(es);
      val1 = term(es);
      if (eval_error)
        return err_value;
      if (f_eq_zero(val1))
        {
          eval_error = ERR_DIV0;
          return err_value;
        }
      val = f_div(val, val1);
      break;

    default:;
    }

  return val;
}

/* evaluate an expression, eval_err will indicate it any expression
   evaluation occurs */
static struct eval_value_t                /* value of the expression */
expr(struct eval_state_t *es)                /* expression evaluator */
{
  enum eval_token_t tok;
  struct eval_value_t val;

  val = term(es);
  if (eval_error)
    return err_value;

  tok = peek_next_token(es);
  switch (tok)
    {
    case tok_plus:
      (void)get_next_token(es);
      val = f_add(val, expr(es));
      if (eval_error)
        return err_value;
      break;

    case tok_minus:
      (void)get_next_token(es);
      val = f_sub(val, expr(es));
      if (eval_error)
        return err_value;
      break;

    default:;
    }

  return val;
}

/* create an evaluator */
struct eval_state_t *                        /* expression evaluator */
eval_new(eval_ident_t f_eval_ident,        /* user ident evaluator */
         void *user_ptr)                /* user ptr passed to ident fn */
{
  struct eval_state_t *es;

  es = (struct eval_state_t*) calloc(1, sizeof(struct eval_state_t));
  if (!es)
    fatal("out of virtual memory");

  es->f_eval_ident = f_eval_ident;
  es->user_ptr = user_ptr;

  return es;
}

/* delete an evaluator */
void
eval_delete(struct eval_state_t *es)        /* evaluator to delete */
{
  free(es);
}

/* evaluate an expression, if an error occurs during evaluation, the
   global variable eval_error will be set to a value other than ERR_NOERR */
struct eval_value_t                        /* value of the expression */
eval_expr(struct eval_state_t *es,        /* expression evaluator */
          const char *p,                        /* ptr to expression string */
          const char **endp)                        /* returns ptr to 1st unused char */
{
  struct eval_value_t val;

  /* initialize the evaluator state */
  eval_error = ERR_NOERR;
  es->p = p;
  *es->tok_buf = '\0';
  es->peek_tok = tok_invalid;

  /* evaluate the expression */
  val = expr(es);

  /* return a pointer to the first character not used in the expression */
  if (endp)
    {
      if (es->peek_tok != tok_invalid)
        {
          /* did not consume peek'ed token, so return last p */
          *endp = es->lastp;
        }
      else
        *endp = es->p;
    }

  return val;
}

/* print an expression value */
void
eval_print(FILE *stream,                /* output stream */
           struct eval_value_t val)        /* expression value to print */
{
  switch (val.type)
    {
    case et_double:
      fprintf(stream, "%f [double]", val.value.as_double);
      break;
    case et_float:
      fprintf(stream, "%f [float]", (double)val.value.as_float);
      break;
    case et_qword:
      myfprintf(stream, "%lu [qword_t]", val.value.as_qword);
      break;
    case et_sqword:
      myfprintf(stream, "%ld [sqword_t]", val.value.as_sqword);
      break;
    case et_addr:
      myfprintf(stream, "0x%p [md_addr_t]", val.value.as_addr);
      break;
    case et_uint:
      fprintf(stream, "%u [uint]", val.value.as_uint);
      break;
    case et_int:
      fprintf(stream, "%d [int]", val.value.as_int);
      break;
    case et_symbol:
      fprintf(stream, "\"%s\" [symbol]", val.value.as_symbol);
      break;
    default:
      panic("bogus expression type");
    }
}

//#ifdef __cplusplus
//}
//#endif
