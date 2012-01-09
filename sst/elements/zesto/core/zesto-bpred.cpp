/* zesto-bpred.cpp - Zesto branch predictor classes
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

/*
 * This file, along with the corresponding header file (zesto-bpred.h), the
 * predictor definitions (in the ZCOMPS-bpred subdirectory), the fusion or
 * meta-prediction algorithms (in the ZCOMPS-fusion subdirectory), and the
 * target predictor definitions (in the ZCOMPS-btb and ZCOMPS-ras
 * subdirectories), provide a much more flexible and extensible interface to
 * the branch direction and branch target prediction mechanisms.
 *
 * All of the code for each branch prediction algorithm (or meta- predictor, or
 * target predictor, ...) is placed in its own, separate .cpp file in the
 * corresponding ZCOMPS-* subdirectory.  Adding new predictors is relatively
 * quick and painless.  The perl script make_def_lists.pl makes sure that all
 * of the code is included during compilation (the script is called from the
 * Makefile, so don't worry about it).
 */

#include <math.h>
#include "simplescalar/sim.h"
#include "simplescalar/stats.h"
#include "simplescalar/valcheck.h"
#include "zesto-core.h"
#include "zesto-bpred.h"

#define BPRED_STAT(x) if(!frozen) {x}

/*======================================================================*/
/* The definitions are all placed in separate files, for organizational
   purposes.                                                            */
/*======================================================================*/

/* COMPONENT BRANCH DIRECTION PREDICTORS
Arguments:
sc       - pointer to a state container/cache for the predictor
PC       - program counter (PC) value
tPC      - taken-branch target address
oPC      - "Oracle" PC or actual next instruction address, also shifted
outcome  - actual (oracle) branch direction
our_pred - what the branch predictor had predicted (some predictors
           need to know which way they predicted because update
           rules may vary depending on the correctness of the
           overall prediction).
 */
#define BPRED_LOOKUP_HEADER \
  bool lookup(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome)
#define BPRED_UPDATE_HEADER \
  void update(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome, const bool our_pred)
#define BPRED_SPEC_UPDATE_HEADER \
  void spec_update(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome, const bool our_pred)
#define BPRED_RECOVER_HEADER \
  void recover(class bpred_sc_t * const scvp, const bool outcome)
#define BPRED_FLUSH_HEADER \
  void flush(class bpred_sc_t * const scvp)
#define BPRED_REG_STATS_HEADER \
  void reg_stats(struct stat_sdb_t * const sdb, struct core_t * const core)
#define BPRED_RESET_STATS_HEADER \
  void reset_stats(void)
#define BPRED_GET_CACHE_HEADER \
  class bpred_sc_t * get_cache(void)
#define BPRED_RET_CACHE_HEADER \
  void ret_cache(class bpred_sc_t * const scvp)

#include "ZCOMPS-bpred.list"

#define BPRED_PARSE_ARGS
/*====================================================*/
/* argument parsing                                   */
/*====================================================*/
static class bpred_dir_t *
bpred_from_string(char * const opt_string)
{
  char name[256];
  char type[256];

  /* the format string "%[^:]" for scanf reads a string consisting of non-':' characters */
  if(sscanf(opt_string,"%[^:]",type) != 1)
    fatal("malformed bpred option string: %s",opt_string);

  /* include the argument parsing code.  BPRED_PARSE_ARGS is defined to
     include only the parsing code and not the other branch predictor code. */
#include "ZCOMPS-bpred.list"

  /* UNKNOWN Predictor Type */
  fatal("Unknown branch predictor type (%s)",opt_string);
}
#undef BPRED_PARSE_ARGS

bpred_dir_t::bpred_dir_t(void)
{
}

bpred_dir_t::~bpred_dir_t()
{
  if(name) free(name); name = NULL;
  if(type) free(type); type = NULL;
}

void bpred_dir_t::init(void)
{
  name = NULL;
  type = NULL;
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
  frozen = 0;
}

void bpred_dir_t::update(
    class bpred_sc_t * const sc,
    const md_addr_t PC,
    const md_addr_t tPC,
    const md_addr_t oPC,
    const bool outcome,
    const bool our_pred)
{
  if(!sc->updated)
  {
    BPRED_STAT(updates++;)
    BPRED_STAT(num_hits += our_pred == outcome;)
    sc->updated = true;
  }
}

void bpred_dir_t::spec_update(
    class bpred_sc_t * const sc,
    const md_addr_t PC,
    const md_addr_t tPC,
    const md_addr_t oPC,
    const bool outcome,
    const bool our_pred)
{
  BPRED_STAT(spec_updates++;)
}

void bpred_dir_t::recover(
    class bpred_sc_t * const sc,
    const bool outcome)
{
  return;
}

void bpred_dir_t::flush(class bpred_sc_t * const sc)
{
  return;
}

void bpred_dir_t::reg_stats(
    struct stat_sdb_t * const sdb,
    struct core_t * const core)
{
  int id = 0;
  if(core)
    id = core->current_thread->id;
  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"c%d.%s.type",id,name);
  sprintf(buf2,"prediction algorithm of %s",name);
  stat_reg_string(sdb, buf, buf2, type, NULL);
  sprintf(buf,"c%d.%s.bits",id,name);
  sprintf(buf2,"total size of %s in bits",name);
  stat_reg_int(sdb, true, buf, buf2, &bits, bits, NULL);
  sprintf(buf,"c%d.%s.size",id,name);
  sprintf(buf2,"total size of %s in KB",name);
  sprintf(buf3,"c%d.%s.bits/8192.0",id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);

  sprintf(buf,"c%d.%s.lookups",id,name);
  sprintf(buf2,"number of prediction lookups in %s (including wrong-path)",name);
  stat_reg_counter(sdb, true, buf, buf2, &lookups, lookups, NULL);
  sprintf(buf,"c%d.%s.updates",id,name);
  sprintf(buf2,"number of prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &updates, updates, NULL);
  sprintf(buf,"c%d.%s.spec_updates",id,name);
  sprintf(buf2,"number of speculative prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &spec_updates, spec_updates, NULL);
  sprintf(buf,"c%d.%s.hits",id,name);
  sprintf(buf2,"number of correct predictions in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_hits, num_hits, NULL);
  sprintf(buf,"c%d.%s.hit_rate",id,name);
  sprintf(buf2,"fraction of correct predictions in %s",name);
  sprintf(buf3,"c%d.%s.hits/c%d.%s.updates",id,name,id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.%s.MPKI",id,name);
  sprintf(buf2,"misses per thousand insts in %s",name);
  sprintf(buf3,"((c%d.%s.updates - c%d.%s.hits) / c%d.commit_insn) * 1000.0",id,name,id,name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
}

void bpred_dir_t::reset_stats(void)
{
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
}

class bpred_sc_t * bpred_dir_t::get_cache(void)
{
  class bpred_sc_t * sc = new bpred_sc_t();
  if(!sc)
    fatal("couldn't calloc default State Cache");
  return sc;
}

void bpred_dir_t::ret_cache(class bpred_sc_t * const sc)
{
  delete(sc);
}

/* FUSION/META-PREDICTION
Arguments:
scvp     - pointer to a state container/cache for the meta-predictor
preds    - the individual predictions of the component predictors
PC       - program counter (PC) value
tPC      - taken-branch target address
oPC      - "Oracle" PC or actual next instruction address
outcome  - actual (oracle) branch direction
our_pred - what the meta-predictor had predicted
 */
#define FUSION_LOOKUP_HEADER \
  bool lookup(class fusion_sc_t * const scvp,const bool * const preds,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const bool outcome)
#define FUSION_UPDATE_HEADER \
  void update(class fusion_sc_t * const scvp,const bool * const preds,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const bool outcome,const bool our_pred)
#define FUSION_SPEC_UPDATE_HEADER \
  void spec_update(class fusion_sc_t * const scvp,const bool * const preds,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const bool outcome,const bool our_pred)
#define FUSION_RECOVER_HEADER \
  void recover(class fusion_sc_t * const scvp,bool const outcome)
#define FUSION_FLUSH_HEADER \
  void flush(class fusion_sc_t * const scvp)
#define FUSION_REG_STATS_HEADER \
  void reg_stats(struct stat_sdb_t * const sdb, struct core_t * const core)
#define FUSION_RESET_STATS_HEADER \
  void reset_stats(void)
#define FUSION_GET_CACHE_HEADER \
  class fusion_sc_t * get_cache(void)
#define FUSION_RET_CACHE_HEADER \
  void ret_cache(class fusion_sc_t * const scvp)

#include "ZCOMPS-fusion.list"

#define BPRED_PARSE_ARGS
/*====================================================*/
/* fusion argument parsing                            */
/*====================================================*/
static class fusion_t *
fusion_from_string(
    const int num_pred,
    struct bpred_dir_t ** bpreds,
    char * const opt_string)
{
  char name[256];
  char type[256];

  /* the format string "%[^:]" for scanf reads a string consisting of non-':' characters */
  if(sscanf(opt_string,"%[^:]",type) != 1)
    fatal("malformed fusion option string: %s",opt_string);

  /* include the argument parsing code.  BPRED_PARSE_ARGS is defined to
     include only the parsing code and not the other predictor code. */
#include "ZCOMPS-fusion.list"

  /* UNKNOWN fusion Type */
  fatal("Unknown predictor fusion type (%s)",opt_string);
}
#undef BPRED_PARSE_ARGS

fusion_t::fusion_t(void)
{
}

fusion_t::~fusion_t(void)
{
  if(name) free(name); name = NULL;
  if(type) free(type); type = NULL;
}

void fusion_t::init(void)
{
  name = NULL;
  type = NULL;
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
  frozen = false;
}

void fusion_t::update(
    class fusion_sc_t * const sc,
    const bool * const preds,
    const md_addr_t PC,
    const md_addr_t tPC,
    const md_addr_t oPC,
    const bool outcome,
    const bool our_pred)
{
  if(!sc->updated)
  {
    BPRED_STAT(updates++;)
    BPRED_STAT(num_hits += our_pred == outcome;)
    sc->updated = true;
  }
}

void fusion_t::spec_update(
    class fusion_sc_t * const sc,
    const bool * const preds,
    const md_addr_t PC,
    const md_addr_t tPC,
    const md_addr_t oPC,
    const bool outcome,
    const bool our_pred)
{
  /* default fusion struct is the same as the default bpred struct */
  BPRED_STAT(spec_updates++;)
  return;
}

void fusion_t::recover(
    class fusion_sc_t * const sc,
    const bool outcome)
{
  return;
}

void fusion_t::flush(class fusion_sc_t * const sc)
{
  return;
}

void fusion_t::reg_stats(
    struct stat_sdb_t * const sdb,
    struct core_t * const core)
{
  int id = 0;
  if(core)
    id = core->current_thread->id;
  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"c%d.%s.type",id,name);
  sprintf(buf2,"fusion algorithm of %s",name);
  stat_reg_string(sdb, buf, buf2, type, NULL);
  sprintf(buf,"c%d.%s.bits",id,name);
  sprintf(buf2,"total size of %s in bits",name);
  stat_reg_int(sdb, true, buf, buf2, &bits, bits, NULL);
  sprintf(buf,"c%d.%s.size",id,name);
  sprintf(buf2,"total size of %s in KB",name);
  sprintf(buf3,"c%d.%s.bits/8192.0",id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);

  sprintf(buf,"c%d.%s.lookups",id,name);
  sprintf(buf2,"number of prediction lookups in %s (including wrong-path)",name);
  stat_reg_counter(sdb, true, buf, buf2, &lookups, lookups, NULL);
  sprintf(buf,"c%d.%s.updates",id,name);
  sprintf(buf2,"number of prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &updates, updates, NULL);
  sprintf(buf,"c%d.%s.spec_updates",id,name);
  sprintf(buf2,"number of speculative prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &spec_updates, spec_updates, NULL);
  sprintf(buf,"c%d.%s.hits",id,name);
  sprintf(buf2,"number of correct predictions in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_hits, num_hits, NULL);
  sprintf(buf,"c%d.%s.hit_rate",id,name);
  sprintf(buf2,"fraction of correct predictions in %s",name);
  sprintf(buf3,"c%d.%s.hits/c%d.%s.updates",id,name,id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.%s.MPKI",id,name);
  sprintf(buf2,"misses per thousand insts in %s",name);
  sprintf(buf3,"((c%d.%s.updates - c%d.%s.hits) / c%d.commit_insn) * 1000.0",id,name,id,name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
}

void fusion_t::reset_stats(void)
{
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
}

class fusion_sc_t * fusion_t::get_cache(void)
{
  struct fusion_sc_t * sc = new fusion_sc_t();
  if(!sc)
    fatal("couldn't calloc default State Cache");
  return sc;
}

void fusion_t::ret_cache(class fusion_sc_t * const sc)
{
  delete(sc);
}

/* BRANCH TARGET PREDICTION (not including subroutine returns)
Arguments:
scvp       - pointer to a state container/cache for the predictor
PC         - program counter (PC) value, *not* shifted
tPC        - taken-branch target address, *not* shifted
oPC        - oracle PC or actual next instruction address, *not* shifted
our_target - what the predictor said the next inst address would be
outcome    - actual (oracle) branch direction
our_pred   - what the branch-predictor had predicted
 */
#define BTB_LOOKUP_HEADER \
  md_addr_t lookup(class BTB_sc_t * const scvp,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const bool outcome,const bool our_pred)
#define BTB_UPDATE_HEADER \
  void update(class BTB_sc_t * const scvp,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const md_addr_t our_target,const bool outcome,const bool our_pred)
#define BTB_SPEC_UPDATE_HEADER \
  void spec_update(class BTB_sc_t * const scvp,const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC,const md_addr_t our_target,const bool outcome,const bool our_pred)
#define BTB_RECOVER_HEADER \
  void recover(class BTB_sc_t * const scvp,const bool outcome)
#define BTB_FLUSH_HEADER \
  void flush(class BTB_sc_t * const scvp)
#define BTB_REG_STATS_HEADER \
  void reg_stats(struct stat_sdb_t * const sdb, struct core_t * const core)
#define BTB_RESET_STATS_HEADER \
  void reset_stats(void)
#define BTB_GET_CACHE_HEADER \
  class BTB_sc_t * get_cache(void)
#define BTB_RET_CACHE_HEADER \
  void ret_cache(class BTB_sc_t * const scvp)


#include "ZCOMPS-btb.list"

/*=================================================*/
/* BTB argument parsing                            */
/*=================================================*/
#define BPRED_PARSE_ARGS
static class BTB_t *
BTB_from_string(char * const opt_string)
{
  char name[256];
  char type[256];

  /* the format string "%[^:]" for scanf reads a string consisting of non-':' characters */
  if(sscanf(opt_string,"%[^:]",type) != 1)
    fatal("malformed BTB option string: %s",opt_string);

  /* include the argument parsing code.  BPRED_PARSE_ARGS is defined to
     include only the parsing code and not the other predictor code. */
#include "ZCOMPS-btb.list"

  /* UNKNOWN BTB Type */
  fatal("Unknown BTB type (%s)",opt_string);
}
#undef BPRED_PARSE_ARGS


BTB_t::BTB_t()
{
}

BTB_t::~BTB_t()
{
  if(name) free(name); name = NULL;
  if(type) free(type); type = NULL;
}

void BTB_t::init(void)
{
  name = NULL;
  type = NULL;
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
  num_nt = 0;
  bits = 0;
  frozen = false;
}

void
BTB_t::spec_update(
    class BTB_sc_t * const sc,
    const md_addr_t PC,
    const md_addr_t targetPC,
    const md_addr_t oraclePC,
    const md_addr_t our_target,
    const bool outcome,
    const bool our_pred)
{
  BPRED_STAT(spec_updates++;)
  return;
}

void
BTB_t::recover(
    class BTB_sc_t * const sc,
    const bool outcome)
{
  return;
}

void BTB_t::flush(class BTB_sc_t * const sc)
{
  return;
}

void
BTB_t::reg_stats(
    struct stat_sdb_t * const sdb,
    struct core_t * const core)
{
  int id = 0;
  if(core)
    id = core->current_thread->id;
  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"c%d.%s.type",id,name);
  sprintf(buf2,"BTB type of %s",name);
  stat_reg_string(sdb, buf, buf2, type, NULL);
  sprintf(buf,"c%d.%s.bits",id,name);
  sprintf(buf2,"total size of %s in bits",name);
  stat_reg_int(sdb, true, buf, buf2, &bits, bits, NULL);
  sprintf(buf,"c%d.%s.size",id,name);
  sprintf(buf2,"total size of %s in KB",name);
  sprintf(buf3,"c%d.%s.bits/8192.0",id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);

  sprintf(buf,"c%d.%s.lookups",id,name);
  sprintf(buf2,"number of prediction lookups in %s (including wrong-path)",name);
  stat_reg_counter(sdb, true, buf, buf2, &lookups, lookups, NULL);
  sprintf(buf,"c%d.%s.updates",id,name);
  sprintf(buf2,"number of prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &updates, updates, NULL);
  sprintf(buf,"c%d.%s.spec_updates",id,name);
  sprintf(buf2,"number of speculative prediction updates in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &spec_updates, spec_updates, NULL);
  sprintf(buf,"c%d.%s.hits",id,name);
  sprintf(buf2,"number of correct predictions in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_hits, num_hits, NULL);
  sprintf(buf,"c%d.%s.hit_rate",id,name);
  sprintf(buf2,"fraction of correct predictions in %s",name);
  sprintf(buf3,"c%d.%s.hits/c%d.%s.updates",id,name,id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.%s.MPKI",id,name);
  sprintf(buf2,"misses per thousand insts in %s",name);
  sprintf(buf3,"((c%d.%s.updates - c%d.%s.hits) / c%d.commit_insn) * 1000.0",id,name,id,name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);

  sprintf(buf,"c%d.%s.num_nt",id,name);
  sprintf(buf2,"number of NT predictions from %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_nt, num_nt, NULL);
  sprintf(buf,"c%d.%s.frac_nt",id,name);
  sprintf(buf2,"fraction of targets predicted NT by %s",name);
  sprintf(buf3,"c%d.%s.num_nt / c%d.%s.updates",id,name,id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
}

void BTB_t::reset_stats()
{
  num_hits = 0;
  lookups = 0;
  updates = 0;
  spec_updates = 0;
  num_nt = 0;
}

class BTB_sc_t * BTB_t::get_cache(void)
{
  class BTB_sc_t * sc = new BTB_sc_t();
  if(!sc)
    fatal("couldn't calloc default State Cache");
  return sc;
}

void BTB_t::ret_cache(class BTB_sc_t * sc)
{
  delete(sc);
}


/*=================================================*/
/* RAS (Return address stack predictor) functions  */
/*=================================================*/



/* RETURN ADDRESS PREDICTION
Arguments:
cpvp       - pointer to a checkpoint (for recovery)
PC         - program counter (PC) value, *not* shifted
ftPC       - fall-through PC (i.e., the return address)
tPC        - taken-branch target address, *not* shifted
oPC        - oracle PC or actual return address
 */
#define RAS_PUSH_HEADER \
  void push(const md_addr_t PC,const md_addr_t ftPC,const md_addr_t tPC,const md_addr_t oPC)
#define RAS_POP_HEADER \
  md_addr_t pop(const md_addr_t PC,const md_addr_t tPC,const md_addr_t oPC)
#define RAS_RECOVER_HEADER \
  void recover(class RAS_chkpt_t * const cpvp)
#define RAS_RECOVER_STATE_HEADER \
  void recover_state(class RAS_chkpt_t * const cpvp)
#define RAS_REG_STATS_HEADER \
  void reg_stats(struct stat_sdb_t * const sdb, struct core_t * const core)
#define RAS_RESET_STATS_HEADER \
  void reset_stats(void)
#define RAS_GET_STATE_HEADER \
  class RAS_chkpt_t * get_state(void)
#define RAS_RET_STATE_HEADER \
  void ret_state(class RAS_chkpt_t * const cpvp)


#include "ZCOMPS-ras.list"


#define BPRED_PARSE_ARGS
static class RAS_t *
RAS_from_string(char * const opt_string)
{
  char name[256];
  char type[256];

  /* the format string "%[^:]" for scanf reads a string consisting of non-':' characters */
  if(sscanf(opt_string,"%[^:]",type) != 1)
    fatal("malformed RAS option string: %s",opt_string);

  /* include the argument parsing code.  BPRED_PARSE_ARGS is defined to
     include only the parsing code and not the other predictor code. */
#include "ZCOMPS-ras.list"

  /* UNKNOWN RAS Type */
  fatal("Unknown RAS type (%s)",opt_string);
}
#undef BPRED_PARSE_ARGS

RAS_t::RAS_t()
{
  init();
}

RAS_t::~RAS_t()
{
  if(name) free(name); name = NULL;
  if(type) free(type); type = NULL;
}

void RAS_t::init(void)
{
  name = NULL;
  type = NULL;
  bits = 0;
  num_hits = 0;
  num_pushes = 0;
  num_pops = 0;
  num_recovers = 0;
  frozen = false;
}

void
RAS_t::push(
    const md_addr_t PC,
    const md_addr_t fallthruPC,
    const md_addr_t targetPC,
    const md_addr_t oraclePC)
{
}

void RAS_t::recover(class RAS_chkpt_t * const cp)
{
}

void RAS_t::recover_state(RAS_chkpt_t * const cp)
{
}

void
RAS_t::reg_stats(
    struct stat_sdb_t * const sdb,
    struct core_t * const core)
{
  int id = 0;
  if(core)
    id = core->current_thread->id;
  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"c%d.%s.type",id,name);
  sprintf(buf2,"RAS type of %s",name);
  stat_reg_string(sdb, buf, buf2, type, NULL);
  sprintf(buf,"c%d.%s.bits",id,name);
  sprintf(buf2,"total size of %s in bits",name);
  stat_reg_int(sdb, true, buf, buf2, &bits, bits, NULL);
  sprintf(buf,"c%d.%s.size",id,name);
  sprintf(buf2,"total size of %s in KB",name);
  sprintf(buf3,"c%d.%s.bits/8192.0",id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.%s.pushes",id,name);
  sprintf(buf2,"number of stack pushes to %s (including wrong-path)",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_pushes, num_pushes, NULL);
  sprintf(buf,"c%d.%s.pops",id,name);
  sprintf(buf2,"number of stack pops to %s (including wrong-path)",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_pops, num_pops, NULL);
  sprintf(buf,"c%d.%s.recover",id,name);
  sprintf(buf2,"number of stack recoveries/repairs to %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_recovers, num_recovers, NULL);
  sprintf(buf,"c%d.%s.hits",id,name);
  sprintf(buf2,"correct predictions in %s",name);
  stat_reg_counter(sdb, true, buf, buf2, &num_hits, num_hits, NULL);
  sprintf(buf,"c%d.%s.hit_rate",id,name);
  sprintf(buf2,"fraction of correct predictions in %s",name);
  sprintf(buf3,"c%d.%s.hits/c%d.%s.pops",id,name,id,name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.%s.MPKI",id,name);
  sprintf(buf2,"misses per thousand insts in %s",name);
  sprintf(buf3,"((c%d.%s.pops - c%d.%s.hits) / c%d.commit_insn) * 1000.0",id,name,id,name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
}

void RAS_t::reset_stats(void)
{
  num_hits = 0;
  num_pushes = 0;
  num_pops = 0;
  num_recovers = 0;
}

class RAS_chkpt_t * RAS_t::get_state(void)
{
  return NULL;
}

void RAS_t::ret_state(class RAS_chkpt_t * const cpvp)
{
}



/*====================================================*/
/* Functions for the top-level branch predictor class
   which contains all of the sub-predictors (e.g.,
   direction, meta, BTB, etc.)                        */
/*====================================================*/
bpred_t::bpred_t(
    const int arg_num_pred         /* num direction preds */,
    char ** const bpred_opts       /* config strings for dir preds */,
    char  * const fusion_opt       /* config string for meta-pred */,
    char  * const dirjmp_BTB_opt   /* direct jmp target pred config */,
    char  * const indirjmp_BTB_opt /* indirect jmp target pred config */,
    char  * const RAS_opt          /* subroutine retn addr pred config */,
    struct core_t * const arg_core
   )
{
  core = arg_core;

  num_pred = arg_num_pred;

  /* component br direction preds */
  bpreds = (class bpred_dir_t**) calloc(arg_num_pred,sizeof(class bpred_dir_t*));
  if(!bpreds)
    fatal("failed to calloc bpred array");
  for(int i=0;i<arg_num_pred;i++)
    bpreds[i] = bpred_from_string(bpred_opts[i]);

  /* meta-predictor */
  fusion = fusion_from_string(num_pred,bpreds,fusion_opt);

  /* target predictors */
  dirjmp_BTB = BTB_from_string(dirjmp_BTB_opt);

  if(indirjmp_BTB_opt)
  {
    if(strcasecmp(indirjmp_BTB_opt,"none")) /* use a different BTB for computed targets? */
      indirjmp_BTB = BTB_from_string(indirjmp_BTB_opt);
    else
      indirjmp_BTB = NULL;
  }
  else
    indirjmp_BTB = NULL;

  ras = RAS_from_string(RAS_opt);

  num_lookups = 0;
  num_updates = 0;
  num_spec_updates = 0;
  num_cond = 0;
  num_call = 0;
  num_ret = 0;
  num_uncond = 0;
  num_dir_hits = 0;  /* for *all* branches, conditional or not */
  num_addr_hits = 0; /* ditto */
  frozen = false;

  init_state_cache_pool(32);
}

bpred_t::~bpred_t()
{
  destroy_state_cache_pool();

  if(indirjmp_BTB)
  {
    delete indirjmp_BTB; indirjmp_BTB = NULL;
  }
  delete dirjmp_BTB; dirjmp_BTB = NULL;

  delete ras; ras = NULL;

  delete fusion;

  for(int i=0;i<(int)num_pred;i++)
  {
    delete bpreds[i]; bpreds[i] = NULL;
  }
  free(bpreds); bpreds = NULL;
}

/*==============================================================
  Returns the address of the next predicted instruction.  This
  includes the outputs from the branch direction predictor,
  BTB(s) and RAS.
 ==============================================================*/
md_addr_t
bpred_t::lookup(
    class bpred_state_cache_t * const sc,
    const unsigned int opflags,
    const md_addr_t PC,
    const md_addr_t fallthruPC,
    const md_addr_t targetPC,
    const md_addr_t oraclePC,
    const bool outcome)
{
  md_addr_t predPC, indirPredPC;
  int cond_dir = true;
  bool dir = 1;

  BPRED_STAT(num_lookups++;)

  /* first, we need to perform lookups in more or less everything so
     that the structures can set up recovery state if needed later. */

  /* TODO: only lookup when needed */
  for(int i=0;i<(int)num_pred;i++)
    sc->preds[i] = bpreds[i]->lookup(sc->pcache[i],PC,targetPC,oraclePC,outcome);

  #ifdef ZESTO_COUNTERS
  if(bpreds)
  {
    core->counters->bpreds.read++;
  }
  #endif

  cond_dir = fusion->lookup(sc->fcache,sc->preds,PC,targetPC,oraclePC,outcome);

  #ifdef ZESTO_COUNTERS
  if(fusion)
  {
    core->counters->fusion.read++;
  }
  #endif

  indirPredPC = predPC = dirjmp_BTB->lookup(sc->dirjmp,PC,targetPC,oraclePC,outcome,dir);

  #ifdef ZESTO_COUNTERS
  if(dirjmp_BTB)
  {
    core->counters->dirjmpBTB.read++;
  }
  #endif  

  if(indirjmp_BTB)
  {
    indirPredPC = indirjmp_BTB->lookup(sc->indirjmp,PC,targetPC,oraclePC,outcome,dir);
    #ifdef ZESTO_COUNTERS
    core->counters->indirjmpBTB.read++;
    #endif
  }

  if(MD_IS_RETURN(opflags)) /* return from subroutine */
  {
    /* use RAS */
    predPC = ras->pop(PC,targetPC,oraclePC);
    sc->our_pred = 1;

    #ifdef ZESTO_COUNTERS
    if(ras)
    {
      core->counters->RAS.read++;
    }
    #endif
  }
  else
  {
    if(opflags&F_COND) /* conditional branches */
    {
      dir = cond_dir;
    }
    else if(MD_IS_CALL(opflags)) /* subroutine call */
    {
      dir = 1;
      /* speculatively push return address on to stack */
      ras->push(PC,fallthruPC,targetPC,oraclePC);

      #ifdef ZESTO_COUNTERS
      if(ras)
      {
        core->counters->RAS.write++;
      }
      #endif
    }
    else /* unconditional jmp/branch */
    {
      dir = 1;
    }

    sc->our_pred = dir;

    /* Get the predicted branch target */
    if(dir && (opflags&F_INDIRJMP)) /* INDIRJMP (computed)*/
      predPC = indirPredPC;
  }
  if(!dir || (predPC == 1) || (predPC == 0))
    predPC = fallthruPC;

  ras->recover_state(sc->ras_checkpoint);
  sc->our_target = predPC;
  return predPC;
}

/*==================================================================*/
/* Branch prediction update: this function is called at commit and
   does not update the branch history registers.                    */
/*==================================================================*/
void
bpred_t::update(
    class bpred_state_cache_t * const sc,
    const unsigned int opflags,
    const md_addr_t PC,
    const md_addr_t targetPC,
    const md_addr_t oraclePC,
    const bool outcome)
{
  /* update stats */
  BPRED_STAT(num_updates++;)
  BPRED_STAT(num_dir_hits += (sc->our_pred == outcome);)
  BPRED_STAT(num_addr_hits += (sc->our_target == oraclePC);)

  if(!MD_IS_RETURN(opflags)) /* RETN's already speculatively handled */
  {
    /* else: no updates for uncond jmps/brs */
    if(opflags&F_COND)
    {
      BPRED_STAT(num_cond++;)
      for(int i=0;i<(int)num_pred;i++)
        bpreds[i]->update(sc->pcache[i],PC,targetPC,oraclePC,outcome,sc->preds[i]);

      #ifdef ZESTO_COUNTERS
      if(bpreds)
      {
        core->counters->bpreds.write++;
      }
      #endif

      fusion->update(sc->fcache,sc->preds,PC,targetPC,oraclePC,outcome,sc->our_pred);

      #ifdef ZESTO_COUNTERS
      if(fusion)
      {
        core->counters->fusion.write++;
      }
      #endif
    }
    else if(MD_IS_CALL(opflags))
    {
      BPRED_STAT(ras->num_pushes++;)
      BPRED_STAT(num_call++;)

      #ifdef ZESTO_COUNTERS
      if(ras)
      {
        core->counters->RAS.write++;
      }
      #endif
    }
    else
      BPRED_STAT(num_uncond++;)

    /* CALL's also speculatively modified the RAS */

    /* update BTBs */
    if(indirjmp_BTB && (opflags&F_INDIRJMP)) /* INDIRJMP, and there's an iBTB */
    {
      indirjmp_BTB->update(sc->indirjmp,PC,targetPC,oraclePC,sc->our_target,outcome,sc->our_pred);

      #ifdef ZESTO_COUNTERS
      core->counters->indirjmpBTB.write++;
      #endif
    }
    else /* DIRJMP or REP */
    {
      dirjmp_BTB->update(sc->dirjmp,PC,targetPC,oraclePC,sc->our_target,outcome,sc->our_pred);
 
      #ifdef ZESTO_COUNTERS
      if(dirjmp_BTB)
      {
        core->counters->dirjmpBTB.write++;
      }
      #endif
    }
  }
  else
  {
    BPRED_STAT(ras->num_pops++;)
    if(sc->our_target == oraclePC)
      BPRED_STAT(ras->num_hits++;)
    BPRED_STAT(num_ret++;)

    #ifdef ZESTO_COUNTERS
    if(ras)
    {
      core->counters->RAS.read++;
    }
    #endif
  }
}

/* Speculative update of branch history registers (and any other state you may
   care to speculatively update).  This is called in the front-end. */
void
bpred_t::spec_update(
    class bpred_state_cache_t * const sc,
    const unsigned int opflags,
    const md_addr_t PC,
    const md_addr_t targetPC,
    const md_addr_t oraclePC,
    const bool outcome
   )
{
  /* update stats */
  BPRED_STAT(num_spec_updates++;)

  /* only update conditional, all are updated with the "final" prediction, not their individual predictions */
  if(opflags&F_COND)
  {
    for(int i=0;i<(int)num_pred;i++)
      bpreds[i]->spec_update(sc->pcache[i],PC,targetPC,oraclePC,outcome,sc->our_pred);

    #ifdef ZESTO_COUNTERS
    if(bpreds)
    {
      core->counters->bpreds.write++;
    }
    #endif

    fusion->spec_update(sc->fcache,sc->preds,PC,targetPC,oraclePC,outcome,sc->our_pred);

    #ifdef ZESTO_COUNTERS
    if(fusion)
    {
      core->counters->fusion.write++;
    }
    #endif

    dirjmp_BTB->spec_update(sc->dirjmp,PC,targetPC,oraclePC,sc->our_target,outcome,sc->our_pred);

    #ifdef ZESTO_COUNTERS
    if(dirjmp_BTB)
    {
      core->counters->dirjmpBTB.write++;
    }
    #endif

    if(indirjmp_BTB)
    {
      indirjmp_BTB->spec_update(sc->indirjmp,PC,targetPC,oraclePC,sc->our_target,outcome,sc->our_pred);

      #ifdef ZESTO_COUNTERS
      core->counters->indirjmpBTB.write++;
      #endif
    }
  }
}


/*==================================================================*/
/* Hopefully uncorrupt the return address stack after a branch
   misprediction.  Also repair branch history registers.            */
/*==================================================================*/

void
bpred_t::recover(
    class bpred_state_cache_t * const sc,
    const bool outcome)
{
  for(int i=0;i<(int)num_pred;i++)
    bpreds[i]->recover(sc->pcache[i],outcome);

  #ifdef ZESTO_COUNTERS
  if(bpreds)
  {
    core->counters->bpreds.write++;
  }
  #endif

  fusion->recover(sc->fcache,outcome);

  #ifdef ZESTO_COUNTERS
  if(fusion)
  {
    core->counters->fusion.write++;
  }
  #endif

  if(ras)
  {
    ras->recover(sc->ras_checkpoint);

    #ifdef ZESTO_COUNTERS
    core->counters->RAS.write++;
    #endif
  }

  dirjmp_BTB->recover(sc->dirjmp,outcome);

  #ifdef ZESTO_COUNTERS
  if(dirjmp_BTB)
  {
    core->counters->dirjmpBTB.write++;
  }
  #endif

  if(indirjmp_BTB)
  {
    indirjmp_BTB->recover(sc->indirjmp,outcome);

    #ifdef ZESTO_COUNTERS
    core->counters->indirjmpBTB.write++;
    #endif
  }
}

/* Same as recover, but this does not shift in the latest branch outcome
   into the branch history registers.  This is used when flushing due to
   non-branch reasons (e.g., pipe-flush from load order violations). */
void
bpred_t::flush(class bpred_state_cache_t * const sc)
{
  for(int i=0;i<(int)num_pred;i++)
    bpreds[i]->flush(sc->pcache[i]);

  #ifdef ZESTO_COUNTERS
  if(bpreds)
  {
    core->counters->bpreds.write++;
  }
  #endif

  fusion->flush(sc->fcache);

  #ifdef ZESTO_COUNTERS
  if(fusion)
  {
    core->counters->fusion.write++;
  }
  #endif

  if(ras)
  {
    ras->recover(sc->ras_checkpoint);

    #ifdef ZESTO_COUNTERS
    core->counters->RAS.write++;
    #endif
  }

  dirjmp_BTB->flush(sc->dirjmp);

  #ifdef ZESTO_COUNTERS
  if(dirjmp_BTB)
  {
    core->counters->dirjmpBTB.write++;
  }
  #endif

  if(indirjmp_BTB)
  {
    indirjmp_BTB->flush(sc->indirjmp);

    #ifdef ZESTO_COUNTERS
    core->counters->indirjmpBTB.write++;
    #endif
  }
}

void
bpred_t::reg_stats(
    struct stat_sdb_t * const sdb,
    struct core_t * const core)
{
  int id = 0;
  if(core)
    id = core->current_thread->id;

  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"c%d.bpred_lookups",id);
  sprintf(buf2,"total branch predictor lookups (including wrong-path)");
  stat_reg_counter(sdb, true, buf, buf2, &num_lookups, num_lookups, NULL);
  sprintf(buf,"c%d.bpred_updates",id);
  sprintf(buf2,"total branch predictor updates");
  stat_reg_counter(sdb, true, buf, buf2, &num_updates, num_lookups, NULL);
  sprintf(buf,"c%d.bpred_cond_br",id);
  sprintf(buf2,"total branch predictor cond_br (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_cond, num_cond, NULL);
  sprintf(buf,"c%d.bpred_calls",id);
  sprintf(buf2,"total branch predictor calls (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_call, num_call, NULL);
  sprintf(buf,"c%d.bpred_rets",id);
  sprintf(buf2,"total branch predictor rets (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_ret, num_ret, NULL);
  sprintf(buf,"c%d.bpred_unconds",id);
  sprintf(buf2,"total branch predictor unconds (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_uncond, num_uncond, NULL);
  sprintf(buf,"c%d.bpred_dir_hits",id);
  sprintf(buf2,"total branch predictor dir_hits (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_dir_hits, num_dir_hits, NULL);
  sprintf(buf,"c%d.bpred_addr_hits",id);
  sprintf(buf2,"total branch predictor addr_hits (non-speculative)");
  stat_reg_counter(sdb, true, buf, buf2, &num_addr_hits, num_addr_hits, NULL);
  sprintf(buf,"c%d.bpred_dir_rate",id);
  sprintf(buf2,"overall branch direction prediction rate");
  sprintf(buf3,"c%d.bpred_dir_hits/c%d.bpred_updates",id,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.bpred_addr_rate",id);
  sprintf(buf2,"overall branch address prediction rate");
  sprintf(buf3,"c%d.bpred_addr_hits/c%d.bpred_updates",id,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.bpred_dir_MPKI",id);
  sprintf(buf2,"overall branch direction mispredictions per Kinst");
  sprintf(buf3,"((c%d.bpred_updates-c%d.bpred_dir_hits)/c%d.commit_insn) * 1000.0",id,id,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);
  sprintf(buf,"c%d.bpred_addr_MPKI",id);
  sprintf(buf2,"overall branch address mispredictions per Kinst");
  sprintf(buf3,"((c%d.bpred_updates-c%d.bpred_addr_hits)/c%d.commit_insn) * 1000.0",id,id,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, NULL);

  for(int i=0;i<(int)num_pred;i++)
    bpreds[i]->reg_stats(sdb,core);

  fusion->reg_stats(sdb,core);

  dirjmp_BTB->reg_stats(sdb,core);
  if(indirjmp_BTB)
    indirjmp_BTB->reg_stats(sdb,core);

  ras->reg_stats(sdb,core);
}

/* Reset branch predictor stats after fast-forwarding/warming */
void bpred_t::reset_stats(void)
{
  for(int i=0;i<(int)num_pred;i++)
    bpreds[i]->reset_stats();
  fusion->reset_stats();
  ras->reset_stats();
  dirjmp_BTB->reset_stats();
  if(indirjmp_BTB)
    indirjmp_BTB->reset_stats();

  num_lookups = 0;
  num_updates = 0;
  num_spec_updates = 0;
  num_cond = 0;
  num_call = 0;
  num_ret = 0;
  num_uncond = 0;
  num_dir_hits = 0;
  num_addr_hits = 0;
}

/* tell the branch predictor components that their stats should now be frozen */
void bpred_t::freeze_stats(void)
{
  frozen = true;
  for(int i=0;i<(int)num_pred;i++)
    bpreds[i]->frozen = true;
  fusion->frozen = true;
  ras->frozen = true;
  dirjmp_BTB->frozen = true;
  if(indirjmp_BTB)
    indirjmp_BTB->frozen = true;
}



/*====================================================================*/
/* These functions return a suitably sized array for storing the
   outcomes, and cache predictor state.  This can then be passed back
   to the update function.  The "State Cache" is analogous to the
   bpred_update structs in the old bpred.[ch], but each predictor
   component may furnish their own struct, so any kind of data may
   be squirreled away without changing any code outside of the
   predictor implementation.  Since the structs aren't known ahead
   of time, a new State Cache must be allocated each time a br/jmp
   instruction is processed, and deallocated when the simulator is
   done with that inst.  To avoid constantly calloc/free-ing these
   State Caches, we maintain a pool of these State Caches, reusing
   the structs whenever we can. */
/*====================================================================*/


void
bpred_t::init_state_cache_pool(const int start_size)
{
  if(start_size <= 0)
    fatal("bpred State Cache must be initialized to 1 or more elements");

  bpredSCC_size = start_size;
  bpredSCC_head = 0;

  bpredSCC = (class bpred_state_cache_t**) calloc(start_size,sizeof(*bpredSCC));
  if(!bpredSCC)
    fatal("couldn't calloc bpred SCC");
}

/* Any state caches available in the free pool?  If so, return one
   and update the pool's pointers.  If not, allocate a new one and
   return that. */
class bpred_state_cache_t *
bpred_t::get_state_cache(void)
{
  class bpred_state_cache_t * sc;

  if(!bpredSCC[bpredSCC_head])
  {
    SC_debt ++;
    sc = new bpred_state_cache_t();
    if(!sc)
      fatal("couldn't calloc prediction container");
    sc->preds = (bool*) calloc(num_pred,sizeof(*sc->preds));
    if(!sc->preds)
      fatal("couldn't calloc prediction container");
    sc->pcache = (class bpred_sc_t**) calloc(num_pred,sizeof(class bpred_sc_t**));
    if(!sc->pcache)
      fatal("couldn't calloc prediction container");
    for(int i=0;i<(int)num_pred;i++)
      sc->pcache[i] = bpreds[i]->get_cache();
    sc->fcache = fusion->get_cache();
    sc->dirjmp = dirjmp_BTB->get_cache();
    if(indirjmp_BTB)
      sc->indirjmp = indirjmp_BTB->get_cache();
    sc->ras_checkpoint = ras->get_state();
  }
  else
  {
    sc = bpredSCC[bpredSCC_head];
    bpredSCC[bpredSCC_head] = NULL;
    if(bpredSCC_head > 0)
      bpredSCC_head--;
  }
  return sc;
}


/* Simulator's finished using the State Cache, so return it
   to the pool.  If returning the State Cache to the pool
   would exceed the pool's capacity, realloc it at twice the
   size so the number of realloc is only the log2 of the
   peak number of State Caches simultaneously in use. */
void
bpred_t::return_state_cache(class bpred_state_cache_t * const sc)
{
  if(bpredSCC[bpredSCC_head] == NULL)
  {
    bpredSCC[bpredSCC_head] = sc;
    return;
  }

  if(bpredSCC_head == (bpredSCC_size-1))
  {
    const int new_size = bpredSCC_size*2;
    bpredSCC = (class bpred_state_cache_t**) realloc(bpredSCC,new_size*sizeof(*bpredSCC));
    if(!bpredSCC)
      fatal("couldn't realloc bpredSCC");
    for(int i=bpredSCC_size;i<new_size;i++)
      bpredSCC[i] = NULL;
    bpredSCC_size = new_size;
  }

  bpredSCC_head ++;
  bpredSCC[bpredSCC_head] = sc;
}

/* At the very end of the simulation, we need to clean up.
   This frees any State Caches still in the pool, as well
   as the array for the pool itself. */
void
bpred_t::destroy_state_cache_pool(void)
{
  struct bpred_state_cache_t * sc;

  for(int i=0;i<bpredSCC_size;i++)
  {
    sc = bpredSCC[i];
    if(sc)
    {
      ras->ret_state(sc->ras_checkpoint);
      if(indirjmp_BTB)
        indirjmp_BTB->ret_cache(sc->indirjmp);
      dirjmp_BTB->ret_cache(sc->dirjmp);
      fusion->ret_cache(sc->fcache);
      for(int j=0;j<(int)num_pred;j++)
        bpreds[j]->ret_cache(sc->pcache[j]);
      free(sc->pcache); sc->pcache = NULL;
      free(sc->preds); sc->preds = NULL;
      delete(sc);
      SC_debt --;
    }
    bpredSCC[i] = NULL;
  }

  /* Did we free the same number of State Caches as we
     allocated? */
  if(SC_debt)
    fprintf(stderr,"warning (bpred): Leaked state caches = %d\n",SC_debt);

  free(bpredSCC);
  bpredSCC = NULL;
  bpredSCC_size = 0;
  bpredSCC_head = 0;
}

#undef BPRED_STAT

