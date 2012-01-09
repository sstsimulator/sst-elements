/* stats.c - statistical package routines */
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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "eval.h"
#include "stats.h"

/* evaluate a stat as an expression */
  struct eval_value_t
stat_eval_ident(struct eval_state_t *es)/* an expression evaluator */
{
  struct stat_sdb_t *sdb = (struct stat_sdb_t*) es->user_ptr;
  struct stat_stat_t *stat;
  static struct eval_value_t err_value = { et_int, { 0 } };
  struct eval_value_t val;

  /* locate the stat variable */
  for (stat = sdb->stats; stat != NULL; stat = stat->next)
  {
    if (!strcmp(stat->name, es->tok_buf))
    {
      /* found it! */
      break;
    }
  }
  if (!stat)
  {
    /* could not find stat variable */
    eval_error = ERR_UNDEFVAR;
    return err_value;
  }
  /* else, return the value of stat */

  /* convert the stat variable value to a typed expression value */
  switch (stat->sc)
  {
    case sc_int:
      val.type = et_int;
      val.value.as_int = *stat->variant.for_int.var;
      break;
    case sc_uint:
      val.type = et_uint;
      val.value.as_uint = *stat->variant.for_uint.var;
      break;
    case sc_qword:
      /* FIXME: cast to double, eval package doesn't support long long's */
      val.type = et_double;
      val.value.as_double = (double)*stat->variant.for_qword.var;
      break;
    case sc_sqword:
      /* FIXME: cast to double, eval package doesn't support long long's */
      val.type = et_double;
      val.value.as_double = (double)*stat->variant.for_sqword.var;
      break;
    case sc_float:
      val.type = et_float;
      val.value.as_float = *stat->variant.for_float.var;
      break;
    case sc_double:
      val.type = et_double;
      val.value.as_double = *stat->variant.for_double.var;
      break;
    case sc_dist:
    case sc_sdist:
      fatal("stat distributions not allowed in formula expressions");
      break;
    case sc_formula:
      {
        /* instantiate a new evaluator to avoid recursion problems */
        struct eval_state_t *es = eval_new(stat_eval_ident, sdb);
        const char *endp;

        val = eval_expr(es, stat->variant.for_formula.formula, &endp);
        if (eval_error != ERR_NOERR || *endp != '\0')
        {
          /* pass through eval_error */
          val = err_value;
        }
        /* else, use value returned */
        eval_delete(es);
      }
      break;
    case sc_note:
      val.type = et_symbol;
      val.value.as_symbol = stat->variant.for_note.note;
      break;
    default:
      panic("bogus stat class");
  }

  return val;
}

/* create a new stats database */
  struct stat_sdb_t *
stat_new(void)
{
  struct stat_sdb_t *sdb;

  sdb = (struct stat_sdb_t *)calloc(1, sizeof(struct stat_sdb_t));
  if (!sdb)
    fatal("out of virtual memory");

  sdb->stats = NULL;
  sdb->evaluator = eval_new(stat_eval_ident, sdb);

  return sdb;
}

/* delete a stats database */
  void
stat_delete(struct stat_sdb_t *sdb)	/* stats database */
{
  int i;
  struct stat_stat_t *stat, *stat_next;
  struct bucket_t *bucket, *bucket_next;

  /* free all individual stat variables */
  for (stat = sdb->stats; stat != NULL; stat = stat_next)
  {
    stat_next = stat->next;
    stat->next = NULL;

    /* free stat */
    switch (stat->sc)
    {
      case sc_int:
      case sc_uint:
      case sc_qword:
      case sc_sqword:
      case sc_float:
      case sc_double:
        /* no other storage to deallocate */
        break;
      case sc_formula:
        free((void*)stat->variant.for_formula.formula);
        break;
      case sc_note:
        free((void*)stat->variant.for_note.note);
        break;
      case sc_dist:
        /* free distribution array */
        free(stat->variant.for_dist.arr);
        stat->variant.for_dist.arr = NULL;
        break;
      case sc_sdist:
        /* free all hash table buckets */
        for (i=0; i<HTAB_SZ; i++)
        {
          for (bucket = stat->variant.for_sdist.sarr[i];
              bucket != NULL;
              bucket = bucket_next)
          {
            bucket_next = bucket->next;
            bucket->next = NULL;
            free(bucket);
          }
          stat->variant.for_sdist.sarr[i] = NULL;
        }
        /* free hash table array */
        free(stat->variant.for_sdist.sarr);
        stat->variant.for_sdist.sarr = NULL;
        break;
      default:
        panic("bogus stat class");
    }
    /* free stat variable record */
    free(stat);
  }
  sdb->stats = NULL;
  eval_delete(sdb->evaluator);
  sdb->evaluator = NULL;
  free(sdb);
}

/* add stat variable STAT to stat database SDB */
  static void
add_stat(struct stat_sdb_t *sdb,	/* stat database */
    struct stat_stat_t *stat)	/* stat variable */
{
  struct stat_stat_t *elt, *prev;

  /* append at end of stat database list */
  for (prev=NULL, elt=sdb->stats; elt != NULL; prev=elt, elt=elt->next)
    /* nada */;

  /* append stat to stats chain */
  if (prev != NULL)
    prev->next = stat;
  else /* prev == NULL */
    sdb->stats = stat;
  stat->next = NULL;
}

/* register a string statistical variable */
  struct stat_stat_t *
stat_reg_string(struct stat_sdb_t *sdb,	/* stat database */
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    const char *var,			/* stat variable */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = TRUE;
  stat->format = format ? format : "%12s";
  stat->sc = sc_string;
  stat->variant.for_string.string = var;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  return stat;
}

/* register an integer statistical variable */
  struct stat_stat_t *
stat_reg_int(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    int *var,			/* stat variable */
    int init_val,		/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12d";
  stat->sc = sc_int;
  stat->variant.for_int.var = var;
  stat->variant.for_int.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* register an unsigned integer statistical variable */
  struct stat_stat_t *
stat_reg_uint(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    unsigned int *var,	/* stat variable */
    unsigned int init_val,	/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12u";
  stat->sc = sc_uint;
  stat->variant.for_uint.var = var;
  stat->variant.for_uint.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* register a qword integer statistical variable */
  struct stat_stat_t *
stat_reg_qword(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    qword_t *var,		/* stat variable */
    qword_t init_val,		/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12lu";
  stat->sc = sc_qword;
  stat->variant.for_qword.var = var;
  stat->variant.for_qword.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* register a signed qword integer statistical variable */
  struct stat_stat_t *
stat_reg_sqword(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    sqword_t *var,		/* stat variable */
    sqword_t init_val,	/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12ld";
  stat->sc = sc_sqword;
  stat->variant.for_sqword.var = var;
  stat->variant.for_sqword.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* register a float statistical variable */
  struct stat_stat_t *
stat_reg_float(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    float *var,		/* stat variable */
    float init_val,		/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12.4f";
  stat->sc = sc_float;
  stat->variant.for_float.var = var;
  stat->variant.for_float.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* register a double statistical variable */
  struct stat_stat_t *
stat_reg_double(struct stat_sdb_t *sdb,	/* stat database */
    int print_me,
    const char *name,		/* stat variable name */
    const char *desc,		/* stat variable description */
    double *var,		/* stat variable */
    double init_val,	/* stat variable initial value */
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12.4f";
  stat->sc = sc_double;
  stat->variant.for_double.var = var;
  stat->variant.for_double.init_val = init_val;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  *var = init_val;

  return stat;
}

/* create an array distribution (w/ fixed size buckets) in stat database SDB,
   the array distribution has ARR_SZ buckets with BUCKET_SZ indicies in each
   bucked, PF specifies the distribution components to print for optional
   format FORMAT; the indicies may be optionally replaced with the strings from
   IMAP, or the entire distribution can be printed with the optional
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
    print_fn_t print_fn)	/* optional user print function */
{
  unsigned int i;
  struct stat_stat_t *stat;
  unsigned int *arr;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = TRUE;
  stat->format = format ? format : NULL;
  stat->sc = sc_dist;
  stat->variant.for_dist.init_val = init_val;
  stat->variant.for_dist.arr_sz = arr_sz;
  stat->variant.for_dist.bucket_sz = bucket_sz;
  stat->variant.for_dist.pf = pf;
  stat->variant.for_dist.imap = imap;
  stat->variant.for_dist.print_fn = print_fn;
  stat->variant.for_dist.overflows = 0;

  arr = (unsigned int *)calloc(arr_sz, sizeof(unsigned int));
  if (!arr)
    fatal("out of virtual memory");
  stat->variant.for_dist.arr = arr;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  /* initialize stat */
  for (i=0; i < arr_sz; i++)
    arr[i] = init_val;

  return stat;
}

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
    print_fn_t print_fn)	/* optional user print function */
{
  struct stat_stat_t *stat;
  struct bucket_t **sarr;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = TRUE;
  stat->format = format ? format : NULL;
  stat->sc = sc_sdist;
  stat->variant.for_sdist.init_val = init_val;
  stat->variant.for_sdist.pf = pf;
  stat->variant.for_sdist.print_fn = print_fn;

  /* allocate hash table */
  sarr = (struct bucket_t **)calloc(HTAB_SZ, sizeof(struct bucket_t *));
  if (!sarr)
    fatal("out of virtual memory");
  stat->variant.for_sdist.sarr = sarr;

  /* link onto SDB chain */
  add_stat(sdb, stat);

  return stat;
}

/* add NSAMPLES to array or sparse array distribution STAT */
  void
stat_add_samples(struct stat_stat_t *stat,/* stat database */
    md_addr_t index,	/* distribution index of samples */
    int nsamples)		/* number of samples to add to dist */
{
  switch (stat->sc)
  {
    case sc_dist:
      {
        unsigned int i = 0;

        /* compute array index */
        i = index / stat->variant.for_dist.bucket_sz;

        /* check for overflow */
        if (i >= stat->variant.for_dist.arr_sz)
          stat->variant.for_dist.overflows += nsamples;
        else
          stat->variant.for_dist.arr[i] += nsamples;
      }
      break;
    case sc_sdist:
      {
        struct bucket_t *bucket;
        int hash = HTAB_HASH(index);

        if (hash < 0 || hash >= HTAB_SZ)
          panic("hash table index overflow");

        /* find bucket */
        for (bucket = stat->variant.for_sdist.sarr[hash];
            bucket != NULL;
            bucket = bucket->next)
        {
          if (bucket->index == index)
            break;
        }
        if (!bucket)
        {
          /* add a new sample bucket */
          bucket = (struct bucket_t *)calloc(1, sizeof(struct bucket_t));
          if (!bucket)
            fatal("out of virtual memory");
          bucket->next = stat->variant.for_sdist.sarr[hash];
          stat->variant.for_sdist.sarr[hash] = bucket;
          bucket->index = index;
          bucket->count = stat->variant.for_sdist.init_val;
        }
        bucket->count += nsamples;
      }
      break;
    default:
      panic("stat variable is not an array distribution");
  }
}

/* add a single sample to array or sparse array distribution STAT */
  void
stat_add_sample(struct stat_stat_t *stat,/* stat variable */
    md_addr_t index)	/* index of sample */
{
  stat_add_samples(stat, index, 1);
}

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
    const char *format)		/* optional variable output format */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = mystrdup(name);
  stat->desc = mystrdup(desc);
  stat->print_me = print_me;
  stat->format = format ? format : "%12.4f";
  stat->sc = sc_formula;
  stat->variant.for_formula.formula = mystrdup(formula);

  /* link onto SDB chain */
  add_stat(sdb, stat);

  return stat;
}

/* register an integer statistical variable */
  struct stat_stat_t *
stat_reg_note(struct stat_sdb_t *sdb,	/* stat database */
              const char *note) /* string to print */
{
  struct stat_stat_t *stat;

  stat = (struct stat_stat_t *)calloc(1, sizeof(struct stat_stat_t));
  if (!stat)
    fatal("out of virtual memory");

  stat->name = "--not a stat--";
  stat->desc = "--not a stat--";
  stat->print_me = TRUE;
  stat->format = "--not a stat--";
  stat->sc = sc_note;
  stat->variant.for_note.note = mystrdup(note);

  /* link onto SDB chain */
  add_stat(sdb, stat);

  return stat;
}


/* compare two indicies in a sparse array hash table, used by qsort() */
  static int
compare_fn(void *p1, void *p2)
{
  struct bucket_t **pb1 = (struct bucket_t**) p1, **pb2 = (struct bucket_t**) p2;

  /* compare indices */
  if ((*pb1)->index < (*pb2)->index)
    return -1;
  else if ((*pb1)->index > (*pb2)->index)
    return 1;
  else /* ((*pb1)->index == (*pb2)->index) */
    return 0;
}

/* print an array distribution */
  void
print_dist(struct stat_stat_t *stat,	/* stat variable */
    FILE *fd)			/* output stream */
{
  unsigned int i, bcount, imax, imin;
  double btotal, bsum, bvar, bavg, bsqsum;
  int pf = stat->variant.for_dist.pf;

  /* count and sum entries */
  bcount = 0; btotal = 0.0; bvar = 0.0; bsqsum = 0.0;
  imax = 0; imin = UINT_MAX;
  for (i=0; i<stat->variant.for_dist.arr_sz; i++)
  {
    bcount++;
    btotal += stat->variant.for_dist.arr[i];
    /* on-line variance computation, tres cool, no!?! */
    bsqsum += ((double)stat->variant.for_dist.arr[i] *
        (double)stat->variant.for_dist.arr[i]);
    bavg = btotal / MAX((double)bcount, 1.0);
    bvar = (bsqsum - ((double)bcount * bavg * bavg)) / 
      (double)(((bcount - 1) > 0) ? (bcount - 1) : 1);
  }

  /* print header */
  fprintf(fd, "\n");
  fprintf(fd, "%-28s # %s\n", stat->name, stat->desc);
  fprintf(fd, "%s.array_size = %u\n",
      stat->name, stat->variant.for_dist.arr_sz);
  fprintf(fd, "%s.bucket_size = %u\n",
      stat->name, stat->variant.for_dist.bucket_sz);

  fprintf(fd, "%s.count = %u\n", stat->name, bcount);
  fprintf(fd, "%s.total = %.0f\n", stat->name, btotal);
  if (bcount > 0)
  {
    fprintf(fd, "%s.imin = %u\n", stat->name, 0U);
    fprintf(fd, "%s.imax = %u\n", stat->name, bcount);
  }
  else
  {
    fprintf(fd, "%s.imin = %d\n", stat->name, -1);
    fprintf(fd, "%s.imax = %d\n", stat->name, -1);
  }
  fprintf(fd, "%s.average = %8.4f\n", stat->name, btotal/MAX(bcount, 1.0));
  fprintf(fd, "%s.std_dev = %8.4f\n", stat->name, sqrt(bvar));
  fprintf(fd, "%s.overflows = %u\n",
      stat->name, stat->variant.for_dist.overflows);

  fprintf(fd, "# pdf == prob dist fn, cdf == cumulative dist fn\n");
  fprintf(fd, "# %14s ", "index");
  if (pf & PF_COUNT)
    fprintf(fd, "%10s ", "count");
  if (pf & PF_PDF)
    fprintf(fd, "%6s ", "pdf");
  if (pf & PF_CDF)
    fprintf(fd, "%6s ", "cdf");
  fprintf(fd, "\n");

  fprintf(fd, "%s.start_dist\n", stat->name);

  if (bcount > 0)
  {
    /* print the array */
    bsum = 0.0;
    for (i=0; i<bcount; i++)
    {
      bsum += (double)stat->variant.for_dist.arr[i];
      if (stat->variant.for_dist.print_fn)
      {
        stat->variant.for_dist.print_fn(stat,
            i,
            stat->variant.for_dist.arr[i],
            bsum,
            btotal);
      }
      else
      {
        if (stat->format == NULL)
        {
          if (stat->variant.for_dist.imap)
            fprintf(fd, "%-16s ", stat->variant.for_dist.imap[i]);
          else
            fprintf(fd, "%16u ",
                i * stat->variant.for_dist.bucket_sz);
          if (pf & PF_COUNT)
            fprintf(fd, "%10u ", stat->variant.for_dist.arr[i]);
          if (pf & PF_PDF)
            fprintf(fd, "%6.2f ",
                (double)stat->variant.for_dist.arr[i] /
                MAX(btotal, 1.0) * 100.0);
          if (pf & PF_CDF)
            fprintf(fd, "%6.2f ", bsum/MAX(btotal, 1.0) * 100.0);
        }
        else
        {
          if (pf == (PF_COUNT|PF_PDF|PF_CDF))
          {
            if (stat->variant.for_dist.imap)
              fprintf(fd, stat->format,
                  stat->variant.for_dist.imap[i],
                  stat->variant.for_dist.arr[i],
                  (double)stat->variant.for_dist.arr[i] /
                  MAX(btotal, 1.0) * 100.0,
                  bsum/MAX(btotal, 1.0) * 100.0);
            else
              fprintf(fd, stat->format,
                  i * stat->variant.for_dist.bucket_sz,
                  stat->variant.for_dist.arr[i],
                  (double)stat->variant.for_dist.arr[i] /
                  MAX(btotal, 1.0) * 100.0,
                  bsum/MAX(btotal, 1.0) * 100.0);
          }
          else
            fatal("distribution format not yet implemented");
        }
        fprintf(fd, "\n");

      }
    }
  }

  fprintf(fd, "%s.end_dist\n", stat->name);
}

/* print a sparse array distribution */
  void
print_sdist(struct stat_stat_t *stat,	/* stat variable */
    FILE *fd)			/* output stream */
{
  unsigned int i, bcount;
  md_addr_t imax, imin;
  double btotal, bsum, bvar, bavg, bsqsum;
  struct bucket_t *bucket;
  int pf = stat->variant.for_sdist.pf;

  /* count and sum entries */
  bcount = 0; btotal = 0.0; bvar = 0.0; bsqsum = 0.0;
  imax = 0; imin = UINT_MAX;
  for (i=0; i<HTAB_SZ; i++)
  {
    for (bucket = stat->variant.for_sdist.sarr[i];
        bucket != NULL;
        bucket = bucket->next)
    {
      bcount++;
      btotal += bucket->count;
      /* on-line variance computation, tres cool, no!?! */
      bsqsum += ((double)bucket->count * (double)bucket->count);
      bavg = btotal / (double)bcount;
      bvar = (bsqsum - ((double)bcount * bavg * bavg)) / 
        (double)(((bcount - 1) > 0) ? (bcount - 1) : 1);
      if (bucket->index < imin)
        imin = bucket->index;
      if (bucket->index > imax)
        imax = bucket->index;
    }
  }

  /* print header */

  fprintf(fd, "\n");
  fprintf(fd, "%-28s # %s\n", stat->name, stat->desc);
  fprintf(fd, "%s.count = %u\n", stat->name, bcount);
  fprintf(fd, "%s.total = %.0f\n", stat->name, btotal);
  if (bcount > 0)
  {
    myfprintf(fd, "%s.imin = 0x%p\n", stat->name, imin);
    myfprintf(fd, "%s.imax = 0x%p\n", stat->name, imax);
  }
  else
  {
    fprintf(fd, "%s.imin = %d\n", stat->name, -1);
    fprintf(fd, "%s.imax = %d\n", stat->name, -1);
  }
  fprintf(fd, "%s.average = %8.4f\n", stat->name, btotal/bcount);
  fprintf(fd, "%s.std_dev = %8.4f\n", stat->name, sqrt(bvar));
  fprintf(fd, "%s.overflows = 0\n", stat->name);

  fprintf(fd, "# pdf == prob dist fn, cdf == cumulative dist fn\n");
  fprintf(fd, "# %14s ", "index");
  if (pf & PF_COUNT)
    fprintf(fd, "%10s ", "count");
  if (pf & PF_PDF)
    fprintf(fd, "%6s ", "pdf");
  if (pf & PF_CDF)
    fprintf(fd, "%6s ", "cdf");
  fprintf(fd, "\n");

  fprintf(fd, "%s.start_dist\n", stat->name);

  if (bcount > 0)
  {
    unsigned int bindex;
    struct bucket_t **barr;

    /* collect all buckets */
    barr = (struct bucket_t **)calloc(bcount, sizeof(struct bucket_t *));
    if (!barr)
      fatal("out of virtual memory");
    for (bindex=0,i=0; i<HTAB_SZ; i++)
    {
      for (bucket = stat->variant.for_sdist.sarr[i];
          bucket != NULL;
          bucket = bucket->next)
      {
        barr[bindex++] = bucket;
      }
    }

    /* sort the array by index */
    qsort(barr, bcount, sizeof(struct bucket_t *), (int (*)(const void*, const void*))compare_fn);

    /* print the array */
    bsum = 0.0;
    for (i=0; i<bcount; i++)
    {
      bsum += (double)barr[i]->count;
      if (stat->variant.for_sdist.print_fn)
      {
        stat->variant.for_sdist.print_fn(stat,
            barr[i]->index,
            barr[i]->count,
            bsum,
            btotal);
      }
      else
      {
        if (stat->format == NULL)
        {
          myfprintf(fd, "0x%p ", barr[i]->index);
          if (pf & PF_COUNT)
            fprintf(fd, "%10u ", barr[i]->count);
          if (pf & PF_PDF)
            fprintf(fd, "%6.2f ",
                (double)barr[i]->count/MAX(btotal, 1.0) * 100.0);
          if (pf & PF_CDF)
            fprintf(fd, "%6.2f ", bsum/MAX(btotal, 1.0) * 100.0);
        }
        else
        {
          if (pf == (PF_COUNT|PF_PDF|PF_CDF))
          {
            myfprintf(fd, stat->format,
                barr[i]->index, barr[i]->count,
                (double)barr[i]->count/MAX(btotal, 1.0)*100.0,
                bsum/MAX(btotal, 1.0) * 100.0);
          }
          else if (pf == (PF_COUNT|PF_PDF))
          {
            myfprintf(fd, stat->format,
                barr[i]->index, barr[i]->count,
                (double)barr[i]->count/MAX(btotal, 1.0)*100.0);
          }
          else if (pf == PF_COUNT)
          {
            myfprintf(fd, stat->format,
                barr[i]->index, barr[i]->count);
          }
          else
            fatal("distribution format not yet implemented");
        }
        fprintf(fd, "\n");
      }
    }

    /* all done, release bucket pointer array */
    free(barr);
  }

//  fprintf(fd, "%s.end_dist\n", stat->name);
}

/* print the value of stat variable STAT */
  void
stat_print_stat(struct stat_sdb_t *sdb,	/* stat database */
    struct stat_stat_t *stat,/* stat variable */
    FILE *fd)		/* output stream */
{
  struct eval_value_t val;

  switch (stat->sc)
  {
    case sc_int:
      fprintf(fd, "%-28s ", stat->name);
      myfprintf(fd, stat->format, *stat->variant.for_int.var);
      fprintf(fd, " # %s", stat->desc);
      break;
    case sc_uint:
      fprintf(fd, "%-28s ", stat->name);
      myfprintf(fd, stat->format, *stat->variant.for_uint.var);
      fprintf(fd, " # %s", stat->desc);
      break;
    case sc_qword:
      {
        char buf[128];

        fprintf(fd, "%-28s ", stat->name);
        mysprintf(buf, stat->format, *stat->variant.for_qword.var);
        fprintf(fd, "%s # %s", buf, stat->desc);
      }
      break;
    case sc_sqword:
      {
        char buf[128];

        fprintf(fd, "%-28s ", stat->name);
        mysprintf(buf, stat->format, *stat->variant.for_sqword.var);
        fprintf(fd, "%s # %s", buf, stat->desc);
      }
      break;
    case sc_float:
      fprintf(fd, "%-28s ", stat->name);
      myfprintf(fd, stat->format, (double)*stat->variant.for_float.var);
      fprintf(fd, " # %s", stat->desc);
      break;
    case sc_double:
      fprintf(fd, "%-28s ", stat->name);
      myfprintf(fd, stat->format, *stat->variant.for_double.var);
      fprintf(fd, " # %s", stat->desc);
      break;
    case sc_dist:
      print_dist(stat, fd);
      break;
    case sc_sdist:
      print_sdist(stat, fd);
      break;
    case sc_formula:
      {
        /* instantiate a new evaluator to avoid recursion problems */
        struct eval_state_t *es = eval_new(stat_eval_ident, sdb);
        const char *endp;

        fprintf(fd, "%-28s ", stat->name);
        val = eval_expr(es, stat->variant.for_formula.formula, &endp);
        if (eval_error != ERR_NOERR || *endp != '\0')
          fprintf(fd, "<error: %s>", eval_err_str[eval_error]);
        else
          myfprintf(fd, stat->format, eval_as_double(val));
        fprintf(fd, " # %s", stat->desc);

        /* done with the evaluator */
        eval_delete(es);
      }
      break;
    case sc_string:
      fprintf(fd, "%-28s ", stat->name);
      myfprintf(fd, stat->format, stat->variant.for_string.string);
      fprintf(fd, " # %s", stat->desc);
      break;
    case sc_note:
      fprintf(fd,"%s",stat->variant.for_note.note);
      break;
    default:
      panic("bogus stat class");
  }
  fprintf(fd, "\n");
}

/* print the value of all stat variables in stat database SDB */
  void
stat_print_stats(struct stat_sdb_t *sdb,/* stat database */
    FILE *fd)		/* output stream */
{
  struct stat_stat_t *stat;

  if (!sdb)
  {
    /* no stats */
    return;
  }

  for (stat=sdb->stats; stat != NULL; stat=stat->next)
  {
    if(stat->print_me)
      stat_print_stat(sdb, stat, fd);
  }
}

/* find a stat variable, returns NULL if it is not found */
  struct stat_stat_t *
stat_find_stat(struct stat_sdb_t *sdb,	/* stat database */
    const char *stat_name)		/* stat name */
{
  struct stat_stat_t *stat;

  for (stat = sdb->stats; stat != NULL; stat = stat->next)
  {
    if (!strcmp(stat->name, stat_name))
      break;
  }
  return stat;
}








