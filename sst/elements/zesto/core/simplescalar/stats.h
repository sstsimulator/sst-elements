/* stats.h - statistical package interfaces */

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

#ifndef STAT_H
#define STAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "host.h"
#include "machine.h"
#include "eval.h"

/*
 * The stats package is a uniform module for handling statistical variables,
 * including counters, distributions, and expressions.  The user must first
 * create a stats database using stat_new(), then statical counters are added
 * to the database using the *_reg_*() functions.  Interfaces are included to
 * allocate and manipulate distributions (histograms) and general expression
 * of other statistical variables constants.  Statistical variables can be
 * located by name using stat_find_stat().  And, statistics can be print in
 * a highly standardized and stylized fashion using stat_print_stats().
 */

/* stat variable classes */
enum stat_class_t {
  sc_int = 0,			/* integer stat */
  sc_uint,			/* unsigned integer stat */
  sc_qword,			/* qword integer stat */
  sc_sqword,			/* signed qword integer stat */
  sc_float,			/* single-precision FP stat */
  sc_double,			/* double-precision FP stat */
  sc_dist,			/* array distribution stat */
  sc_sdist,			/* sparse array distribution stat */
  sc_formula,			/* stat expression formula */
  sc_string,    /* named string */
  sc_note,      /* comment/note string */
  sc_NUM
};

/* sparse array distributions are implemented with a hash table */
#define HTAB_SZ			1024
#define HTAB_HASH(I)		((((I) >> 8) ^ (I)) & (HTAB_SZ - 1))

/* hash table bucket definition */
struct bucket_t {
  struct bucket_t *next;	/* pointer to the next bucket */
  md_addr_t index;		/* bucket index - as large as an addr */
  unsigned int count;		/* bucket count */
};

/* forward declaration */
struct stat_stat_t;

/* enable distribution components:  index, count, probability, cumulative */
#define PF_COUNT		0x0001
#define PF_PDF			0x0002
#define PF_CDF			0x0004
#define PF_ALL			(PF_COUNT|PF_PDF|PF_CDF)

/* user-defined print function, if this option is selected, a function of this
   form is called for each bucket in the distribution, in ascending index
   order */
typedef void
(*print_fn_t)(struct stat_stat_t *stat,	/* the stat variable being printed */
	      md_addr_t index,		/* entry index to print */
	      int count,		/* entry count */
	      double sum,		/* cumulative sum */
	      double total);		/* total count for distribution */

/* statistical variable definition */
struct stat_stat_t {
  struct stat_stat_t *next;	/* pointer to next stat in database list */
  const char *name;			/* stat name */
  const char *desc;			/* stat description */
  const char *format;			/* stat output print format */
  enum stat_class_t sc;		/* stat class */
  int print_me;  /* set to FALSE to avoid printing */
  union stat_variant_t {
    /* sc == sc_int */
    struct stat_for_int_t {
      int *var;			/* integer stat variable */
      int init_val;		/* initial integer value */
    } for_int;
    /* sc == sc_uint */
    struct stat_for_uint_t {
      unsigned int *var;	/* unsigned integer stat variable */
      unsigned int init_val;	/* initial unsigned integer value */
    } for_uint;
    /* sc == sc_qword */
    struct stat_for_qword_t {
      qword_t *var;		/* qword integer stat variable */
      qword_t init_val;		/* qword integer value */
    } for_qword;
    /* sc == sc_sqword */
    struct stat_for_sqword_t {
      sqword_t *var;		/* signed qword integer stat variable */
      sqword_t init_val;	/* signed qword integer value */
    } for_sqword;
    /* sc == sc_float */
    struct stat_for_float_t {
      float *var;		/* float stat variable */
      float init_val;		/* initial float value */
    } for_float;
    /* sc == sc_double */
    struct stat_for_double_t {
      double *var;		/* double stat variable */
      double init_val;		/* initial double value */
    } for_double;
    /* sc == sc_dist */
    struct stat_for_dist_t {
      unsigned int init_val;	/* initial dist value */
      unsigned int *arr;	/* non-sparse array pointer */
      unsigned int arr_sz;	/* array size */
      unsigned int bucket_sz;	/* array bucket size */
      int pf;			/* printables */
      const char **imap;		/* index -> string map */
      print_fn_t print_fn;	/* optional user-specified print fn */
      unsigned int overflows;	/* total overflows in stat_add_samples() */
    } for_dist;
    /* sc == sc_sdist */
    struct stat_for_sdist_t {
      unsigned int init_val;	/* initial dist value */
      struct bucket_t **sarr;	/* sparse array pointer */
      int pf;			/* printables */
      print_fn_t print_fn;	/* optional user-specified print fn */
    } for_sdist;
    /* sc == sc_formula */
    struct stat_for_formula_t {
      const char *formula;		/* stat formula, see eval.h for format */
    } for_formula;
    struct stat_for_string_t {
      const char *string;		/* stat string */
    } for_string;
    struct stat_for_note_t {
      const char *note;		/* stat note/comment */
    } for_note;
  } variant;
};

/* statistical database */
struct stat_sdb_t {
  struct stat_stat_t *stats;		/* list of stats in database */
  struct eval_state_t *evaluator;	/* an expression evaluator */
};

/* evaluate a stat as an expression */
struct eval_value_t
stat_eval_ident(struct eval_state_t *es);/* expression stat to evaluate */

/* create a new stats database */
struct stat_sdb_t *stat_new(void);

/* delete a stats database */
void
stat_delete(struct stat_sdb_t *sdb);	/* stats database */

/* register an integer statistical variable */
struct stat_stat_t *
stat_reg_int(struct stat_sdb_t *sdb,	/* stat database */
       int print_me,
	     const char *name,		/* stat variable name */
	     const char *desc,		/* stat variable description */
	     int *var,			/* stat variable */
	     int init_val,		/* stat variable initial value */
	     const char *format);		/* optional variable output format */

/* register an unsigned integer statistical variable */
struct stat_stat_t *
stat_reg_uint(struct stat_sdb_t *sdb,	/* stat database */
        int print_me,
	      const char *name,		/* stat variable name */
	      const char *desc,		/* stat variable description */
	      unsigned int *var,	/* stat variable */
	      unsigned int init_val,	/* stat variable initial value */
	      const char *format);		/* optional variable output format */

/* register a qword integer statistical variable */
struct stat_stat_t *
stat_reg_qword(struct stat_sdb_t *sdb,	/* stat database */
         int print_me,
	       const char *name,		/* stat variable name */
	       const char *desc,		/* stat variable description */
	       qword_t *var,		/* stat variable */
	       qword_t init_val,	/* stat variable initial value */
	       const char *format);		/* optional variable output format */

/* register a signed qword integer statistical variable */
struct stat_stat_t *
stat_reg_sqword(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
		const char *name,		/* stat variable name */
		const char *desc,		/* stat variable description */
		sqword_t *var,		/* stat variable */
		sqword_t init_val,	/* stat variable initial value */
		const char *format);		/* optional variable output format */

/* register a float statistical variable */
struct stat_stat_t *
stat_reg_float(struct stat_sdb_t *sdb,	/* stat database */
         int print_me,
	       const char *name,		/* stat variable name */
	       const char *desc,		/* stat variable description */
	       float *var,		/* stat variable */
	       float init_val,		/* stat variable initial value */
	       const char *format);		/* optional variable output format */

/* register a double statistical variable */
struct stat_stat_t *
stat_reg_double(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
		const char *name,		/* stat variable name */
		const char *desc,		/* stat variable description */
		double *var,		/* stat variable */
		double init_val,	/* stat variable initial value */
		const char *format);		/* optional variable output format */

/* create an array distribution (w/ fixed size buckets) in stat database SDB,
   the array distribution has ARR_SZ buckets with BUCKET_SZ indicies in each
   bucked, PF specifies the distribution components to print for optional
   format FORMAT; the indicies may be optionally replaced with the strings
   from IMAP, or the entire distribution can be printed with the optional
   user-specified print function PRINT_FN */
struct stat_stat_t *
stat_reg_dist(struct stat_sdb_t *sdb,	/* stat database */
	      const char *name,		/* stat variable name */
	      const char *desc,		/* stat variable description */
	      unsigned int init_val,	/* dist initial value */
	      unsigned int arr_sz,	/* array size */
	      unsigned int bucket_sz,	/* array bucket size */
	      int pf,			/* print format, use PF_* defs */
	      const char *format,		/* optional variable output format */
	      const char **imap,		/* optional index -> string map */
	      print_fn_t print_fn);	/* optional user print function */

/* create a sparse array distribution in stat database SDB, while the sparse
   array consumes more memory per bucket than an array distribution, it can
   efficiently map any number of indicies from 0 to 2^32-1, PF specifies the
   distribution components to print for optional format FORMAT; the indicies
   may be optionally replaced with the strings from IMAP, or the entire
   distribution can be printed with the optional user-specified print function
   PRINT_FN */
struct stat_stat_t *
stat_reg_sdist(struct stat_sdb_t *sdb,	/* stat database */
	       const char *name,		/* stat variable name */
	       const char *desc,		/* stat variable description */
	       unsigned int init_val,	/* dist initial value */
	       int pf,			/* print format, use PF_* defs */
	       const char *format,		/* optional variable output format */
	       print_fn_t print_fn);	/* optional user print function */

/* add NSAMPLES to array or sparse array distribution STAT */
void
stat_add_samples(struct stat_stat_t *stat,/* stat database */
		 md_addr_t index,	/* distribution index of samples */
		 int nsamples);		/* number of samples to add to dist */

/* add a single sample to array or sparse array distribution STAT */
void
stat_add_sample(struct stat_stat_t *stat,/* stat variable */
		md_addr_t index);	/* index of sample */

/* register a double statistical formula, the formula is evaluated when the
   statistic is printed, the formula expression may reference any registered
   statistical variable and, in addition, the standard operators '(', ')', '+',
   '-', '*', and '/', and literal (i.e., C-format decimal, hexidecimal, and
   octal) constants are also supported; NOTE: all terms are immediately
   converted to double values and the result is a double value, see eval.h
   for more information on formulas */
struct stat_stat_t *
stat_reg_formula(struct stat_sdb_t *sdb,/* stat database */
       int print_me,
		 const char *name,		/* stat variable name */
		 const char *desc,		/* stat variable description */
		 const char *formula,		/* formula expression */
		 const char *format);		/* optional variable output format */

struct stat_stat_t *
stat_reg_string(struct stat_sdb_t *sdb,	/* stat database */
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    const char *var,			/* stat variable */
    const char *format);	/* optional variable output format */

/* register a stat output note/comment */
struct stat_stat_t *
stat_reg_note(struct stat_sdb_t *sdb,	/* stat database */
	            const char *note);

/* print the value of stat variable STAT */
void
stat_print_stat(struct stat_sdb_t *sdb,	/* stat database */
		struct stat_stat_t *stat,/* stat variable */
		FILE *fd);		/* output stream */

/* print the value of all stat variables in stat database SDB */
void
stat_print_stats(struct stat_sdb_t *sdb,/* stat database */
		 FILE *fd);		/* output stream */


/* find a stat variable, returns NULL if it is not found */
struct stat_stat_t *
stat_find_stat(struct stat_sdb_t *sdb,	/* stat database */
	       const char *stat_name);	/* stat name */
	       

/* print a sparse array distribution */
void
print_sdist(struct stat_stat_t *stat,	/* stat variable */
	    FILE *fd);			/* output stream */

/* print an array distribution */
void
print_dist(struct stat_stat_t *stat,	/* stat variable */
	   FILE *fd);			/* output stream */

#ifdef __cplusplus
}
#endif

#endif /* STAT_H */





