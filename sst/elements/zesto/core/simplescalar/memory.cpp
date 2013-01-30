/* memory.c - flat memory space routines
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

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "options.h"
#include "stats.h"
#include "memory.h"

/* FYI, the following functions are used to emulate unaligned quadword
   memory accesses on architectures that require aligned quadword
   memory accesses, e.g., ARM emulation on SPARC hardware... */
#ifdef TARGET_HAS_UNALIGNED_QWORD

/* read a quadword, unaligned */
  qword_t
MEM_QREAD(struct mem_t *mem, md_addr_t addr)
{
  qword_t temp;
  mem_access(mem, Read, addr, &temp, sizeof(qword_t));
  return temp;
}

/* write a quadword, unaligned */
  void
MEM_QWRITE(struct mem_t *mem, md_addr_t addr, qword_t val)
{
  mem_access(mem, Write, addr, &val, sizeof(qword_t));
}
#endif /* TARGET_HAS_UNALIGNED_QWORD */

/* create a flat memory space */
  struct mem_t *
mem_create(char *name)			/* name of the memory space */
{
  struct mem_t *mem;

  mem = (struct mem_t*) calloc(1, sizeof(struct mem_t));
  if (!mem)
    fatal("out of virtual memory");

  mem->name = mystrdup(name);
  return mem;
}

static inline void recency_list_insert(struct mem_t * mem, struct mem_pte_t * pte)
{

  /* insert PTE into recency list */
  pte->lru_next = mem->recency_mru;
  if(mem->recency_mru)
    mem->recency_mru->lru_prev = pte;
  mem->recency_mru = pte;
  pte->lru_prev = NULL;
  if(mem->recency_lru == NULL)
    mem->recency_lru = pte; /* inserting first pte, so it's both lru and mru */
}

static inline void recency_list_remove(struct mem_t * mem, struct mem_pte_t * pte)
{
  if(mem->recency_mru == pte)
    mem->recency_mru = pte->lru_next;
  if(mem->recency_lru == pte)
    mem->recency_lru = pte->lru_prev;
  if(pte->lru_next)
    pte->lru_next->lru_prev = pte->lru_prev;
  if(pte->lru_prev)
    pte->lru_prev->lru_next = pte->lru_next;
}

/*************************************************************/
/* The following code is for supporting the simulation of
   multiple, large virtual memory spaces.  Since the simulator
   itself is only operating out of a 32-bit space, emulating
   multiple 32-bit spaces can obviously run out of room.  So we
   manually page the emulated virtual memory spaces to disk. */
/*************************************************************/
static qword_t num_core_pages = 0;
/* assume a max of 1.5GB for all simulated memory (this is real memory
   used by the simulator to simulate *all* of the virtual memory for
   all processes/threads.  We manually page stuff to disk to keep
   this under control.  Otherwise there's no way we can simulate 16
   (or more?) processes' worth of virtual memory in our own measly
   32-bit address sapce. */
static const qword_t max_core_pages = ((1<<30)+(1<<29))/MD_PAGE_SIZE; /* use 1.5GB for memory pages */
static int num_locked_pages = 0; /* num mmaped pages that we don't write to backing file */
static struct mem_page_link_t * page_free_list = NULL;
static struct mem_page_link_t * link_free_list = NULL;

/* make sure you don't write current_addr back out, as that's what we're about to write */
static void write_core_to_backing_file(struct mem_t * mem, md_addr_t current_addr)
{
  /* find LRU pte */
  struct mem_pte_t * evictee = mem->recency_lru;
  while(evictee)
  {
    if(!evictee->no_dealloc && (evictee->addr != current_addr))
      break;
    evictee = evictee->lru_prev;
  }
  if(evictee)
  {
    if(!mem->backing_file)
    {
      /* allocate a temp file for paging simulated memory to disk */
      mem->backing_file = tmpfile();
      mem->backing_file_tail = 0LL;
      if(!mem->backing_file)
        fatal("failed to create memory/core backing file for %s",mem->name);
    }

    if(evictee->dirty)
    {
      if(evictee->backing_offset == -1)
      {
        evictee->backing_offset = mem->backing_file_tail;
        mem->backing_file_tail += MD_PAGE_SIZE;
      }
      if(fseeko(mem->backing_file,evictee->backing_offset,SEEK_SET))
      {
        perror("fseeko call failed");
        fatal("couldn't write out page %u for %s",evictee->addr,mem->name);
      }
      if(fwrite(evictee->page,MD_PAGE_SIZE,1,mem->backing_file) < 1)
      {
        perror("fwrite call failed");
        fatal("couldn't write out page %u for %s",evictee->addr,mem->name);
      }
      evictee->dirty = FALSE; /* not dirty anymore */
    }

    /* 1. get page link (allocate if necessary)
       2. memset page
       3. place page in link, and link in free list
       4. delink recency lru/mru pointers
     */
    struct mem_page_link_t * pl = link_free_list;
    if(!pl)
    {
      pl = (struct mem_page_link_t*) calloc(1,sizeof(*pl));
      if(!pl)
        fatal("failed to calloc mem_page_link");
    }
    //memset(evictee->page,0,MD_PAGE_SIZE);
    clear_page(evictee->page);
    pl->page = evictee->page;
    pl->next = page_free_list;
    page_free_list = pl;
    num_core_pages --;

    recency_list_remove(mem,evictee);
    evictee->page = NULL;
  }
  else
    warnonce("we needed to page out memory for %s (addr=0x%x), but we couldn't find such a page.  consider increasing the memory allocated for simulated memory.",mem->name,current_addr);
}

static void read_core_from_backing_file(struct mem_t * mem, struct mem_pte_t * pte)
{
  /* 1. get page/disk info
     2. get blank page (allocate if necessary)
     3. place page link on free list if necessary
     4. fread disk to page
     5. reinsert page into recency lists
   */
  struct mem_page_link_t * pl = page_free_list;
  byte_t * page = NULL;
  assert(pte->page == NULL); /* this pte had better not already have a page allocated */
  if(pl)
  {
    page_free_list = pl->next;
    page = pl->page;
    pl->page = NULL;
    pl->next = link_free_list;
    link_free_list = pl;
  }
  else
  {
    //page = (byte_t*) calloc(1,MD_PAGE_SIZE);
    posix_memalign((void**)&page,MD_PAGE_SIZE,MD_PAGE_SIZE);
    if(!page)
      fatal("failed to calloc memory page");
    clear_page(page);
  }

  if(pte->backing_offset != -1)
  {
    assert(mem->backing_file);
    assert(!pte->page);
    assert(!pte->no_dealloc);

    if(fseeko(mem->backing_file,pte->backing_offset,SEEK_SET))
    {
      perror("fseeko call failed");
      fatal("couldn't reload page %u for %s",pte->addr,mem->name);
    }
    if(fread(page,MD_PAGE_SIZE,1,mem->backing_file) < 1)
    {
      perror("fread call failed");
      fatal("couldn't reload page %u for %s",pte->addr,mem->name);
    }
  }
  else /* no allocated page in backing file: this page was on disk
          when memory was wiped so just load in a zeroed out page.
        */
  {
    /* in which case we don't really need to do anything because we
       either just calloc'd the page, or we got it off the free
       list, but all pages also get memset'd before getting inserted
       on the free list. */
  }

  pte->page = page;
  recency_list_insert(mem,pte);
  num_core_pages ++;
}

/* Used when resetting an eio trace to run from beginning.  This may
   be necessary since normally the first touch of a page would
   return a zeroed-out page, but reusing the memory pages could
   leave data from the previous run.  We also need to reset the
   backing file information. */
void wipe_memory(struct mem_t * mem)
{
  int i;
  for(i=0; i< MEM_PTAB_SIZE; i++)
  {
    struct mem_pte_t * pte = mem->ptab[i];

    while(pte)
    {
      if(pte->page)
        //memset(pte->page,0,MD_PAGE_SIZE);
        clear_page(pte->page);
      pte->backing_offset = -1;
      pte->dirty = TRUE; /* need to mark dirty to ensure that this
                            zero'd page gets written back to disk
                            (same as mem_newpage). */
      pte = pte->next;
    }
  }
  if(mem->backing_file)
    fclose(mem->backing_file);
  mem->backing_file = NULL;
  mem->backing_file_tail = 0;
}


/* Code for providing simulated virtual-to-physical address
   translation.  This allocates physical pages to virtual pages on a
   first-come-first-serve basis.  In this fashion, pages from
   different cores/processes will be a little more distributed (more
   realistic), which also helps to reduce pathological aliasing
   effects (e.g., having all N cores' stacks map to the same exact
   cache sets). */
static md_paddr_t next_ppn_to_allocate = 0x00000100; /* arbitrary starting point; NOTE: this is a page number, not a page starting address */
#define V2P_HASH_SIZE 65536
#define V2P_HASH_MASK (V2P_HASH_SIZE-1)
struct v2p_node_t {
  int thread_id;
  md_addr_t vpn; /* virtual page number */
  md_paddr_t ppn; /* physical page number */
  struct v2p_node_t * next;
  struct v2p_node_t * p2v_next; /* for linking the p2v table */
};

static struct v2p_node_t *v2p_hash_table[V2P_HASH_SIZE];
static struct v2p_node_t *p2v_list_head = NULL;

md_paddr_t v2p_translate(int thread_id, md_addr_t virt_addr)
{
  md_addr_t VPN = virt_addr >> PAGE_SHIFT;
  int index = VPN & (V2P_HASH_SIZE-1);
  struct v2p_node_t * p = v2p_hash_table[index], * prev = NULL;

  while(p)
  {
    if((p->thread_id == thread_id) && (p->vpn == VPN))
    {
      /* hit! */
      break;
    }
    prev = p;
    p = p->next;
  }

  if(!p) /* page miss: allocate a new physical page */
  {
    p = (struct v2p_node_t*) calloc(1,sizeof(*p));
    if(!p)
      fatal("couldn't calloc a new page table entry");
    p->thread_id = thread_id;
    p->vpn = VPN;
    p->ppn = next_ppn_to_allocate++;
    p->next = v2p_hash_table[index];

    p->p2v_next = p2v_list_head;
    p2v_list_head = p;

    v2p_hash_table[index] = p;
  }
  else if(prev)
  {
    /* move p to the front of the list (MRU) */
    prev->next = p->next;
    p->next = v2p_hash_table[index];
    v2p_hash_table[index] = p;
  }

  /* construct the final physical page number */
  return (p->ppn << PAGE_SHIFT) + (virt_addr & (PAGE_SIZE-1));
}

/* Inverse lookup: given a physical page, returns the core-id of
   the owner.  TODO: put into separate hash table. */
int page_owner(md_paddr_t paddr)
{
  md_paddr_t PPN = paddr >> PAGE_SHIFT;
  int tid = -1, hit = 0;
  struct v2p_node_t * p = p2v_list_head, * prev = NULL;

  while(p)
  {
    if(p->ppn == PPN)
    {
      tid = p->thread_id;
      hit = 1;
      break;
    }
    prev = p;
    p = p->p2v_next;
  }

  /* hit, move to front of list */
  if(hit && prev)
  {
    /* move p to the front of the list (MRU) */
    prev->p2v_next = p->p2v_next;
    p->p2v_next = p2v_list_head;
    p2v_list_head = p;
  }

  if(hit)
    return tid;
  else
    return DO_NOT_TRANSLATE;
}


/*************************************************************/


/* translate address ADDR in memory space MEM, returns pointer to host page */
  byte_t *
mem_translate(struct mem_t *mem,	/* memory space to access */
    md_addr_t addr,		/* virtual address to translate */
    int dirty) /* mark this page as dirty */
{
  struct mem_pte_t *pte, *prev;

  /* locate accessed PTE */
  for (prev=NULL, pte=mem->ptab[MEM_PTAB_SET(addr)];
      pte != NULL;
      prev=pte, pte=pte->next)
  {
    if (pte->tag == MEM_PTAB_TAG(addr))
    {
      /* move this PTE to head of the bucket list */
      if (prev)
      {
        prev->next = pte->next;
        pte->next = mem->ptab[MEM_PTAB_SET(addr)];
        mem->ptab[MEM_PTAB_SET(addr)] = pte;
      }

      if(!pte->page)
      {
        /* make room if necessary */
        if(max_core_pages && (num_core_pages >= max_core_pages))
          write_core_to_backing_file(mem,pte->addr);
        /* page in from disk */
        read_core_from_backing_file(mem,pte);
      }
      else
      {
        /* update recency list */
        assert(pte->lru_prev || (mem->recency_mru == pte));
        assert(pte->lru_next || (mem->recency_lru == pte));
      }

      if(mem->recency_mru != pte) /* don't need to move if alreayd in mru position */
      {
        recency_list_remove(mem,pte);
        recency_list_insert(mem,pte);
      }

      pte->dirty |= dirty;

      return pte->page;
    }
  }

  /* no translation found, return NULL */
  return NULL;
}

/* allocate a memory page */
  void
mem_newpage(struct mem_t *mem,		/* memory space to allocate in */
    md_addr_t addr)		/* virtual address to allocate */
{
  byte_t *page = NULL;
  struct mem_pte_t *pte;

  if(max_core_pages && (num_core_pages >= max_core_pages))
    write_core_to_backing_file(mem,addr);

  /* see if we have any spare pages lying around */
  struct mem_page_link_t * pl = page_free_list;
  if(pl)
  {
    page_free_list = pl->next;
    page = pl->page;
    pl->page = NULL;
    pl->next = link_free_list;
    link_free_list = pl;
  }
  else /* if not, alloc a new one */
  {
    posix_memalign((void**)&page,MD_PAGE_SIZE,MD_PAGE_SIZE);
    if(!page)
      fatal("failed to calloc memory page");
    clear_page(page);
  }
  *page = 0; /* touch the page */

  /* generate a new PTE */
  pte = (struct mem_pte_t*) calloc(1, sizeof(struct mem_pte_t));
  if (!pte)
    fatal("out of virtual memory");
  pte->tag = MEM_PTAB_TAG(addr);
  pte->addr = addr;
  pte->page = page;
  pte->backing_offset = -1;
  pte->dirty = TRUE;

  /* insert PTE into inverted hash table */
  pte->next = mem->ptab[MEM_PTAB_SET(addr)];
  mem->ptab[MEM_PTAB_SET(addr)] = pte;

  recency_list_insert(mem,pte);

  /* one more page allocated */
  mem->page_count++;
  num_core_pages++;
}

// skumar - for implementing mmap syscall

/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses
 */
  md_addr_t
mem_newmap(struct mem_t *mem,            /* memory space to access */
    md_addr_t    addr,            /* virtual address to map to */
    size_t       length,
    int mmap)	
{
  int num_pages;
  int i;
  md_addr_t comp_addr;
  struct mem_pte_t *pte;
  /* first check alignment */
  if((addr & (MD_PAGE_SIZE-1))!=0) {
    fprintf(stderr, "mem_newmap address %x, not page aligned\n", addr);
    abort();
  }

  num_pages = length / MD_PAGE_SIZE + ((length % MD_PAGE_SIZE>0)? 1 : 0);
  for(i=0;i<num_pages;i++) {
    /* check if pages already allocated */
    comp_addr = addr+i*MD_PAGE_SIZE;
    if(MEM_PAGE(mem, comp_addr, 1)) { /* new pages are marked dirty to make sure they get written out if necessary */
#ifdef DEBUG
      if(debugging) 
        fprintf(stderr, "mem_newmap warning addr %x(page %d), already been allocated\n", comp_addr, i);
#endif
      // abort();
      continue;
    }

    /* make room if necessary */
    if(max_core_pages && (num_core_pages >= max_core_pages))
      write_core_to_backing_file(mem,comp_addr);

    /* generate a new PTE */
    pte = (struct mem_pte_t*) calloc(1, sizeof(struct mem_pte_t));
    if (!pte)
      fatal("out of virtual memory");
    pte->tag = MEM_PTAB_TAG(comp_addr);
    pte->addr = addr;
    pte->page = (byte_t*) comp_addr;
    pte->from_mmap_syscall = mmap;
    pte->backing_offset = -1;
    pte->no_dealloc = TRUE;
    num_locked_pages++;

    /* insert PTE into inverted hash table */
    pte->next = mem->ptab[MEM_PTAB_SET(comp_addr)];
    mem->ptab[MEM_PTAB_SET(comp_addr)] = pte;

    recency_list_insert(mem,pte);

    /* one more page allocated */
    mem->page_count++;
    num_core_pages++;
  }
  return addr;

}

/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses, but point to our stuff
 */
  md_addr_t
mem_newmap2(struct mem_t *mem,            /* memory space to access */
    md_addr_t    addr,            /* virtual address to map to */
    md_addr_t    our_addr,        /* Our stuff (return from system mmap) */
    size_t       length,
    int mmap /* call from mmap or mmap2? */)	
{
  int num_pages;
  int i;
  md_addr_t comp_addr;
  struct mem_pte_t *pte;
  /* first check alignment */
  if((addr & (MD_PAGE_SIZE-1))!=0) {
    fprintf(stderr, "mem_newmap address %x, not page aligned\n", addr);
    abort();
  }
  if((((unsigned long long int)our_addr) & (MD_PAGE_SIZE-1))!=0) {
    fprintf(stderr, "mem_newmap our_addr %x, not page aligned\n", addr);
    abort();
  }

  num_pages = length / MD_PAGE_SIZE + ((length % MD_PAGE_SIZE>0)? 1 : 0);
  for(i=0;i<num_pages;i++) {
    /* check if pages already allocated */
    comp_addr = addr+i*MD_PAGE_SIZE;
    if(MEM_PAGE(mem, comp_addr, 1)) {
#ifdef DEBUG
      if(debugging) 
        fprintf(stderr, "mem_newmap warning addr %x(page %d), already been allocated\n", comp_addr, i);
#endif
      continue;
    }

    /* make room if necessary */
    if(max_core_pages && (num_core_pages >= max_core_pages))
      write_core_to_backing_file(mem,comp_addr);

    /* generate a new PTE */
    pte = (struct mem_pte_t*) calloc(1, sizeof(struct mem_pte_t));
    if (!pte)
      fatal("out of virtual memory");
    pte->tag = MEM_PTAB_TAG(comp_addr);
    pte->addr = addr;
    pte->page = (byte_t*) our_addr + i * MD_PAGE_SIZE;
    pte->from_mmap_syscall = mmap;
    pte->backing_offset = -1;
    pte->no_dealloc = TRUE;
    num_locked_pages++;

    /* insert PTE into inverted hash table */
    pte->next = mem->ptab[MEM_PTAB_SET(comp_addr)];
    mem->ptab[MEM_PTAB_SET(comp_addr)] = pte;

    recency_list_insert(mem,pte);

    /* one more page allocated */
    mem->page_count++;
    num_core_pages++;
  }
  return addr;

}

/* TODO: I think delmap will not work if the pages that were originally mmapped were
   loaded in through an EIO checkpoint... not sure what's the best way to deal with
   this. */
/* given a set of pages, delete them from the page table.
 */
  void
mem_delmap(struct mem_t *mem,            /* memory space to access */
    md_addr_t    addr,            /* virtual address to remove */
    size_t       length)	
{
  int num_pages;
  int i;
  md_addr_t comp_addr = addr;
  struct mem_pte_t *temp, *before_temp = NULL;

  /* first check alignment */
  if((addr & (MD_PAGE_SIZE-1))!=0) {
    fprintf(stderr, "mem_delmap address %x, not page aligned\n", addr);
    abort();
  }

  num_pages = length / MD_PAGE_SIZE + ((length % MD_PAGE_SIZE>0)? 1 : 0);
  for(i=0;i<num_pages;i++) {
    /* check if pages already allocated */
    comp_addr = addr+i*MD_PAGE_SIZE;
    int set = MEM_PTAB_SET(comp_addr);

    if(!MEM_PAGE(mem, comp_addr, 0)) {
      // this is OK -- pin does this all the time.
      continue;
    }

    for (temp = mem->ptab[set] ; temp ; before_temp = temp, temp = temp->next)
    {
      if ((temp->tag == MEM_PTAB_TAG(comp_addr) && temp->from_mmap_syscall))
      {
        if (before_temp)
        {
          before_temp->next = temp->next;
        }
        else
        {
          mem->ptab[set] = temp->next;
        }

        recency_list_remove(mem,temp);

        temp->page = NULL;
        temp->next = NULL;
        temp->from_mmap_syscall = FALSE;
        assert(temp->no_dealloc);
        temp->no_dealloc = FALSE;
        temp->tag = (md_addr_t)NULL;
        num_locked_pages--;
        free( temp );

        /* one less page allocated */
        mem->page_count--;

        break;
      }
    }
  }
}
// <--- 

/* check for memory related faults */
  enum md_fault_type
mem_check_fault(struct mem_t *mem,	/* memory */
    enum mem_cmd cmd,	/* Read or Write */
    md_addr_t addr,		/* target addresss */
    int nbytes)		/* number of bytes of access */
{
  /* check alignments */
  if (/* check size */(nbytes & (nbytes-1)) != 0
      || /* check max size */nbytes > MD_PAGE_SIZE)
    return md_fault_access;

#ifdef TARGET_ARM
  if (/* check natural alignment */(addr & (((nbytes > 4) ? 4:nbytes)-1)) != 0)
    return md_fault_alignment;
#else
  if (/* check natural alignment */(addr & (nbytes-1)) != 0)
    return md_fault_alignment;
#endif /* TARGET_ARM */

  return md_fault_none;
}

/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
  enum md_fault_type
mem_access(
    int core_id,
    struct mem_t *mem,		/* memory space to access */
    enum mem_cmd cmd,		/* Read (from sim mem) or Write */
    md_addr_t addr,		/* target address to access */
    void *vp,			/* host memory address to access */
    int nbytes)			/* number of bytes to access */
{
  enum md_fault_type fault;
  byte_t *p = (byte_t*)vp;

  /* check for faults */
  fault = mem_check_fault(mem, cmd, addr, nbytes);
  if (fault != md_fault_none) return fault;

  /* perform the copy */
  if (cmd == Read) {

    while (nbytes-- > 0) {
      *((byte_t *)p) = MEM_READ_BYTE(mem, addr);
      p += sizeof(byte_t);
      addr += sizeof(byte_t);
    }
  }
  else {

    while (nbytes-- > 0) {
      MEM_WRITE_BYTE(mem, addr, *((byte_t *)p));
      p += sizeof(byte_t);
      addr += sizeof(byte_t);
    }
  }

  /* no fault... */
  return md_fault_none;
}

/* register memory system-specific statistics */
  void
mem_reg_stats(struct mem_t *mem,	/* memory space to declare */
    struct stat_sdb_t *sdb)	/* stats data base */
{
  char buf[512], buf1[512];

  stat_reg_note(sdb,"\n#### SIMULATED MEMORY STATS ####");
  sprintf(buf, "%s.page_count", mem->name);
  stat_reg_counter(sdb, TRUE, buf, "total number of pages allocated",
      &mem->page_count, mem->page_count, NULL);

  sprintf(buf, "%s.page_mem", mem->name);
  sprintf(buf1, "%s.page_count * %d / 1024", mem->name, MD_PAGE_SIZE);
  stat_reg_formula(sdb, TRUE, buf, "total size of memory pages allocated",
      buf1, "%11.0fk");
}

/* initialize memory system, call before loader.c */
  void
mem_init(struct mem_t *mem)	/* memory space to initialize */
{
  int i;

  /* initialize the first level page table to all empty */
  for (i=0; i < MEM_PTAB_SIZE; i++)
    mem->ptab[i] = NULL;

  mem->page_count = 0;
}

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
  enum md_fault_type
mem_strcpy(
    int core_id,
    mem_access_fn mem_fn,	/* user-specified memory accessor */
    struct mem_t *mem,		/* memory space to access */
    enum mem_cmd cmd,		/* Read (from sim mem) or Write */
    md_addr_t addr,		/* target address to access */
    char *s)
{
  int n = 0;
  char c;
  enum md_fault_type fault;

  switch (cmd)
  {
    case Read:
      /* copy until string terminator ('\0') is encountered */
      do {
        fault = mem_fn(core_id, mem, Read, addr++, &c, 1);
        if (fault != md_fault_none)
          return fault;
        *s++ = c;
        n++;
      } while (c);
      break;

    case Write:
      /* copy until string terminator ('\0') is encountered */
      do {
        c = *s++;
        fault = mem_fn(core_id, mem, Write, addr++, &c, 1);
        if (fault != md_fault_none)
          return fault;
        n++;
      } while (c);
      break;

    default:
      return md_fault_internal;
  }

  /* no faults... */
  return md_fault_none;
}

/* copy NBYTES to/from simulated memory space, returns any faults */
  enum md_fault_type
mem_bcopy(
    int core_id,
    mem_access_fn mem_fn,		/* user-specified memory accessor */
    struct mem_t *mem,		/* memory space to access */
    enum mem_cmd cmd,		/* Read (from sim mem) or Write */
    md_addr_t addr,		/* target address to access */
    void *vp,			/* host memory address to access */
    int nbytes)
{
  byte_t *p = (byte_t*)vp;
  enum md_fault_type fault;
  byte_t* tempP;

  /* copy NBYTES bytes to/from simulator memory */
  while (nbytes-- > 0)
  {
    tempP = p;
    fault = mem_fn(core_id, mem, cmd, addr, p, 1);
    addr++;
    if(tempP == p)
    {
      p++;
    }
    //p++;
    if (fault != md_fault_none)
      return fault;
  }

  /* no faults... */
  return md_fault_none;
}

