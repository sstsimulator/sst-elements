#ifndef ZESTO_EXEC_INCLUDED
#define ZESTO_EXEC_INCLUDED

/* zesto-exec.h - Zesto execute stage class
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

/*----------SOM--------------------*/
/*----------EOM--------------------*/

class core_exec_t
{
  public:

  /* watchdog for detecting dead/live-locked simulations */
  tick_t last_completed;
  zcounter_t last_completed_count;
  seq_t last_Mop_seq;

  core_exec_t(void);
  virtual ~core_exec_t();
  virtual void reg_stats(struct stat_sdb_t *const sdb) = 0;
  virtual void freeze_stats(void) = 0;
  virtual void update_occupancy(void) = 0;
  void update_last_completed(tick_t now);

  /* "step()" functions */
  virtual void ALU_exec(void) = 0;
  virtual void LDST_exec(void) = 0;
  virtual void RS_schedule(void) = 0;
  virtual void LDQ_schedule(void) = 0;

  virtual void recover(const struct Mop_t * const Mop) = 0;
  virtual void recover(void) = 0;

  /* adds uop to a ready queue; called by alloc and exec */
  virtual void insert_ready_uop(struct uop_t * const uop) = 0;

  /* RS management functions
     RS_available() returns true if an RS entry is available
     RS_insert() inserts a uop into the RS
     RS_fuse_insert() adds a fused uop body to the uop head
       (and any previously fuse_inserted uops) to an already
       allocated RS entry (i.e., that alloc'd to the head)
     RS_deallocate() removes a uop from the RS; in the case
       of fused uops, should be called when the last uop exec's */
  virtual bool RS_available(int port) = 0;
  virtual void RS_insert(struct uop_t * const uop) = 0;
  virtual void RS_fuse_insert(struct uop_t * const uop) = 0;
  virtual void RS_deallocate(struct uop_t * const uop) = 0;

  /* LDQ management functions
     LDQ_availble() returns true if a LDQ entry is available
     LDQ_insert() inserts a load uop into the LDQ
     LDQ_deallocate() removes a uop from the LDQ (called at commit)
     LDQ_squash() squashes and deallocates the LDQ entry */
  virtual bool LDQ_available(void) = 0;
  virtual void LDQ_insert(struct uop_t * const uop) = 0;
  virtual void LDQ_deallocate(struct uop_t * const uop) = 0;
  virtual void LDQ_squash(struct uop_t * const dead_uop) = 0;

  /* STQ management functions
     STQ_availble() returns true if a STQ entry is available
     STQ_insert_sta() allocates a STQ entry to the STA uop
     STQ_insert_std() adds the STD uop to the same STQ entry
     STQ_deallocate_sta() removes the STA uop from the STQ
     STQ_deallocate_std() removes the STD uop from the STQ
     STQ_deallocate_senior() removes the store-writeback from the STQ
     STQ_squash_sta() squashes the STA uop from the STQ entry and
        deallocates the STQ entry
     STQ_squash_std() squashes and removes the STD uop from the STQ
     STQ_squash_senior() squashes and removes the store-writeback
        from the senior STQ
   */
  virtual bool STQ_available(void) = 0;
  virtual void STQ_insert_sta(struct uop_t * const uop) = 0;
  virtual void STQ_insert_std(struct uop_t * const uop) = 0;
  virtual void STQ_deallocate_sta(void) = 0;
  virtual bool STQ_deallocate_std(struct uop_t * const uop) = 0;
  virtual void STQ_deallocate_senior(void) = 0;
  virtual void STQ_squash_sta(struct uop_t * const dead_uop) = 0;
  virtual void STQ_squash_std(struct uop_t * const dead_uop) = 0;
  virtual void STQ_squash_senior(void) = 0;


  /*Service all the data callbacks from the memory subsystem - Both loads and stores*/
  virtual void memory_callbacks(void) = 0;

  /* Just a sanity-check after a recovery that the exec-stage's
     microarchitectural state is consistent. */
  virtual void recover_check_assertions(void) = 0;

  virtual bool has_load_arrived(int) = 0 ;

  virtual unsigned long long int get_load_latency(int LDQ_index) = 0 ;

  protected:
  struct core_t * core;
};

void exec_reg_options(struct opt_odb_t * odb, struct core_knobs_t * knobs);
class core_exec_t * exec_create(const char * exec_opt_string, struct core_t * core);

#endif /* ZESTO_EXEC_INCLUDED */
