/* memory.h - flat memory space interfaces 
 *
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
 * Copyright © 2009 by Gabriel H. Loh and the Georgia Tech Research Corporation
 * Atlanta, GA  30332-0415
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING ZESTO, YOU ARE AGREEING TO THESE
 * TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the SimpleScalar
 * Toolset (property of SimpleScalar LLC), and as such, those portions are
 * bound by the corresponding legal terms and conditions.  All source files
 * derived directly or in part from the SimpleScalar Toolset bear the original
 * user agreement.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the Georgia Tech Research Corporation nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * 4. Zesto is distributed freely for commercial and non-commercial use.  Note,
 * however, that the portions derived from the SimpleScalar Toolset are bound
 * by the terms and agreements set forth by SimpleScalar, LLC.  In particular:
 * 
 *   "Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 *   downloaded, compiled, executed, copied, and modified solely for nonprofit,
 *   educational, noncommercial research, and noncommercial scholarship
 *   purposes provided that this notice in its entirety accompanies all copies.
 *   Copies of the modified software can be delivered to persons who use it
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice in its
 *   entirety accompanies all copies."
 * 
 * User is responsible for reading and adhering to the terms set forth by
 * SimpleScalar, LLC where appropriate.
 * 
 * 5. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 6. Noncommercial and nonprofit users may distribute copies of Zesto in
 * compiled or executable form as set forth in Section 2, provided that either:
 * (A) it is accompanied by the corresponding machine-readable source code, or
 * (B) it is accompanied by a written offer, with no time limit, to give anyone
 * a machine-readable copy of the corresponding source code in return for
 * reimbursement of the cost of distribution. This written offer must permit
 * verbatim duplication by anyone, or (C) it is distributed by someone who
 * received only the executable form, and is accompanied by a copy of the
 * written offer of source code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266 Ferst Drive,
 * Georgia Institute of Technology, Atlanta, GA 30332-0765
 */


#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "options.h"
#include "stats.h"

/* number of entries in page translation hash table (must be power-of-two) */
#define MEM_PTAB_SIZE    (256*1024)
#define MEM_LOG_PTAB_SIZE  18
#define MAX_LEV                 3


/* log2(page_size) assuming 4KB pages */
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

/* this maps a virtual address to a low address where we're
   pretending that the page table information is stored.  The shift
   by PAGE_SHIFT yields the virtual page number, the masking is
   just to hash the address down to something that's less than the
   start of the .text section, and the additional offset is so that
   we don't generate a really low (e.g., NULL) address.
   WE KEEP THE SAME INTERFACE FOR QEMU WHERE WE ASSUME ALL PAGE TABLES ARE MAPPED IN A LOWER MEMORY AREA
   ANY TLB MISS GOES TO THIS ADDRESS FOR FETCHING THE PHYSICAL PAGE NUMBERS
 */
#define PAGE_TABLE_ADDR(T,A) ((((((A)>>PAGE_SHIFT)<<4)+(T)) + 0x00080000) & 0x03ffffff)
#define DO_NOT_TRANSLATE (-1)


/* memory access command */
enum mem_cmd {
  Read,      /* read memory from target (simulated prog) to host */
  Write      /* write memory from host (simulator) to target */
};

/* page table entry */
struct mem_pte_t {
  struct mem_pte_t *next;  /* next translation in this bucket */
  md_addr_t tag;    /* virtual page number tag */
  md_addr_t addr;   /* virtual address */
  int dirty;         /* has this page been modified? */
  byte_t *page;      /* page pointer */
  off_t backing_offset; /* offset (for fseeko) for where this page is stored in the backing file */
  int from_mmap_syscall; /* TRUE if mapped from mmap/mmap2 call */
  int no_dealloc; /* if true, do not ever deallocate/page-out to backing file */

  /* these form a doubly-linked list through all pte's to track usage recency */
  struct mem_pte_t * lru_prev;
  struct mem_pte_t * lru_next;
};

/* just a simple container struct to track unused pages */
struct mem_page_link_t {
  byte_t * page;
  struct mem_page_link_t * next;
};

/* memory object */
struct mem_t {
  /* memory object state */
  char *name;        /* name of this memory space */
  struct mem_pte_t *ptab[MEM_PTAB_SIZE];/* inverted page table */
  FILE * backing_file; /* disk file for storing memory image */
  off_t backing_file_tail; /* offset to allocate next page to */
  struct mem_pte_t * recency_lru; /* oldest (LRU) */
  struct mem_pte_t * recency_mru; /* youngest (MRU) */

  zcounter_t page_count;      /* total number of pages allocated (in core and backing file) */
};

/* memory access function type, this is a generic function exported for the
   purpose of access the simulated vitual memory space */
typedef enum md_fault_type
(*mem_access_fn)(
    int core_id,
    struct mem_t *mem,  /* memory space to access */
    enum mem_cmd cmd,  /* Read or Write */
    md_addr_t addr,  /* target memory address to access */
    void *p,    /* where to copy to/from */
    int nbytes);    /* transfer length in bytes */

/*
 * virtual to host page translation macros
 */

/* compute page table set */
#define MEM_PTAB_SET(ADDR)            \
  (((ADDR) >> MD_LOG_PAGE_SIZE) & (MEM_PTAB_SIZE - 1))

/* compute page table tag */
#define MEM_PTAB_TAG(ADDR)            \
  ((ADDR) >> (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))

/* convert a pte entry at idx to a block address */
#define MEM_PTE_ADDR(PTE, IDX)            \
  (((PTE)->tag << (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))    \
   | ((IDX) << MD_LOG_PAGE_SIZE))

/* locate host page for virtual address ADDR, returns NULL if unallocated */
#define MEM_PAGE(MEM, ADDR, DIRTY)  mem_translate((MEM), (ADDR), (DIRTY))

/* compute address of access within a host page */
#define MEM_OFFSET(ADDR)  ((ADDR) & (MD_PAGE_SIZE - 1))

#define MEM_ADDR2HOST(MEM, ADDR) MEM_PAGE(MEM, ADDR, 0)+MEM_OFFSET(ADDR)

/* memory tickle function, allocates pages when they are first written */
#define MEM_TICKLE(MEM, ADDR)            \
  ((!MEM_PAGE(MEM, ADDR, 0)            \
    ? (/* allocate page at address ADDR */        \
      mem_newpage(MEM, ADDR))            \
    : (/* nada... */ (void)0)))            

/* memory page iterator */
#define MEM_FORALL(MEM, ITER, PTE)          \
  for ((ITER)=0; (ITER) < MEM_PTAB_SIZE; (ITER)++)      \
    for ((PTE)=(MEM)->ptab[(ITER)]; (PTE) != NULL; (PTE)=(PTE)->next)


/*
 * memory accessor macros
 */

#ifdef ZESTO_ORACLE_C
/* these macros are to support speculative reads/writes of memory
   due to instructions in-flight (uncommitted) in the machine */
#define MEM_READ(MEM, ADDR, TYPE)          \
  (core->oracle->spec_read_byte((ADDR),&_mem_read_tmp) ? _mem_read_tmp :             \
  (MEM_PAGE(MEM, (md_addr_t)(ADDR),0)          \
   ? *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR),0) + MEM_OFFSET(ADDR)))  \
   : /* page not yet allocated, return zero value */ 0))

#define MEM_WRITE(MEM, ADDR, TYPE, VAL)          \
  core->oracle->spec_write_byte((ADDR),(VAL))

#define MEM_WRITE_BYTE_NON_SPEC(MEM, ADDR, VAL)          \
  (MEM_TICKLE(MEM, (md_addr_t)(ADDR)),          \
   *((byte_t *)(MEM_PAGE(MEM, (md_addr_t)(ADDR),1) + MEM_OFFSET(ADDR))) = (VAL))

#else /* regular non-speculative versions... */

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_READ(MEM, ADDR, TYPE)          \
  (MEM_PAGE(MEM, (md_addr_t)(ADDR),0)          \
   ? *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR),0) + MEM_OFFSET(ADDR)))  \
   : /* page not yet allocated, return zero value */ 0)

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_WRITE(MEM, ADDR, TYPE, VAL)          \
  (MEM_TICKLE(MEM, (md_addr_t)(ADDR)),          \
   *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR),1) + MEM_OFFSET(ADDR))) = (VAL))
#endif

/* fast memory accessor macros, typed versions */
#define MEM_READ_BYTE(MEM, ADDR)  MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)  MEM_READ(MEM, ADDR, sbyte_t)
#define MEM_READ_WORD(MEM, ADDR)  MD_SWAPW(MEM_READ(MEM, ADDR, word_t))
#define MEM_READ_SWORD(MEM, ADDR)  MD_SWAPW(MEM_READ(MEM, ADDR, sword_t))
#define MEM_READ_DWORD(MEM, ADDR)  MD_SWAPD(MEM_READ(MEM, ADDR, dword_t))
#define MEM_READ_SDWORD(MEM, ADDR)  MD_SWAPD(MEM_READ(MEM, ADDR, sdword_t))

#ifdef HOST_HAS_QWORD
#ifdef TARGET_HAS_UNALIGNED_QWORD
qword_t MEM_QREAD(struct mem_t *mem, md_addr_t addr);
#define MEM_READ_QWORD(MEM, ADDR)  MD_SWAPQ(MEM_QREAD(MEM, ADDR))
#define MEM_READ_SQWORD(MEM, ADDR)  MD_SWAPQ(MEM_QREAD(MEM, ADDR))
#else /* !TARGET_HAS_UNALIGNED_QWORD */
#define MEM_READ_QWORD(MEM, ADDR)  MD_SWAPQ(MEM_READ(MEM, ADDR, qword_t))
#define MEM_READ_SQWORD(MEM, ADDR)  MD_SWAPQ(MEM_READ(MEM, ADDR, sqword_t))
#endif
#endif /* HOST_HAS_QWORD */

#define MEM_WRITE_BYTE(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
#define MEM_WRITE_WORD(MEM, ADDR, VAL)    MEM_WRITE(MEM, ADDR, word_t, MD_SWAPW(VAL));
#define MEM_WRITE_SWORD(MEM, ADDR, VAL)   MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL));
#define MEM_WRITE_DWORD(MEM, ADDR, VAL)   MEM_WRITE(MEM, ADDR, dword_t, MD_SWAPD(VAL));
#define MEM_WRITE_SDWORD(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, sdword_t, MD_SWAPD(VAL));
#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, sfloat_t, MD_SWAPD(VAL));
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, dfloat_t, MD_SWAPQ(VAL));


#ifdef HOST_HAS_QWORD
#ifdef TARGET_HAS_UNALIGNED_QWORD
void MEM_QWRITE(struct mem_t *mem, md_addr_t addr, qword_t val);
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)          \
  MEM_QWRITE(MEM, ADDR, MD_SWAPQ(VAL))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)        \
  MEM_QWRITE(MEM, ADDR, MD_SWAPQ(VAL))
#else /* !TARGET_HAS_UNALIGNED_QWORD */
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)          \
  MEM_WRITE(MEM, ADDR, qword_t, MD_SWAPQ(VAL))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)        \
  MEM_WRITE(MEM, ADDR, sqword_t, MD_SWAPQ(VAL))
#endif
#endif /* HOST_HAS_QWORD */

/* create a flat memory space */
struct mem_t *
mem_create(char *name);      /* name of the memory space */

/* translate address ADDR in memory space MEM, returns pointer to host page */
byte_t *
mem_translate(struct mem_t *mem,  /* memory space to access */
    md_addr_t addr,     /* virtual address to translate */
    int dirty); /* should this page be marked as dirty? */

/* allocate a memory page */
void
mem_newpage(struct mem_t *mem,    /* memory space to allocate in */
    md_addr_t addr);    /* virtual address to allocate */

/* zero out *ALL* of memory and reset backing file info */
void wipe_memory(struct mem_t * mem);

/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses
 */
md_addr_t
mem_newmap(struct mem_t *mem,           /* memory space to access */
    md_addr_t    addr,            /* virtual address to map to */
    size_t       length,
    int mmap);

/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses, but point to something
 * we allocated ourselves.
 */
md_addr_t
mem_newmap2(struct mem_t *mem,           /* memory space to access */
    md_addr_t    addr,            /* virtual address to map to */
    md_addr_t    our_addr,        /* our stuff (return value from mmap) */
    size_t       length,
    int mmap);

/* given a set of pages, this removes them from our map.  Necessary
 * for munmap.
 */
void
mem_delmap(struct mem_t *mem,           /* memory space to access */
    md_addr_t    addr,            /* virtual address to delete */
    size_t       length);


/* check for memory faults */
enum md_fault_type
mem_check_fault(struct mem_t *mem,  /* memory space to access */
    enum mem_cmd cmd,  /* Read (from sim mem) or Write */
    md_addr_t addr,    /* target address to access */
    int nbytes);    /* number of bytes to access */

/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
enum md_fault_type
mem_access(
    int core_id,
    struct mem_t *mem,    /* memory space to access */
    enum mem_cmd cmd,    /* Read (from sim mem) or Write */
    md_addr_t addr,    /* target address to access */
    void *vp,      /* host memory address to access */
    int nbytes);      /* number of bytes to access */

/* register memory system-specific statistics */
void
mem_reg_stats(struct mem_t *mem,  /* memory space to declare */
    struct stat_sdb_t *sdb);  /* stats data base */

/* initialize memory system, call before loader.c */
void
mem_init(struct mem_t *mem);  /* memory space to initialize */

/*
 * memory accessor routines, these routines require a memory access function
 * definition to access memory, the memory access function provides a "hook"
 * for programs to instrument memory accesses, this is used by various
 * simulators for various reasons; for the default operation - direct access
 * to the memory system, pass mem_access() as the memory access function
 */

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
enum md_fault_type
mem_strcpy(
    int core_id,
    mem_access_fn mem_fn,  /* user-specified memory accessor */
    struct mem_t *mem,    /* memory space to access */
    enum mem_cmd cmd,    /* Read (from sim mem) or Write */
    md_addr_t addr,    /* target address to access */
    char *s);      /* host memory string buffer */

/* copy NBYTES to/from simulated memory space, returns any faults */
enum md_fault_type
mem_bcopy(
    int core_id,
    mem_access_fn mem_fn,    /* user-specified memory accessor */
    struct mem_t *mem,    /* memory space to access */
    enum mem_cmd cmd,    /* Read (from sim mem) or Write */
    md_addr_t addr,    /* target address to access */
    void *vp,      /* host memory address to access */
    int nbytes);      /* number of bytes to access */


/* maps each (core-id,virtual-address) pair to a simulated physical address */
md_paddr_t v2p_translate(int core_id, md_addr_t virt_addr);
/* given a physical address, return the corresponding core-id */
int page_owner(md_paddr_t paddr);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */
