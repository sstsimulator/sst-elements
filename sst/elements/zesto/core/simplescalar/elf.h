/*
 * elf.h - SimpleScalar ELF definitions
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: elf.h,v 1.1 2009/01/30 04:48:07 loh Exp $
 *
 * $Log: elf.h,v $
 * Revision 1.1  2009/01/30 04:48:07  loh
 * This is the version for the initial pre-release (v0.1).
 *
 * Changes in this check-in include a variety of speed optimizations (including
 * fast modulo operations, work-checking flags to skip functions when there's
 * nothing to do), lots of const declarations, configurable MSHR request
 * prioritization, knobbed whether prefetches occur on misses only or for all
 * requests, additional modifications to the WBB to act more like a victim
 * cache, processing of system call memory accesses through the cache hierarchy,
 * split-line data-cache accesses for both loads and stores, better IPC stats
 * for multi-core, a more simplified and faster STM model, and preliminary
 * tracing support.
 *
 * Revision 1.3  2006/07/26 20:34:57  loh
 * Architected register state properly sync'd.  Added uop implementation for
 * micro-coded REP.  The other sim-*'s updated to work again.  Sim-uop removed.
 *
 * Revision 1.2  2006/07/24 19:57:04  loh
 * Target-ISA files weren't included properly
 *
 * Revision 1.1.1.1  2006/01/17 00:31:04  cristiano
 * Simplescalar with System and Multi-threading Interactions Log
 *
 * Revision 1.3  2005/10/12 22:32:10  grh6v
 * Added a bunch of instructions, and fixed up the loader to do the elf
 * aux stuff correctly.  Still missing all the instructions that have
 * to do with the segment registers.
 *
 * Revision 1.2  2005/10/08 21:37:07  grh6v
 * Load things by segment, not section.
 *
 * Revision 1.1.1.1  2004/02/18 06:02:45  wei
 * simplesim-4.0
 *
 * Revision 1.1  2003/09/10 14:09:48  ahamel
 * Initial version for SimpleScalar 4.0
 *
 * Revision 1.1.1.1  2003/01/30 15:01:42  ahamel
 * Imported sources
 *
 * Revision 1.1.1.1  2001/03/23 06:22:57  taustin
 * Initial checkin...
 *
 *
 * Revision 1.1.1.1  2000/11/29 14:53:54  cu-cs
 * Grand unification of arm sources.
 *
 *
 * Revision 1.1.2.1  2000/07/21 18:34:59  taustin
 * More progress on the SimpleScalar/ARM target.
 *
 * Revision 1.1.1.1  2000/05/26 15:22:27  taustin
 * SimpleScalar Tool Set
 *
 *
 * Revision 1.2  1999/12/31 18:57:49  taustin
 * quad_t naming conflicts removed
 *
 * Revision 1.1  1998/08/27 16:54:03  taustin
 * Initial revision
 *
 * Revision 1.1  1998/05/06  01:09:18  calder
 * Initial revision
 *
 * Revision 1.1  1997/04/16  22:13:35  taustin
 * Initial revision
 *
 *
 */

/* SimpleScalar ELF definitions */

#ifndef ELF_H
#define ELF_H

#include "machine.h"


/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#define EI_MAG0         0               /* File identification byte 0 index */
#define ELFMAG0         0x7f            /* Magic number byte 0 */

#define EI_MAG1         1               /* File identification byte 1 index */
#define ELFMAG1         'E'             /* Magic number byte 1 */

#define EI_MAG2         2               /* File identification byte 2 index */
#define ELFMAG2         'L'             /* Magic number byte 2 */

#define EI_MAG3         3               /* File identification byte 3 index */
#define ELFMAG3         'F'             /* Magic number byte 3 */


/* Legal values for e_type (object file type).  */

#define ET_NONE         0               /* No file type */
#define ET_REL          1               /* Relocatable file */
#define ET_EXEC         2               /* Executable file */
#define ET_DYN          3               /* Shared object file */
#define ET_CORE         4               /* Core file */
#define ET_NUM          5               /* Number of defined types.  */
#define ET_LOPROC       0xff00          /* Processor-specific */
#define ET_HIPROC       0xffff          /* Processor-specific */


/* Legal values for e_machine (architecture).  */

#define EM_NONE         0               /* No machine */
#define EM_M32          1               /* AT&T WE 32100 */
#define EM_SPARC        2               /* SUN SPARC */
#define EM_386          3               /* Intel 80386 */
#define EM_68K          4               /* Motorola m68k family */
#define EM_88K          5               /* Motorola m88k family */
#define EM_486          6               /* Intel 80486 */
#define EM_860          7               /* Intel 80860 */
#define EM_MIPS         8               /* MIPS R3000 big-endian */
#define EM_S370         9               /* Amdahl */
#define EM_MIPS_RS4_BE 10               /* MIPS R4000 big-endian */
#define EM_PARISC      15               /* HPPA */
#define EM_SPARC32PLUS 18               /* Sun's "v8plus" */
#define EM_PPC         20               /* PowerPC */
#define EM_ARM	       40		/* ARM */
#define EM_SPARCV9     43               /* SPARC v9 64-bit */


/* Special section indices.  */

#define SHN_UNDEF       0               /* Undefined section */
#define SHN_LORESERVE   0xff00          /* Start of reserved indices */
#define SHN_LOPROC      0xff00          /* Start of processor-specific */
#define SHN_HIPROC      0xff1f          /* End of processor-specific */
#define SHN_ABS         0xfff1          /* Associated symbol is absolute */
#define SHN_COMMON      0xfff2          /* Associated symbol is common */
#define SHN_HIRESERVE   0xffff          /* End of reserved indices */


/* Legal values for sh_type (section type).  */

#define SHT_NULL        0               /* Section header table entry unused */
#define SHT_PROGBITS    1               /* Program data */
#define SHT_SYMTAB      2               /* Symbol table */
#define SHT_STRTAB      3               /* String table */
#define SHT_RELA        4               /* Relocation entries with addends */
#define SHT_HASH        5               /* Symbol hash table */
#define SHT_DYNAMIC     6               /* Dynamic linking information */
#define SHT_NOTE        7               /* Notes */
#define SHT_NOBITS      8               /* Program space with no data (bss) */
#define SHT_REL         9               /* Relocation entries, no addends */
#define SHT_SHLIB       10              /* Reserved */
#define SHT_DYNSYM      11              /* Dynamic linker symbol table */
#define SHT_NUM         12              /* Number of defined types.  */
#define SHT_LOSUNW      0x6ffffffd      /* Sun-specific low bound.  */
#define SHT_GNU_verdef  0x6ffffffd      /* Version definition section.  */
#define SHT_GNU_verneed 0x6ffffffe      /* Version needs section.  */
#define SHT_GNU_versym  0x6fffffff      /* Version symbol table.  */
#define SHT_HISUNW      0x6fffffff      /* Sun-specific high bound.  */
#define SHT_LOPROC      0x70000000      /* Start of processor-specific */
#define SHT_HIPROC      0x7fffffff      /* End of processor-specific */
#define SHT_LOUSER      0x80000000      /* Start of application-specific */
#define SHT_HIUSER      0x8fffffff      /* End of application-specific */


/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE       (1 << 0)        /* Writable */
#define SHF_ALLOC       (1 << 1)        /* Occupies memory during execution */
#define SHF_EXECINSTR   (1 << 2)        /* Executable */
#define SHF_MASKPROC    0xf0000000      /* Processor-specific */

#define EI_NIDENT (16)


struct elf_filehdr
{
  unsigned char e_ident[EI_NIDENT];	/* Magic number and other info */
  word_t e_type;			/* Object file type */
  word_t e_machine;			/* Architecture */
  dword_t e_version;			/* Object file version */
  md_addr_t e_entry;			/* Entry point virtual address */
  dword_t e_phoff;			/* Program header table file offset */
  dword_t e_shoff;			/* Section header table file offset */
  dword_t e_flags;			/* Processor-specific flags */
  word_t e_ehsize;			/* ELF header size in bytes */
  word_t e_phentsize;			/* Program header table entry size */
  word_t e_phnum;			/* Program header table entry count */
  word_t e_shentsize;			/* Section header table entry size */
  word_t e_shnum;			/* Section header table entry count */
  word_t e_shstrndx;			/* Section header string table index */
};

struct elf_scnhdr
{
	dword_t sh_name;			/* Section name (string tbl index) */
	dword_t sh_type;			/* Section type */
	dword_t sh_flags;		/* Section flags */
	md_addr_t sh_addr;		/* Section virtual addr at execution */
	dword_t sh_offset;		/* Section file offset */
	dword_t sh_size;			/* Section size in bytes */
	dword_t sh_link;			/* Link to another section */
	dword_t sh_info;			/* Additional section information */
	dword_t sh_addralign;		/* Section alignment */
	dword_t sh_entsize;		/* Entry size if section holds table */
};

#define PF_R            4
#define PF_W            2
#define PF_X            1               

struct elf_phdr {      
	dword_t    p_type;
	dword_t     p_offset;
	md_addr_t    p_vaddr;
	md_addr_t    p_paddr;
	dword_t    p_filesz; 
	dword_t    p_memsz;        
	dword_t    p_flags;
	dword_t    p_align;                                                          
} ;   

#define ELF_BASE 0x80000000
#define ELF_PAGESTART(x) ((x) & ~(4096-1))
#define ELF_PAGEOFFSET(x) ((x) & (4096-1))

#define PT_LOAD 1

#define AT_NULL         0               /* End of vector */
#define AT_IGNORE       1               /* Entry should be ignored */
#define AT_EXECFD       2               /* File descriptor of program */
#define AT_PHDR         3               /* Program headers for program */
#define AT_PHENT        4               /* Size of program header entry */
#define AT_PHNUM        5               /* Number of program headers */
#define AT_PAGESZ       6               /* System page size */
#define AT_BASE         7               /* Base address of interpreter */
#define AT_FLAGS        8               /* Flags */
#define AT_ENTRY        9               /* Entry point of program */
#define AT_NOTELF       10              /* Program is not ELF */
#define AT_UID          11              /* Real uid */
#define AT_EUID         12              /* Effective uid */
#define AT_GID          13              /* Real gid */
#define AT_EGID         14              /* Effective gid */

#define AT_PLATFORM     15              /* String identifying platform.  */
#define AT_HWCAP        16              /* Machine dependent hints about
                                           processor capabilities.  */

#define AT_CLKTCK       17              /* Frequency of times() */


#endif /* ELF_H */
