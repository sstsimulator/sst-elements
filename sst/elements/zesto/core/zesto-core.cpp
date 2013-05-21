/* zesto-core.cpp - Zesto core (single pipeline) class
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
 */

//std c++ libs

//std c libs
#include <stddef.h>
#include <signal.h>

//SST libs

//simplescalar libs

//zesto libs
#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"


bool core_t::static_members_initialized = false;
struct core_t::uop_array_t * core_t::uop_array_pool[MD_MAX_FLOWLEN+2+1];
struct odep_t * core_t::odep_free_pool = NULL;
int core_t::odep_free_pool_debt = 0;

/* CONSTRUCTOR */
core_t::core_t(SST::ComponentId_t cid, SST::Component::Params_t & params):
  SST::Component(cid),
  current_thread(NULL),
  num_emergency_recoveries(0), last_emergency_recovery_count(0),
  oracle(NULL), fetch(NULL), decode(NULL), alloc(NULL),
  exec(NULL), commit(NULL), request_id(0), global_action_id(0)  
{
  //interface with SST simulator
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  if (params.find("clockFreq") == params.end()) {
	_abort(zesto_core, "clock frequency not specified\n");
  }

  registerClock( params["clockFreq"],
		new SST::Clock::Handler<core_t>(this,
					&core_t::tick));

  //SST Links
  cache_link = configureLink ("cache_link",
				new SST::Event::Handler<core_t>(this,
					&core_t::cache_response_handler));
  assert(cache_link);


  if(params.find("nodeId") == params.end()) {
    _abort(zesto_core, "couldn't find node id\n");
  }
  id = params.find_integer("nodeId");  

  /* sim_pre_init */
  core_pre_init();

  if ( params.find("configFile") == params.end() ) {
    _abort(zesto_core, "couldn't find zesto config file\n");
  }
  
  /* input config */
  sim.config[1] = (char*)"-config";
  sim.config[2] = new char[params["configFile"].length()+1];
  strcpy(sim.config[2], params[ "configFile" ].c_str());

  /* register and process options */
  core_reg_options();

  /* initialize instruction decoder */
  md_init_decoder();

  global_seq = 0;

  assert(sizeof(struct uop_array_t) % 16 == 0);

  if(!static_members_initialized)
  {
    memzero(uop_array_pool,sizeof(uop_array_pool));
    static_members_initialized = true;
  }

  /* sim_post_init */
  core_post_init();

  core_reg_stats();

  stat.sim_start_time = time((time_t *)NULL);

  //delete
  delete[] sim.config[2];

  #ifdef ZESTO_COUNTERS
  counters = new core_counters_t();
  #endif

}

/* core pre-initialization (core-based sim_pre_init) */
void core_t::core_pre_init(void)
{
  knobs = (struct core_knobs_t*)calloc(1,sizeof(core_knobs_t));
 
  if(!knobs)
    fatal("Zesto core knobs failure\n");

  memzero(knobs,sizeof(knobs));
  memzero(&stat,sizeof(stat));
  memzero(&sim,sizeof(sim));

  knobs->model = (char*)"ATOM";

  knobs->memory.IL1PF_opt_str[0] = (char*)"nextline";
  knobs->memory.IL1_num_PF = 1;

  knobs->fetch.byteQ_size = 4;
  knobs->fetch.byteQ_linesize = 16;
  knobs->fetch.depth = 2;
  knobs->fetch.width = 4;
  knobs->fetch.IQ_size = 8;

  knobs->fetch.bpred_opt_str[0] = (char*)"2lev:local:1024:1024:10:1";
  knobs->fetch.bpred_opt_str[1] = (char*)"2lev:global:1:1024:12:1";
  knobs->fetch.num_bpred_components = 2;

  knobs->decode.depth = 3;
  knobs->decode.width = 4;
  knobs->decode.target_stage = 1;
  knobs->decode.branch_decode_limit = 1;

  knobs->decode.decoders[0] = 4;
  for(int i=1;i<MAX_DECODE_WIDTH;i++)
    knobs->decode.decoders[i] = 1;
  knobs->decode.num_decoder_specs = 4;

  knobs->decode.MS_latency = 0;

  knobs->decode.uopQ_size = 8;

  knobs->alloc.depth = 2;
  knobs->alloc.width = 4;

  knobs->exec.RS_size = 20;
  knobs->exec.LDQ_size = 20;
  knobs->exec.STQ_size = 16;

  knobs->exec.num_exec_ports = 4;
  knobs->exec.payload_depth = 1;
  knobs->exec.fp_penalty = 0;

  knobs->exec.port_binding[FU_IEU].num_FUs = 2;
  knobs->exec.fu_bindings[FU_IEU][0] = 0;
  knobs->exec.fu_bindings[FU_IEU][1] = 1;
  knobs->exec.latency[FU_IEU] = 1;
  knobs->exec.issue_rate[FU_IEU] = 1;

  knobs->exec.port_binding[FU_JEU].num_FUs = 1;
  knobs->exec.fu_bindings[FU_JEU][0] = 0;
  knobs->exec.latency[FU_JEU] = 1;
  knobs->exec.issue_rate[FU_JEU] = 1;

  knobs->exec.port_binding[FU_IMUL].num_FUs = 1;
  knobs->exec.fu_bindings[FU_IMUL][0] = 2;
  knobs->exec.latency[FU_IMUL] = 4;
  knobs->exec.issue_rate[FU_IMUL] = 1;

  knobs->exec.port_binding[FU_SHIFT].num_FUs = 1;
  knobs->exec.fu_bindings[FU_SHIFT][0] = 0;
  knobs->exec.latency[FU_SHIFT] = 1;
  knobs->exec.issue_rate[FU_SHIFT] = 1;

  knobs->exec.port_binding[FU_FADD].num_FUs = 1;
  knobs->exec.fu_bindings[FU_FADD][0] = 0;
  knobs->exec.latency[FU_FADD] = 3;
  knobs->exec.issue_rate[FU_FADD] = 1;

  knobs->exec.port_binding[FU_FMUL].num_FUs = 1;
  knobs->exec.fu_bindings[FU_FMUL][0] = 1;
  knobs->exec.latency[FU_FMUL] = 5;
  knobs->exec.issue_rate[FU_FMUL] = 2;

  knobs->exec.port_binding[FU_FCPLX].num_FUs = 1;
  knobs->exec.fu_bindings[FU_FCPLX][0] = 2;
  knobs->exec.latency[FU_FCPLX] = 58;
  knobs->exec.issue_rate[FU_FCPLX] = 58;

  knobs->exec.port_binding[FU_IDIV].num_FUs = 1;
  knobs->exec.fu_bindings[FU_IDIV][0] = 2;
  knobs->exec.latency[FU_IDIV] = 13;
  knobs->exec.issue_rate[FU_IDIV] = 13;

  knobs->exec.port_binding[FU_FDIV].num_FUs = 1;
  knobs->exec.fu_bindings[FU_FDIV][0] = 2;
  knobs->exec.latency[FU_FDIV] = 32;
  knobs->exec.issue_rate[FU_FDIV] = 24;

  knobs->exec.port_binding[FU_LD].num_FUs = 1;
  knobs->exec.fu_bindings[FU_LD][0] = 1;
  knobs->exec.latency[FU_LD] = 1;
  knobs->exec.issue_rate[FU_LD] = 1;

  knobs->exec.port_binding[FU_STA].num_FUs = 1;
  knobs->exec.fu_bindings[FU_STA][0] = 2;
  knobs->exec.latency[FU_STA] = 1;
  knobs->exec.issue_rate[FU_STA] = 1;

  knobs->exec.port_binding[FU_STD].num_FUs = 1;
  knobs->exec.fu_bindings[FU_STD][0] = 3;
  knobs->exec.latency[FU_STD] = 1;
  knobs->exec.issue_rate[FU_STD] = 1;

  knobs->memory.DL2PF_opt_str[0] = (char*)"nextline";
  knobs->memory.DL2_num_PF = 1;
  knobs->memory.DL2_MSHR_cmd = (char*)"RPWB";

  knobs->memory.DL1PF_opt_str[0] = (char*)"nextline";
  knobs->memory.DL1_num_PF = 1;
  knobs->memory.DL1_MSHR_cmd = (char*)"RWBP";

  knobs->commit.ROB_size = 64;
  knobs->commit.width = 4;
}

/* core post-initialization (core-based sim_post_init) */
void core_t::core_post_init(void)
{
  /* TBD - single-threaded */
  current_thread = (struct thread_t*)calloc(1,sizeof(thread_t));

  if(!current_thread)
    fatal("Zesto core thread failure\n");

  current_thread->id = current_thread->current_core = id;
  current_thread->idle_count = current_thread->insn_count = 0;

  /* regs_init */
  memset(&current_thread->regs,0,sizeof(current_thread->regs));

  current_thread->regs.regs_PC = BOOT_PC;
  current_thread->active = true;

  /* install signal handler for debug assistance */
  //signal(SIGSEGV,my_SIGSEGV_handler);

  /* create core pipeline */
  oracle = new core_oracle_t(this);
  commit = commit_create(knobs->model,this);
  exec = exec_create(knobs->model,this);
  alloc = alloc_create(knobs->model,this);
  decode = decode_create(knobs->model,this);
  fetch = fetch_create(knobs->model,this);
}

void core_t::core_reg_options(void)
{
  char filename[128];
  sprintf(filename,"core%d.out",id);

  fileout = fopen(filename,"w");
  if(!fileout)
    fileout = stderr;

  /* options database */
  core_odb = opt_new(orphan_fn);

  if(!core_odb)
    fatal("Zesto core stats database failure\n");

  /* core options headerline */
  char headerline[1024];
  sprintf(headerline,"#### Zesto Core#%d Configured Options ####",id);
  opt_reg_header(core_odb,headerline);

  /* random seed */
  opt_reg_int(core_odb, (char*)"-seed", (char*)"random number generator seed (0 for timer seed)",
    &sim.rand_seed, /* default */1, /* print */TRUE, NULL);

  /* scheduling priority */
  opt_reg_int(core_odb, (char*)"-nice", (char*)"simulator scheduling priority", 
    &sim.nice_priority, /* default */NICE_DEFAULT_VALUE, /* print */TRUE, NULL);

  /* ignored flag used to terminate list options */
  opt_reg_flag(core_odb, (char*)"-",(char*)"ignored flag",
    &sim.ignored_flag, /*default*/ false, /*print*/false,/*format*/NULL);

  /* fast forwarding */
  opt_reg_long_long(core_odb, (char*)"-fastfwd", (char*)"number of inst's to skip before timing simulation",
    &sim.fastfwd, /* default */0, /* print */true, /* format */NULL);

  /* trace limit */
  opt_reg_long_long(core_odb, (char*)"-tracelimit", (char*)"maximum number of instructions per trace",
    &sim.trace_limit, /* default */0, /* print */true, /* format */NULL);

  /* instruction limit */
  opt_reg_long_long(core_odb, (char*)"-max:inst", (char*)"maximum number of inst's to execute",
    &sim.max_insts, /* default */0, /* print */true, /* format */NULL);
  opt_reg_long_long(core_odb, (char*)"-max:uops", (char*)"maximum number of uop's to execute",
    &sim.max_uops, /* default */0, /* print */true, /* format */NULL);
  opt_reg_long_long(core_odb, (char*)"-max:cycles", (char*)"maximum number of cycles to execute",
    &sim.max_cycles, /* default */0, /* print */true, /* format */NULL);
  opt_reg_int(core_odb, (char*)"-heartbeat", (char*)"frequency for which to print out simulator heartbeat",
    &sim.heartbeat, /* default */0, /* print */true, /* format */NULL);
  opt_reg_string(core_odb, (char*)"-model",(char*)"pipeline model type",
    &knobs->model, /*default*/ (char*)"STM", /*print*/true,/*format*/NULL);
  opt_reg_double(core_odb,(char*)"-clock",(char*)"core clock frequency in MHz",
    &clock_frequency, /*default*/2000, /*print*/TRUE, NULL);

  fetch_reg_options(core_odb,knobs);
  decode_reg_options(core_odb,knobs);
  alloc_reg_options(core_odb,knobs);
  exec_reg_options(core_odb,knobs);
  commit_reg_options(core_odb,knobs);

  /* process options */
  opt_process_options(core_odb,3,sim.config);

  /* display options */
  opt_print_options(core_odb,fileout,TRUE,TRUE);

  fflush(fileout);
}

/* assign a new, unique id */
seq_t core_t::new_action_id(void)
{
  global_action_id++;
  return global_action_id;
}

/* Returns an array of uop structs; manages its own free pool by
   size.  The input to this function should be the Mop's uop flow
   length.  The implementation of this is slightly (very?) ugly; see
   the definition of struct uop_array_t.  We basically define a
   struct that contains a pointer for linking everything up in the
   free lists, but we don't actually want the outside world to know
   about these pointers, so we actually return a pointer that is !=
   to the original address returned by calloc.  If you're familiar
   with the original cache structures from the old SimpleScalar, we
   declare our uop_array_t similar to that. */
struct uop_t * core_t::get_uop_array(const int size)
{
  struct uop_array_t * p;

  if(uop_array_pool[size])
  {
    p = uop_array_pool[size];
    uop_array_pool[size] = p->next;
    p->next = NULL;
    assert(p->size == size);
  }
  else
  {
    //p = (struct uop_array_t*) calloc(1,sizeof(*p)+size*sizeof(struct uop_t));
    posix_memalign((void**)&p,16,sizeof(*p)+size*sizeof(struct uop_t)); // force all uops to be 16-byte aligned
    if(!p)
      fatal("couldn't calloc new uop array");
    p->size = size;
    p->next = NULL;
  }
  /* initialize the uop array */
  for(int i=0;i<size;i++)
    uop_init(&p->uop[i]);
  return p->uop;
}

void core_t::return_uop_array(struct uop_t * const p)
{
  struct uop_array_t * ap;
  byte_t * bp = (byte_t*)p;
  bp -= offsetof(struct uop_array_t,uop);
  ap = (struct uop_array_t *) bp;

  assert(ap->next == NULL);
  ap->next = uop_array_pool[ap->size];
  uop_array_pool[ap->size] = ap;
}

/* Alloc/dealloc of the linked-list container nodes */
struct odep_t * core_t::get_odep_link(void)
{
  struct odep_t * p = NULL;
  if(odep_free_pool)
  {
    p = odep_free_pool;
    odep_free_pool = p->next;
  }
  else
  {
    p = (struct odep_t*) calloc(1,sizeof(*p));
    if(!p)
      fatal("couldn't calloc an odep_t node");
  }
  assert(p);
  p->next = NULL;
  odep_free_pool_debt++;
  return p;
}

void core_t::return_odep_link(struct odep_t * const p)
{
  p->next = odep_free_pool;
  odep_free_pool = p;
  p->uop = NULL;
  odep_free_pool_debt--;
  /* p->next used for free list, will be cleared on "get" */
}

/* all sizes/loop lengths known at compile time; compiler
   should be able to optimize this pretty well.  Assumes
   uop is aligned to 16 bytes. */
void core_t::zero_uop(struct uop_t * const uop)
{
#if USE_SSE_MOVE
  char * addr = (char*) uop;
  int bytes = sizeof(*uop);
  int remainder = bytes - (bytes>>7)*128;

  /* zero xmm0 */
  asm ("xorps %%xmm0, %%xmm0"
       : : : "%xmm0");
  /* clear the uop 64 bytes at a time */
  for(int i=0;i<bytes>>7;i++)
  {
    asm ("movaps %%xmm0,    (%0)\n\t"
         "movaps %%xmm0,  16(%0)\n\t"
         "movaps %%xmm0,  32(%0)\n\t"
         "movaps %%xmm0,  48(%0)\n\t"
         "movaps %%xmm0,  64(%0)\n\t"
         "movaps %%xmm0,  80(%0)\n\t"
         "movaps %%xmm0,  96(%0)\n\t"
         "movaps %%xmm0, 112(%0)\n\t"
         : : "r"(addr) : "memory");
    addr += 128;
  }

  /* handle any remaining bytes; optimizer should remove this
     when sizeof(uop) has no remainder */
  for(int i=0;i<remainder>>3;i++)
  {
    asm ("movlps %%xmm0,   (%0)\n\t"
         : : "r"(addr) : "memory");
    addr += 8;
  }
#else
  memset(uop,0,sizeof(*uop));
#endif
}

void core_t::zero_Mop(struct Mop_t * const Mop)
{
#if USE_SSE_MOVE
  char * addr = (char*) Mop;
  int bytes = sizeof(*Mop);
  int remainder = bytes - (bytes>>6)*64;

  /* zero xmm0 */
  asm ("xorps %%xmm0, %%xmm0"
       : : : "%xmm0");
  /* clear the uop 64 bytes at a time */
  for(int i=0;i<bytes>>6;i++)
  {
    asm ("movaps %%xmm0,   (%0)\n\t"
         "movaps %%xmm0, 16(%0)\n\t"
         "movaps %%xmm0, 32(%0)\n\t"
         "movaps %%xmm0, 48(%0)\n\t"
         : : "r"(addr) : "memory");
    addr += 64;
  }

  /* handle any remaining bytes */
  for(int i=0;i<remainder>>3;i++)
  {
    asm ("movlps %%xmm0,   (%0)\n\t"
         : : "r"(addr) : "memory");
    addr += 8;
  }
#else
  memset(Mop,0,sizeof(*Mop));
#endif
}


/* Initialize a uop struct */
void core_t::uop_init(struct uop_t * const uop)
{
  int i;
  zero_uop(uop);
  memset(&uop->alloc,-1,sizeof(uop->alloc));
  uop->core = this;
  uop->decode.Mop_seq = (seq_t)-1;
  uop->decode.uop_seq = (seq_t)-1;
  uop->alloc.port_assignment = -1;

  uop->timing.when_decoded = TICK_T_MAX;
  uop->timing.when_allocated = TICK_T_MAX;
  for(i=0;i<MAX_IDEPS;i++)
  {
    uop->timing.when_itag_ready[i] = TICK_T_MAX;
    uop->timing.when_ival_ready[i] = TICK_T_MAX;
  }
  uop->timing.when_otag_ready = TICK_T_MAX;
  uop->timing.when_ready = TICK_T_MAX;
  uop->timing.when_issued = TICK_T_MAX;
  uop->timing.when_exec = TICK_T_MAX;
  uop->timing.when_completed = TICK_T_MAX;

  uop->exec.action_id = new_action_id();
  uop->exec.when_data_loaded = TICK_T_MAX;
  uop->exec.when_addr_translated = TICK_T_MAX;
}


void core_t::emergency_recovery(void)
{
  struct Mop_t Mop_v;
  struct Mop_t * Mop = & Mop_v;

  unsigned int insn_count = stat.eio_commit_insn;

  if((stat.eio_commit_insn == last_emergency_recovery_count)&&(num_emergency_recoveries > 0))
    fatal("After previous attempted recovery, thread %d is still getting wedged... giving up.",current_thread->id);

  memset(Mop,0,sizeof(*Mop));
  fprintf(stderr, "### Emergency recovery for thread %d, resetting to inst-count: %lld\n", current_thread->id,stat.eio_commit_insn);

  /* reset core state */
  stat.eio_commit_insn = 0;
  oracle->complete_flush();
  commit->recover();
  exec->recover();
  alloc->recover();
  decode->recover();
  fetch->recover(/*new PC*/0);

  md_addr_t nextPC;
  fetch_next_pc(&nextPC, this);
  

  current_thread->regs.regs_PC=nextPC;
  current_thread->regs.regs_NPC=nextPC;
  fetch->PC = current_thread->regs.regs_PC;
  fetch->bogus = false;
  num_emergency_recoveries++;
  last_emergency_recovery_count = stat.eio_commit_insn;

  /* reset stats */
  fetch->reset_bpred_stats();

  stat.eio_commit_insn = insn_count;

  oracle->hosed = false;
}

void core_t::print_stats(void)
{
  /* get stats time */
  stat.sim_end_time = time((time_t *)NULL);
  stat.sim_elapsed_time = MAX(stat.sim_end_time - stat.sim_start_time, 1);

  /* print simulation stats */
  stat_print_stats(core_sdb, fileout);

  fprintf(fileout, "#### End of Stats ####\n");

  fclose(fileout);
}

bool core_t::tick(tick_t Cycle)
{
    sim_cycle = Cycle;

    if(current_thread->active)
    {
      stat.final_sim_cycle = sim_cycle;

      commit->step();
      exec->LDST_exec();
      exec->memory_callbacks();
      exec->ALU_exec();
      exec->LDQ_schedule();
      exec->RS_schedule();
      alloc->step();
      decode->step();
      fetch->step();

      oracle->update_occupancy();
      fetch->update_occupancy();
      decode->update_occupancy();
      exec->update_occupancy();
      commit->update_occupancy();

      if(oracle->hosed)
        emergency_recovery();

  #ifdef ZESTO_COUNTERS
  tick_t sampling_cycle = (tick_t)(sampling_period*(clock_frequency*1e6));

  if((sim_cycle > 0)&&(sim_cycle%sampling_cycle == 0))
  {
    char ModuleID[64];
    double power = 0;

    counters->time_tick = (double)sim_cycle/(clock_frequency*1e6); // update time_tick
    counters->period = (double)sampling_cycle/(clock_frequency*1e6);

    // branch prediction (fetch)
    sprintf(ModuleID,"core%d:fusion",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->fusion);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:bpred1",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->bpreds);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:bpred2",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->bpreds);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:RAS",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->RAS);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:dirjmpBTB",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->dirjmpBTB);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:indirjmpBTB",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->indirjmpBTB);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // fetch
    sprintf(ModuleID,"core%d:byteQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->byteQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:instQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->instQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // decode
/*
    sprintf(ModuleID,"core%d:predecoder",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->pre_decoder);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
*/
    sprintf(ModuleID,"core%d:instruction_decoder",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->decoder.instruction_decoder);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:operand_decoder",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->decoder.operand_decoder);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:uop_sequencer",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->decoder.uop_sequencer);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:uopQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->uopQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // renaming (alloc)
    sprintf(ModuleID,"core%d:freelist",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->freelist);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:RAT",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->RAT);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:operand_dependency",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->operand_dependency);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // memory
    sprintf(ModuleID,"core%d:loadQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->loadQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:storeQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->storeQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:memory_dependency",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->memory_dependency);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // exec
    sprintf(ModuleID,"core%d:issue_select",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->issue_select);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:RS",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->RS);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:payload",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->payload);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    for(int i = 0; i < knobs->exec.num_exec_ports; i++)
    {
      sprintf(ModuleID,"core%d:port%d:FU",id,i);
      energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->FU[i]);
      power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    }

    // commit
    sprintf(ModuleID,"core%d:ROB",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->ROB);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:commit_select",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->commit_select);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // bypass 
    sprintf(ModuleID,"core%d:exec_bypass",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->exec_bypass);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:load_bypass",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->load_bypass);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // architectural registers
    sprintf(ModuleID,"core%d:GPREG",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->registers.integer);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:FPREG",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->registers.floating);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:SEGREG",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->registers.segment);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:CREG",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->registers.control);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:FLAGREG",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->registers.flag);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    // pipeline latches
    sprintf(ModuleID,"core%d:program_counter",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.PC);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:BQ2PD",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.BQ2PD);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:PD2IQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.PD2IQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:IQ2ID",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.IQ2ID);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;    
    sprintf(ModuleID,"core%d:ID2uQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.ID2uQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:uQ2RR",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.uQ2RR);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:RR2RS",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.RR2RS);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:ARF2RS",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.ARF2RS);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:ROB2RS",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.ROB2RS);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:RS2PR",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.RS2PR);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:PR2FU",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.PR2FU);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:FU2ROB",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.FU2ROB);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:ROB2CM",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.ROB2CM);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:LQ2ROB",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.LQ2ROB);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:SQ2LQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.SQ2LQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:LQ2L1",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.LQ2L1);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:L12LQ",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.L12LQ);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;
    sprintf(ModuleID,"core%d:SQ2L1",id);
    energy_introspector->compute_power(counters->time_tick,counters->period,string(ModuleID),counters->latch.SQ2L1);
    power += energy_introspector->pull_data<power_t>(counters->time_tick,"module",string(ModuleID),"power").total;

    cout << "core power = " << power << "W" << endl;

    counters->reset();
  }
  #endif
  }
  return false;
}


void core_t::core_reg_stats(void)
{
//  unsigned int i;
//  char buf[4096];
//  char buf2[4096];

  bool is_DPM = strcasecmp(knobs->model,"DPM") == 0;

  if(!is_DPM&&strcasecmp(knobs->model,"ATOM"))
    fatal("unknown core model %s",knobs->model);

  core_sdb = stat_new();

  oracle->reg_stats(core_sdb);
  fetch->reg_stats(core_sdb);
  decode->reg_stats(core_sdb);
  alloc->reg_stats(core_sdb);
  exec->reg_stats(core_sdb);
  commit->reg_stats(core_sdb);
}


void core_t::cache_response_handler(SST::Event *ev)
{
  //printf("received by zesto\n");
  cache_req *req = dynamic_cast<cache_req *>(ev); 
  m_input_from_cache.push_back(req);
}


