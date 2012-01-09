/* zesto-cache.cpp - Zesto cache structure
 * 
 *	Modified by: Dhruv Choudhary
 *	Date: 27th July 2010
 *	Organizations: Georgia Institute of Technology
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

#include <limits.h>
#include "thread.h"
#include "stats.h"
#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-cache.h"
#include "zesto-prefetch.h"

#include "zesto-exec.h"
#include "zesto-commit.h"

struct cache_t * cache_create_llc(
    struct core_t * const core,
    const char * const name,
    const bool read_only,
    const int sets,
    const int assoc,
    const int linesize,
    const char rp, /* replacement policy */
    const char ap, /* allocation policy */
    const char wp, /* write policy */
    const char wc, /* write combining */
    const int banks,
    const int bank_width, /* in bytes; i.e., the interleaving granularity */
    const int latency, /* in cycles */
    const int WBB_size, /* writeback-buffer size, in cache lines (for evictions) */
    const int MSHR_size, /* MSHR size (per bank), in requests */
    const int MSHR_banks, /* number of MSHR banks */
    const int begin_index,/* In case a shared cache what chunk lies with this cache*/
    const int end_index,
    struct cache_t * const next_level_cache, /* e.g., for the DL1, this should point to the L2 */
    struct bus_t * const next_bus /* e.g., for the DL1, this should point to the bus between DL1 and L2 */
    )
{
  int i;
  struct cache_t * cp = (struct cache_t*) calloc(1,sizeof(*cp));
  if(!cp)
    fatal("failed to calloc cache %s",name);

  enum repl_policy_t replacement_policy;
  enum alloc_policy_t allocate_policy;
  enum write_policy_t write_policy;
  int write_combining;

  cp->core = core;

  switch(mytoupper(rp)) {
    case 'L': replacement_policy = REPLACE_LRU; break;
    case 'M': replacement_policy = REPLACE_MRU; break;
    case 'R': replacement_policy = REPLACE_RANDOM; break;
    case 'N': replacement_policy = REPLACE_NMRU; break;
    case 'P': replacement_policy = REPLACE_PLRU; break;
    case 'C': replacement_policy = REPLACE_CLOCK; break;
    default: fatal("unrecognized cache replacement policy");
  }
  switch(mytoupper(ap)) {
    case 'W': allocate_policy = WRITE_ALLOC; break;
    case 'N': allocate_policy = NO_WRITE_ALLOC; break;
    default: fatal("unrecognized cache allocation policy (W=write-alloc,N=no-write-alloc)");
  }
  switch(mytoupper(wp)) {
    case 'T': write_policy = WRITE_THROUGH; break;
    case 'B': write_policy = WRITE_BACK; break;
    default: fatal("unrecognized cache write policy (T=write-Through,B=write-Back)");
  }
  switch(mytoupper(wc)) {
    case 'C': write_combining = true; break;
    case 'N': write_combining = false; break;
    default: fatal("unrecognized cache write combining option (C=write-Combining,N=No-write-combining)");
  }

  cp->name = strdup(name);
  if(!cp->name)
    fatal("failed to strdup %s's name",name);
  cp->read_only = (enum read_only_t) read_only;
  cp->sets = sets;
  cp->assoc = assoc;
  cp->log2_assoc = (int)(ceil(log(assoc)/log(2.0)));
  cp->linesize = linesize;
  cp->replacement_policy = replacement_policy;
  cp->allocate_policy = allocate_policy;
  cp->write_policy = write_policy;
  cp->banks = banks;
  cp->bank_width = bank_width;
  cp->bank_mask = banks-1;
  cp->latency = latency;
  cp->write_combining = write_combining;
  cp->WBB_size = WBB_size;
  cp->MSHR_size = MSHR_size;
  cp->MSHR_banks = MSHR_banks;
  cp->MSHR_mask = MSHR_banks-1;
  cp->next_level = next_level_cache;
  cp->next_bus = next_bus;
  cp->check_for_work = true;
  cp->check_for_MSHR_fill_work = true;

  if((cp->replacement_policy == REPLACE_PLRU) && (assoc & (assoc-1)))
    fatal("Tree-based PLRU only works when associativity is a power of two");
  if((cp->replacement_policy == REPLACE_CLOCK) && (assoc > 64))
    fatal("Clock-based PLRU only works when associativity is <= 64");

  /* bits to shift to get to the tag */
  cp->addr_shift = (int)ceil(log(linesize)/log(2.0));
  /* bits to shift to get the bank number */
/* ----- SoM ----- */
/* Shouldn't this be the "banks" instead of "linesize" ? 
  cp->bank_shift = (int)ceil(log(linesize)/log(2.0));
*/
  cp->bank_shift = (int)ceil(log(banks)/log(2.0));
/* ----- EoM ----- */

  if((sets & (sets-1)) != 0)
    fatal("%s cache sets must be power of two");
  if((linesize & (linesize-1)) != 0)
    fatal("%s linesize must be power of two");
  if((banks & (banks-1)) != 0)
    fatal("%s banks must be power of two");

  cp->blocks = (struct cache_line_t**) calloc(sets,sizeof(*cp->blocks));
  if(!cp->blocks)
    fatal("failed to calloc cp->blocks");
  for(i=0;i<sets;i++)
  {
    /* allocate as one big block! */
    struct cache_line_t * line = (struct cache_line_t*) calloc(assoc,sizeof(*line));
    memset(line,0,assoc*sizeof(*line)); // for some weird reason valgrind was reporting that "line" was being used uninitialized, despite being calloc'd above?
    if(!line)
      fatal("failed to calloc cache line for %s",name);

    for(int j=0;j<assoc;j++)
    {
      line[j].way = j;
      line[j].next = &line[j+1];
    }
    line[assoc-1].next = NULL;
    cp->blocks[i] = line;
  }

  cp->heap_size = 1 << ((int) rint(ceil(log(latency+1)/log(2.0))));

  cp->pipe = (struct cache_action_t**) calloc(banks,sizeof(*cp->pipe));
  cp->pipe_num = (int*) calloc(banks,sizeof(*cp->pipe_num));
  cp->fill_pipe = (struct cache_fill_t**) calloc(banks,sizeof(*cp->fill_pipe));
  cp->fill_num = (int*) calloc(banks,sizeof(*cp->fill_num));
  if(!cp->pipe || !cp->fill_pipe || !cp->pipe_num || !cp->fill_num)
    fatal("failed to calloc cp->pipe for %s",name);
  for(i=0;i<banks;i++)
  {
    cp->pipe[i] = (struct cache_action_t*) calloc(cp->heap_size,sizeof(**cp->pipe));
    if(!cp->pipe[i])
      fatal("failed to calloc cp->pipe[%d] for %s",i,name);
    cp->fill_pipe[i] = (struct cache_fill_t*) calloc(cp->heap_size,sizeof(**cp->fill_pipe));
    if(!cp->fill_pipe[i])
      fatal("failed to calloc cp->fill_pipe[%d] for %s",i,name);
  }

  cp->MSHR = (struct cache_action_t**) calloc(MSHR_banks,sizeof(*cp->MSHR));
  if(!cp->MSHR)
    fatal("failed to calloc cp->MSHR for %s",name);
  for(i=0;i<MSHR_banks;i++)
  {
    cp->MSHR[i] = (struct cache_action_t*) calloc(MSHR_size,sizeof(**cp->MSHR));
    if(!cp->MSHR[i])
      fatal("failed to calloc cp->MSHR[%d] for %s",i,name);
  }

  cp->MSHR_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_num));
  if(!cp->MSHR_num)
    fatal("failed to calloc cp->MSHR_num");
  cp->MSHR_num_pf = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_num_pf));
  if(!cp->MSHR_num_pf)
    fatal("failed to calloc cp->MSHR_num_pf");
  cp->MSHR_fill_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_fill_num));
  if(!cp->MSHR_fill_num)
    fatal("failed to calloc cp->MSHR_fill_num");
  cp->MSHR_unprocessed_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_unprocessed_num));
  if(!cp->MSHR_unprocessed_num)
    fatal("failed to calloc cp->MSHR_unprocessed_num");

  cp->WBB_num = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_num));
  if(!cp->WBB_num)
    fatal("failed to calloc cp->WBB_num");
  cp->WBB_head = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_head));
  if(!cp->WBB_head)
    fatal("failed to calloc cp->WBB_head");
  cp->WBB_tail = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_tail));
  if(!cp->WBB_tail)
    fatal("failed to calloc cp->WBB_tail");

  cp->WBB = (struct cache_line_t**) calloc(MSHR_banks,sizeof(*cp->WBB));
  if(!cp->WBB)
    fatal("failed to calloc cp->WBB");
  for(i=0;i<MSHR_banks;i++)
  {
    cp->WBB[i] = (struct cache_line_t*) calloc(WBB_size,sizeof(**cp->WBB));
    if(!cp->WBB[i])
      fatal("failed to calloc cp->WBB[%d] for %s",i,name);
  }

  if(!strcasecmp(name,"LLC"))
  {
    cp->stat.core_lookups = (zcounter_t*) calloc(num_threads,sizeof(*cp->stat.core_lookups));
    cp->stat.core_misses = (zcounter_t*) calloc(num_threads,sizeof(*cp->stat.core_misses));
    if(!cp->stat.core_lookups || !cp->stat.core_misses)
      fatal("failed to calloc cp->stat.core_{lookups|misses} for %s",name);
  }

  return cp;
}

void LLC_reg_stats(
    struct stat_sdb_t * const sdb,
    struct cache_t * const cp)
{
}




/* Returns true if the prefetch should get inserted into the cache */
static int prefetch_filter_lookup(
    struct prefetch_filter_t * const p,
    const md_paddr_t addr)
{
  if(sim_cycle >= (p->last_reset + p->reset_interval))
  {
    for(int i=0;i<p->num_entries;i++)
      p->table[i] = 3;
    p->last_reset = sim_cycle - (sim_cycle % p->reset_interval);
    return true;
  }

  int index = addr & p->mask;
  return (p->table[index] >= 2);
}

/* Update the filter's predictor based on whether a prefetch was useful or not. */
static void prefetch_filter_update(
    struct prefetch_filter_t * const p,
    const md_paddr_t addr,
    const int useful)
{
  const int index = addr & p->mask;
  if(sim_cycle > (p->last_reset + p->reset_interval))
  {
    for(int i=0;i<p->num_entries;i++)
      p->table[i] = 3;
    p->last_reset = sim_cycle - (sim_cycle % p->reset_interval);
  }
  if(useful)
  {
    if(p->table[index] < 3)
      p->table[index]++;
  }
  else
  {
    if(p->table[index] > 0)
      p->table[index]--;
  }
}


/* Check to see if a given address can be found in the cache.  This is only a "peek" function
   in that it does not update any hit/miss stats, although it does update replacement state. */
struct cache_line_t * cache_is_hit_llc(
    struct cache_t * const cp,
    const enum cache_command cmd,
    const md_paddr_t addr,
    struct core_t * const core)
{
  const md_paddr_t block_addr = addr >> cp->addr_shift;
  const int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];
  struct cache_line_t * prev = NULL;

  while(p) /* search all of the ways */
  {
    if(p->valid && (p->tag == block_addr)) /* hit */
    {
      if(cmd != CACHE_WRITEBACK)
        switch(cp->replacement_policy)
        {
          case REPLACE_PLRU: /* tree-based pseudo-LRU */
          {
            int bitmask = cp->blocks[index][0].meta;
            const int way = p->way;
            for(int i=0;i<cp->log2_assoc;i++)
            {
              const int pos = (way >> (cp->log2_assoc-i)) + (1<<i);
              
              if((way>>(cp->log2_assoc-i-1)) & 1)
                bitmask |= (1<<pos);
              else
                bitmask &= ~(1<<pos);
            }
            cp->blocks[index][0].meta = bitmask;
          }
            /* NO BREAK IN THE CASE STATEMENT HERE: Do the LRU ordering, too.
               This doesn't affect the behavior of the replacement policy, but
               for highly-associative caches it'll speed up any subsequent
               searches (assuming temporal locality) */

          case REPLACE_RANDOM: /* similar: random doesn't need it, but this can speed up searches */
          case REPLACE_LRU:
          case REPLACE_NMRU:
            {
              if(prev) /* insert back at front of list */
              {
                prev->next = p->next;
                p->next = cp->blocks[index];
                cp->blocks[index] = p;
              }
              break;
            }
          case REPLACE_MRU:
            {
              if(p->next) /* only move node if not already at end of list */
              {
                /* remove node */
                if(prev)
                  prev->next = p->next;
                else
                  cp->blocks[index] = p->next;

                /* go to end of list */
                prev = p->next;
                while(prev->next)
                  prev = prev->next;
                
                /* stick ourselves there */
                prev->next = p;
                p->next = NULL;
              }
              break;
            }
          case REPLACE_CLOCK:
          {
            cp->blocks[index][0].meta |= 1ULL<<p->way; /* set referenced bit */
            break;
          }
          default:
            fatal("policy not yet implemented");
        }

      if(cmd == CACHE_WRITE || cmd == CACHE_WRITEBACK)
      {
        cache_assert(cp->read_only != CACHE_READONLY,NULL);
        if(cp->write_policy == WRITE_BACK) /* write-thru doesn't have dirty lines */
          p->dirty = true;
      }

      return p;
    }
    prev = p;
    p = p->next;
  }

  /* miss */
  return NULL;
}

/* Check to see if a given address can be found in the cache.  This is a *true* "peek"
   function; the replacement state is not touched. */
static struct cache_line_t * cache_peek_llc(
    const struct cache_t * const cp,
    const md_paddr_t addr)
{
  const md_paddr_t block_addr = addr >> cp->addr_shift;
  const int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];

  while(p) /* search all of the ways */
  {
    if(p->valid && (p->tag == block_addr)) /* hit */
    {
      return p;
    }
    p = p->next;
  }

  /* miss */
  return NULL;
}

/* Overwrite the victim line with a new block.  This function only gets called after
   the previous evictee (as determined by the replacement policy) has already been
   removed, which means there is at least one invalid line (which we use for the
   newly inserted line). */
void cache_insert_block_llc(
    struct cache_t * const cp,
    const enum cache_command cmd,
    const md_paddr_t addr,
    core_t * const core)
{
  //////fprintf(stdout,"\n[%lld][Uncore%d]cache_insert_block_llc",sim_cycle,cp->uncore->node_ip);
  /* assumes block not already present */
  const md_paddr_t block_addr = addr >> cp->addr_shift;
  const int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];
  struct cache_line_t *prev = NULL;

  /* there had better be an invalid line now - cache_get_evictee should
     have already returned a line to be invalidated */
  while(p)
  {
    if(!p->valid)
      break;
    prev = p;
    p = p->next;
  }

  cache_assert(p,(void)0);
  p->tag = block_addr;
  p->core = core;
  p->valid = true;
  if(cmd == CACHE_WRITE || cmd == CACHE_WRITEBACK)
    p->dirty = true;
  else
    p->dirty = false;
  if(cmd == CACHE_PREFETCH)
  {
    p->prefetched = true;
    CACHE_STAT(cp->stat.prefetch_insertions++;)
  }

  if(cmd != CACHE_WRITEBACK) /* writebacks don't update replacement state */
  {
    switch(cp->replacement_policy)
    {
      case REPLACE_PLRU: /* tree-based pseudo-LRU */
      {
        int bitmask = cp->blocks[index][0].meta;
        const int way = p->way;
        for(int i=0;i<cp->log2_assoc;i++)
        {
          int pos = (way >> (cp->log2_assoc-i)) + (1<<i);
          
          if((way>>(cp->log2_assoc-i-1)) & 1)
            bitmask |= (1<<pos);
          else
            bitmask &= ~(1<<pos);
        }
        cp->blocks[index][0].meta = bitmask;
      }
      /* same comment about case statement fall-through as in cache_is_hit() */
      case REPLACE_RANDOM:
      case REPLACE_LRU:
      case REPLACE_NMRU:
        if(prev) /* put to front of list */
        {
          prev->next = p->next;
          p->next = cp->blocks[index];
          cp->blocks[index] = p;
        }
        break;
      case REPLACE_MRU:
        if(p->next) /* only move node if not already at end of list */
        {
          /* remove node */
          if(prev)
            prev->next = p->next;
          else
            cp->blocks[index] = p->next;

          /* go to end of list */
          prev = p->next;
          while(prev->next)
            prev = prev->next;
          
          /* stick ourselves there */
          prev->next = p;
          p->next = NULL;
        }
        break;
      case REPLACE_CLOCK:
      {
        /* do not set referenced bit */
        break;
      }
      default:
        fatal("policy not yet implemented");
    }
  }
}

/* caller of get_evictee is responsible for writing back (if needed) and
   invalidating the entry prior to inserting a new block */
struct cache_line_t * cache_get_evictee_llc(
    struct cache_t * const cp,
    const md_paddr_t addr,
    core_t * const core)
{
  int block_addr = addr >> cp->addr_shift;
  int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];

  switch(cp->replacement_policy)
  {
    /* this works for both LRU and MRU, since MRU just sorts its recency list backwards */
    case REPLACE_LRU:
    case REPLACE_MRU:
    {
      while(p)
      {
        if(!p->next || !p->valid) /* take any invalid line, else take the last one (LRU) */
          return p;

        p = p->next;
      }
      break;
    }
    case REPLACE_RANDOM:
    {
      /* use an invalid block if possible */
      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;

        p = p->next;
      }

      if(!p) /* no invalid line, pick at random */
      {
        const int pos = random() % cp->assoc;
        p = cp->blocks[index];

        for(int i=0;i<pos;i++)
          p = p->next;

        return p;
      }

      break;
    }
    case REPLACE_NMRU:
    {
      /* use an invalid block if possible */
      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;
        p = p->next;
      }

      if((cp->assoc > 1) && !p) /* no invalid line, pick at random from non-MRU */
      {
        const int pos = random() % (cp->assoc-1);
        p = cp->blocks[index];
        p = p->next; /* skip MRU */

        for(int i=0;i<pos;i++)
          p = p->next;
      }
      return p;
    }
    case REPLACE_PLRU:
    {
      int bitmask = cp->blocks[index][0].meta;
      int i;
      int node = 1;

      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;
        p = p->next;
      }

      for(i=0;i<cp->log2_assoc;i++)
      {
        const int bit = (bitmask >> node) & 1;

        node = (node<<1) + !bit;
      }

      const int way = node & ~(1<<cp->log2_assoc);

      p = cp->blocks[index];
      for(i=0;i<cp->assoc;i++)
      {
        if(p->way==way)
          break;
        p = p->next;
      }

      return p;
    }
    case REPLACE_CLOCK:
    {
      int just_in_case = 0;

      while(1)
      {
        qword_t way = cp->blocks[index][1].meta;
        struct cache_line_t * p = &cp->blocks[index][way];

        /* increment clock */
        cp->blocks[index][1].meta = modinc(way,cp->assoc); //(way+1) % cp->assoc;

        if(!p->valid) /* take any invalid line */
        {
          cp->blocks[index][0].meta &= ~(1ULL<<p->way); /* make sure referenced bit is clear */
          return p;
        }
        else if(!((cp->blocks[index][0].meta >> p->way) & 1)) /* not referenced */
        {
          return p;
        }
        else
        {
          cp->blocks[index][0].meta &= ~(1ULL<<p->way); /* clear referenced bit */
        }

        just_in_case++;
        if(just_in_case > (2*cp->assoc))
          fatal("Clock-PLRU has gone around twice without finding an evictee for %s",cp->name);
      }
    }
    default:
      fatal("policy not yet implemented");
  }

  fatal("unreachable code");
}

/* input state: assumes that the new node has just been inserted at
   insert_position and all remaining nodes obey the heap property */
static void cache_heap_balance(
    struct cache_action_t * const pipe,
    const int insert_position)
{
  int pos = insert_position;
  //struct cache_action_t tmp;
  while(pos > 1)
  {
    int parent = pos >> 1;
    if(pipe[parent].pipe_exit_time > pipe[pos].pipe_exit_time)
    {
      /*
      tmp = pipe[parent];
      pipe[parent] = pipe[pos];
      pipe[pos] = tmp;
      */
      memswap(&pipe[parent],&pipe[pos],sizeof(*pipe));
      pos = parent;
    }
    else
      return;
  }
}

/* input state: pipe_num is the number of elements in the heap
   prior to removing the root node. */
static void cache_heap_remove(
    struct cache_action_t * const pipe,
    const int pipe_num)
{
  if(pipe_num == 1) /* only one node to remove */
  {
    pipe[1].cb = NULL;
    pipe[1].pipe_exit_time = TICK_T_MAX;
    return;
  }

  pipe[1] = pipe[pipe_num]; /* move last node to root */
  //struct cache_action_t tmp;

  /* delete previous last node */
  pipe[pipe_num].cb = NULL;
  pipe[pipe_num].pipe_exit_time = TICK_T_MAX;

  /* push node down until heap property re-established */
  int pos = 1;
  while(1)
  {
    int Lindex = pos<<1;     // index of left child
    int Rindex = (pos<<1)+1; // index of right child
    tick_t myValue = pipe[pos].pipe_exit_time;

    tick_t Lvalue = TICK_T_MAX;
    tick_t Rvalue = TICK_T_MAX;
    if(Lindex < pipe_num) /* valid index */
      Lvalue = pipe[Lindex].pipe_exit_time;
    if(Rindex < pipe_num) /* valid index */
      Rvalue = pipe[Rindex].pipe_exit_time;

    if(((myValue > Lvalue) && (Lvalue != TICK_T_MAX)) ||
       ((myValue > Rvalue) && (Rvalue != TICK_T_MAX)))
    {
      if(Rvalue == TICK_T_MAX) /* swap pos with L */
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        return;
      }
      else if((myValue > Lvalue) && (Lvalue < Rvalue))
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        pos = Lindex;
      }
      else /* swap pos with R */
      {
        /* tmp = pipe[Rindex];
        pipe[Rindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Rindex],&pipe[pos],sizeof(*pipe));
        pos = Rindex;
      }
    }
    else
      return;
  }
}


/* Can a cache access get enqueued into the cache?  Returns true if it
   can.  Cases when it can't include, for example, more than one request
   per cycle goes to the same bank, or if a bank has been locked up. */
int cache_enqueuable_llc(
    const struct cache_t * const cp,
    const int thread_id,
    const md_paddr_t addr)
{
  md_paddr_t paddr;
  if(thread_id == DO_NOT_TRANSLATE)
    paddr = addr;
  else
    paddr = addr;	//v2p_translate(thread_id,addr);
  const int bank = GET_BANK(paddr);
  if(cp->pipe_num[bank] < cp->latency)
    return true;
  else
    return false;
}

/* Enqueue a cache access request to the cache.  Assumes you already
   called cache_enqueuable. */
void cache_enqueue_llc(
    struct core_t * const core,
    struct cache_t * const cp,
    struct cache_t * const prev_cp,
    const enum cache_command cmd,
    const int thread_id,
    const md_addr_t PC,
    const md_paddr_t addr,
    const seq_t action_id,
    const int MSHR_bank,
    const int MSHR_index,
    void * const op,
    void (*const cb)(void *),
    void (*const miss_cb)(void *, int),
    bool (*const translated_cb)(void *,seq_t),
    seq_t (*const get_action_id)(void *) )
{
  md_paddr_t paddr;

  //if(strcmp(cp->name,"IL1")==0)
	//fprintf(stdout,"\n[%lld]IL1 enqueue_llc %llx cmd %d ",sim_cycle, addr, cmd );

  //if(strcmp(cp->name,"DL1")==0)
	//fprintf(stdout,"\n[%lld]DL1 enqueue_llc %llx cmd %d ",sim_cycle, addr, cmd );

  //if(strcmp(cp->name,"LLC")==0)
	//fprintf(stdout,"\n[%lld][UnCore%d]LLC enqueue_llc %llx cmd %d ",sim_cycle,cp->uncore->node_ip, addr, cmd );



  if(thread_id == DO_NOT_TRANSLATE)
    paddr = addr;
  else
    paddr = addr;	//v2p_translate(thread_id,addr); /* for the IL1/DL1, we need a virtual-to-physical translation */
  const int bank = GET_BANK(paddr);

  /* heap initial insertion position */
  const int insert_position = cp->pipe_num[bank]+1;
  cache_assert(cp->pipe[bank][insert_position].cb == NULL,(void)0);
  cache_assert(cb != NULL,(void)0);

  cp->pipe[bank][insert_position].core = core;
  cp->pipe[bank][insert_position].prev_cp = prev_cp;
  cp->pipe[bank][insert_position].cmd = cmd;
  cp->pipe[bank][insert_position].PC = PC;
  cp->pipe[bank][insert_position].paddr = paddr;
  cp->pipe[bank][insert_position].op = op;
  cp->pipe[bank][insert_position].action_id = action_id;
  cp->pipe[bank][insert_position].MSHR_bank = MSHR_bank;
  cp->pipe[bank][insert_position].MSHR_index = MSHR_index;
  cp->pipe[bank][insert_position].cb = cb;
  cp->pipe[bank][insert_position].miss_cb = miss_cb;
  cp->pipe[bank][insert_position].translated_cb = translated_cb;
  cp->pipe[bank][insert_position].get_action_id = get_action_id;
  cp->pipe[bank][insert_position].when_started = sim_cycle;
  cp->pipe[bank][insert_position].when_returned = TICK_T_MAX;


  cp->pipe[bank][insert_position].pipe_exit_time = sim_cycle+cp->latency;

  cp->pipe[bank][insert_position].miss_cb_invoked = false;

  assert(insert_position < cp->heap_size);
  cache_heap_balance(cp->pipe[bank],insert_position);

  cp->pipe_num[bank]++;
  //cache_assert(cp->pipe_num[bank] <= cp->latency,(void)0);

  cp->check_for_work = true;
  cp->check_for_pipe_work = true;

}

/* Returns true if there's at least one free entry in the write-back buffer */
static inline int WBB_available(
    const struct cache_t * const cp,
    const md_paddr_t paddr)
{
  const int bank = GET_MSHR_BANK(paddr);
  return cp->WBB_num[bank] < cp->WBB_size;
}

/* Insert a writeback request into the WBB; assumes you already called WBB_available
   to make sure there's room. */
static void WBB_insert(
    struct cache_t * const cp,
    struct cache_line_t * const cache_block)
{
  /* don't need to cache invalid blocks */
  if(!cache_block->valid)
    return;

  CACHE_STAT(cp->stat.WBB_insertions++;)
  const int bank = GET_MSHR_BANK(cache_block->tag << cp->addr_shift);

  if(cp->write_combining)
  {
    int i;
    for(i=0;i<cp->WBB_num[bank];i++)
    {
      int index = mod2m(cp->WBB_head[bank]+i,cp->WBB_size); //(cp->WBB_head[bank] + i) % cp->WBB_size;
      if(cp->WBB[bank][index].valid && (cp->WBB[bank][index].tag == cache_block->tag))
      {
        /* if can combine with another write that's already
           waiting in the WBB, then don't need to add new request */
        CACHE_STAT(cp->stat.WBB_combines++;)
        return;
      }
    }
  }

  struct cache_line_t * p = &cp->WBB[bank][cp->WBB_tail[bank]];

  p->core = cache_block->core;
  p->tag = cache_block->tag;
  p->valid = cache_block->valid;
  p->dirty = cache_block->dirty;
  p->victim = false;

  cp->WBB_num[bank]++;
  cache_assert(cp->WBB_num[bank] <= cp->WBB_size,(void)0);
  cp->WBB_tail[bank] = modinc(cp->WBB_tail[bank],cp->WBB_size); //(cp->WBB_tail[bank] + 1) % cp->WBB_size;
  cp->check_for_work = true;
  cp->check_for_WBB_work = true;
  cp->counters.WBB.write++;
}

/* insert if room, but this line doesn't need to be written back to next level */
static void WBB_victim_insert(
    struct cache_t * const cp,
    struct cache_line_t * const cache_block)
{
  /* don't need to cache invalid blocks */
  if(!cache_block->valid)
    return;

  const int bank = GET_MSHR_BANK(cache_block->tag << cp->addr_shift);

  if(cp->WBB_num[bank] >= cp->WBB_size) /* can't do anything if full */
    return;

  const int num_available = cp->WBB_size - cp->WBB_num[bank];

  /* random insertion among non-dirty lines */
  int index = mod2m(cp->WBB_head[bank]+cp->WBB_num[bank],cp->WBB_size); // tail index
  index += random() % num_available;
  index = mod2m(index,cp->WBB_size);

  struct cache_line_t * p = &cp->WBB[bank][index];

  /* Victim lines written to can end up out-of-order with respect to the
     regular circular queue order.  In this rare case, we just skip the
     victim insertion because we cannot overwrite the dirty line.  One could
     try to re-search for a non-dirty line, but that gets ugly. */
  if(p->dirty)
    return;

  CACHE_STAT(cp->stat.WBB_victim_insertions++;)
  p->core = cache_block->core;
  p->tag = cache_block->tag;
  p->valid = cache_block->valid;
  cache_assert(!cache_block->dirty,(void)0);
  p->dirty = false;
  p->victim = true;
  cp->counters.WBB.write++;
}

/* Returns true if at least one MSHR entry is free/available. */
static inline int MSHR_available(
    const struct cache_t * const cp,
    const md_paddr_t paddr)
{
  const int bank = GET_MSHR_BANK(paddr);
  return cp->MSHR_num[bank] < cp->MSHR_size;
}

/* Returns a pointer to a free MSHR entry; assumes you already called
   MSHR_available to make sure there's room */
static struct cache_action_t * MSHR_allocate(
    struct cache_t * const cp,
    const md_paddr_t paddr,
    const enum cache_command cmd)
{
  const int bank = GET_MSHR_BANK(paddr);

  struct cache_action_t * prev = NULL;

  /* Check to see if there are any other requests to the same
     cache line; if so, coalesce the requests.  Note, this
     implementation of request-combining still uses one MSHR
     entry per request, but only one is sent to the next level
     of the cache hierarchy. */
  for(int i=0;i<cp->MSHR_size;i++)
  {
    if((cp->MSHR[bank][i].cb != NULL) && 
        (cp->MSHR[bank][i].paddr >> cp->addr_shift) == (paddr >> cp->addr_shift) &&
        (cp->MSHR[bank][i].MSHR_link == NULL) &&
        (cp->MSHR[bank][i].when_returned == TICK_T_MAX))
    {
      cp->stat.MSHR_combos++;
      prev = &cp->MSHR[bank][i];
      break;
    }
  }

  for(int i=0;i<cp->MSHR_size;i++)
    if(cp->MSHR[bank][i].cb == NULL)
    {
      cp->MSHR_num[bank]++;
      cache_assert(cp->MSHR_num[bank] <= cp->MSHR_size,NULL);
      cp->check_for_work = true;
      cp->check_for_MSHR_work = true;
      cache_assert(cp->MSHR[bank][i].MSHR_link == NULL,NULL);
      cp->MSHR[bank][i].MSHR_link = NULL;
      cp->counters.MSHR.write++;
      if(prev)
      {
        /* if this request can be combined with others,
           then link it onto the end of the chain */
        prev->MSHR_link = &cp->MSHR[bank][i];
        cp->MSHR[bank][i].when_returned = prev->when_returned;
        cp->MSHR[bank][i].MSHR_linked = true;
      }
      else
      {
        cp->MSHR[bank][i].MSHR_linked = false;
      }
      return &cp->MSHR[bank][i];
    }

  fatal("request for MSHR failed");
}

static void dummy_callback(void * p)
{
  /* this is just a place holder for cast-out writebacks
     in a write-back cache */
}

/* Called when a LLC cache miss gets serviced. */
void fill_arrived_llc(
    struct cache_t * const cp,
    unsigned long long int address)
{
  bool flag=false;
  for(int b=0;b<cp->MSHR_banks;b++)
  {
      int bank = (b) & cp->MSHR_mask;
      if(cp->MSHR_fill_num[bank])
      {
        /* find oldest returned request to process */
        for(int i=0;i<cp->MSHR_size;i++)
	{
		struct cache_action_t * MSHR = &cp->MSHR[bank][i];
		if( (MSHR->paddr >> cp->addr_shift) == (address >> cp->addr_shift) )
		{
			flag=true;
			if(MSHR->cb != NULL) /* original request was squashed */
				MSHR->when_returned = sim_cycle;
			MSHR->MSHR_linked=false;
			MSHR->MSHR_link=NULL;
		}
        }
      }
  }
  if(!flag)
  {
	fprintf(stdout,"\nunknown %llx",address);
  }

  cp->check_for_work = true;
  cp->check_for_MSHR_fill_work = true;

 
  /* NOTE: recursive calls to fill_arrived will be called
     by the MSHR_fill handling code in cache_process */
}


/* input state: assumes that the new node has just been inserted at
   insert_position and all remaining nodes obey the heap property */
static void fill_heap_balance(
    struct cache_fill_t * const pipe,
    const int insert_position)
{
  int pos = insert_position;
  //struct cache_fill_t tmp;
  while(pos > 1)
  {
    int parent = pos >> 1;
    if(pipe[parent].pipe_exit_time > pipe[pos].pipe_exit_time)
    {
      /* tmp = pipe[parent];
      pipe[parent] = pipe[pos];
      pipe[pos] = tmp; */
      memswap(&pipe[parent],&pipe[pos],sizeof(*pipe));
      pos = parent;
    }
    else
      return;
  }
}

/* input state: pipe_num is the number of elements in the heap
   prior to removing the root node. */
static void fill_heap_remove(
    struct cache_fill_t * const pipe,
    const int pipe_num)
{
  if(pipe_num == 1) /* only one node to remove */
  {
    pipe[1].valid = false;
    pipe[1].pipe_exit_time = TICK_T_MAX;
    return;
  }

  pipe[1] = pipe[pipe_num]; /* move last node to root */
  //struct cache_fill_t tmp;

  /* delete previous last node */
  pipe[pipe_num].valid = false;
  pipe[pipe_num].pipe_exit_time = TICK_T_MAX;

  /* push node down until heap property re-established */
  int pos = 1;
  while(1)
  {
    int Lindex = pos<<1;     // index of left child
    int Rindex = (pos<<1)+1; // index of right child
    tick_t myValue = pipe[pos].pipe_exit_time;

    tick_t Lvalue = TICK_T_MAX;
    tick_t Rvalue = TICK_T_MAX;
    if(Lindex < pipe_num) /* valid index */
      Lvalue = pipe[Lindex].pipe_exit_time;
    if(Rindex < pipe_num) /* valid index */
      Rvalue = pipe[Rindex].pipe_exit_time;

    if(((myValue > Lvalue) && (Lvalue != TICK_T_MAX)) ||
       ((myValue > Rvalue) && (Rvalue != TICK_T_MAX)))
    {
      if(Rvalue == TICK_T_MAX) /* swap pos with L */
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        return;
      }
      else if((myValue > Lvalue) && (Lvalue < Rvalue))
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        pos = Lindex;
      }
      else /* swap pos with R */
      {
        /* tmp = pipe[Rindex];
        pipe[Rindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Rindex],&pipe[pos],sizeof(*pipe));
        pos = Rindex;
      }
    }
    else
      return;
  }
}
/* We are assuming separate ports for cache fills; returns true if the fill pipeline
   can accept a fill request. */
static inline bool cache_fillable_llc(
    const struct cache_t * const cp,
    const md_paddr_t paddr)
{
  const int bank = GET_BANK(paddr);
  if(cp->fill_num[bank] < cp->latency)
    return true;
  else
    return false;
}

/* Do the actual fill; assume you already called cache_fillable */
static inline void cache_fill_llc(
    struct cache_t * const cp,
    const enum cache_command cmd,
    const md_paddr_t paddr,
    struct core_t * core)
{
  const int bank = GET_BANK(paddr);
  const int insert_position = cp->fill_num[bank]+1;
  cache_assert(!cp->fill_pipe[bank][insert_position].valid,(void)0);


  //if(strcmp(cp->name,"LLC")==0)
	//fprintf(stdout,"\n[%lld]LLC fill_llc %llx cmd %d ",sim_cycle, paddr, cmd );

  cp->fill_pipe[bank][insert_position].valid = true;
  cp->fill_pipe[bank][insert_position].cmd = cmd;
  cp->fill_pipe[bank][insert_position].paddr = paddr;
  cp->fill_pipe[bank][insert_position].core = core;

  cp->fill_pipe[bank][insert_position].pipe_exit_time = sim_cycle+cp->latency;

  assert(insert_position < cp->heap_size);
  fill_heap_balance(cp->fill_pipe[bank],insert_position);

  cp->fill_num[bank]++;
  cp->counters.linefill.write++;
  cache_assert(cp->fill_num[bank] <= cp->latency,(void)0);
  cp->check_for_work = true;
  cp->check_for_fill_work = true;
}

/* simulate one cycle of the cache */
void cache_process_llc(struct cache_t * const cp)
{
  int b;

  bool work_found = false;
  bool MSHR_fill_work_found = false;
  bool fill_work_found = false;
  bool pipe_work_found = false;
  bool MSHR_work_found = false;
  bool WBB_work_found = false;

//if(cp->uncore->id==2)
/*if(sim_cycle>2295090)
{
  fprintf(stdout,"\n[%lld][Uncore%d] MSHR entries :",sim_cycle,cp->uncore->node_ip);
  for(int b=0;b<cp->MSHR_banks;b++)
  {
	fprintf(stdout,"%d ", cp->MSHR_fill_num[b]);
		for(int i=0;i<cp->MSHR_size;i++)
			if(cp->MSHR[b][i].when_returned==TICK_T_MAX)
				fprintf(stdout,"\n 0x%llx %lld ",cp->MSHR[b][i].paddr, cp->MSHR[b][i].when_started);
		//exit(0);
  }
}*/


  //int start_point = random() & cp->bank_mask; /* randomized arbitration */
  int start_point = cp->start_point; /* round-robin arbitration */
  cp->start_point = modinc(cp->start_point,cp->banks);

  /* process write-backs */
  if(cp->check_for_WBB_work)
    for(b=0;b<cp->MSHR_banks;b++)
    {
      int bank = (start_point+b) & cp->MSHR_mask;
      if(cp->WBB_num[bank]) /* there are cast-outs pending to be written back */
      {
        struct cache_line_t * WBB = &cp->WBB[bank][cp->WBB_head[bank]];

        work_found = true;
        WBB_work_found = true;
        
        
        
      }
    }
  cp->check_for_WBB_work = WBB_work_found;

  /*Check if any spills have arrived from memory*/
  if(cp->check_for_MSHR_fill_work)
    for(int b=0;b<cp->MSHR_banks;b++)
    {
      int bank = (start_point + b) & cp->MSHR_mask;
      if(cp->MSHR_fill_num[bank])
      {
        work_found = true;
        MSHR_fill_work_found = true;
        tick_t oldest = TICK_T_MAX;
        int old_index = -1;

        /* find oldest returned request to process */
        for(int i=0;i<cp->MSHR_size;i++)
        {
          struct cache_action_t * MSHR = &cp->MSHR[bank][i];
          if(MSHR->cb && (MSHR->when_returned <= sim_cycle) &&
                  cache_fillable_llc(cp,MSHR->paddr))
          {
            if(MSHR->when_enqueued < oldest)
            {
              oldest = MSHR->when_enqueued;
              old_index = i;
            }
          }
        }

        if(old_index < 0)
          continue;

        /* process returned MSHR requests that now need to fill the cache */
        struct cache_action_t * MSHR = &cp->MSHR[bank][old_index];
        bool bus_available = !MSHR->prev_cp || bus_free(MSHR->prev_cp->next_bus);

        if(MSHR->cb && (MSHR->when_returned <= sim_cycle) && bus_available)
        {
          if(!MSHR->MSHR_linked)
          {
            int insert = true;
            if(cp->PF_filter && (MSHR->cmd == CACHE_PREFETCH))
            {
              insert = prefetch_filter_lookup(cp->PF_filter,(MSHR->paddr>>cp->addr_shift));
              cp->counters.prefetch.filter.read++;
            }

            /* if using prefetch buffer, insert there instead */
            if((MSHR->cmd == CACHE_PREFETCH) && (cp->PF_buffer) && insert)
            {
              struct prefetch_buffer_t *p, *evictee = NULL, *prev = NULL;
              p = cp->PF_buffer;
              while(p)
              {
                if((MSHR->paddr>>cp->addr_shift) == (p->addr>>cp->addr_shift)) /* already been prefetched */
                  break;
                if(!p->next || (p->addr == (md_paddr_t)-1))
                {
                  evictee = p;
                  break;
                }
                prev = p;
                p = p->next;
              }

              if(evictee) /* not found, overwrite entry */
              {
                p->addr = MSHR->paddr;
                cp->counters.prefetch.buffer.write++;
              }

              /* else someone else already prefetched this addr */
              /* move to MRU position if not already there */
              if(prev)
              {
                prev->next = p->next;
                p->next = cp->PF_buffer;
                cp->PF_buffer = p;
              }
            }
            else if(  (MSHR->cmd == CACHE_READ) || (MSHR->cmd == CACHE_PREFETCH) ||
                     ((MSHR->cmd == CACHE_WRITE || MSHR->cmd == CACHE_WRITEBACK) && (cp->allocate_policy == WRITE_ALLOC)))
            {
              if(insert)
                cache_fill_llc(cp,MSHR->cmd,MSHR->paddr,MSHR->core);
            }
          }

          if(MSHR->prev_cp)
          {
            /* everyone but the response from a writeback transfers a full cache line */
            bus_use(MSHR->prev_cp->next_bus,(MSHR->cmd==CACHE_WRITEBACK)?1:MSHR->prev_cp->linesize,MSHR->cmd==CACHE_PREFETCH);
            fill_arrived(MSHR->prev_cp,MSHR->MSHR_bank,MSHR->MSHR_index);
          }

          /* deallocate the MSHR */
          if(MSHR->cmd == CACHE_PREFETCH)
          {
            cp->MSHR_num_pf[bank]--;
            cache_assert(cp->MSHR_num_pf[bank] >= 0,(void)0);
          }
          MSHR->cb = NULL;
          cp->MSHR_num[bank]--;
          cp->MSHR_fill_num[bank]--;
          cache_assert(cp->MSHR_num[bank] >= 0,(void)0);
          cache_assert(cp->MSHR_fill_num[bank] >= 0,(void)0);
          cp->counters.MSHR.read++;
        }
      }
    }
  cp->check_for_MSHR_fill_work = MSHR_fill_work_found;

  /* process cache fills: */
  if(cp->check_for_fill_work)
    for(b=0;b<cp->banks;b++)
    {
      int bank = (start_point + b) & cp->bank_mask;

      struct cache_fill_t * cf = &cp->fill_pipe[bank][1];
      if(cp->fill_num[bank])
      {
        work_found = true;
        fill_work_found = true;
        if(cf->valid && cf->pipe_exit_time <= sim_cycle)
        {
          if(!cache_peek_llc(cp,cf->paddr)) /* make sure hasn't been filled in the meantime */
          {
            struct cache_line_t * p = cache_get_evictee_llc(cp,cf->paddr,cf->core);
            if(p->valid)
            {
              if(p->dirty)
              {
                if(cp->write_policy == WRITE_BACK)
                {
                  if(!WBB_available(cp,cf->paddr))
                    goto no_WBB_available; /* can't go until WBB is available */
                  WBB_insert(cp,p);
                }
              }//TODO: invalidate the higher level of caches
	      cache_invalidate(cp->prev_level,p->tag << cp->addr_shift ,NULL);
	      if(cache_peek_llc(cp->prev_level,p->tag << cp->addr_shift))
		fatal("line still present in upper level");
            }

            if(p->prefetched && cp->PF_filter)
            {
              prefetch_filter_update(cp->PF_filter,p->tag,p->prefetch_used);
              cp->counters.prefetch.filter.write++;
            }
            p->valid = p->dirty = false; /* this removes the copy in the cache since the WBB has it now */

            cache_insert_block_llc(cp,cf->cmd,cf->paddr,cf->core);
          }
          fill_heap_remove(cp->fill_pipe[bank],cp->fill_num[bank]);
          cp->fill_num[bank]--;
          cp->counters.linefill.read++;
          cache_assert(cp->fill_num[bank] >= 0,(void)0);
        }
      }
    no_WBB_available:
      ;
    }
  cp->check_for_fill_work = fill_work_found;

  /* check last stage of cache pipes, process accesses */
  if(cp->check_for_pipe_work)
    for(b=0;b<cp->banks;b++)
    {
      int bank = (start_point + b) & cp->bank_mask;

      struct cache_action_t * ca = &cp->pipe[bank][1]; // heap root
      if(cp->pipe_num[bank])
      {
        /* These two determine whether to check the prefetcher(s) for
           new address(es) to prefetch.  We only do so when the request
           leaves the cache-lookup pipeline (request dequeued) to prevent
           double-prefetching */
        bool do_prefetch = !cp->prefetch_on_miss;
        bool request_dequeued = false;

        work_found = true;
        pipe_work_found = true;
        if(ca->cb)
        {
          if(ca->pipe_exit_time <= sim_cycle)
          {
            struct cache_line_t * line = cache_is_hit_llc(cp,ca->cmd,ca->paddr,ca->core);
            if(line != NULL) /* if cache hit */
            {
              if((ca->cmd == CACHE_WRITE || ca->cmd == CACHE_WRITEBACK) && (cp->write_policy == WRITE_THROUGH))
              {
                if(!WBB_available(cp,ca->paddr))
                  continue; /* can't go until WBB is available */

                struct cache_line_t tmp_line;
                tmp_line.core = ca->core;
                tmp_line.tag = ca->paddr >> cp->addr_shift;
                tmp_line.valid = true;
                tmp_line.dirty = false;

                /* on a write-through cache, use a WBB entry to send write to the next level */
                WBB_insert(cp,&tmp_line);
              }

              /* For L1 caches only: (L2/LLC handled by MSHR processing later)
                 invoke the call-back function; the action-id check makes sure the request is
                 still valid (e.g., a request is invalid if the original uop that initiated
                 the request had been flushed due to a branch misprediction). */
              if(!ca->prev_cp && ca->op && (ca->action_id == ca->get_action_id(ca->op)))
              {
                ca->cb(ca->op);
              }

         

              if(ca->cmd == CACHE_PREFETCH)
              {
                CACHE_STAT(cp->stat.prefetch_lookups++;)
                cp->counters.cache.read++;
              }
              else
              {
                if(ca->cmd == CACHE_READ)
                {
                  CACHE_STAT(cp->stat.load_lookups++;)
                  cp->counters.cache.read++;
                }
                else if(ca->cmd == CACHE_WRITE)
                {
                  CACHE_STAT(cp->stat.store_lookups++;)
                  cp->counters.cache.write++;
                }
                else if(ca->cmd == CACHE_WRITEBACK)
                {
                  CACHE_STAT(cp->stat.writeback_lookups++;)
                  cp->counters.cache.write++;
                }
                if(line->prefetched && !line->prefetch_used)
                {
                  line->prefetch_used = true;
                  CACHE_STAT(cp->stat.prefetch_useful_insertions++;)
                }
              }

              /* fill previous level as appropriate */
              if(ca->prev_cp)
                fill_arrived(ca->prev_cp,ca->MSHR_bank,ca->MSHR_index);
            }
            else /* miss in main data array */
            {
              int last_chance_hit = false;

              if((ca->cmd == CACHE_READ) ||
                 (ca->cmd == CACHE_PREFETCH) ||
                 ((ca->cmd == CACHE_WRITE || ca->cmd == CACHE_WRITEBACK) && cp->write_combining))
              {
                /* last chance: check in WBBs - this implements something like a victim cache */
                int j;

                int this_bank = GET_MSHR_BANK(ca->paddr);
                int index = cp->WBB_head[this_bank];
                for(j=0;j<cp->WBB_size;j++)
                {
                  if(cp->WBB[this_bank][index].tag == (ca->paddr >> cp->addr_shift))
                  {
                    /* we're only write-combining with other existing writes;
                       i.e., no combining with non-dirty victim entries.
                       TODO: fix this by changing the WBB processing loop to
                       not assume a circular queue organization but to just
                       manually rescan the queue each time.
                     */
                    if(ca->cmd == CACHE_WRITE || ca->cmd == CACHE_WRITEBACK)
                    {
                      if(cp->WBB[this_bank][index].dirty)
                      {
                        CACHE_STAT(cp->stat.WBB_combines++;)
                        last_chance_hit = true;
                        break;
                      }
                    }
                    else
                    {
                      CACHE_STAT(cp->stat.WBB_hits++;)
                      CACHE_STAT(if(!cp->WBB[this_bank][index].victim) cp->stat.WBB_victim_hits++;)
                      last_chance_hit = true;
                      break;
                    }
                    cp->counters.WBB.read++;
                  }
                  index = modinc(index,cp->WBB_size);
                }

                if(cp->PF_buffer && !last_chance_hit)
                {
                  struct prefetch_buffer_t * p = cp->PF_buffer, * prev = NULL;
                  while(p)
                  {
                    if((p->addr>>cp->addr_shift) == (ca->paddr>>cp->addr_shift))
                    {
                      last_chance_hit = true;
                      break;
                    }
                    prev = p;
                    p = p->next;
                  }

                  if(p) /* hit in prefetch buffer */
                  {
                    last_chance_hit = true;
                    cp->counters.prefetch.buffer.read++;
                    /* insert block into cache */
                    struct cache_line_t * evictee = cache_get_evictee_llc(cp,ca->paddr,ca->core);
                    int ok_to_insert = true;
                    if((cp->write_policy == WRITE_BACK) && evictee->valid && evictee->dirty)
                    {
                      if(WBB_available(cp,ca->paddr))
                      {
                        WBB_insert(cp,evictee);
                      }
                      else /* we'd like to insert the prefetched line into the cache, but
                              in order to do so, we need to writeback the evictee but
                              we can't get a WBB.  */
                      {
                        ok_to_insert = false;
                      }
                    }

                    if(ok_to_insert && cache_fillable_llc(cp,ca->paddr))
                    {
		      cache_invalidate(cp->prev_level,evictee->tag << cp->addr_shift,NULL);
		      if(cache_peek_llc(cp->prev_level,evictee->tag << cp->addr_shift))
			fatal("line still present in upper level");
                      evictee->valid = evictee->dirty = false;
                      cache_fill_llc(cp,CACHE_PREFETCH,p->addr,ca->core);
                    }
                    else /* if we couldn't do the insertion for some reason (e.g., no WBB
                            entry for dirty evictee or now fill bandwidth, move the
                            block back to the MRU position in the PF-buffer and hope
                            for better luck next time. */
                    {
                      if(prev)
                      {
                        prev->next = p->next;
                        p->next = cp->PF_buffer;
                        cp->PF_buffer = p;
                      }
                    }
                  }
                }
              }

              if(last_chance_hit)
              {
                /* only invoke callback if this is an L1 cache (see earlier comment
                   for a regular cache hit. */
                if(!ca->prev_cp && ca->op && (ca->action_id == ca->get_action_id(ca->op)))
                  ca->cb(ca->op);

                if(ca->cmd == CACHE_READ)
                  CACHE_STAT(cp->stat.load_lookups++;)
                else if(ca->cmd == CACHE_PREFETCH)
                  CACHE_STAT(cp->stat.prefetch_lookups++;)
                else if(ca->cmd == CACHE_WRITE)
                  CACHE_STAT(cp->stat.store_lookups++;)
                else if(ca->cmd == CACHE_WRITEBACK)
                  CACHE_STAT(cp->stat.writeback_lookups++;)

                /* fill previous level as appropriate */
                if(ca->prev_cp)
                  fill_arrived(ca->prev_cp,ca->MSHR_bank,ca->MSHR_index);
              }
              else /* ok, we really missed */
              {
                cp->counters.prefetch.buffer.read_tag++;
                cp->counters.WBB.read_tag++;
                /* place in MSHR */
                do_prefetch = true;
		///////fprintf(stdout,"\n[%lld]%s Missed in data Array %llx %d ",sim_cycle, cp->name, ca->paddr,ca->cmd);

                if(MSHR_available(cp,ca->paddr))
                {
		  if(ca->cmd!=CACHE_WRITEBACK)
		  {
		  	////////fprintf(stdout,"\n[%lld]%s Allocating MSHR",sim_cycle, cp->name);
                  	struct cache_action_t * MSHR = MSHR_allocate(cp,ca->paddr,ca->cmd);
                  	bool MSHR_linked = MSHR->MSHR_linked;
                  	*MSHR = *ca;
                  	MSHR->when_enqueued = sim_cycle;
                  	MSHR->when_started = TICK_T_MAX;
                  	MSHR->when_returned = TICK_T_MAX;
                  	MSHR->core = ca->core;
                  	MSHR->MSHR_linked = MSHR_linked;
                  	int this_bank = GET_MSHR_BANK(ca->paddr);
                  	if(MSHR_linked)
                  	  MSHR->when_started = sim_cycle;
                  	else
                  	{
                  	  cp->MSHR_unprocessed_num[this_bank]++;
                  	  cache_assert(cp->MSHR_unprocessed_num[this_bank] <= cp->MSHR_size,(void)0);
                  	}
                  	cp->MSHR_fill_num[this_bank]++;
                  	cache_assert(cp->MSHR_fill_num[this_bank] <= cp->MSHR_size,(void)0);
	
                  	/* update miss stats when requests *leaves* the cache pipeline to avoid double-counting */
                  	if(ca->core)
                  	{
                  	  CACHE_STAT(cp->stat.core_lookups[ca->core->current_thread->id]++;)
                  	  CACHE_STAT(cp->stat.core_misses[ca->core->current_thread->id]++;)
                      cp->counters.cache.read_tag++;
                  	}
	
                  	if(ca->cmd == CACHE_READ)
                  	{
                  	  CACHE_STAT(cp->stat.load_lookups++;)
                  	  CACHE_STAT(cp->stat.load_misses++;)
                      cp->counters.cache.read_tag++;
                  	}
                  	else if(ca->cmd == CACHE_PREFETCH)
                  	{
                  	  CACHE_STAT(cp->stat.prefetch_lookups++;)
                  	  CACHE_STAT(cp->stat.prefetch_misses++;)
                  	  cp->MSHR_num_pf[MSHR->MSHR_bank]++;
                  	  cache_assert(cp->MSHR_num_pf[MSHR->MSHR_bank] <= cp->MSHR_size,(void)0);
                      cp->counters.cache.read_tag++;
                  	}
                  	else if(ca->cmd == CACHE_WRITE)
                  	{
                  	  CACHE_STAT(cp->stat.store_lookups++;)
                  	  CACHE_STAT(cp->stat.store_misses++;)
                      cp->counters.cache.read_tag++;
                  	}
                  	else if(ca->cmd == CACHE_WRITEBACK)
                  	{
                  	  CACHE_STAT(cp->stat.writeback_lookups++;)
                  	  CACHE_STAT(cp->stat.writeback_misses++;)
                      cp->counters.cache.read_tag++;
                  	}
		  }
                }
                else
                  continue; /* this circumvents the "ca->cb = NULL" at the end of the loop, thereby leaving the ca in the pipe */
              }
            }

            cache_heap_remove(cp->pipe[bank],cp->pipe_num[bank]);
            cp->pipe_num[bank]--;
            cache_assert(cp->pipe_num[bank] >= 0,(void)0);
            request_dequeued = true;

            if(do_prefetch && request_dequeued)
              if(ca->PC && (ca->cmd == CACHE_READ))
              {
                int ii;
                for(ii=0;ii<cp->num_prefetchers;ii++)
                {
                  md_paddr_t pf_addr = cp->prefetcher[ii]->lookup(ca->PC,ca->paddr);

                  if(pf_addr & ~(PAGE_SIZE-1)) /* don't prefetch from zeroth page */
                  {
                    int j;

                    /* search PFF to see if pf_addr already requested */
                    int already_requested = false;
                    int index = cp->PFF_head;;
                    for(j=0;j<cp->PFF_num;j++)
                    {
                      if(cp->PFF[index].addr == pf_addr)
                      {
                        already_requested = true;
                        break;
                      }
                      index = modinc(index,cp->PFF_size);
                    }
                    if(already_requested)
                      continue; /* for(i=0;...) */

                    /* if FIFO full, overwrite oldest */
                    if(cp->PFF_num == cp->PFF_size)
                    {
                      cp->PFF_head = modinc(cp->PFF_head,cp->PFF_size); //(cp->PFF_head + 1) % cp->PFF_size;
                      cp->PFF_num--;
                      cp->counters.prefetch.FIFO.read++;
                      cache_assert(cp->PFF_num >= 0,(void)0);
                    }

                    cp->PFF[cp->PFF_tail].PC = ca->PC;
                    cp->PFF[cp->PFF_tail].core = ca->core;
                    cp->PFF[cp->PFF_tail].addr = pf_addr;
                    cp->PFF_tail = modinc(cp->PFF_tail,cp->PFF_size); //(cp->PFF_tail + 1) % cp->PFF_size;
                    cp->PFF_num++;
                    cp->counters.prefetch.FIFO.write++;
                    cache_assert(cp->PFF_num <= cp->PFF_size,(void)0);
                  }
                }
              }
          }
        }
        else /* !ca->cb */
        {
          cache_heap_remove(cp->pipe[bank],cp->pipe_num[bank]);
          cp->pipe_num[bank]--;
          cache_assert(cp->pipe_num[bank] >= 0,(void)0);
        }
      }
    }
  cp->check_for_pipe_work = pipe_work_found;


/*Check MSHR for requests waiting to be sent to lower levels of memory*/
  if(cp->check_for_MSHR_work)
  {
	//if(strcmp(cp->name,"LLC")==0)
	  //    fprintf(stdout,"\n[%lld]%s Bus to Next level available", sim_cycle, cp->name);
        /* Prioritized handling of MSHR requests by type (read, write, prefetch, writeback) */
        int sent_something = false;
        int c;

        for(c=0;c<4 && !sent_something;c++)
        {
          enum cache_command cmd = cp->MSHR_cmd_order[c];

          for(b=0;b<cp->MSHR_banks;b++)
          {
            int bank = (start_point+b) & cp->MSHR_mask;
            if(cp->MSHR_num[bank])
            {
              work_found = true;
              MSHR_work_found = true;
              /* find oldest not-processed entry */
              if((!cp->next_bus || bus_free(cp->next_bus)) && (cp->MSHR_unprocessed_num[bank] > 0))
              {
                tick_t oldest_age = TICK_T_MAX;
                int index = -1;

                for(int i=0;i<cp->MSHR_size;i++)
                {
                  struct cache_action_t * MSHR = &cp->MSHR[bank][i];
                  if((MSHR->cmd == cmd) && MSHR->cb && (MSHR->when_started == TICK_T_MAX)) 
                  {
                    if(MSHR->when_enqueued < oldest_age)
                    {
                      oldest_age = MSHR->when_enqueued;
                      index = i;
                    }
                  }
                }

		/*Try to send the oldest one first*/

                if(index!=-1)
                {
                  struct cache_action_t * MSHR = &cp->MSHR[bank][index];

                  assert((MSHR->cmd == cmd) && MSHR->cb);

                  if(MSHR->MSHR_linked)
                  {
                    /* this entry combined/coalesced, but still need to invoke miss_cb */
                    MSHR->when_started = sim_cycle;
                    cp->MSHR_unprocessed_num[bank]--;
                    cache_assert(cp->MSHR_unprocessed_num[bank] >= 0,(void)0);
                    if(MSHR->miss_cb && MSHR->op && (MSHR->action_id == MSHR->get_action_id(MSHR->op)))
                      MSHR->miss_cb(MSHR->op,cp->next_level->latency); /* restarts speculative scheduling */
                    sent_something = true;
                  }
                  else
                  {
                    cache_assert(!MSHR->MSHR_linked,(void)0);


                  }
                }

		/*Now sending all that can be sent if oldest cannot be sent*/
                for(int i=0;i<cp->MSHR_size;i++)
                {
                  struct cache_action_t * MSHR = &cp->MSHR[bank][i];

                  
		  if((MSHR->cmd == cmd) && MSHR->cb && (MSHR->when_started == TICK_T_MAX))
		  {
			assert((MSHR->cmd == cmd) && MSHR->cb);
                  	if(MSHR->MSHR_linked)
                  	{
                    		/* this entry combined/coalesced, but still need to invoke miss_cb */
                    		MSHR->when_started = sim_cycle;
                  		cp->MSHR_unprocessed_num[bank]--;
                   		cache_assert(cp->MSHR_unprocessed_num[bank] >= 0,(void)0);
                   	 	if(MSHR->miss_cb && MSHR->op && (MSHR->action_id == MSHR->get_action_id(MSHR->op)))
                     		 MSHR->miss_cb(MSHR->op,cp->next_level->latency); /* restarts speculative scheduling */
                    		sent_something = true;
                  	}
                        else
                        {
                    		cache_assert(!MSHR->MSHR_linked,(void)0);
  
                	}
		  }
                }
		/*Finished sending all requests*/
              }
            }
          }
        }
  }
  cp->check_for_MSHR_work = MSHR_work_found;

  /* occupancy stats */
  int bank;
  int max_size = cp->MSHR_banks * cp->MSHR_size;
  int total_occ = 0;
  for(bank=0;bank<cp->MSHR_banks;bank++)
  {
    total_occ += cp->MSHR_num[bank];
  }
  CACHE_STAT(cp->stat.MSHR_occupancy += total_occ;)
  CACHE_STAT(cp->stat.MSHR_full_cycles += (total_occ == max_size);)

  max_size = cp->MSHR_banks * cp->WBB_size;
  total_occ = 0;
  for(bank=0;bank<cp->MSHR_banks;bank++)
  {
    total_occ += cp->WBB_num[bank];
  }
  CACHE_STAT(cp->stat.WBB_occupancy += total_occ;)
  CACHE_STAT(cp->stat.WBB_full_cycles += (total_occ == max_size);)

  cp->check_for_work = work_found;
}

/* Attempt to enqueue a prefetch request, based on the predicted
   prefetch addresses in the prefetch FIFO (PFF) */
static void cache_prefetch_llc(struct cache_t * const cp)
{
  /* if the PF controller says the bus hasn't been too busy */
  if(!cp->PF_sample_interval || (cp->PF_state == PF_OK))
  {
    /* check prefetch FIFO for new prefetch requests - max one per cycle */
    if(cp->PFF && cp->PFF_num)
    {
      md_paddr_t pf_addr = cp->PFF[cp->PFF_head].addr;
      struct core_t * core = cp->PFF[cp->PFF_head].core; // tracks originating/owner core
      const int bank = GET_MSHR_BANK(pf_addr);

      if((cp->MSHR_num[bank] < cp->prefetch_threshold) /* if MSHR is too full, don't add more requests */
         && (cp->MSHR_num_pf[bank] < cp->prefetch_max))
      {
        md_addr_t pf_PC = cp->PFF[cp->PFF_head].PC;
        if(cache_enqueuable(cp,DO_NOT_TRANSLATE,pf_addr))
        {
          cache_enqueue(core,cp,NULL,CACHE_PREFETCH,DO_NOT_TRANSLATE,pf_PC,pf_addr,(seq_t)-1,bank,NO_MSHR,NULL,dummy_callback,NULL,NULL,NULL);
          cp->PFF_head = modinc(cp->PFF_head,cp->PFF_size); //(cp->PFF_head+1) % cp->PFF_size;
          cp->PFF_num --;
          cp->counters.prefetch.FIFO.read++;
          cache_assert(cp->PFF_num >= 0,(void)0);
        }
      }
    }
  }
}

/* Update the bus-utilization-based prefetch controller. */
static void prefetch_controller_update_llc(struct cache_t * const cp)
{
  if(cp->PF_sample_interval && ((sim_cycle % cp->PF_sample_interval) == 0))
  {

  }
}

/* Make use of the bus, thereby making it NOT free for some number of cycles hence */


