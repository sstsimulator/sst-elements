/* commit-atom.cpp - Detailed Pipeline Model */
/*
 * __COPYRIGHT__ GT
 */

#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(commit_opt_string,"ATOM"))
    return new core_commit_atom_t(core);
#else

class core_commit_atom_t:public core_commit_t
{
  enum commit_stall_t {CSTALL_NONE,      /* no stall */
                       CSTALL_NOT_READY, /* oldest inst not done (no uops finished) */
                       CSTALL_PARTIAL,   /* oldest inst not done (but some uops finished) */
                       CSTALL_EMPTY,     /* ROB is empty, nothing to commit */
                       CSTALL_JECLEAR_INFLIGHT, /* Mop is done, but its jeclear hasn't been handled yet */
                       CSTALL_MAX_BRANCHES, /* exceeded maximum number of branches committed per cycle */
                       CSTALL_num
                     };

  public:

  core_commit_atom_t(struct core_t * const core);
  virtual void reg_stats(struct stat_sdb_t * const sdb);
  virtual void update_occupancy(void);

  virtual void step(void);
  virtual void recover(const struct Mop_t * const Mop);
  virtual void recover(void);

  virtual bool ROB_available(void);
  virtual bool ROB_empty(void);
  virtual void ROB_insert(struct uop_t * const uop);
  virtual void ROB_fuse_insert(struct uop_t * const uop);

  protected:

  struct uop_t ** ROB; /*For inorder cores ROBs are just placeholders. They do not contribute as a microarchitectural structure*/
  int ROB_head;
  int ROB_tail;
  int ROB_num;
  int ROB_eff_num;
  FILE* load_stall_trace;
  bool load_stall_on; /* signifies that a head of ROB load stall is in effect right now */
  unsigned long long int stall_start_time; /* Time at which last load stall started */

  static const char *commit_stall_str[CSTALL_num];

  /* additional temps to track timing of REP insts */
  tick_t when_rep_fetch_started;
  tick_t when_rep_fetched;
  tick_t when_rep_decode_started;
  tick_t when_rep_commit_started;
};

/* number of buckets in uop-flow-length histogram */
#define FLOW_HISTO_SIZE 9

/* VARIABLES/TYPES */

const char *core_commit_atom_t::commit_stall_str[CSTALL_num] = {
  "no stall                   ",
  "oldest inst not done       ",
  "oldest inst partially done ",
  "ROB is empty               ",
  "Mop done, jeclear in flight",
  "branch commit limit        "
};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_commit_atom_t::core_commit_atom_t(struct core_t * const arg_core):
  ROB_head(0), ROB_tail(0), ROB_num(0), ROB_eff_num(0),
  when_rep_fetch_started(0), when_rep_fetched(0),
  when_rep_decode_started(0), when_rep_commit_started(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;
  ROB = (struct uop_t**) calloc(knobs->commit.ROB_size,sizeof(*ROB));
  if(!ROB)
    fatal("couldn't calloc ROB");
}

void
core_commit_atom_t::reg_stats(struct stat_sdb_t * const sdb)
{
  char buf[1024];
  char buf2[1024];

  sprintf(buf,"\n#### Zesto Core#%d Commit Stats ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.commit_bytes",core->id);
  stat_reg_counter(sdb, true, buf, "total number of bytes committed", &core->stat.commit_bytes, core->stat.commit_bytes, NULL);
  sprintf(buf,"c%d.commit_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions committed", &core->stat.commit_insn, core->stat.commit_insn, NULL);
  sprintf(buf,"c%d.commit_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of uops committed", &core->stat.commit_uops, core->stat.commit_uops, NULL);
  sprintf(buf,"c%d.commit_eff_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of effective uops committed", &core->stat.commit_eff_uops, core->stat.commit_eff_uops, NULL);
  sprintf(buf,"c%d.commit_BPC",core->id);
  sprintf(buf2,"c%d.commit_bytes/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "BPC (bytes per cycle) at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_IPC",core->id);
  sprintf(buf2,"c%d.commit_insn/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "IPC at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_uPC",core->id);
  sprintf(buf2,"c%d.commit_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uPC at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_euPC",core->id);
  sprintf(buf2,"c%d.commit_eff_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "effective uPC at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_byte_per_inst",core->id);
  sprintf(buf2,"c%d.commit_bytes/c%d.commit_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average bytes per instruction", buf2, NULL);
  sprintf(buf,"c%d.commit_byte_per_uop",core->id);
  sprintf(buf2,"c%d.commit_bytes/c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average bytes per uop", buf2, NULL);
  sprintf(buf,"c%d.commit_byte_per_eff_uop",core->id);
  sprintf(buf2,"c%d.commit_bytes/c%d.commit_eff_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average bytes per effective uop", buf2, NULL);
  sprintf(buf,"c%d.avg_commit_flowlen",core->id);
  sprintf(buf2,"c%d.commit_uops/c%d.commit_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uops per instruction at commit", buf2, NULL);
  sprintf(buf,"c%d.avg_commit_eff_flowlen",core->id);
  sprintf(buf2,"c%d.commit_eff_uops/c%d.commit_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "effective uops per instruction at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_fusions",core->id);
  stat_reg_counter(sdb, true, buf, "total number of fused uops committed", &core->stat.commit_fusions, core->stat.commit_fusions, NULL);
  sprintf(buf,"c%d.commit_fusion_uops",core->id);
  sprintf(buf2,"(c%d.commit_eff_uops - c%d.commit_uops) + c%d.commit_fusions",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fused uops at commit", buf2, "%12.0f");
  sprintf(buf,"c%d.commit_frac_fusion_uops",core->id);
  sprintf(buf2,"c%d.commit_fusion_uops / c%d.commit_eff_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of effective uops fused at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_fusion_compression",core->id);
  sprintf(buf2,"(c%d.commit_eff_uops - c%d.commit_uops) / c%d.commit_eff_uops",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of effective uops compressed via fusion at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_fusion_expansion",core->id);
  sprintf(buf2,"c%d.commit_eff_uops / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average number of effective uops per uop (fused or standalone) commit", buf2, NULL);

  sprintf(buf,"c%d.commit_dead_lock_flushes",core->id);
  stat_reg_counter(sdb, true, buf, "total number of pipe-flushes due to dead-locked pipeline", &core->stat.commit_deadlock_flushes, core->stat.commit_deadlock_flushes, NULL);
  sprintf(buf,"c%d.ROB_empty",core->id);
  sprintf(buf,"c%d.ROB_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total ROB occupancy", &core->stat.ROB_occupancy, core->stat.ROB_occupancy, NULL);
  sprintf(buf,"c%d.ROB_eff_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total ROB effective_occupancy", &core->stat.ROB_eff_occupancy, core->stat.ROB_eff_occupancy, NULL);
  sprintf(buf,"c%d.ROB_empty",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles ROB was empty", &core->stat.ROB_empty_cycles, core->stat.ROB_empty_cycles, NULL);
  sprintf(buf,"c%d.ROB_full",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles ROB was full", &core->stat.ROB_full_cycles, core->stat.ROB_full_cycles, NULL);
  sprintf(buf,"c%d.ROB_avg",core->id);
  sprintf(buf2,"c%d.ROB_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average ROB occupancy", buf2, NULL);
  sprintf(buf,"c%d.ROB_eff_avg",core->id);
  sprintf(buf2,"c%d.ROB_eff_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average ROB effective occupancy", buf2, NULL);
  sprintf(buf,"c%d.ROB_frac_empty",core->id);
  sprintf(buf2,"c%d.ROB_empty/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles ROB was empty", buf2, NULL);
  sprintf(buf,"c%d.ROB_frac_full",core->id);
  sprintf(buf2,"c%d.ROB_full/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles ROB was full", buf2, NULL);
  sprintf(buf,"c%d.DL1_store_split_accesses",core->id);
  stat_reg_counter(sdb, true, buf, "number of stores requiring split accesses", &core->stat.DL1_store_split_accesses, core->stat.DL1_store_split_accesses, NULL);
  sprintf(buf,"c%d.DL1_store_split_frac",core->id);
  sprintf(buf2,"c%d.DL1_store_split_accesses/(c%d.DL1.store_lookups-c%d.DL1_store_split_accesses)",core->id,core->id,core->id); /* need to subtract since each split access generated two store accesses */
  stat_reg_formula(sdb, true, buf, "fraction of stores requiring split accesses", buf2, NULL);

  sprintf(buf,"c%d.commit_load_stalls",core->id);
  stat_reg_counter(sdb, true, buf, "stalls due to long latency loads", &core->stat.commit_load_stalls, core->stat.commit_load_stalls, NULL);

  sprintf(buf,"c%d.commit_stall",core->current_thread->id);
  core->stat.commit_stall = stat_reg_dist(sdb, buf,
                                           "breakdown of stalls at commit",
                                           /* initial value */0,
                                           /* array size */CSTALL_num,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */commit_stall_str,
                                           /* print fn */NULL);

  sprintf(buf,"\n#### Zesto Core#%d Timing Stats ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.sim_cycle",core->id);
  stat_reg_qword(sdb, true, buf, "total number of cycles when last instruction (or uop) committed", (qword_t*) &core->stat.final_sim_cycle, core->stat.final_sim_cycle, NULL);
  /* cumulative slip cycles (not printed) */
  sprintf(buf,"c%d.Mop_fetch_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total Mop fetch slip cycles", (qword_t*) &core->stat.Mop_fetch_slip, 0, NULL);
  sprintf(buf,"c%d.Mop_f2d_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total Mop fetch-to-decode slip cycles", (qword_t*) &core->stat.Mop_fetch2decode_slip, 0, NULL);
  sprintf(buf,"c%d.Mop_decode_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total Mop decode slip cycles", (qword_t*) &core->stat.Mop_decode_slip, 0, NULL);
  sprintf(buf,"c%d.uop_d2a_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop decode-to-alloc slip cycles", (qword_t*) &core->stat.uop_decode2alloc_slip, 0, NULL);
  sprintf(buf,"c%d.uop_a2r_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop alloc-to-ready slip cycles", (qword_t*) &core->stat.uop_alloc2ready_slip, 0, NULL);
  sprintf(buf,"c%d.uop_r2i_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop ready-to-issue slip cycles", (qword_t*) &core->stat.uop_ready2issue_slip, 0, NULL);
  sprintf(buf,"c%d.uop_i2e_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop issue-to-exec slip cycles", (qword_t*) &core->stat.uop_issue2exec_slip, 0, NULL);
  sprintf(buf,"c%d.uop_e2w_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop exec-to-WB slip cycles", (qword_t*) &core->stat.uop_exec2complete_slip, 0, NULL);
  sprintf(buf,"c%d.uop_w2c_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total uop WB-to-commit slip cycles", (qword_t*) &core->stat.uop_complete2commit_slip, 0, NULL);
  sprintf(buf,"c%d.Mop_d2c_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total Mop decode-to-commit slip cycles", (qword_t*) &core->stat.Mop_decode2commit_slip, 0, NULL);
  sprintf(buf,"c%d.Mop_commit_Tslip",core->current_thread->id);
  stat_reg_qword(sdb, false, buf, "total Mop commit slip cycles", (qword_t*) &core->stat.Mop_commit_slip, 0, NULL);
  /* average slip cycles */
  sprintf(buf,"c%d.Mop_fetch_avg_slip",core->id);
  sprintf(buf2,"c%d.Mop_fetch_Tslip / (c%d.commit_insn - c%d.num_traps)",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop fetch average delay", buf2, NULL);
  sprintf(buf,"c%d.Mop_f2d_avg_slip",core->id);
  sprintf(buf2,"c%d.Mop_f2d_Tslip / (c%d.commit_insn - c%d.num_traps)",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop fetch-to-decode average delay", buf2, NULL);
  sprintf(buf,"c%d.Mop_decode_avg_slip",core->id);
  sprintf(buf2,"c%d.Mop_decode_Tslip / (c%d.commit_insn - c%d.num_traps)",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop decode average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_d2a_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_d2a_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop decode-to-alloc average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_a2r_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_a2r_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop alloc-to-ready average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_r2i_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_r2i_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop ready-to-issue average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_i2e_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_i2e_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop issue-to-exec average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_e2w_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_e2w_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop exec-to-WB average delay", buf2, NULL);
  sprintf(buf,"c%d.uop_w2c_avg_slip",core->id);
  sprintf(buf2,"c%d.uop_w2c_Tslip / c%d.commit_uops",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uop WB-to-commit average delay", buf2, NULL);
  sprintf(buf,"c%d.Mop_d2c_avg_slip",core->id);
  sprintf(buf2,"c%d.Mop_d2c_Tslip / (c%d.commit_insn - c%d.num_traps)",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop decode-to-commit average delay", buf2, NULL);
  sprintf(buf,"c%d.Mop_commit_avg_slip",core->id);
  sprintf(buf2,"c%d.Mop_commit_Tslip / (c%d.commit_insn - c%d.num_traps)",core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop commit average delay", buf2, NULL);
  sprintf(buf,"c%d.Mop_avg_end_to_end",core->id);
  sprintf(buf2,"c%d.Mop_fetch_avg_slip + c%d.Mop_f2d_avg_slip + c%d.Mop_d2c_avg_slip + c%d.Mop_commit_avg_slip",core->id,core->id,core->id,core->id);
  stat_reg_formula(sdb, true, buf, "Mop average end-to-end pipeline delay", buf2, NULL);

  /* instruction distribution stats */
  sprintf(buf,"\n#### Zesto Core#%d Instruction Stats (no wrong-path) ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.num_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions committed", &core->stat.commit_insn, core->stat.commit_insn, NULL);
  sprintf(buf,"c%d.num_refs",core->id);
  stat_reg_counter(sdb, true, buf, "total number of loads and stores committed", &core->stat.commit_refs, core->stat.commit_refs, NULL);
  sprintf(buf,"c%d.num_loads",core->id);
  stat_reg_counter(sdb, true, buf, "total number of loads committed", &core->stat.commit_loads, core->stat.commit_loads, NULL);
  sprintf(buf2,"c%d.num_refs - c%d.num_loads",core->id,core->id);
  sprintf(buf,"c%d.num_stores",core->id);
  stat_reg_formula(sdb, true, buf, "total number of stores committed", buf2, "%12.0f");
  sprintf(buf,"c%d.num_branches",core->id);
  stat_reg_counter(sdb, true, buf, "total number of branches committed", &core->stat.commit_branches, core->stat.commit_branches, NULL);
  sprintf(buf,"c%d.num_traps",core->id);
  stat_reg_counter(sdb, true, buf, "total number of traps committed", &core->stat.commit_traps, core->stat.commit_traps, NULL);
  sprintf(buf,"c%d.num_rep_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of REP insts committed", &core->stat.commit_rep_insn, core->stat.commit_rep_insn, NULL);
  sprintf(buf,"c%d.num_rep_iter",core->id);
  stat_reg_counter(sdb, true, buf, "total number of REP iterations committed", &core->stat.commit_rep_iterations, core->stat.commit_rep_iterations, NULL);
  sprintf(buf,"c%d.num_rep_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of uops in REP insts committed", &core->stat.commit_rep_uops, core->stat.commit_rep_uops, NULL);
  sprintf(buf,"c%d.num_avg_reps",core->id);
  sprintf(buf2,"c%d.num_rep_iter/c%d.num_rep_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average iterations per REP inst", buf2, "%12.2f");
  sprintf(buf,"c%d.num_avg_rep_uops",core->id);
  sprintf(buf2,"c%d.num_rep_uops/c%d.num_rep_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average uops per REP inst", buf2, "%12.2f");
  sprintf(buf,"c%d.num_UROM_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of insn using the UROM committed", &core->stat.commit_UROM_insn, core->stat.commit_UROM_insn, NULL);
  sprintf(buf,"c%d.num_UROM_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of uops using the UROM committed", &core->stat.commit_UROM_uops, core->stat.commit_UROM_uops, NULL);
  sprintf(buf,"c%d.num_UROM_eff_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of effective uops using the UROM committed", &core->stat.commit_UROM_eff_uops, core->stat.commit_UROM_eff_uops, NULL);
  sprintf(buf,"c%d.num_avg_UROM_uops",core->id);
  sprintf(buf2,"c%d.num_UROM_uops/c%d.num_UROM_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average uops per UROM inst", buf2, "%12.2f");
  sprintf(buf,"c%d.avg_flowlen",core->id);
  sprintf(buf2,"c%d.commit_uops/c%d.commit_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average uops per instruction", buf2, NULL);
  sprintf(buf,"c%d.avg_eff_flowlen",core->id);
  sprintf(buf2,"c%d.commit_eff_uops/c%d.commit_insn",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average effective uops per instruction", buf2, NULL);
  sprintf(buf,"c%d.flow_lengths",core->current_thread->id);
  core->stat.flow_histo = stat_reg_dist(sdb, buf,
                                           "histogram of uop flow lengths",
                                           /* initial value */0,
                                           /* array size */FLOW_HISTO_SIZE,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */NULL,
                                           /* print fn */NULL);
  sprintf(buf,"c%d.eff_flow_lengths",core->current_thread->id);
  core->stat.eff_flow_histo = stat_reg_dist(sdb, buf,
                                           "histogram of effective uop flow lengths",
                                           /* initial value */0,
                                           /* array size */FLOW_HISTO_SIZE,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */NULL,
                                           /* print fn */NULL);
}

void core_commit_atom_t::update_occupancy(void)
{
    /* ROB */
  core->stat.ROB_occupancy += ROB_num;
  core->stat.ROB_eff_occupancy += ROB_eff_num;
  if(ROB_num >= core->knobs->commit.ROB_size)
    core->stat.ROB_full_cycles++;
  if(ROB_num <= 0)
    core->stat.ROB_empty_cycles++;
}

/*************************/
/* MAIN COMMIT FUNCTIONS */
/*************************/

/* In-order instruction commit.  Individual uops cannot commit
   until it is guaranteed that the entire Mop's worth of uops will
   commit. */
void core_commit_atom_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int commit_count = 0;
  enum commit_stall_t stall_reason = CSTALL_NONE;
  int branches_committed = 0;

  /* This is just a deadlock watchdog.  If something got messed up
     in the pipeline and no forward progress is being made, this
     code will eventually detect it and flush the pipeline in an
     attempt to un-wedge the processor.  If the processor then
     deadlocks again without having first made any more forward
     progress, we give up and kill the simulator. */
  if((core->sim_cycle - core->exec->last_completed) > deadlock_threshold)
  {
    if(core->exec->last_completed_count == core->stat.eio_commit_insn)
    {  
      char buf[256];
      fflush(stdout);
      snprintf(buf,sizeof(buf),"At cycle %lld, core[%d] has not completed a uop in %d cycles... definite deadlock",(long long)core->sim_cycle,core->current_thread->id,deadlock_threshold);
#ifdef ZTRACE
      ztrace_print("DEADLOCK DETECTED: TERMINATING SIMULATION");
#endif
      zesto_fatal(buf,(void)0);
    }
    else
    {
      struct Mop_t * Mop = ROB[ROB_head]->Mop;
#ifdef ZDEBUG
      fprintf(stdout,"\n[%d][Core%d]COMMIT deadlock ",core->sim_cycle,core->current_thread->id,core->sim_cycle);
      md_print_insn(Mop,stdout);
      fprintf(stdout," MOP=%d last_Mop_seq= %d \n",Mop->oracle.seq,core->exec->last_Mop_seq);
#endif
      fflush(stdout);
      warn("At cycle %lld, core[%d] has not completed a uop in %d cycles... possible deadlock, flushing pipeline",(long long)core->sim_cycle,core->current_thread->id,deadlock_threshold);
#ifdef ZTRACE
      ztrace_print("DEADLOCK DETECTED: FLUSHING PIPELINE");
#endif
      

      /* flush the entire pipeline, correct path or not... */
      core->oracle->complete_flush();
      /*core->commit->*/recover();
      core->exec->recover();
      core->alloc->recover();
      core->decode->recover();
      core->fetch->recover(core->current_thread->regs.regs_NPC);
      ZESTO_STAT(stat_add_sample(core->stat.commit_stall, (int)CSTALL_EMPTY);)
      ZESTO_STAT(core->stat.commit_deadlock_flushes++;)
      core->exec->last_completed = core->sim_cycle; /* so we don't do this again next cycle */
      core->exec->last_completed_count = core->stat.eio_commit_insn;
    }
    return;
  }

  /* deallocate at most one store from the STQ store buffer per cycle The terminology senior is still the same because the out of order core */
  core->exec->STQ_deallocate_senior();

  /* MAIN COMMIT LOOP */
  for(commit_count=0;commit_count<knobs->commit.width;commit_count++)
  {
    if(ROB_num <= 0) /* nothing to commit */
    {
      stall_reason = CSTALL_EMPTY;
      break;
    }

    /* This is not a real ROB its just a place keeper for the simulation to keep track of Mop sequences */
    struct Mop_t * Mop = ROB[ROB_head]->Mop;

    /* For branches, don't commit until the corresponding jeclear
       (if any) has been processed by the front-end. */
    if(Mop->commit.jeclear_in_flight)
    {
      stall_reason = CSTALL_JECLEAR_INFLIGHT;
      break;
    }
    
    if(Mop->decode.is_ctrl && knobs->commit.branch_limit && (branches_committed >= knobs->commit.branch_limit))
    {
      stall_reason = CSTALL_MAX_BRANCHES;
      break;
    }

    if(Mop->oracle.spec_mode)
    {
      fprintf(stdout,"\n[%lld][Core%d]",core->sim_cycle,core->id );
      md_print_insn(Mop,stdout);
      fprintf(stdout,"  %lld \n\n",core->sim_cycle);
      fflush(stdout);
      fflush(stderr);
      zesto_fatal("oldest instruction in processor is on wrong-path",(void)0);
    }
  
    /* Are all uops in the Mop completed? */
    if(Mop->commit.complete_index != -1) /* still some outstanding insts */
    {
      while(Mop->uop[Mop->commit.complete_index].exec.commit_done)
      {
        struct uop_t * uop = &Mop->uop[Mop->commit.complete_index];

        Mop->commit.complete_index += uop->decode.has_imm ? 3 : 1;
        if(Mop->commit.complete_index >= Mop->decode.flow_length)
        {
          Mop->commit.complete_index = -1; /* Mark this Mop as all done */
#ifdef ZTRACE
          ztrace_print(Mop,"c|complete|all uops completed execution");
#endif
          if(Mop->fetch.bpred_update)
          {
            core->fetch->bpred->update(Mop->fetch.bpred_update,Mop->decode.opflags,
            Mop->fetch.PC, Mop->decode.targetPC, Mop->oracle.NextPC, (Mop->oracle.NextPC != (Mop->fetch.PC + Mop->fetch.inst.len)));
            core->fetch->bpred->return_state_cache(Mop->fetch.bpred_update);
            Mop->fetch.bpred_update = NULL;
          }
          break;
        }
      }
    }

    if(Mop->commit.complete_index == -1) /* commit the uops if the Mop is done */
    {
      struct uop_t * uop = ROB[ROB_head];
      zesto_assert(uop->timing.when_completed <= core->sim_cycle,(void)0);
#ifdef ZDEBUG
      if(core->sim_cycle > PRINT_CYCLE )
        fprintf(stdout,"\n[%lld][Core%d]COMMIT MOP=%d UOP=%d at ROB entry %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,ROB_head,uop->decode.in_fusion);
#endif
      zesto_assert(uop == &Mop->uop[Mop->commit.commit_index],(void)0);

      if(uop->decode.BOM && (uop->Mop->timing.when_commit_started == TICK_T_MAX))
        uop->Mop->timing.when_commit_started = core->sim_cycle;

      /* any remaining transactions in-flight (only for loads)
         should now be ignored - such load requests may exist, for
         example as a result of a load that completes early due to
         a hit in the STQ while the cache request is still making
         its way through the memory hierarchy. */
      if(uop->decode.is_load)
        uop->exec.action_id = core->new_action_id();

#ifdef ZTRACE
      ztrace_print(uop,"c|commit|uop committed");
#endif

      uop->alloc.ROB_index = -1;

      if(!uop->Mop->decode.is_trap)/*TODO Fix these stats for fused uops*/
      {
        ZESTO_STAT(core->stat.uop_decode2alloc_slip += uop->timing.when_allocated - uop->timing.when_decoded;)
        ZESTO_STAT(core->stat.uop_alloc2ready_slip += uop->timing.when_ready - uop->timing.when_allocated;)
        ZESTO_STAT(core->stat.uop_ready2issue_slip += uop->timing.when_issued - uop->timing.when_ready;)
        ZESTO_STAT(core->stat.uop_issue2exec_slip += uop->timing.when_exec - uop->timing.when_issued;)
        ZESTO_STAT(core->stat.uop_exec2complete_slip += uop->timing.when_completed - uop->timing.when_exec;)
        ZESTO_STAT(core->stat.uop_complete2commit_slip += core->sim_cycle - uop->timing.when_completed;)
        zesto_assert(uop->timing.when_exec != TICK_T_MAX,(void)0);
      }

      /* remove uop from ROB */
      if((!uop->decode.in_fusion) || (uop->decode.fusion_next == NULL)) /* fusion dealloc's on fusion-tail */
      {
        #ifdef ZESTO_COUNTERS
        core->counters->ROB.read++;
        core->counters->commit_select.read++;
        core->counters->latch.ROB2CM.read++;
        #endif

        ROB[ROB_head] = NULL;
        ROB_num --;
        ROB_eff_num --;
        ROB_head = modinc(ROB_head,knobs->commit.ROB_size); //(ROB_head+1) % knobs->commit.ROB_size;
        if(uop->decode.EOM)
          uop->Mop->timing.when_commit_finished = core->sim_cycle;
#ifdef ZDEBUG
        if(core->sim_cycle > PRINT_CYCLE )
          fprintf(stdout,"\n[%lld][Core%d]COMMIT MOP=%d UOP=%d uop_commit_index %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,Mop->commit.commit_index,uop->decode.in_fusion);
#endif

        /* this cleans up idep/odep ptrs, register mappings, and
           commit stores to the real (non-spec) memory system */
        core->oracle->commit_uop(uop);

        /* mark uop as committed in Mop */
        Mop->commit.commit_index += uop->decode.has_imm ? 3 : 1;
      }	
      else /* fusion body doesn't count toward commit width */
      {
        if(uop->decode.in_fusion)
        {
          ZESTO_STAT(core->stat.commit_fusions++;)
        }
        commit_count--;
        /* XXX: For INORDER CORE all fused uops execute with the fusion head. */
        if(uop->decode.is_fusion_head)
        {
          struct uop_t * fusion_uop = uop->decode.fusion_head;
          zesto_assert(fusion_uop == uop, (void)0);
          while(fusion_uop)
          {
#ifdef ZDEBUG
            if(core->sim_cycle > PRINT_CYCLE )
              fprintf(stdout,"\n[%lld][Core%d]COMMIT MOP=%d UOP=%d uop_commit_index %d in_fusion %d ",core->sim_cycle,core->id,fusion_uop->decode.Mop_seq,fusion_uop->decode.uop_seq,Mop->commit.commit_index,fusion_uop->decode.in_fusion);
#endif
            if(fusion_uop->decode.EOM)
              fusion_uop->Mop->timing.when_commit_finished = core->sim_cycle;
            core->oracle->commit_uop(fusion_uop);
            Mop->commit.commit_index += fusion_uop->decode.has_imm ? 3 : 1;
            fusion_uop = fusion_uop->decode.fusion_next;
            ROB_eff_num --;
          }			
        }

        ROB[ROB_head] = NULL;
        ROB_num --;
        ROB_head = modinc(ROB_head,knobs->commit.ROB_size); //(ROB_head+1) % knobs->commit.ROB_size;
        zesto_assert(ROB_eff_num >= 0,(void)0);
      }	
	
      if(Mop->commit.commit_index >= Mop->decode.flow_length) //TODO FIX the stat engines for fused uops
      {
        Mop->commit.commit_index = -1; /* The entire Mop has been committed */

        /* Update stats */
        if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
        {
          core->stat.eio_commit_insn++;
          ZESTO_STAT(core->stat.commit_insn++;)
          ZESTO_STAT(core->stat.commit_bytes += Mop->fetch.inst.len;) /* REP counts as only 1 fetch */
        }
	
        if(Mop->decode.is_ctrl)
          branches_committed++;
	
        ZESTO_STAT(core->stat.commit_uops += Mop->stat.num_uops;)
        ZESTO_STAT(core->stat.commit_eff_uops += Mop->stat.num_eff_uops;)
        ZESTO_STAT(core->stat.commit_branches += Mop->stat.num_branches;)
        ZESTO_STAT(core->stat.commit_refs += Mop->stat.num_refs;)
        ZESTO_STAT(core->stat.commit_loads += Mop->stat.num_loads;)

        if(Mop->fetch.inst.rep)
        {
          if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
            ZESTO_STAT(core->stat.commit_rep_insn++;)
          if(!Mop->oracle.zero_rep)
            ZESTO_STAT(core->stat.commit_rep_iterations++;)
          ZESTO_STAT(core->stat.commit_rep_uops += Mop->stat.num_uops;)
        }
        if(Mop->stat.num_uops > knobs->decode.max_uops[0])
        {
          ZESTO_STAT(core->stat.commit_UROM_insn++;)
          ZESTO_STAT(core->stat.commit_UROM_uops += Mop->stat.num_uops;)
          ZESTO_STAT(core->stat.commit_UROM_eff_uops += Mop->stat.num_eff_uops;)
        }

        core->stat.flow_count += Mop->stat.num_uops;
        core->stat.eff_flow_count += Mop->stat.num_eff_uops;
	
        if(Mop->fetch.inst.rep) /* all iterations of a REP count as one macro-op! */
        {
          if(Mop->uop[0].decode.BOM)
          {	
            when_rep_fetch_started = Mop->timing.when_fetch_started;
            when_rep_fetched = Mop->timing.when_fetched;
            zesto_assert(Mop->timing.when_fetched != TICK_T_MAX,(void)0);
            zesto_assert(Mop->timing.when_fetch_started != TICK_T_MAX,(void)0);
            zesto_assert(Mop->timing.when_fetched != 0,(void)0);
            zesto_assert(Mop->timing.when_fetch_started != 0,(void)0);
            when_rep_decode_started = Mop->timing.when_decode_started;
            when_rep_commit_started = Mop->timing.when_commit_started;
          }
          if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
          {
            ZESTO_STAT(stat_add_sample(core->stat.flow_histo, core->stat.flow_count);)
            ZESTO_STAT(stat_add_sample(core->stat.eff_flow_histo, core->stat.eff_flow_count);)
            core->stat.flow_count = 0;
            core->stat.eff_flow_count = 0;

            if(!Mop->decode.is_trap)
            {
              ZESTO_STAT(core->stat.Mop_fetch_slip += when_rep_fetched - when_rep_fetch_started;)
              ZESTO_STAT(core->stat.Mop_fetch2decode_slip += when_rep_decode_started - when_rep_fetched;)
              ZESTO_STAT(core->stat.Mop_decode_slip += Mop->timing.when_decode_finished - when_rep_decode_started;) /* from decode of first Mop's first uop to decode of last Mop's last uop */
              ZESTO_STAT(core->stat.Mop_decode2commit_slip += when_rep_commit_started - Mop->timing.when_decode_finished;)
              ZESTO_STAT(core->stat.Mop_commit_slip += Mop->timing.when_commit_finished - when_rep_commit_started;)
            }
            else
              ZESTO_STAT(core->stat.commit_traps++;)
          }
        }
        else
        {
          ZESTO_STAT(stat_add_sample(core->stat.flow_histo, core->stat.flow_count);)
          ZESTO_STAT(stat_add_sample(core->stat.eff_flow_histo, core->stat.eff_flow_count);)
          core->stat.flow_count = 0;
          core->stat.eff_flow_count = 0;
	
          if(!Mop->decode.is_trap)
          {
            ZESTO_STAT(core->stat.Mop_fetch_slip += Mop->timing.when_fetched - Mop->timing.when_fetch_started;)
            ZESTO_STAT(core->stat.Mop_fetch2decode_slip += Mop->timing.when_decode_started - Mop->timing.when_fetched;)
            ZESTO_STAT(core->stat.Mop_decode_slip += Mop->timing.when_decode_finished - Mop->timing.when_decode_started;)
            ZESTO_STAT(core->stat.Mop_decode2commit_slip += Mop->timing.when_commit_started - Mop->timing.when_decode_finished;)
            ZESTO_STAT(core->stat.Mop_commit_slip += Mop->timing.when_commit_finished - Mop->timing.when_commit_started;)
            zesto_assert(Mop->timing.when_commit_finished != TICK_T_MAX,(void)0);
            zesto_assert(Mop->timing.when_fetched != TICK_T_MAX,(void)0);
            zesto_assert(Mop->timing.when_fetch_started != TICK_T_MAX,(void)0);
            zesto_assert(Mop->timing.when_fetched != 0,(void)0);
            zesto_assert(Mop->timing.when_fetch_started != 0,(void)0);
          }
          else
            ZESTO_STAT(core->stat.commit_traps++;)
        }
#ifdef ZTRACE
        ztrace_print(Mop,"c|commit:EOM=%d:trap=%d|all uops in Mop committed; Mop retired",
        Mop->uop[Mop->decode.last_uop_index].decode.EOM,Mop->decode.is_trap);
#endif
        /*If the instruction was interrupted, recover oracle state*/ 
        if(Mop->decode.is_intr==true)
        {
#ifdef ZDEBUG
          if(core->sim_cycle > PRINT_CYCLE )
            fprintf(stdout,"\n[%lld][Core%d]Instruction interrupted - recovering pipeline",core->sim_cycle,core->id);
          if(core->sim_cycle > PRINT_CYCLE )
            md_print_insn(Mop,stdout);
          if(core->sim_cycle > PRINT_CYCLE )
            fprintf(stdout,"  %d \n",core->sim_cycle);
#endif	
          Mop->fetch.branch_mispred = true;
          core->oracle->pipe_recover(Mop,Mop->oracle.NextPC);
        }
        /* Let the oracle know that we are done with this Mop. */
        if(Mop->fetch.branch_mispred)
          core->exec->last_Mop_seq = Mop->oracle.seq + 2;
        else
          core->exec->last_Mop_seq = Mop->oracle.seq + 1;
#ifdef ZDEBUG
        if(core->sim_cycle > PRINT_CYCLE )
          fprintf(stdout,"\n[%lld][Core%d]Commited MOP=%d Next_Mop_Seq %d  ",core->sim_cycle,core->id, Mop->oracle.seq, core->exec->last_Mop_seq);
        if(core->sim_cycle > PRINT_CYCLE )
          md_print_insn(Mop,stdout);
#endif
        core->oracle->commit(Mop);
      }/*End of Mop*/
    }
    else
    {
      if(Mop->commit.complete_index == 0)
        stall_reason = CSTALL_NOT_READY;
      else
        stall_reason = CSTALL_PARTIAL;
      break; /* oldest Mop not done yet */
    }

    /*****************/
    /* finish early? */
    /*****************/
    if(core->current_thread->active)
    {
      if ((core->sim.max_cycles && core->sim_cycle >= core->sim.max_cycles)||
        (core->sim.max_insts && core->stat.commit_insn >= core->sim.max_insts)||
        (core->sim.max_uops && core->stat.commit_uops >= core->sim.max_uops))
      {
        core->stat.final_sim_cycle = core->sim_cycle; /* make note of when this core stopped simulating */
        if(core->sim.max_cycles && core->sim_cycle >= core->sim.max_cycles)
          fprintf(stderr,"# Simulation cycle ");
        else if(core->sim.max_insts && core->sim.max_uops)
          fprintf(stderr,"# Committed instruction/uop ");
        else if(core->sim.max_insts)
          fprintf(stderr,"# Committed instruction ");
        else
          fprintf(stderr,"# Committed uop ");
        fprintf(stderr,"limit reached for core %d.\n",core->current_thread->id);
	
        core->current_thread->active = false;
        core->fetch->bpred->freeze_stats();
        core->exec->freeze_stats();
        core->print_stats();

        /* start this core over */
        //longjmp(sim_exit_buf, /* exitcode + fudge */0 + 1);
      }	
    }
  }
  ZESTO_STAT(stat_add_sample(core->stat.commit_stall, (int)stall_reason);)
}

/* Walk ROB from youngest uop until we find the requested Mop.
   (NOTE: We stop at any uop belonging to the Mop.  We assume
   that recovery only occurs on Mop boundaries.)
   Release resources (PREGs, RS/ROB/LSQ entries, etc. as we go).
   If Mop == NULL, we're blowing away the entire latch. */
void
core_commit_atom_t::recover(const struct Mop_t * const Mop)
{
  assert(Mop != NULL);
  struct core_knobs_t * knobs = core->knobs;
  /**/ 
  core->exec->last_Mop_seq = core->global_seq + 1;

  if(ROB_num > 0)
  {
    /* requested uop should always be in the ROB */
    int index = moddec(ROB_tail,knobs->commit.ROB_size); //(ROB_tail-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;

    /* if there's only the one inst in the pipe, then we don't need to drain */
    if(knobs->alloc.drain_flush && (ROB[index]->Mop != Mop))
      core->alloc->start_drain();

    while(ROB[index] && (ROB[index]->Mop != Mop))
    {
      int i;
      struct uop_t * dead_uop = ROB[index];

      zesto_assert(ROB_num > 0,(void)0);

      if(dead_uop->decode.in_fusion)
      {
        zesto_assert(dead_uop->decode.is_fusion_head,(void)0);
        while(dead_uop->decode.fusion_next)
          dead_uop = dead_uop->decode.fusion_next;
        zesto_assert(dead_uop != dead_uop->decode.fusion_head,(void)0);
      }

      while(dead_uop)
      {
        /* squash this instruction - this invalidates all in-flight actions (e.g., uop execution, cache accesses) */
        dead_uop->exec.action_id = core->new_action_id();
        if(dead_uop->timing.when_allocated != TICK_T_MAX)
          ROB_eff_num--;

        if(dead_uop->alloc.RS_index != -1) /* currently in RS */
          core->exec->RS_deallocate(dead_uop);

        /* In the following, we have to check it the uop has even been allocated yet... this has
           to do with our non-atomic implementation of allocation for fused-uops */
        if(dead_uop->decode.is_std) /* dealloc when we get to the STA */
        {
          if(dead_uop->timing.when_allocated != TICK_T_MAX)
            core->exec->STQ_squash_std(dead_uop);
        }
        else if(dead_uop->decode.is_sta)
        {
          if(dead_uop->timing.when_allocated != TICK_T_MAX)
            core->exec->STQ_squash_sta(dead_uop);
        }

        /* clean up idep/odep pointers.  Since we're working our
           way back from the most speculative instruction, we only
           need to clean-up our parent's forward-pointers (odeps)
           and our own back-pointers (our fwd-ptrs would have
           already cleaned-up our own children). */
        for(i=0;i<MAX_IDEPS;i++)
        {
          struct uop_t * parent = dead_uop->exec.idep_uop[i];
          if(parent) /* I have an active parent */
          {
            struct odep_t * prev, * current;
            prev = NULL;
            current = parent->exec.odep_uop;
            while(current)
            {
              if((current->uop == dead_uop) && (current->op_num == i))
                break;
              prev = current;
              current = current->next;
            }

            zesto_assert(current,(void)0);

            /* remove self from parent's odep list */
            if(prev)
              prev->next = current->next;
            else
              parent->exec.odep_uop = current->next;

            /* eliminate my own idep */
            dead_uop->exec.idep_uop[i] = NULL;

            /* return the odep struct */
            core->return_odep_link(current);
          }
        }

        zesto_assert(dead_uop->exec.odep_uop == NULL,(void)0);

        if((!dead_uop->decode.in_fusion) || dead_uop->decode.is_fusion_head)
          dead_uop = NULL;
        else
        {
          /* this is ugly... need to traverse fused uop linked-list
             in reverse; most fused uops are short, so we're just
             going to re-traverse... if longer fused uops are
             supported, it may make sense to implement a
             doubly-linked list instead. */
          struct uop_t * p = ROB[index];
          while(p->decode.fusion_next != dead_uop)
            p = p->decode.fusion_next;

          dead_uop = p;
        }
      }

      #ifdef ZESTO_COUNTERS
      core->counters->ROB.write_tag++;
      #endif

      ROB[index] = NULL;
      ROB_tail = moddec(ROB_tail,knobs->commit.ROB_size); //(ROB_tail-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;
      ROB_num --;
      zesto_assert(ROB_num >= 0,(void)0);
      zesto_assert(ROB_eff_num >= 0,(void)0);

      index = moddec(index,knobs->commit.ROB_size); //(index-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;
    }
  }
}

void
core_commit_atom_t::recover(void)
{
  struct core_knobs_t * knobs = core->knobs;
  core->exec->last_Mop_seq = core->global_seq;
  if(ROB_num > 0)
  {
    /* requested uop should always be in the ROB */
    int index = moddec(ROB_tail,knobs->commit.ROB_size); //(ROB_tail-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;

    while(ROB[index])
    {
      int i;
      struct uop_t * dead_uop = ROB[index];

      zesto_assert(ROB_num > 0,(void)0);

      if(dead_uop->decode.in_fusion)
      {
        zesto_assert(dead_uop->decode.is_fusion_head,(void)0);
        while(dead_uop->decode.fusion_next)
          dead_uop = dead_uop->decode.fusion_next;
        zesto_assert(dead_uop != dead_uop->decode.fusion_head,(void)0);
      }

      while(dead_uop)
      {
        /* squash this instruction - this invalidates all in-flight actions (e.g., uop execution, cache accesses) */
        dead_uop->exec.action_id = core->new_action_id();
        if(dead_uop->timing.when_allocated != TICK_T_MAX)
          ROB_eff_num--;

        /* update allocation scoreboard if appropriate */
        /* if(uop->alloc.RS_index != -1) */
        /* fusion messes up this check */
        if(dead_uop->timing.when_exec == TICK_T_MAX && (dead_uop->alloc.port_assignment != -1))
          core->alloc->RS_deallocate(dead_uop);

        if(dead_uop->alloc.RS_index != -1) /* currently in RS */
          core->exec->RS_deallocate(dead_uop);


        /* In the following, we have to check it the uop has even
           been allocated yet... this has to do with our non-atomic
           implementation of allocation for fused-uops */
        if(dead_uop->decode.is_load)
        {
          if(dead_uop->timing.when_allocated != TICK_T_MAX)
            core->exec->LDQ_squash(dead_uop);
        }
        else if(dead_uop->decode.is_std) /* dealloc when we get to the STA */
        {
          if(dead_uop->timing.when_allocated != TICK_T_MAX)
            core->exec->STQ_squash_std(dead_uop);
        }
        else if(dead_uop->decode.is_sta)
        {
          if(dead_uop->timing.when_allocated != TICK_T_MAX)
            core->exec->STQ_squash_sta(dead_uop);
        }

        /* clean up idep/odep pointers.  Since we're working our
           way back from the most speculative instruction, we only
           need to clean-up our parent's forward-pointers (odeps)
           and our own back-pointers (our fwd-ptrs would have
           already cleaned-up our own children). */
        for(i=0;i<MAX_IDEPS;i++)
        {
          struct uop_t * parent = dead_uop->exec.idep_uop[i];
          if(parent) /* I have an active parent */
          {
            struct odep_t * prev, * current;
            prev = NULL;
            current = parent->exec.odep_uop;
            while(current)
            {
              if((current->uop == dead_uop) && (current->op_num == i))
                break;
              prev = current;
              current = current->next;
            }

            zesto_assert(current,(void)0);

            /* remove self from parent's odep list */
            if(prev)
              prev->next = current->next;
            else
              parent->exec.odep_uop = current->next;

            /* eliminate my own idep */
            dead_uop->exec.idep_uop[i] = NULL;

            /* return the odep struct */
            core->return_odep_link(current);
          }
        }

        zesto_assert(dead_uop->exec.odep_uop == NULL,(void)0);

        if((!dead_uop->decode.in_fusion) || dead_uop->decode.is_fusion_head)
          dead_uop = NULL;
        else
        {
          /* this is ugly... need to traverse fused uop linked-list
             in reverse; most fused uops are short, so we're just
             going to retraverse... if longer fused uops are
             supported, it may make sense to implement a
             doubly-linked list instead. */
          struct uop_t * p = ROB[index];
          while(p->decode.fusion_next != dead_uop)
            p = p->decode.fusion_next;

          dead_uop = p;
        }
      }

      #ifdef ZESTO_COUNTERS
      core->counters->ROB.write_tag++;
      #endif

      ROB[index] = NULL;
      ROB_tail = moddec(ROB_tail,knobs->commit.ROB_size); //(ROB_tail-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;
      ROB_num --;
      zesto_assert(ROB_num >= 0,(void)0);
      zesto_assert(ROB_eff_num >= 0,(void)0);

      index = moddec(index,knobs->commit.ROB_size); //(index-1+knobs->commit.ROB_size) % knobs->commit.ROB_size;
    }
  }

  core->exec->STQ_squash_senior();

  core->exec->recover_check_assertions();
  zesto_assert(ROB_num == 0,(void)0);
}

bool core_commit_atom_t::ROB_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return ROB_num < knobs->commit.ROB_size;
}

bool core_commit_atom_t::ROB_empty(void)
{
  return 0 == ROB_num;
}

void core_commit_atom_t::ROB_insert(struct uop_t * const uop)
{
  struct core_knobs_t * knobs = core->knobs;
  ROB[ROB_tail] = uop;
  uop->alloc.ROB_index = ROB_tail;
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
	fprintf(stdout,"\n[%lld][Core%d]ALLOC ROB_insert MOP=%d UOP=%d in ROB %d in_fusion %d",core->sim_cycle, core->id, uop->decode.Mop_seq, uop->decode.uop_seq, ROB_tail, uop->decode.in_fusion);
#endif
  ROB_num++;
  ROB_eff_num++;
  ROB_tail = modinc(ROB_tail,knobs->commit.ROB_size); //(ROB_tail+1) % knobs->commit.ROB_size;

  #ifdef ZESTO_COUNTERS
  core->counters->ROB.write++;
  #endif
}

void core_commit_atom_t::ROB_fuse_insert(struct uop_t * const uop)
{
  uop->alloc.ROB_index = uop->decode.fusion_head->alloc.ROB_index;
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
	fprintf(stdout,"\n[%lld][Core%d]ALLOC ROB_insert_fuse MOP=%d UOP=%d in ROB %d in_fusion %d",core->sim_cycle, core->id, uop->decode.Mop_seq, uop->decode.uop_seq, uop->alloc.ROB_index, uop->decode.in_fusion);
#endif
  ROB_eff_num++;

  #ifdef ZESTO_COUNTERS
  core->counters->ROB.write++;
  #endif
}

#endif
