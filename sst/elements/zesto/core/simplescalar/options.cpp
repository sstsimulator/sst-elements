/* options.c - options package routines */
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

/* ----- This file is modified for Power-API ----- */
/* Modified list:
   1. disabled duplication options check to redefine the options in order
      for the parameter synchronization between Zesto and Power-API
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <float.h>

#include "host.h"
#include "misc.h"
#include "options.h"

/* set to surpress notes */
bool opt_ignore_notes = FALSE;

/* create a new option database */
struct opt_odb_t *
opt_new(orphan_fn_t orphan_fn)	/* user-specified orphan parser */
{
  struct opt_odb_t *odb;

  odb = (struct opt_odb_t *)calloc(1, sizeof(struct opt_odb_t));
  if (!odb)
    fatal("out of virtual memory");

  odb->options = NULL;
  odb->orphan_fn = orphan_fn;
  odb->header = NULL;
  odb->notes = NULL;

  return odb;
}

/* free an option database */
void
opt_delete(struct opt_odb_t *odb)	/* option database */
{
  struct opt_opt_t *opt, *opt_next;
  struct opt_note_t *note, *note_next;

  /* free all options */
  for (opt=odb->options; opt; opt=opt_next)
    {
      opt_next = opt->next;
      opt->next = NULL;
      free(opt);
    }

  /* free all notes */
  for (note = odb->notes; note != NULL; note = note_next)
    {
      note_next = note->next;
      note->next = NULL;
      free(note);
    }
  odb->notes = NULL;

  free(odb);
}

/* add option OPT to option database ODB */
static void
add_option(struct opt_odb_t *odb,	/* option database */
	   struct opt_opt_t *opt)	/* option variable */
{
  struct opt_opt_t *elt, *prev;

  /* sanity checks on option name */
  if (opt->name[0] != '-')
    panic("option `%s' must start with a `-'", opt->name);

  /* add to end of option list */
  for (prev=NULL, elt=odb->options; elt != NULL; prev=elt, elt=elt->next)
    {
      /* sanity checks on option name */
      if (elt->name[0] == opt->name[0] && !strcmp(elt->name, opt->name))
	panic("option `%s' is multiply defined", opt->name);
    }

  if (prev != NULL)
    prev->next = opt;
  else /* prev == NULL */
    odb->options = opt;
  opt->next = NULL;
}

/* register an integer option variable */
void
opt_reg_int(struct opt_odb_t *odb,	/* option data base */
	    char *name,			/* option name */
	    char *desc,			/* option description */
	    int *var,			/* target variable */
	    int def_val,		/* default variable value */
	    int print,			/* print during `-dumpconfig'? */
	    char *format)		/* optional value print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20d";
  opt->oc = oc_int;
  opt->variant.for_int.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register an integer option list */
void
opt_reg_int_list(struct opt_odb_t *odb,/* option database */
		 char *name,		/* option name */
		 char *desc,		/* option description */
		 int *vars,		/* pointer to option array */
		 int nvars,		/* total entries in option array */
		 int *nelt,		/* number of entries parsed */
		 int *def_val,		/* default value of option array */
		 int print,		/* print during `-dumpconfig'? */
		 char *format,		/* optional user print format */
		 int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%d";
  opt->oc = oc_int;
  opt->variant.for_int.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register an long long integer option variable */
void
opt_reg_long_long(struct opt_odb_t *odb,	/* option database */
	     char *name,		/* option name */
	     char *desc,		/* option description */
	     long long *var,		/* pointer to option variable */
	     long long def_val,	/* default value of option variable */
	     int print,			/* print during `-dumpconfig'? */
	     char *format)		/* optional user print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20llu";
  opt->oc = oc_long_long;
  opt->variant.for_long_long.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register an unsigned integer option variable */
void
opt_reg_uint(struct opt_odb_t *odb,	/* option database */
	     char *name,		/* option name */
	     char *desc,		/* option description */
	     unsigned int *var,		/* pointer to option variable */
	     unsigned int def_val,	/* default value of option variable */
	     int print,			/* print during `-dumpconfig'? */
	     char *format)		/* optional user print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20u";
  opt->oc = oc_uint;
  opt->variant.for_uint.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register an unsigned integer option list */
void
opt_reg_uint_list(struct opt_odb_t *odb,/* option database */
		  char *name,		/* option name */
		  char *desc,		/* option description */
		  unsigned int *vars,	/* pointer to option array */
		  int nvars,		/* total entries in option array */
		  int *nelt,		/* number of elements parsed */
		  unsigned int *def_val,/* default value of option array */
		  int print,		/* print opt during `-dumpconfig'? */
		  char *format,		/* optional user print format */
		  int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%u";
  opt->oc = oc_uint;
  opt->variant.for_uint.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a single-precision floating point option variable */
void
opt_reg_float(struct opt_odb_t *odb,	/* option data base */
	      char *name,		/* option name */
	      char *desc,		/* option description */
	      float *var,		/* target option variable */
	      float def_val,		/* default variable value */
	      int print,		/* print during `-dumpconfig'? */
	      char *format)		/* optional value print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20.4f";
  opt->oc = oc_float;
  opt->variant.for_float.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a single-precision floating point option array */
void
opt_reg_float_list(struct opt_odb_t *odb,/* option data base */
		   char *name,		/* option name */
		   char *desc,		/* option description */
		   float *vars,		/* target array */
		   int nvars,		/* target array size */
		   int *nelt,		/* number of args parsed goes here */
		   float *def_val,	/* default variable value */
		   int print,		/* print during `-dumpconfig'? */
		   char *format,	/* optional value print format */
		   int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%.4f";
  opt->oc = oc_float;
  opt->variant.for_float.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a double-precision floating point option variable */
void
opt_reg_double(struct opt_odb_t *odb,	/* option data base */
	       char *name,		/* option name */
	       char *desc,		/* option description */
	       double *var,		/* target variable */
	       double def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       char *format)		/* option value print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20.4f";
  opt->oc = oc_double;
  opt->variant.for_double.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a double-precision floating point option array */
void
opt_reg_double_list(struct opt_odb_t *odb, /* option data base */
		    char *name,		/* option name */
		    char *desc,		/* option description */
		    double *vars,	/* target array */
		    int nvars,		/* target array size */
		    int *nelt,		/* number of args parsed goes here */
		    double *def_val,	/* default variable value */
		    int print,		/* print during `-dumpconfig'? */
		    char *format,	/* option value print format */
		    int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%.4f";
  opt->oc = oc_double;
  opt->variant.for_double.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* bind an enumeration string to an enumeration value */
static int
bind_to_enum(char *str,			/* string to bind to an enum */
	     char **emap,		/* enumeration string map */
	     char *eval,			/* enumeration value map, optional */
	     int emap_sz,		/* size of maps */
	     char *res)			/* enumeration string value result */
{
  int i;

  /* string enumeration string map */
  for (i=0; i<emap_sz; i++)
    {
      if (!strcmp(str, emap[i]))
	{
	  if (eval)
	    {
	      /* bind to eval value */
	      *res = eval[i];
	    }
	  else
	    {
	      /* bind to string index */
	      *res = i;
	    }
	  return TRUE;
	}
    }

  /* not found */
  *res = -1;
  return FALSE;
}

/* bind a enumeration value to an enumeration string */
static char *
bind_to_str(int val,			/* enumeration value */
	    char **emap,		/* enumeration string map */
	    char *eval,			/* enumeration value map, optional */
	    int emap_sz)		/* size of maps */
{
  int i;

  if (eval)
    {
      /* bind to first matching eval value */
      for (i=0; i<emap_sz; i++)
	{
	  if (eval[i] == val)
	    {
	      /* found */
	      return emap[i];
	    }
	}
      /* not found */
      return NULL;
    }
  else
    {
      /* bind to string at index */
      if (val >= emap_sz)
	{
	  /* invalid index */
	  return NULL;
	}
      /* else, index is in range */
      return emap[val];
    }
}

/* register an enumeration option variable, NOTE: all enumeration option
   variables must be of type `int', since true enum variables may be allocated
   with variable sizes by some compilers */
void
opt_reg_enum(struct opt_odb_t *odb,	/* option data base */
	     char *name,		/* option name */
	     char *desc,		/* option description */
	     char *var,			/* target variable */
	     char *def_val,		/* default variable value */
	     char **emap,		/* enumeration string map */
	     char *eval,			/* enumeration value map, optional */
	     int emap_sz,		/* size of maps */
	     int print,			/* print during `-dumpconfig'? */
	     char *format)		/* option value print format */
{
  char enum_val;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20s";
  opt->oc = oc_enum;
  opt->variant.for_enum.var = var;
  opt->variant.for_enum.emap = emap;
  opt->variant.for_enum.eval = eval;
  opt->variant.for_enum.emap_sz = emap_sz;
  if (def_val)
    {
      if (!bind_to_enum(def_val, emap, eval, emap_sz, &enum_val))
	fatal("could not bind default value for option `%s'", name);
    }
  else
    enum_val = 0;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = enum_val;
}

/* register an enumeration option array, NOTE: all enumeration option variables
   must be of type `int', since true enum variables may be allocated with
   variable sizes by some compilers */
void
opt_reg_enum_list(struct opt_odb_t *odb,/* option data base */
		  char *name,		/* option name */
		  char *desc,		/* option description */
		  char *vars,		/* target array */
		  int nvars,		/* target array size */
		  int *nelt,		/* number of args parsed goes here */
		  char *def_val,	/* default variable value */
		  char **emap,		/* enumeration string map */
		  char *eval,		/* enumeration value map, optional */
		  int emap_sz,		/* size of maps */
		  int print,		/* print during `-dumpconfig'? */
		  char *format,		/* option value print format */
		  int accrue)		/* accrue list across uses */
{
  int i;
  char enum_val;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%s";
  opt->oc = oc_enum;
  opt->variant.for_enum.var = vars;
  opt->variant.for_enum.emap = emap;
  opt->variant.for_enum.eval = eval;
  opt->variant.for_enum.emap_sz = emap_sz;
  if (def_val)
    {
      if (!bind_to_enum(def_val, emap, eval, emap_sz, &enum_val))
	fatal("could not bind default value for option `%s'", name);
    }
  else
    enum_val = 0;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = enum_val;
}

/* pre-defined boolean flag operands */
#define NUM_FLAGS		28
static char *flag_emap[NUM_FLAGS] = {
  (char*)"true", (char*)"t", (char*)"T", (char*)"True", (char*)"TRUE", (char*)"1", (char*)"y", (char*)"Y", (char*)"yes", (char*)"Yes", (char*)"YES",
  (char*)"on", (char*)"On", (char*)"ON",
  (char*)"false", (char*)"f", (char*)"F", (char*)"False", (char*)"FALSE", (char*)"0", (char*)"n", (char*)"N", (char*)"no", (char*)"No", (char*)"NO",
  (char*)"off", (char*)"Off", (char*)"OFF"
};
static bool flag_eval[NUM_FLAGS] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* register a boolean flag option variable */
void
opt_reg_flag(struct opt_odb_t *odb,	/* option data base */
	     char *name,		/* option name */
	     char *desc,		/* option description */
	     bool *var,			/* target variable */
	     bool def_val,		/* default variable value */
	     int print,			/* print during `-dumpconfig'? */
	     char *format)		/* optional value print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20s";
  opt->oc = oc_flag;
  opt->variant.for_enum.var = (char*)var;
  opt->variant.for_enum.emap = flag_emap;
  opt->variant.for_enum.eval = (char*)flag_eval;
  opt->variant.for_enum.emap_sz = NUM_FLAGS;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a boolean flag option array */
void
opt_reg_flag_list(struct opt_odb_t *odb,/* option database */
		  char *name,		/* option name */
		  char *desc,		/* option description */
		  bool *vars,		/* pointer to option array */
		  int nvars,		/* total entries in option array */
		  int *nelt,		/* number of elements parsed */
		  bool *def_val,		/* default array value */
		  int print,		/* print during `-dumpconfig'? */
		  char *format,		/* optional value print format */
		  int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%s";
  opt->oc = oc_flag;
  opt->variant.for_enum.var = (char*)vars;
  opt->variant.for_enum.emap = flag_emap;
  opt->variant.for_enum.eval = (char*)flag_eval;
  opt->variant.for_enum.emap_sz = NUM_FLAGS;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* register a string option variable */
void
opt_reg_string(struct opt_odb_t *odb,	/* option data base */
	       char *name,		/* option name */
	       char *desc,		/* option description */
	       char **var,		/* pointer to string option variable */
	       char *def_val,		/* default variable value */
	       int print,		/* print during `-dumpconfig'? */
	       char *format)		/* optional value print format */
{
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = 1;
  opt->nelt = NULL;
  opt->format = format ? format : "%20s";
  opt->oc = oc_string;
  opt->variant.for_string.var = var;
  opt->print = print;
  opt->accrue = FALSE;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  *var = def_val;
}

/* register a string option array */
void
opt_reg_string_list(struct opt_odb_t *odb,/* option data base */
		    char *name,		/* option name */
		    char *desc,		/* option description */
		    char **vars,	/* pointer to option string array */
		    int nvars,		/* target array size */
		    int *nelt,		/* number of args parsed goes here */
		    char **def_val,	/* default variable value */
		    int print,		/* print during `-dumpconfig'? */
		    char *format,	/* optional value print format */
		    int accrue)		/* accrue list across uses */
{
  int i;
  struct opt_opt_t *opt;

  opt = (struct opt_opt_t *)calloc(1, sizeof(struct opt_opt_t));
  if (!opt)
    fatal("out of virtual memory");

  opt->name = name;
  opt->desc = desc;
  opt->nvars = nvars;
  opt->nelt = nelt;
  opt->format = format ? format : "%s";
  opt->oc = oc_string;
  opt->variant.for_string.var = vars;
  opt->print = print;
  opt->accrue = accrue;

  /* place on ODB's option list */
  opt->next = NULL;
  add_option(odb, opt);

  /* set default value */
  for (i=0; i < *nelt; i++)
    vars[i] = def_val[i];
}

/* process command line arguments, returns index of next argument to parse */
int
process_option(struct opt_odb_t *odb,	/* option database */
	       int index,		/* index of the first arg to parse */
	       int argc,		/* total number of arguments */
	       char **argv)		/* argument string array */
{
  int cnt, ent, nvars;
  char *endp;
  double tmp;
  struct opt_opt_t *opt;

  //fprintf(stderr,"%s %s",opt->name,argv[index+1]);
  /* locate the option in the option database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    {
      if (!strcmp(opt->name, argv[index]))
	break;
    }
  if (!opt)
    {
      /* no one registered this option */
      fatal("option `%s' is undefined", argv[index]);
    }
  index++;

  /* process option arguments */
  switch (opt->oc)
    {
    case oc_long_long:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !myisdigit((long long)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || myisdigit((long long)argv[index][1])))
	{
	  opt->variant.for_long_long.var[ent] = strtoll(argv[index], &endp, 0);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_int:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !myisdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || myisdigit((int)argv[index][1])))
	{
	  opt->variant.for_int.var[ent] = strtol(argv[index], &endp, 0);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_uint:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  opt->variant.for_uint.var[ent] = strtoul(argv[index], &endp, 0);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_float:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !myisdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || myisdigit((int)argv[index][1])))
	{
	  tmp = strtod(argv[index], &endp);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  if (tmp < 0 || tmp > FLT_MAX)
	    {
	      /* over/underflow */
	      fatal("FP over/underflow for argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  opt->variant.for_float.var[ent] = (float)tmp;
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_double:
      /* this option needs at least one argument */
      if (index >= argc
	  || (argv[index][0] == '-' && !myisdigit((int)argv[index][1])))
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars &&
	     (argv[index][0] != '-' || myisdigit((int)argv[index][1])))
	{
	  opt->variant.for_double.var[ent] = strtod(argv[index], &endp);
	  if (*endp)
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_enum:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  if (!bind_to_enum(argv[index],
			    opt->variant.for_enum.emap,
			    opt->variant.for_enum.eval,
			    opt->variant.for_enum.emap_sz,
			    &opt->variant.for_enum.var[ent]))
	    {
	      /* could not parse entire argument */
	      fatal("could not parse argument `%s' of option `%s'",
		    argv[index], opt->name);
	    }
	  /* else, argument converted correctly */
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    case oc_flag:
      /* check if this option has at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available, default value for flag is true */
	  opt->variant.for_enum.var[0] = TRUE;
	  break;
	}
      /* else, parse boolean argument(s) */
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  if (!bind_to_enum(argv[index],
			    opt->variant.for_enum.emap,
			    opt->variant.for_enum.eval,
			    opt->variant.for_enum.emap_sz,
			    &opt->variant.for_enum.var[ent]))
	    {
	      /* could not parse entire argument, default to true */
	      opt->variant.for_enum.var[ent] = TRUE;
	      break;
	    }
	  else
	    {
	      /* else, argument converted correctly */
	      if (opt->nelt)
		(*opt->nelt)++;
	      cnt++; index++; ent++;
	    }
	}
      break;
    case oc_string:
      /* this option needs at least one argument */
      if (index >= argc || argv[index][0] == '-')
	{
	  /* no arguments available */
	  fatal("option `%s' requires an argument", opt->name);
	}
      cnt = 0;
      if (opt->accrue)
	{
	  ent = opt->nelt ? *opt->nelt : 0;
	  nvars = 1;
	  if (ent >= opt->nvars)
	    fatal("too many invocations of option `%s'", opt->name);
	}
      else
	{
	  ent = 0;
	  if (opt->nelt)
	    *opt->nelt = 0;
	  nvars = opt->nvars;
	}
      /* parse all arguments */
      while (index < argc && cnt < nvars && argv[index][0] != '-')
	{
	  opt->variant.for_string.var[ent] = argv[index];
	  if (opt->nelt)
	    (*opt->nelt)++;
	  cnt++; index++; ent++;
	}
      break;
    default:
      panic("bogus option class");
    }

  return index;
}

/* forward declarations */
static void process_file(struct opt_odb_t *odb, char *fname, int depth);
static void dump_config(struct opt_odb_t *odb, char *fname);

/* process a command line, internal version that tracks `-config' depth */
static void
__opt_process_options(struct opt_odb_t *odb,	/* option data base */
		      int argc,			/* number of arguments */
		      char **argv,		/* argument array */
		      int depth)		/* `-config' option depth */
{
  int index, do_dumpconfig;
  char *dumpconfig_name;

  index = 0;
  do_dumpconfig = FALSE;
  dumpconfig_name = NULL;
  /* visit all command line arguments */
  while (index < argc)
    {
      /* process any encountered orphans */
      while (index < argc && argv[index][0] != '-')
	{
	  if (depth > 0)
	    {
	      /* orphans are not allowed during config file processing */
	      fatal("orphan `%s' encountered during config file processing",
		    argv[index]);
	    }
	  /* else, call the user-stalled orphan handler */
	  if (odb->orphan_fn)
	    {
	      if (!odb->orphan_fn(index+1, argc, argv))
		{
		  /* done processing command line */
		  goto done_processing;
		}
	    }
	  else
	    {
	      /* no one claimed this option */
	      fatal("orphan argument `%s' was unclaimed", argv[index]);
	    }
	  /* go to next option */
	}

      /* done with command line? */
      if (index == argc)
	{
	  /* done processing command line */
	  goto done_processing;
	}
      /* when finished, argv[index] is an option to parse */

      /* process builtin options */
      if (!strcmp(argv[index], "-config"))
	{
	  /* handle `-config' builtin option */
	  index++;
	  if (index >= argc || argv[index][0] == '-')
	    {
	      /* no arguments available */
	      fatal("option `-config' requires an argument");
	    }
	  process_file(odb, argv[index], depth);
	  index++;
	}
      else if (!strcmp(argv[index], "-dumpconfig"))
	{
	  /* this is performed *last* */
	  do_dumpconfig = TRUE;
	  /* handle `-dumpconfig' builtin option */
	  index++;
	  if (index >= argc
	      || (argv[index][0] == '-' && argv[index][1] != '\0'))
	    {
	      /* no arguments available */
	      fatal("option `-dumpconfig' requires an argument");
	    }
	  dumpconfig_name = argv[index];
	  index++;
	}
      else
	{
	  /* process user-installed option */
	  index = process_option(odb, index, argc, argv);
	}
    }
 done_processing:

  if (do_dumpconfig)
    dump_config(odb, dumpconfig_name);
}

/* process command line arguments */
void
opt_process_options(struct opt_odb_t *odb,	/* option data base */
		    int argc,			/* number of arguments */
		    char **argv)		/* argument array */
{
  /* need at least two entries to have an option */
  if (argc < 2)
    return;

  /* process the command, starting at `-config' depth zero */
  __opt_process_options(odb, argc-1, argv+1, /* top level */0);
}

/* handle `-config' builtin option */
#define MAX_LINE_ARGS		256
#define MAX_FILENAME_LEN	1024
static void
process_file(struct opt_odb_t *odb, char *fname, int depth)
{
  int largc;
  char *largv[MAX_LINE_ARGS];
  char line[1024], *p, *q;
  char cwd[MAX_FILENAME_LEN];
  char *header = '\0';
  FILE *fd;

  fd = fopen(fname, "r");
  if (!fd)
    fatal("could not open configuration file `%s'", fname);

  if (!getcwd(cwd, MAX_FILENAME_LEN))
    fatal("can't get cwd");

  header = strrchr(fname, '/');

/*  
  if (header != NULL)
  {
    *header = '\0';
    if (chdir(fname) == -1)
      fatal("can't chdir to %s\n", fname);
    *header = '/';
  }
*/

  while (!feof(fd))
    {
      line[0] = '\n';
      /* read a line from the file and chop */
      fgets(line, 1024, fd);
      if (line[0] == '\0' || line[0] == '\n')
	continue;
      if (line[strlen(line)-1] == '\n')
	line[strlen(line)-1] = '\0';

      /* process one line from the file */
      largc = 0; p = line;
      while (*p)
	{
	  /* skip whitespace */
	  while (*p != '\0' && (*p == ' ' || *p == '\t'))
	    p++;

	  /* ignore empty lines and comments */
	  if (*p == '\0' || *p == '#')
	    break;

	  /* skip to the end of the argument */
	  q = p;
	  while (*q != '\0' && *q != ' ' && *q != '\t')
	    q++;
	  if (*q)
	    {
	      *q = '\0';
	      q++;
	    }

	  /* marshall an option array */
	  largv[largc++] = mystrdup(p);

	  if (largc == MAX_LINE_ARGS)
	    {
	      if (chdir(cwd) == -1)
		fatal("can't chdir back to %s\n", cwd);
	      fatal("option line too complex in file `%s'", fname);
	    }

	  /* go to next argument */
	  p = q;
	}

      /* process the line */
      if (largc > 0)
	__opt_process_options(odb, largc, largv, depth+1);
      /* else, empty line */
    }

  fclose(fd);

  if (chdir(cwd) == -1)
    fatal("can't chdir back to %s\n", cwd);
}

/* print the value of an option */
void
opt_print_option(struct opt_opt_t *opt,/* option variable */
		 FILE *fd)		/* output stream */
{
  int i, nelt;

  switch (opt->oc)
    {
    case oc_long_long:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_long_long.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_int:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_int.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_uint:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_uint.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_float:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, (double)opt->variant.for_float.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_double:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  fprintf(fd, opt->format, opt->variant.for_double.var[i]);
	  fprintf(fd, " ");
	}
      break;
    case oc_enum:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  char *estr = bind_to_str(opt->variant.for_enum.var[i],
				   opt->variant.for_enum.emap,
				   opt->variant.for_enum.eval,
				   opt->variant.for_enum.emap_sz);
	  if (!estr)
	    panic("could not bind enum `%d' for option `%s'",
		  opt->variant.for_enum.var[i], opt->name);

	  fprintf(fd, opt->format, estr);
	  fprintf(fd, " ");
	}
      break;
    case oc_flag:
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      for (i=0; i<nelt; i++)
	{
	  char *estr = bind_to_str(opt->variant.for_enum.var[i],
				   opt->variant.for_enum.emap,
				   opt->variant.for_enum.eval,
				   opt->variant.for_enum.emap_sz);
	  if (!estr)
	    panic("could not bind boolean `%d' for option `%s'",
		  opt->variant.for_enum.var[i], opt->name);
	  
	  fprintf(fd, opt->format, estr);
	  fprintf(fd, " ");
	}
      break;
    case oc_string:
      if (!opt->nvars)
	{
	  fprintf(fd, "%20s ", "<null>");
	  break;
	}
      if (opt->nelt)
	nelt = *(opt->nelt);
      else
	nelt = 1;
      if (nelt == 0)
	{
	  fprintf(fd, "%20s ", "<null>");
	  break;
	}
      else
	{
	  for (i=0; i<nelt; i++)
	    {
	      fprintf(fd, opt->format,
		      (opt->variant.for_string.var[i]
		       ? opt->variant.for_string.var[i]
		       : "<null>"));
	      fprintf(fd, " ");
	    }
	}
      break;
    default:
      panic("bogus option class");
    }
}

/* print any options header */
static void
print_option_header(struct opt_odb_t *odb,/* options database */
		    FILE *fd)		/* output stream */
{
  if (!odb->header)
    return;

  fprintf(fd, "\n%s\n", odb->header);
}

/* print option notes */
static void
print_option_notes(struct opt_odb_t *odb,/* options database */
		   FILE *fd)		/* output stream */
{
  struct opt_note_t *note;

  if (!odb->notes || opt_ignore_notes)
    return;

  fprintf(fd, "\n");
  for (note=odb->notes; note != NULL; note=note->next)
    fprintf(fd, "%s\n", note->note);
  fprintf(fd, "\n");
}

/* builtin options */
static struct opt_opt_t dumpconfig_opt =
  { NULL, "-dumpconfig", "dump configuration to a file",
    0, NULL, NULL, FALSE, FALSE, oc_string };
static struct opt_opt_t config_opt =
  { &dumpconfig_opt, "-config", "load configuration from a file",
    0, NULL, NULL, FALSE, FALSE, oc_string };
static struct opt_opt_t *builtin_options = &config_opt;

/* return non-zero if the option is a NULL-valued string option */
int					/* non-zero if null string option */
opt_null_string(struct opt_opt_t *opt)
{
  return (opt != NULL
	  && opt->oc == oc_string
	  && (opt->nvars == 0
	      || (opt->nelt != NULL && *opt->nelt == 0)
	      || (opt->nelt == NULL
		  && (opt->variant.for_string.var == NULL
		      || opt->variant.for_string.var[0] == NULL))));
}

/* print all options and current values */
void
opt_print_options(struct opt_odb_t *odb,/* option data base */
		  FILE *fd,		/* output stream */
		  int terse,		/* print terse options? */
		  int notes)		/* include notes? */
{
  struct opt_opt_t *opt;

  if (!odb)
    {
      /* no options */
      return;
    }

  /* print any options header */
  if (notes)
    print_option_header(odb, fd);

  /* dump out builtin options */
  for (opt=builtin_options; opt != NULL; opt=opt->next)
    {
      if (terse)
	fprintf(fd, "# %-27s # %s\n", opt->name, opt->desc);
      else
	{
	  fprintf(fd, "# %s\n", opt->desc);
	  fprintf(fd, "# %-22s\n\n", opt->name);
	}
    }

  /* dump out options from options database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    {
      if (terse)
	{
	  if (!opt->print || opt_null_string(opt))
	      fprintf(fd, "# %-14s ", opt->name);
	  else
	    fprintf(fd, "%-16s ", opt->name);
	  opt_print_option(opt, fd);
	  if (opt->desc)
	    fprintf(fd, "# %-22s", opt->desc);
	  fprintf(fd, "\n");
	}
      else
	{
	  if (opt->desc)
	    fprintf(fd, "# %s\n", opt->desc);
	  if (!opt->print || opt_null_string(opt))
	    fprintf(fd, "# %-20s ", opt->name);
	  else
	    fprintf(fd, "%-22s ", opt->name);
	  opt_print_option(opt, fd);
	  fprintf(fd, "\n\n");
	}
    }

  /* print option notes */
  if (notes)
    print_option_notes(odb, fd);
}

/* print help information for an option */
static void
print_help(struct opt_opt_t *opt,	/* option variable */
	   FILE *fd)			/* output stream */
{
  char *s = NULL;

  fprintf(fd, "%-20s ", opt->name);
  switch (opt->oc)
    {
    case oc_long_long:
      if (opt->nvars == 1)
	s = (char*)"<long long>";
      else
	s = (char*)"<long long list...>";
      break;
    case oc_int:
      if (opt->nvars == 1)
	s = (char*)"<int>";
      else
	s = (char*)"<int list...>";
      break;
    case oc_uint:
      if (opt->nvars == 1)
	s = (char*)"<uint>";
      else
	s = (char*)"<uint list...>";
      break;
    case oc_float:
      if (opt->nvars == 1)
	s = (char*)"<float>";
      else
	s = (char*)"<float list...>";
      break;
    case oc_double:
      if (opt->nvars == 1)
	s = (char*)"<double>";
      else
	s = (char*)"<double list...>";
      break;
    case oc_enum:
      if (opt->nvars == 1)
	s = (char*)"<enum>";
      else
	s = (char*)"<enum list...>";
      break;
    case oc_flag:
      if (opt->nvars == 1)
	s = (char*)"<true|false>";
      else
	s = (char*)"<true|false list...>";
      break;
    case oc_string:
      if (opt->nvars == 0 || opt->nvars == 1)
	s = (char*)"<string>";
      else
	s = (char*)"<string list...>";
      break;
    default:
      panic("bogus option class");
    }
  fprintf(fd, "%-16s # ", s);
  opt_print_option(opt, fd);
  fprintf(fd, "# %-22s\n", opt->desc);
}

/* print option help page with default values */
void
opt_print_help(struct opt_odb_t *odb,	/* option data base */
	       FILE *fd)		/* output stream */
{
  struct opt_opt_t *opt;

  /* print any options header */
  print_option_header(odb, fd);

  fprintf(fd, "#\n");
  fprintf(fd, "%-20s %-16s # %-20s # %-22s\n",
	  "# -option", "<args>", "<default>", "description");
  fprintf(fd, "#\n");

  /* print out help info for builtin options */
  for (opt=builtin_options; opt != NULL; opt=opt->next)
    print_help(opt, fd);

  /* print out help info for options in options database */
  for (opt=odb->options; opt != NULL; opt=opt->next)
    print_help(opt, fd);

  /* print option notes */
  print_option_notes(odb, fd);
}

/* handle `-dumpconfig' builtin option, print options from file argument */
static void
dump_config(struct opt_odb_t *odb,	/* option data base */
	    char *fname)		/* output file name, "-" == stdout */
{
  FILE *fd;

  /* open output file stream */
  if (!strcmp(fname, "-"))
    fd = stderr;
  else
    {
      fd = fopen(fname, "w");
      if (!fd)
	fatal("could not open file `%s'", fname);
    }

  /* print current option values to output stream */
  opt_print_options(odb, fd, /* long */FALSE, /* !notes */FALSE);

  /* close output stream, if not a standard stream */
  if (fd != stdout && fd != stderr)
    fclose(fd);
}

/* find an option by name in the option database, returns NULL if not found */
struct opt_opt_t *
opt_find_option(struct opt_odb_t *odb,	/* option database */
		char *opt_name)	/* option name */
{
  struct opt_opt_t *opt;

  /* search builtin options */
  for (opt = builtin_options; opt != NULL; opt = opt->next)
    {
      if (!strcmp(opt->name, opt_name))
	{
	  /* located option */
	  return opt;
	}
    }

  /* search user-installed options */
  for (opt = odb->options; opt != NULL; opt = opt->next)
    {
      if (!strcmp(opt->name, opt_name))
	{
	  /* located option */
	  return opt;
	}
    }
  /* not found */
  return NULL;
}

/* register an options header, the header is printed before the option list */
void
opt_reg_header(struct opt_odb_t *odb,	/* option database */
	       char *header)		/* options header string */
{
  odb->header = header;
}

/* register an option note, notes are printed after the list of options */
void
opt_reg_note(struct opt_odb_t *odb,	/* option database */
	     char *note_str)		/* option note */
{
  struct opt_note_t *note, *elt, *prev;

  note = (struct opt_note_t *)calloc(1, sizeof(struct opt_note_t));
  if (!note)
    fatal("out of virtual memory");

  /* record note */
  note->next = NULL;
  note->note = note_str;

  /* add to end of option notes list */
  for (prev=NULL, elt=odb->notes; elt != NULL; prev=elt, elt=elt->next)
    /* nada */;

  if (prev != NULL)
    prev->next = note;
  else /* prev == NULL */
    odb->notes = note;
  note->next = NULL;
}
