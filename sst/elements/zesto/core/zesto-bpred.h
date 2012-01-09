#ifndef ZESTO_BPRED_INCLUDED
#define ZESTO_BPRED_INCLUDED

/* zesto-bpred.h - Zesto branch predictor classes
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
 */

/* Top-level "branch predictor" this handles direction, target, return address, etc. */
class bpred_t
{
  public:
  core_t *core;

  unsigned int num_pred;
  class bpred_dir_t ** bpreds;     /* branch direction predictor state(s) */
  class fusion_t * fusion;         /* meta-predictor state, combines preds from
                                       the branch direction predictors */
  class RAS_t * ras;                /* subroutine return address predictor state */
  class BTB_t * dirjmp_BTB;         /* direct (pc-relative) br/jmp target predictor */
  class BTB_t * indirjmp_BTB;       /* indirect (computed) br/jmp target predictor;
                                       if only one BTB is used for all targets, then
                                       these pointers will point to the same place. */
  int bpredSCC_size;
  int bpredSCC_head;
  class bpred_state_cache_t ** bpredSCC;
  int SC_debt;

  /* stats on branch type distributions */
  zcounter_t num_lookups;
  zcounter_t num_updates;
  zcounter_t num_spec_updates;
  zcounter_t num_cond;
  zcounter_t num_call;
  zcounter_t num_ret;
  zcounter_t num_uncond;
  zcounter_t num_dir_hits;  /* for *all* branches, conditional or not */
  zcounter_t num_addr_hits; /* ditto */
  bool frozen;

  bpred_t(int, char**, char*, char*, char*, char*, struct core_t * const arg_core);
  ~bpred_t();

  /* PC = current PC of the branch being predicted
     fallthruPC = PC if branch is not taken (varies in x86 due to non-uniform instruction lengths)
     targetPC = PC if branch is taken
     oraclePC = correct next-instruction PC
     outcome = correct taken/not-taken decision */
  md_addr_t lookup(
      class bpred_state_cache_t * const scvp,
      const unsigned int opflags,
      const md_addr_t PC,
      const md_addr_t fallthruPC,
      const md_addr_t targetPC,
      const md_addr_t oraclePC,
      const bool outcome);

  void update(
      class bpred_state_cache_t * const scvp,
      const unsigned int opflags,
      const md_addr_t PC,
      const md_addr_t targetPC,
      const md_addr_t oraclePC,
      const bool outcome);

  void spec_update(
      class bpred_state_cache_t * const scvp,
      const unsigned int opflags,
      const md_addr_t PC,
      const md_addr_t targetPC,
      const md_addr_t oraclePC,
      const bool outcome);

  void recover(
      class bpred_state_cache_t * const scvp,
      bool outcome);

  void flush(class bpred_state_cache_t * const scvp);

  void reg_stats(
      struct stat_sdb_t * const sdb,
      struct core_t * const core);

  void reset_stats();
  void freeze_stats();

  /* instead of the bpred_update structs used in the
     old bpred.[ch], each predictor can provide its
     own structs for caching away whatever state it
     wants to between lookup and update. */
  class bpred_state_cache_t * get_state_cache(void);
  void   return_state_cache(class bpred_state_cache_t * const /* state cache being returned */);

  private: /* called from constructor/destructor */
  void   init_state_cache_pool(int /* initial pool size */);
  void   destroy_state_cache_pool(void);
};


/* TODO: split dir, fusion, btb, ras into separate files */

/* default State Cache; everyone should have an updated
   flag: you can be speculatively updated multiple times, but
   should only be "really" updated once. */
class bpred_sc_t
{
  public:
  bool updated;
  bpred_sc_t(): updated(false) {};
};

/* Branch direction predictor */
class bpred_dir_t
{
  friend class bpred_t;

  protected:
  char * name;
  char * type;
  int bits;

  void init(void);

  zcounter_t num_hits;
  zcounter_t lookups;
  zcounter_t updates;
  zcounter_t spec_updates;
  bool frozen; /* set to TRUE when the core has reached its inst limit and stats should not longer be updated */

  public:
           bpred_dir_t();
  virtual ~bpred_dir_t();

  virtual bool lookup(
      class bpred_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome) = 0;

  virtual void update(
      class bpred_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome,
      const bool our_pred);

  virtual void spec_update(
      class bpred_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome,
      const bool our_pred);

  virtual void recover(
      class bpred_sc_t * const scvp,
      const bool outcome);

  virtual void flush(class bpred_sc_t * const scvp);

  virtual void reg_stats(
      struct stat_sdb_t * const sdb,
      struct core_t * const core);

  virtual void reset_stats(void);

  virtual class bpred_sc_t * get_cache(void);

  virtual void ret_cache(class bpred_sc_t * const scvp);
};

class fusion_sc_t:public bpred_sc_t
{
};

/* Branch direction meta-predictor */
class fusion_t
{
  friend class bpred_t;

  protected:
  char * name;
  char * type;
  int bits;
  int num_pred;

  zcounter_t num_hits;
  zcounter_t lookups;
  zcounter_t updates;
  zcounter_t spec_updates;
  bool frozen; /* set to TRUE when the core has reached its inst limit and stats should not longer be updated */

  void init(void);

  public:
           fusion_t();
  virtual ~fusion_t();

  virtual bool lookup(
      class fusion_sc_t * const scvp,
      const bool * const preds,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome) = 0;

  virtual void update(
      class fusion_sc_t * const scvp,
      const bool * const preds,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome,
      const bool our_pred);

  virtual void spec_update(
      class fusion_sc_t * const scvp,
      const bool * const preds,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome,
      const bool our_pred);

  virtual void recover(
      class fusion_sc_t * const scvp,
      const bool outcome);

  virtual void flush(class fusion_sc_t * const scvp);

  virtual void reg_stats(
      struct stat_sdb_t * const sdb,
      struct core_t * const core);

  virtual void reset_stats(void);

  virtual class fusion_sc_t * get_cache(void);

  virtual void ret_cache(class fusion_sc_t * const scvp);
};

class BTB_sc_t:public bpred_sc_t
{
};
/* general target address predictor */
class BTB_t
{
  friend class bpred_t;

  protected:
  char * name;
  char * type;
  int bits;

  zcounter_t num_hits;
  zcounter_t lookups;
  zcounter_t updates;
  zcounter_t spec_updates;
  zcounter_t num_nt;
  bool frozen;

  void init(void);

  public:

           BTB_t();
  virtual ~BTB_t();

  virtual md_addr_t lookup(
      class BTB_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const bool outcome,
      const bool our_pred) = 0;

  virtual void update(
      class BTB_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const md_addr_t our_target,
      const bool outcome,
      const bool our_pred) = 0;

  virtual void spec_update(
      class BTB_sc_t * const scvp,
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC,
      const md_addr_t our_target,
      const bool outcome,
      const bool our_pred);

  virtual void recover(
      class BTB_sc_t * const scvp,
      const bool outcome);

  virtual void flush(class BTB_sc_t * const scvp);

  virtual void reg_stats(
      struct stat_sdb_t * const sdb,
      struct core_t * const core);

  virtual void reset_stats();

  virtual class BTB_sc_t * get_cache(void);

  virtual void ret_cache(class BTB_sc_t * const scvp);
};

/* subroutine return address predictor */
class RAS_chkpt_t
{
  public:
};

class RAS_t
{
  friend class bpred_t;

  protected:
  char * name;
  char * type;
  int bits;

  zcounter_t num_hits;
  zcounter_t num_pushes;
  zcounter_t num_pops;
  zcounter_t num_recovers;
  bool frozen;

  void init(void);

  public:

           RAS_t();
  virtual ~RAS_t();

  virtual void push(
      const md_addr_t PC,
      const md_addr_t fallthruPC,
      const md_addr_t tPC,
      const md_addr_t oPC);

  virtual md_addr_t pop(
      const md_addr_t PC,
      const md_addr_t tPC,
      const md_addr_t oPC) = 0;

  virtual void recover(class RAS_chkpt_t * const cpvp); /* actually perform a recovery */

  virtual void recover_state(class RAS_chkpt_t * const cpvp);

  virtual void reg_stats(
      struct stat_sdb_t * const sdb,
      struct core_t * const core);

  virtual void reset_stats();

  virtual class RAS_chkpt_t * get_state(void);

  virtual void ret_state(class RAS_chkpt_t * const cpvp);
};

/* This is analogous to the various update_ptr's in the original
   bpred.[ch].  The type of information that a predictor might
   want to cache away to avoid redundant computations (like
   calculating indices and what not) will vary greatly from one
   predictor to the next, and so we allow each predictor to
   define any kind of state container or cache that they wish
   to.  As part of the standard predictor interface, each predictor
   provides functions for allocating and destroying these
   containers. */
class bpred_state_cache_t
{
  friend class bpred_t;

  protected:
  bool * preds;               /* predictions (direction of each component predictor) */
  md_addr_t our_target;       /* the predicted target */

  class bpred_sc_t **pcache;              /* br direction predictors' state containers/caches */
  class fusion_sc_t * fcache;              /* fusion/meta-predictor's state cache */
  class BTB_sc_t * dirjmp;              /* direct br/jmp target predictor's state cache */
  class BTB_sc_t * indirjmp;            /* indirect br/jmp target predictor's state cache */
  class RAS_chkpt_t * ras_checkpoint;   /* RAS recovery information */

  public:
  bool   our_pred;            /* the direction prediction */

  bpred_state_cache_t(void): preds(NULL), our_target(0), pcache(NULL), fcache(NULL), dirjmp(NULL), indirjmp(NULL),
                             ras_checkpoint(NULL), our_pred(false) {};
};

/*----------------------------------------------------------*/
/* saturating two bit counter implemented with no branching */
/*----------------------------------------------------------*/
typedef unsigned char my2bc_t;

#define _2BC_TABLE 0xD4E8
#define MY2BC_DIR_MASK 0x01
#define MY2BC_HYST_MASK 0x02
#define MY2BC_DIR(ctr) ((ctr)&MY2BC_DIR_MASK)
#define MY2BC_HYST(ctr) ((ctr)&MY2BC_HYST_MASK)

#define MY2BC_UPDATE(ctr,dir) (ctr)=(_2BC_TABLE>>(((ctr<<1)+(dir))<<1))&3
/* _2BC_table is a bit packed version of the state transition table
   for the 2bC:

   The bits are switched in their places (i.e. bit 0 = direction bit
   and bit 1 = confidence/hysteresis bit).  This allows the extraction
   of the direction bit with a simple AND instead of a SHIFT+AND.

   prev
   2bC
   LD  inc  dec
      +--------
   00 | 10   00  (8)
   01 | 11   10  (E)
   10 | 01   00  (4)
   11 | 11   01  (D)

   (LD: 'L'east Signif. Bit of 2bc,
   'D'irection Bit)

   The previous counter value and the branch outcome direction are
   used to shift the packed table by the proper amount, and then the
   two bits are masked out.

 */

/* similar to MY2BC_UPDATE, but these macros inc/dec (saturating) if
   the predicate p is true.

      (COND_INC)          (COND_DEC)
   LD   p=1 p=0        LD   p=1 p=0
      +--------           +--------
   00 | 10  00  (8)    00 | 00  00  (0)
   01 | 11  01  (D)    01 | 10  01  (9)
   10 | 01  10  (6)    10 | 00  10  (2)
   11 | 11  11  (F)    11 | 01  11  (7)
 */
#define _2BC_CINC_TABLE 0xF6D8
#define _2BC_CDEC_TABLE 0x7290

#define MY2BC_COND_INC(ctr,p) (ctr)=(_2BC_CINC_TABLE>>(((ctr<<1)+(p))<<1))&3
#define MY2BC_COND_DEC(ctr,p) (ctr)=(_2BC_CDEC_TABLE>>(((ctr<<1)+(p))<<1))&3

#define MY2BC_STRONG_TAKEN (3)
#define MY2BC_WEAKLY_TAKEN (1)
#define MY2BC_WEAKLY_NT (2)
#define MY2BC_STRONG_NT (0)



#endif /* ZESTO_BPRED_INCLUDED */
