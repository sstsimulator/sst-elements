/* symbol.h - program symbol and line data interfaces */

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

#ifndef SYMBOL_H
#define SYMBOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"

enum sym_seg_t {
  ss_data,			/* data segment symbol */
  ss_text,			/* text segment symbol */
  ss_NUM
};

/* internal program symbol format */
struct sym_sym_t {
  char *name;			/* symbol name */
  enum sym_seg_t seg;		/* symbol segment */
  int initialized;		/* initialized? (if data segment) */
  int pub;			/* externally visible? */
  int local;			/* compiler local symbol? */
  md_addr_t addr;		/* symbol address value */
  int size;			/* bytes to next symbol */
};

/* symbol database in no particular order */
extern struct sym_sym_t *sym_db;

/* all symbol sorted by address */
extern int sym_nsyms;
extern struct sym_sym_t **sym_syms;

/* all symbols sorted by name */
extern struct sym_sym_t **sym_syms_by_name;

/* text symbols sorted by address */
extern int sym_ntextsyms;
extern struct sym_sym_t **sym_textsyms;

/* text symbols sorted by name */
extern struct sym_sym_t **sym_textsyms_by_name;

/* data symbols sorted by address */
extern int sym_ndatasyms;
extern struct sym_sym_t **sym_datasyms;

/* data symbols sorted by name */
extern struct sym_sym_t **sym_datasyms_by_name;

/* load symbols out of FNAME */
void
sym_loadsyms(char *fname,		/* file name containing symbols */
	     int load_locals);		/* load local symbols */

/* dump symbol SYM to output stream FD */
void
sym_dumpsym(struct sym_sym_t *sym,	/* symbol to display */
	    FILE *fd);			/* output stream */

/* dump all symbols to output stream FD */
void
sym_dumpsyms(FILE *fd);			/* output stream */

/* dump all symbol state to output stream FD */
void
sym_dumpstate(FILE *fd);		/* output stream */

/* symbol databases available */
enum sym_db_t {
  sdb_any,			/* search all symbols */
  sdb_text,			/* search text symbols */
  sdb_data,			/* search data symbols */
  sdb_NUM
};

/* bind address ADDR to a symbol in symbol database DB, the address must
   match exactly if EXACT is non-zero, the index of the symbol in the
   requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *			/* symbol found, or NULL */
sym_bind_addr(md_addr_t addr,		/* address of symbol to locate */
	      int *pindex,		/* ptr to index result var */
	      int exact,		/* require exact address match? */
	      enum sym_db_t db);	/* symbol database to search */

/* bind name NAME to a symbol in symbol database DB, the index of the symbol
   in the requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *				/* symbol found, or NULL */
sym_bind_name(char *name,			/* symbol name to locate */
	      int *pindex,			/* ptr to index result var */
	      enum sym_db_t db);		/* symbol database to search */

#ifdef __cplusplus
}
#endif

#endif /* SYMBOL_H */

