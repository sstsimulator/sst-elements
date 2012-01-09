#ifndef ZESTO_ORACLE_INCLUDED
#define ZESTO_ORACLE_INCLUDED

/* zesto-oracle.h - Zesto oracle functional simulator class
 *
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
 *
 * NOTE: This file (zesto-oracle.c) contains code directly and
 * indirectly derived from previous SimpleScalar source files.
 * These sections are demarkated with "<SIMPLESCALAR>" and
 * "</SIMPLESCALAR>" to specify the start and end, respectively, of
 * such source code.  Such code is bound by the combination of terms
 * and agreements from both Zesto and SimpleScalar.  In case of any
 * conflicting terms (for example, but not limited to, use by
 * commercial entities), the more restrictive terms shall take
 * precedence (e.g., commercial and for-profit entities may not
 * make use of the code without a license from SimpleScalar, LLC).
 * The SimpleScalar terms and agreements are replicated below as per
 * their original requirements.
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
 */

#include "simplescalar/sim.h"
#include "simplescalar/memory.h"
 /*#include "qsim.h"*/

/* The following macros are used to pretty similarly to regular fatal and assert
   calls, with the exception that when *not* in DEBUG mode, the failure does not
   immediately terminate the program.  Instead, the retval argument is immediately
   returned by the enclosing function, and the oracle's hosed bit is set.  Upon
   detecting that it is hosed, the oracle will attempt to flush the pipeline and
   then restore the state of the processor.  This allows you to still get results
   even when you haven't debugged every last corner case.  Make sure you check the
   number of emergency recoveries in the simulator's output stats; if this number is
   small relative to the number of total simulated cycles, then your bug probably
   won't have much statistically significant impact on your results.  If it's a
   large number, then go fix your bug(s)! */
#ifdef DEBUG
#define zesto_fatal(msg, retval) fatal(msg)
#else
#define zesto_fatal(msg, retval) { \
  core->oracle->hosed = TRUE; \
  fprintf(stderr,"fatal (%s,%d:thread %d): ",__FILE__,__LINE__,core->current_thread->id); \
  fprintf(stderr,"%s\n",msg); \
  return (retval); \
}
#endif

#ifdef DEBUG
#define zesto_assert(cond, retval) assert(cond)
#else
#define zesto_assert(cond, retval) { \
  if(!(cond)) { \
    core->oracle->hosed = TRUE; \
    fprintf(stderr,"assertion failed (%s,%d:thread %d): ",__FILE__,__LINE__,core->current_thread->id); \
    fprintf(stderr,"%s\n",#cond); \
    return (retval); \
  } \
}
#endif

class core_oracle_t {

  /* struct for tracking all in-flight writers of registers */
  struct map_node_t {
    struct uop_t * uop;
    struct map_node_t * prev;
    struct map_node_t * next;
  };

  /* structs for tracking pre-commit memory writes */
#define MEM_HASH_SIZE 32768
#define MEM_HASH_MASK (MEM_HASH_SIZE-1)

  struct spec_mem_t {
    struct {
      struct spec_byte_t * head;
      struct spec_byte_t * tail;
    } hash[MEM_HASH_SIZE];
  };

  public:

  bool spec_mode;  /* are we currently on a wrong-path? */

  bool hosed; /* set to TRUE when something in the architected state (core->arch_state) has been seriously
                corrupted. */

  bool trap_on;	/* this is set when the TRAP instruction is encountered and reset when IRET is encountered*/
  core_oracle_t(struct core_t * const core);
  void reg_stats(struct stat_sdb_t * const sdb);
  void update_occupancy(void);

  struct Mop_t * get_Mop(const int index);
  int get_index(const struct Mop_t * const Mop);
  int next_index(const int index);

  bool spec_read_byte(const md_addr_t addr, byte_t * const valp);
  struct spec_byte_t * spec_write_byte(const md_addr_t addr, const byte_t val);

  struct Mop_t * exec(const md_addr_t requested_PC);
  void consume(const struct Mop_t * const Mop);
  void commit_uop(struct uop_t * const uop);
  void commit(const struct Mop_t * const commit_Mop);

  void recover(const struct Mop_t * const Mop); /* recovers oracle state */
  void pipe_recover(struct Mop_t * const Mop, const md_addr_t New_PC); /* unrolls pipeline state */
  void pipe_flush(struct Mop_t * const Mop);

  void complete_flush(void);
  void reset_execution(void);

  protected:

  struct Mop_t * MopQ;
  int MopQ_head;
  int MopQ_tail;
  int MopQ_num;
  int MopQ_size;
  struct Mop_t * current_Mop; /* pointer to track a Mop that has been executed but not consumed (i.e. due to fetch stall) */

  static struct map_node_t * map_free_pool;  /* for decode.dep_map */
  static int map_free_pool_debt;
  static struct spec_byte_t * spec_mem_free_pool; /* for oracle spec-memory map */

  struct core_t * core;
  struct spec_mem_t spec_mem_map;
  /* dependency tracking used by oracle */
  struct {
    struct map_node_t * head[MD_TOTAL_REGS];
    struct map_node_t * tail[MD_TOTAL_REGS];
  } dep_map;

  void undo(struct Mop_t * const Mop);

  void install_mapping(struct uop_t * const uop);
  void commit_mapping(const struct uop_t * const uop);
  void undo_mapping(const struct uop_t * const uop);
  struct map_node_t * get_map_node(void);
  void return_map_node(struct map_node_t * const p);

  void install_dependencies(struct uop_t * const uop);
  void commit_dependencies(struct uop_t * const uop);
  void undo_dependencies(struct uop_t * const uop);

  struct spec_byte_t * get_spec_mem_node(void);
  void return_spec_mem_node(struct spec_byte_t * const p);

  void commit_write_byte(struct spec_byte_t * const p);
  void squash_write_byte(struct spec_byte_t * const p);

  void cleanup_aborted_mop(struct Mop_t * const Mop);

  static const seq_t DUMMY_SYSCALL_ACTION_ID = ULL(0x2E570515CA111D);
  static const unsigned DUMMY_SYSCALL_OP = 0x515CA11;
};



/*Fetch an instruction with its memops*/
void md_fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, struct core_t * core);

/*Return the PC of the next instruction, Also return a bool value that informs that the instruction was followed by an interrupt*/
bool md_fetch_next_pc(md_addr_t *nextPC, struct core_t * core);

#endif /* ZESTO_ORACLE_INCLUDED */
