/* exec-atom.cpp - In order Atom Model */
/*
 * __COPYRIGHT__ GT
 */
//#include "simple-cache/cache_messages.h"
#include "../zesto-core.h"


#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(exec_opt_string,"ATOM"))
    return new core_exec_atom_t(core);
#else

class core_exec_atom_t : public core_exec_t
{
  /* readyQ for scheduling */
  struct readyQ_node_t {
    struct uop_t * uop;
    seq_t uop_seq; /* seq id of uop when inserted - for proper sorting even after uop recycled */
    seq_t action_id;
    tick_t when_assigned;
    struct readyQ_node_t * next;
  };

  /* struct for a squashable in-flight uop (for example, a uop making its way
     down an ALU pipeline).  Changing the original uop's tag will make the tags
     no longer match, thereby invalidating the in-flight action. */
  struct uop_action_t {
    struct uop_t * uop;
    seq_t action_id;
    bool completed;
    bool load_enqueued;
    bool load_received;
  };

  /* struct for a generic pipelined ALU */
  struct ALU_t {
    struct uop_action_t * pipe;
    int occupancy;
    int latency;    /* number of cycles from start of execution to end */
    int issue_rate; /* number of cycles between issuing independent instructions on this ALU */
    tick_t when_scheduleable; /* cycle when next instruction can be scheduled for this ALU */
    tick_t when_executable; /* cycle when next instruction can actually start executing on this ALU */
  };

  public:

  core_exec_atom_t(struct core_t * const core);
  virtual void reg_stats(struct stat_sdb_t * const sdb);
  virtual void freeze_stats(void);
  virtual void update_occupancy(void);

  virtual void ALU_exec(void);
  virtual void LDST_exec(void);
  virtual void RS_schedule(void);
  virtual void LDQ_schedule(void);

  virtual void recover(const struct Mop_t * const Mop);
  virtual void recover(void);

  virtual void insert_ready_uop(struct uop_t * const uop);

  virtual bool RS_available(int port);
  virtual void RS_insert(struct uop_t * const uop);
  virtual void RS_fuse_insert(struct uop_t * const uop);
  virtual void RS_deallocate(struct uop_t * const uop);

  virtual bool LDQ_available(void);
  virtual void LDQ_insert(struct uop_t * const uop);
  virtual void LDQ_deallocate(struct uop_t * const uop);
  virtual void LDQ_squash(struct uop_t * const dead_uop);

  virtual bool STQ_available(void);
  virtual void STQ_insert_sta(struct uop_t * const uop);
  virtual void STQ_insert_std(struct uop_t * const uop);
  virtual void STQ_deallocate_sta(void);
  virtual bool STQ_deallocate_std(struct uop_t * const uop);
  virtual void STQ_deallocate_senior(void);
  virtual void STQ_squash_sta(struct uop_t * const dead_uop);
  virtual void STQ_squash_std(struct uop_t * const dead_uop);
  virtual void STQ_squash_senior(void);

  virtual void recover_check_assertions(void);

  virtual bool has_load_arrived(int LDQ_index); /*This function checks if a particular load has arrived fully from a cache*/
 
  virtual unsigned long long int get_load_latency(int LDQ_index);

  protected:
  struct readyQ_node_t * readyQ_free_pool; /* for scheduling readyQ's */
#ifdef DEBUG
  int readyQ_free_pool_debt; /* for debugging memory leaks */
#endif

  struct uop_t ** RS;
  int RS_num;
  int RS_eff_num;

  struct LDQ_t {
    struct uop_t * uop;
    md_addr_t virt_addr;
    md_paddr_t phys_addr;
    bool first_byte_requested;
    bool last_byte_requested;
    bool first_byte_arrived;
    bool last_byte_arrived;
    int mem_size;
    bool addr_valid;
    int store_color;   /* STQ index of most recent store before this load */
    bool hit_in_STQ;    /* received value from STQ */
    tick_t when_issued; /* when load actually issued */
    tick_t when_completed; /* when load is  actually completely finished*/
    bool speculative_broadcast; /* made a speculative tag broadcast */
    bool partial_forward; /* true if blocked on an earlier partially matching store */
  }*LDQ;
  int LDQ_head;
  int LDQ_tail;
  int LDQ_num;

  struct STQ_t {
    struct uop_t * sta;
    struct uop_t * std;
    struct uop_t * dl1_uop; //Placeholders for store uops which get returned  when the Mop commits
    struct uop_t * dtlb_uop;
    seq_t uop_seq;
    md_addr_t virt_addr;
    md_paddr_t phys_addr;
    bool first_byte_requested;
    bool last_byte_requested;
    bool first_byte_written;
    bool last_byte_written;
    int mem_size;
    union val_t value;
    bool addr_valid;
    bool value_valid;
    int next_load; /* LDQ index of next load in program order */

    /* for commit */
    bool translation_complete; /* dtlb access finished */
    bool write_complete; /* write access finished */
    seq_t action_id; /* need to squash commits when a core resets (multi-core mode only) */
  } * STQ;
  int STQ_head;
  int STQ_tail;
  int STQ_num;
  int STQ_senior_num;
  int STQ_senior_head;
  bool partial_forward_throttle; /* used to control load-issuing in the presence of partial-matching stores */

  struct exec_port_t {
    struct uop_action_t * payload_pipe;
    int occupancy;
    struct ALU_t * FU[NUM_FU_CLASSES];
    struct readyQ_node_t * readyQ;
    struct ALU_t * STQ; /* store-queue lookup/search pipeline for load execution */
    tick_t when_bypass_used; /* to make sure only one inst writes back per cycle, which
                                could happen due to insts with different latencies */
    int num_FU_types; /* the number of FU's bound to this port */
    enum md_fu_class * FU_types; /* the corresponding types of those FUs */
  } * port;
  bool check_for_work; /* used to skip ALU exec when there's no uops */

  struct memdep_t * memdep;

  /* various exec utility functions */
  struct readyQ_node_t * get_readyQ_node(void);
  void return_readyQ_node(struct readyQ_node_t * const p);
  bool check_load_issue_conditions(const struct uop_t * const uop);
  void snatch_back(struct uop_t * const replayed_uop);

  void load_writeback(unsigned long long int);

  /* callbacks need to be static */
  static void DL1_callback(void * const op);
  static void DL1_split_callback(void * const op);
  static void DTLB_callback(void * const op);
  static bool translated_callback(void * const op,const seq_t);
  static seq_t get_uop_action_id(void * const op);
  static void load_miss_reschedule(void * const op, const int new_pred_latency);

  /* callbacks used by commit for stores */
  static void store_dl1_callback(void * const op);
  static void store_dl1_split_callback(void * const op);
  static void store_dtlb_callback(void * const op);
  static bool store_translated_callback(void * const op,const seq_t);

  virtual void memory_callbacks(void);
};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_exec_atom_t::core_exec_atom_t(struct core_t * const arg_core):
  readyQ_free_pool(NULL),
#ifdef DEBUG
  readyQ_free_pool_debt(0),
#endif
  RS_num(0), RS_eff_num(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  RS = (struct uop_t**) calloc(knobs->exec.RS_size,sizeof(*RS));
  if(!RS)
    fatal("couldn't calloc RS");

  LDQ = (core_exec_atom_t::LDQ_t*) calloc(knobs->exec.LDQ_size,sizeof(*LDQ));
  if(!LDQ)
    fatal("couldn't calloc LDQ");

  STQ = (core_exec_atom_t::STQ_t*) calloc(knobs->exec.STQ_size,sizeof(*STQ));
  if(!STQ)
    fatal("couldn't calloc STQ");

  int i;
  /* This shouldn't be necessary, but I threw it in because valgrind (memcheck)
     was reporting that STQ[i].sta was being used uninitialized. -GL */
  for(i=0;i<knobs->exec.STQ_size;i++)
    STQ[i].sta = NULL;

  /************************************/
  /* execution port payload pipelines */
  /************************************/
  port = (core_exec_atom_t::exec_port_t*) calloc(knobs->exec.num_exec_ports,sizeof(*port));
  if(!port)
    fatal("couldn't calloc exec ports");
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    port[i].payload_pipe = (struct uop_action_t*) calloc(knobs->exec.payload_depth,sizeof(*port->payload_pipe));
    if(!port[i].payload_pipe)
      fatal("couldn't calloc payload pipe");
  }

  /***************************/
  /* execution port bindings */
  /***************************/
  for(i=0;i<NUM_FU_CLASSES;i++)
  {
    int j;
    knobs->exec.port_binding[i].ports = (int*) calloc(knobs->exec.port_binding[i].num_FUs,sizeof(int));
    if(!knobs->exec.port_binding[i].ports) fatal("couldn't calloc %s ports",MD_FU_NAME(i));
    for(j=0;j<knobs->exec.port_binding[i].num_FUs;j++)
    {
      if((knobs->exec.fu_bindings[i][j] < 0) || (knobs->exec.fu_bindings[i][j] >= knobs->exec.num_exec_ports))
        fatal("port binding for %s is negative or exceeds the execution width (should be > 0 and < %d)",MD_FU_NAME(i),knobs->exec.num_exec_ports);
      knobs->exec.port_binding[i].ports[j] = knobs->exec.fu_bindings[i][j];
    }
  }

  /***************************************/
  /* functional unit execution pipelines */
  /***************************************/
  for(i=0;i<NUM_FU_CLASSES;i++)
  {
    int j;
    for(j=0;j<knobs->exec.port_binding[i].num_FUs;j++)
    {
      int port_num = knobs->exec.port_binding[i].ports[j];
      int latency = knobs->exec.latency[i];
      int issue_rate = knobs->exec.issue_rate[i];
      port[port_num].FU[i] = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
      if(!port[port_num].FU[i])
        fatal("couldn't calloc IEU");
      port[port_num].FU[i]->latency = latency;
      port[port_num].FU[i]->issue_rate = issue_rate;
      port[port_num].FU[i]->pipe = (struct uop_action_t*) calloc(latency,sizeof(struct uop_action_t));
      if(!port[port_num].FU[i]->pipe)
        fatal("couldn't calloc %s function unit execution pipeline",MD_FU_NAME(i));

      if(i==FU_LD) /* load has AGEN and STQ access pipelines */
      {
        port[port_num].STQ = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
        latency = 3;//CAFFEINE: core->memory.DL1->latency; /* assume STQ latency matched to DL1's */
        port[port_num].STQ->latency = latency;
        port[port_num].STQ->issue_rate = issue_rate;
        port[port_num].STQ->pipe = (struct uop_action_t*) calloc(latency,sizeof(struct uop_action_t));
        if(!port[port_num].STQ->pipe)
          fatal("couldn't calloc load's STQ exec pipe on port %d",j);
      }
    }
  }

  /* shortened list of the FU's available on each port (to speed up FU loop in ALU_exec) */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    port[i].num_FU_types = 0;
    int j;
    for(j=0;j<NUM_FU_CLASSES;j++)
      if(port[i].FU[j])
        port[i].num_FU_types++;
    port[i].FU_types = (enum md_fu_class *)calloc(port[i].num_FU_types,sizeof(enum md_fu_class));
    if(!port[i].FU_types)
      fatal("couldn't calloc FU_types array on port %d",i);
    int k=0;
    for(j=0;j<NUM_FU_CLASSES;j++)
    {
      if(port[i].FU[j])
      {
        port[i].FU_types[k] = (enum md_fu_class)j;
        k++;
      }
    }
  }

  last_Mop_seq = 0 ; // core->global_seq starts from zero
  
  memdep = memdep_create(knobs->exec.memdep_opt_str,core);

  check_for_work = true;
}

void
core_exec_atom_t::reg_stats(struct stat_sdb_t * const sdb)
{
  char buf[1024];
  char buf2[1024];

  sprintf(buf,"\n#### Zesto Core#%d Exec Stats ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.exec_uops_issued",core->id);
  stat_reg_counter(sdb, true, buf, "number of uops issued", &core->stat.exec_uops_issued, core->stat.exec_uops_issued, NULL);
  sprintf(buf,"c%d.exec_uPC",core->id);
  sprintf(buf2,"c%d.exec_uops_issued/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average number of uops executed per cycle", buf2, NULL);
  sprintf(buf,"c%d.exec_uops_replayed",core->id);
  stat_reg_counter(sdb, true, buf, "number of uops replayed", &core->stat.exec_uops_replayed, core->stat.exec_uops_replayed, NULL);
  sprintf(buf,"c%d.exec_avg_replays",core->id);
  sprintf(buf2,"c%d.exec_uops_replayed/c%d.exec_uops_issued",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average replays per uop", buf2, NULL);
  sprintf(buf,"c%d.exec_uops_snatched",core->id);
  stat_reg_counter(sdb, true, buf, "number of uops snatched-back", &core->stat.exec_uops_snatched_back, core->stat.exec_uops_snatched_back, NULL);
  sprintf(buf,"c%d.exec_avg_snatched",core->id);
  sprintf(buf2,"c%d.exec_uops_snatched/c%d.exec_uops_issued",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average snatch-backs per uop", buf2, NULL);
  sprintf(buf,"c%d.num_jeclear",core->id);
  stat_reg_counter(sdb, true, buf, "number of branch mispredictions", &core->stat.num_jeclear, core->stat.num_jeclear, NULL);
  sprintf(buf,"c%d.num_wp_jeclear",core->id);
  stat_reg_counter(sdb, true, buf, "number of branch mispredictions in the shadow of an earlier mispred", &core->stat.num_wp_jeclear, core->stat.num_wp_jeclear, NULL);
  sprintf(buf,"c%d.load_nukes",core->id);
  stat_reg_counter(sdb, true, buf, "num pipeflushes due to load-store order violation", &core->stat.load_nukes, core->stat.load_nukes, NULL);
  sprintf(buf,"c%d.wp_load_nukes",core->id);
  stat_reg_counter(sdb, true, buf, "num pipeflushes due to load-store order violation on wrong-path", &core->stat.wp_load_nukes, core->stat.wp_load_nukes, NULL);
  sprintf(buf,"c%d.DL1_load_split_accesses",core->id);
  stat_reg_counter(sdb, true, buf, "number of loads requiring split accesses", &core->stat.DL1_load_split_accesses, core->stat.DL1_load_split_accesses, NULL);
  sprintf(buf,"c%d.DL1_load_split_frac",core->id);
  sprintf(buf2,"c%d.DL1_load_split_accesses/(c%d.DL1.load_lookups-c%d.DL1_load_split_accesses)",core->id,core->id,core->id); /* need to subtract since each split access generated two load accesses */
  stat_reg_formula(sdb, true, buf, "fraction of loads requiring split accesses", buf2, NULL);

  sprintf(buf,"c%d.RS_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total RS occupancy", &core->stat.RS_occupancy, core->stat.RS_occupancy, NULL);
  sprintf(buf,"c%d.RS_eff_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total RS effective occupancy", &core->stat.RS_eff_occupancy, core->stat.RS_eff_occupancy, NULL);
  sprintf(buf,"c%d.RS_empty",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was empty", &core->stat.RS_empty_cycles, core->stat.RS_empty_cycles, NULL);
  sprintf(buf,"c%d.RS_full",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was full", &core->stat.RS_full_cycles, core->stat.RS_full_cycles, NULL);
  sprintf(buf,"c%d.RS_avg",core->id);
  sprintf(buf2,"c%d.RS_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average RS occupancy", buf2, NULL);
  sprintf(buf,"c%d.RS_eff_avg",core->id);
  sprintf(buf2,"c%d.RS_eff_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "effective average RS occupancy", buf2, NULL);
  sprintf(buf,"c%d.RS_frac_empty",core->id);
  sprintf(buf2,"c%d.RS_empty/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles RS was empty", buf2, NULL);
  sprintf(buf,"c%d.RS_frac_full",core->id);
  sprintf(buf2,"c%d.RS_full/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles RS was full", buf2, NULL);

  sprintf(buf,"c%d.LDQ_occupancy",core->id);
  stat_reg_counter(sdb, true, buf, "total LDQ occupancy", &core->stat.LDQ_occupancy, core->stat.LDQ_occupancy, NULL);
  sprintf(buf,"c%d.LDQ_empty",core->id);
  stat_reg_counter(sdb, true, buf, "total cycles LDQ was empty", &core->stat.LDQ_empty_cycles, core->stat.LDQ_empty_cycles, NULL);
  sprintf(buf,"c%d.LDQ_full",core->id);
  stat_reg_counter(sdb, true, buf, "total cycles LDQ was full", &core->stat.LDQ_full_cycles, core->stat.LDQ_full_cycles, NULL);
  sprintf(buf,"c%d.LDQ_avg",core->id);
  sprintf(buf2,"c%d.LDQ_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average LDQ occupancy", buf2, NULL);
  sprintf(buf,"c%d.LDQ_frac_empty",core->id);
  sprintf(buf2,"c%d.LDQ_empty/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles LDQ was empty", buf2, NULL);
  sprintf(buf,"c%d.LDQ_frac_full",core->id);
  sprintf(buf2,"c%d.LDQ_full/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles LDQ was full", buf2, NULL);

  sprintf(buf,"c%d.STQ_occupancy",core->id);
  stat_reg_counter(sdb, true, buf, "total STQ occupancy", &core->stat.STQ_occupancy, core->stat.STQ_occupancy, NULL);
  sprintf(buf,"c%d.STQ_empty",core->id);
  stat_reg_counter(sdb, true, buf, "total cycles STQ was empty", &core->stat.STQ_empty_cycles, core->stat.STQ_empty_cycles, NULL);
  sprintf(buf,"c%d.STQ_full",core->id);
  stat_reg_counter(sdb, true, buf, "total cycles STQ was full", &core->stat.STQ_full_cycles, core->stat.STQ_full_cycles, NULL);
  sprintf(buf,"c%d.STQ_avg",core->id);
  sprintf(buf2,"c%d.STQ_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average STQ occupancy", buf2, NULL);
  sprintf(buf,"c%d.STQ_frac_empty",core->id);
  sprintf(buf2,"c%d.STQ_empty/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles STQ was empty", buf2, NULL);
  sprintf(buf,"c%d.STQ_frac_full",core->id);
  sprintf(buf2,"c%d.STQ_full/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles STQ was full", buf2, NULL);

  memdep->reg_stats(sdb, core);
}

void core_exec_atom_t::freeze_stats(void)
{
  memdep->freeze_stats();
}

void core_exec_atom_t::update_occupancy(void)
{
  /* RS */
  core->stat.RS_occupancy += RS_num;
  core->stat.RS_eff_occupancy += RS_eff_num;
  if(RS_num >= core->knobs->exec.RS_size)
    core->stat.RS_full_cycles++;
  if(RS_num <= 0)
    core->stat.RS_empty_cycles++;

  /* LDQ */
  core->stat.LDQ_occupancy += LDQ_num;
  if(LDQ_num >= core->knobs->exec.LDQ_size)
    core->stat.LDQ_full_cycles++;
  if(LDQ_num <= 0)
    core->stat.LDQ_empty_cycles++;

  /* STQ */
  core->stat.STQ_occupancy += STQ_num;
  if(STQ_num >= core->knobs->exec.STQ_size)
    core->stat.STQ_full_cycles++;
  if(STQ_num <= 0)
    core->stat.STQ_empty_cycles++;
}

/* Functions to support dependency tracking 
   NOTE: "Ready" Queue is somewhat of a misnomer... uops are placed in the
   readyQ when all of their data-flow parents have issued (although not
   necessarily executed).  However, that doesn't necessarily mean that the
   corresponding input *values* are "ready" due to non-zero schedule-to-
   execute latencies. */
core_exec_atom_t::readyQ_node_t * core_exec_atom_t::get_readyQ_node(void)
{
  struct readyQ_node_t * p = NULL;
  if(readyQ_free_pool)
  {
    p = readyQ_free_pool;
    readyQ_free_pool = p->next;
  }
  else
  {
    p = (struct readyQ_node_t*) calloc(1,sizeof(*p));
    if(!p)
      fatal("couldn't calloc a readyQ node");
  }
  assert(p);
  p->next = NULL;
  p->when_assigned = core->sim_cycle;
#ifdef DEBUG
  readyQ_free_pool_debt++;
#endif
  return p;
}

void core_exec_atom_t::return_readyQ_node(struct readyQ_node_t * const p)
{
  assert(p);
  assert(p->uop);
  p->next = readyQ_free_pool;
  readyQ_free_pool = p;
  p->uop = NULL;
  p->when_assigned = -1;
#ifdef DEBUG
  readyQ_free_pool_debt--;
#endif
}

/* Add the uop to the corresponding readyQ (based on port binding - we maintain
   one readyQ per execution port) */
void core_exec_atom_t::insert_ready_uop(struct uop_t * const uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((uop->alloc.port_assignment >= 0) && (uop->alloc.port_assignment < knobs->exec.num_exec_ports),(void)0);
  zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
  zesto_assert(uop->timing.when_exec == TICK_T_MAX,(void)0);
  zesto_assert(uop->timing.when_issued == TICK_T_MAX,(void)0);
  zesto_assert(!uop->exec.in_readyQ,(void)0);

  struct readyQ_node_t ** RQ = &port[uop->alloc.port_assignment].readyQ;
  struct readyQ_node_t * new_node = get_readyQ_node();
  new_node->uop = uop;
  new_node->uop_seq = uop->decode.uop_seq;
  uop->exec.in_readyQ = true;
  uop->exec.action_id = core->new_action_id();
  new_node->action_id = uop->exec.action_id;

  if(!*RQ) /* empty ready queue */
  {
    *RQ = new_node;
    return;
  }

  struct readyQ_node_t * current = *RQ, * prev = NULL;

  /* insert in age order: first find insertion point */
  while(current && (current->uop_seq < uop->decode.uop_seq))
  {
    prev = current;
    current = current->next;
  }

  if(!prev) /* uop is oldest */
  {
    new_node->next = *RQ;
    *RQ = new_node;
  }
  else
  {
    new_node->next = current;
    prev->next = new_node;
  }
}

/*****************************/
/* MAIN SCHED/EXEC FUNCTIONS */
/*****************************/

void core_exec_atom_t::RS_schedule(void) /* for uops in the RS */
{
  struct core_knobs_t * knobs = core->knobs;
  int i;

  /* All instructions leave the reservation stations as long as the next entry in the payload pipe is free, 
  Even if some operands are not ready it still goes to the payload pipe which is equivalent to the Register fetch stage */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {

    if(RS[i]) /* There is a uop in the Reservation stations. */
    {
      /*Every Port is connected to a single RS because this is an inorder pipeline*/
      int port_num = RS[i]->alloc.port_assignment;
      zesto_assert(i == port_num,(void)0);
      if(port[port_num].payload_pipe[0].uop == NULL) /* port is free */
      {
        struct uop_t * uop = RS[i];

        if( (port[port_num].FU[uop->decode.FU_class]->when_scheduleable <= core->sim_cycle) && ((!uop->decode.in_fusion) || uop->decode.fusion_head->alloc.full_fusion_allocated))
        {
          zesto_assert(uop->alloc.port_assignment == port_num,(void)0);
          /*Getting a new action id from for th uop. For fued uops only the fusion head is given a action ID*/
          uop->exec.action_id = core->new_action_id();
          port[port_num].payload_pipe[0].uop = uop;
          port[port_num].payload_pipe[0].action_id = uop->exec.action_id;
          port[port_num].occupancy++;
          zesto_assert(port[port_num].occupancy <= knobs->exec.payload_depth,(void)0);
          uop->timing.when_issued = core->sim_cycle;
          check_for_work = true;

          #ifdef ZESTO_COUNTERS
          core->counters->payload.write++;
          core->counters->RS.read++;
          core->counters->issue_select.read++;
          core->counters->latch.RS2PR.read++;
          #endif

#ifdef ZDEBUG
          if(core->sim_cycle > PRINT_CYCLE )
            fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d in RS_schedule %d in_fusion %d spec_mode %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uop->alloc.port_assignment,uop->decode.in_fusion,uop->Mop->oracle.spec_mode);
#endif

#ifdef ZTRACE
          ztrace_print(uop,"e|RS-issue|uop issued to payload RAM");
#endif
          if(uop->decode.is_load)
          {
//            int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;
            //TODO caffein-sim add memlatency uop->timing.when_otag_ready = core->sim_cycle + port[i].FU[uop->decode.FU_class]->latency + core->memory.DL1->latency + fp_penalty;
            // uop->timing.when_otag_ready = core->sim_cycle + port[i].FU[uop->decode.FU_class]->latency + 3 + fp_penalty;
            uop->timing.when_otag_ready = TICK_T_MAX;
          }
          else
          {
            int fp_penalty = ((REG_IS_FPR(uop->decode.odep_name) && !(uop->decode.opflags & F_FCOMP)) ||
              (!REG_IS_FPR(uop->decode.odep_name) && (uop->decode.opflags & F_FCOMP)))?knobs->exec.fp_penalty:0;
            uop->timing.when_otag_ready = core->sim_cycle + port[port_num].FU[uop->decode.FU_class]->latency + fp_penalty;
          }

          port[port_num].FU[uop->decode.FU_class]->when_scheduleable = core->sim_cycle + port[port_num].FU[uop->decode.FU_class]->issue_rate;

          /*All fusion uops pass through the pipeline as a single uop and so odeps of all of them need to be set correctly*/
          if(uop->decode.in_fusion)
          {
            struct uop_t * fusion_uop = uop->decode.fusion_head;
            zesto_assert(fusion_uop == uop, (void)0);
            while(fusion_uop)
            {
              struct odep_t * odep = fusion_uop->exec.odep_uop;
              while(odep)
              {
                int j;
                tick_t when_ready = 0;
                odep->uop->timing.when_itag_ready[odep->op_num] = fusion_uop->timing.when_otag_ready;
                for(j=0;j<MAX_IDEPS;j++)
                {
                  if(when_ready < odep->uop->timing.when_itag_ready[j])
                    when_ready = odep->uop->timing.when_itag_ready[j];
                }
                odep->uop->timing.when_ready = when_ready;
                odep = odep->next;
              }
              fusion_uop = fusion_uop->decode.fusion_next;
              ZESTO_STAT(core->stat.exec_uops_issued++;)
            }	  
          }
          else /* All uops that are not fused go independently and hence need to be set correctly. */
          {
            /* tag broadcast to dependents */
            struct odep_t * odep = uop->exec.odep_uop;
            while(odep)
            {
              int j;
              tick_t when_ready = 0;
              odep->uop->timing.when_itag_ready[odep->op_num] = uop->timing.when_otag_ready;
              for(j=0;j<MAX_IDEPS;j++)
              {
                if(when_ready < odep->uop->timing.when_itag_ready[j])
                  when_ready = odep->uop->timing.when_itag_ready[j];
              }
              odep->uop->timing.when_ready = when_ready;
              odep = odep->next;
            }
            ZESTO_STAT(core->stat.exec_uops_issued++;)
          }	

          #ifdef ZESTO_COUNTERS
          core->counters->RS.write_tag++;
          #endif

          RS[i] = NULL;
          RS_num--;
          if(uop->decode.in_fusion)
          {
            struct uop_t * fusion_uop = uop->decode.fusion_head;
            zesto_assert(fusion_uop == uop, (void)0);
            while(fusion_uop)
            {
              fusion_uop->alloc.RS_index = -1;
              core->alloc->RS_deallocate(fusion_uop);
              RS_eff_num--;
              fusion_uop = fusion_uop->decode.fusion_next;
            }	  
          }
          else /* update port loading table */
          {
            uop->alloc.RS_index = -1;
            core->alloc->RS_deallocate(uop);
          }
        }
      }
    }
  }
}

/* returns true if load is allowed to issue (or is predicted to be ok) */
bool core_exec_atom_t::check_load_issue_conditions(const struct uop_t * const uop)
{
  return true;
}

/* helper function to remove issued uops after a latency misprediction */
void core_exec_atom_t::snatch_back(struct uop_t * const replayed_uop)
{
}

/* The callback functions below (after load_writeback) mark flags
   in the uop to specify the completion of each task, and only when
   all are done do we call the load-writeback function to finish
   off execution of the load. */

void core_exec_atom_t::load_writeback(unsigned long long int addr)
{
  struct core_knobs_t * knobs = core->knobs;
  //for(i=0;i<knobs->exec.port_binding[FU_LD].num_FUs;i++)
  for(int i=0;i<knobs->exec.num_exec_ports;i++)
  {
    int j;
     
    for(j=0;j<port[i].num_FU_types;j++)
    {
      enum md_fu_class FU_type = port[i].FU_types[j];
      struct ALU_t * FU = port[i].FU[FU_type];
      int stage;
      if(FU)
        stage = FU->latency-1;

      /* For inorder cores, all stages of all pipes can handle loads and so we need to scan 
         all of thme to see which loadis returned. This is done in reverse order so that 
         the earliest load is reversed. */
      if(FU && (FU->occupancy > 0) )
      for(;stage>0;stage--)
      {
        /* load_enqueued field of uop_action should be set and the uop or one of its fused ops 
           should be a load and the fused uop should not have been killed. */
      if(FU->pipe[stage].uop)
        if(FU->pipe[stage].load_enqueued && (FU->pipe[stage].uop->decode.is_load || FU->pipe[stage].uop->decode.in_fusion ) && (FU->pipe[stage].action_id == FU->pipe[stage].uop->exec.action_id) )
        {
          zesto_assert(FU->pipe[stage].load_received == false, (void)0);
          struct uop_t * uop = FU->pipe[stage].uop;
          if(uop->decode.in_fusion)  /*It is possible that the load is not the head uop in a fused set and so we have to scan the whole set*/
          {
            struct uop_t * fusion_uop = uop->decode.fusion_head;
            zesto_assert(fusion_uop == uop, (void)0);
            while(uop)
            {
              if(uop->decode.is_load)
                break;
              uop = uop->decode.fusion_next;
            }
            /*if none of the uops in the fused set is a load load_enqueued cannot be set BUG!*/	  
            zesto_assert(uop != NULL, (void)0);
          }

          /*STQs are used as store buffers for the inorder core*/
          if(uop && uop->decode.is_load)//Check if uop exists..it may have finished execution due to a store to load forward
          {
            if( (uop->oracle.phys_addr & 0xffffffc0) == addr)
            {
              #ifdef ZESTO_COUNTERS
              core->counters->latch.L12LQ.read++;
              #endif

              int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;
              port[uop->alloc.port_assignment].when_bypass_used = core->sim_cycle+fp_penalty;
              uop->exec.ovalue = uop->oracle.ovalue; /* XXX: just using oracle value for now */
              uop->exec.ovalue_valid = true;
              zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
              uop->timing.when_completed = core->sim_cycle+fp_penalty;
              uop->exec.when_data_loaded = core->sim_cycle;
              uop->exec.exec_complete=true;
              FU->pipe[stage].load_received=true;
              FU->pipe[stage].load_enqueued = false;

#ifdef ZDEBUG
              if(core->sim_cycle > PRINT_CYCLE )
                fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d in load_writeback ADDR %llx in_fusion %d port %d FU_type %d stage %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,addr,uop->decode.in_fusion,i,j,stage);
#endif

              if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC) && (uop->Mop->decode.is_intr != true) ) /* XXX: for RETN */
              {
#ifdef ZDEBUG
                if(core->sim_cycle > PRINT_CYCLE )
                  fprintf(stdout,"\n[%lld][Core%d]load/RETN mispred detected (no STQ hit)",core->sim_cycle,core->id);
                if(core->sim_cycle > PRINT_CYCLE )
                  md_print_insn(uop->Mop,stdout);
                if(core->sim_cycle > PRINT_CYCLE )
                  fprintf(stdout,"  %d \n",core->sim_cycle);
#endif
                uop->Mop->fetch.branch_mispred = true;
                core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                ZESTO_STAT(core->stat.num_jeclear++;)
                if(uop->Mop->oracle.spec_mode)
                  ZESTO_STAT(core->stat.num_wp_jeclear++;)
#ifdef ZTRACE
      ztrace_print(uop,"e|jeclear|load/RETN mispred detected (no STQ hit)");
#endif
              }

              #ifdef ZESTO_COUNTERS
              if(uop->exec.when_data_loaded != TICK_T_MAX)
              {
                if((!uop->decode.in_fusion)||(uop->decode.in_fusion&&(uop->decode.fusion_next == NULL))) // this uop is done
                {
                  core->counters->ROB.write++;
                }
              }
              core->counters->load_bypass.read++;
              #endif

              /* we thought this output would be ready later in the future, but it's ready now! */
              if(uop->timing.when_otag_ready > (core->sim_cycle+fp_penalty))
                uop->timing.when_otag_ready = core->sim_cycle+fp_penalty;

              /* bypass output value to dependents, but also check to see if
                 dependents were already speculatively scheduled; if not,
                 wake them up. */
              struct odep_t * odep = uop->exec.odep_uop;

              while(odep)
              {
                /* check scheduling info (tags) */
                if(odep->uop->timing.when_itag_ready[odep->op_num] > (core->sim_cycle+fp_penalty))
                {
                  int j;
                  tick_t when_ready = 0;
	
                  odep->uop->timing.when_itag_ready[odep->op_num] = core->sim_cycle+fp_penalty;
	
                  for(j=0;j<MAX_IDEPS;j++)
                    if(when_ready < odep->uop->timing.when_itag_ready[j])
                      when_ready = odep->uop->timing.when_itag_ready[j];
	
                  if(when_ready < TICK_T_MAX)
                  {
                    odep->uop->timing.when_ready = when_ready;
                  }
                }

                #ifdef ZESTO_COUNTERS
                if(uop->exec.when_data_loaded != TICK_T_MAX)
                {
                  if((odep->uop->timing.when_allocated != TICK_T_MAX)&&(!odep->uop->exec.ivalue_valid[odep->op_num]))
                  {
                    core->counters->RS.write++;
                  }
                }
                #endif
	
                /* bypass value if the odep is outside of the fused uop */
                if(uop->decode.fusion_head != odep->uop->decode.fusion_head)
                  zesto_assert(!odep->uop->exec.ivalue_valid[odep->op_num],(void)0);
                odep->uop->exec.ivalue_valid[odep->op_num] = true;
                if(odep->aflags) /* shouldn't happen for loads? */
                {
                  warn("load modified flags?\n");
                  odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
                }
                else
                  odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
                odep->uop->timing.when_ival_ready[odep->op_num] = core->sim_cycle+fp_penalty;
                odep = odep->next;
              }	
            }
          }
        }
      }	
    } 
  }
}

/* used for accesses that can be serviced entirely by one cacheline,
   or by the first access of a split-access */
void core_exec_atom_t::DL1_callback(void * const op)
{
}

/* used only for the second access of a split access */
void core_exec_atom_t::DL1_split_callback(void * const op)
{
}

void core_exec_atom_t::DTLB_callback(void * const op)
{
}

/* returns true if TLB translation has completed */
bool core_exec_atom_t::translated_callback(void * const op, const seq_t action_id)
{
  return true;
}

/* Used by the cache processing functions to recover the id of the
   uop without needing to know about the uop struct. */
seq_t core_exec_atom_t::get_uop_action_id(void * const op)
{
  struct uop_t const * uop = (struct uop_t*) op;
  return uop->exec.action_id;
}

/* This function takes care of uops that have been misscheduled due
   to a latency misprediction (e.g., they got scheduled assuming
   the parent uop would hit in the DL1, but it turns out that the
   load latency will be longer due to a cache miss).  Uops may be
   speculatively rescheduled based on the next anticipated latency
   (e.g., L2 hit latency). */
void core_exec_atom_t::load_miss_reschedule(void * const op, const int new_pred_latency)
{
}

/* process loads exiting the STQ search pipeline, update caches */
void core_exec_atom_t::LDST_exec(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;
  
  /* Enqueue Loads to the cache - Loads are enqueued in the load pipeline up to the pipeline latency */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    int j;
     
    for(j=0;j<port[i].num_FU_types;j++)
    {
      enum md_fu_class FU_type = port[i].FU_types[j];
      struct ALU_t * FU = port[i].FU[FU_type];
      int stage;
      if(FU)
        stage = FU->latency-1;

      if(FU && (FU->occupancy > 0) )
      for(;stage>0;stage--)
      {
        if(!FU->pipe[stage].load_enqueued && !FU->pipe[stage].load_received && FU->pipe[stage].uop)
        {
          struct uop_t * uop = FU->pipe[stage].uop;

          if(uop->decode.in_fusion)
          {
            struct uop_t * fusion_uop = uop->decode.fusion_head;
            zesto_assert(fusion_uop == uop, (void)0);
            while(uop)
            {
              if(uop->decode.is_load)
                break;
              uop = uop->decode.fusion_next;
            }
            /* If none of them is a load, continue to the previous stage */
            if(uop==NULL)
              continue;
          }

          if(uop && uop->decode.is_load)
          {
	          cache_req* request = new cache_req(core->request_id, core->id, uop->oracle.phys_addr, OpMemLd, INITIAL);
            
            uop->exec.when_data_loaded = TICK_T_MAX;
            uop->exec.when_addr_translated = TICK_T_MAX;

            #ifdef ZDEBUG
            if(core->sim_cycle > PRINT_CYCLE )
              fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d in load_enqueue ADDR %llx in_fusion %d port %d FU_type %d stage %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uop->oracle.phys_addr,uop->decode.in_fusion,i,j,stage);
            #endif  
            
            /* Issue a new Preq and corresponding Mreq */
            core_t::Preq preq;
            //preq.index = index;
            preq.index = 0;
	    preq.thread_id = core->current_thread->id;
            preq.action_id = uop->exec.action_id;
            core->my_map.insert(std::pair<int, core_t::Preq>(core->request_id, preq));
            core->request_id++;
            /* Marking the load is already enqueued into the memory system */	
            FU->pipe[stage].load_enqueued =true;
            FU->pipe[stage].load_received =false;

            /* Actually enqueueing the request to cache*/
            core->cache_link->send(request); 
          }
        }
      }
    }	
  }  
}

/* Schedule load uops to execute from the LDQ.  Load execution occurs in a two-step
   process: first the address gets computed (issuing from the RS), and then the
   cache/STQ search occur (issuing from the LDQ). */
void core_exec_atom_t::LDQ_schedule(void)
{
}

/* Process actual execution (in ALUs) of uops, as well as shuffling of
   uops through the payload latch. */
void core_exec_atom_t::ALU_exec(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;

  if(check_for_work == false)
    return;

  bool work_found = false;

  /* Process Functional Units */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    int j;
    for(j=0;j<port[i].num_FU_types;j++)
    {
      enum md_fu_class FU_type = port[i].FU_types[j];
      struct ALU_t * FU = port[i].FU[FU_type];
      
      if(FU && (FU->occupancy > 0))
      {
        work_found = true;
        /* process last stage of FU pipeline (those uops completing execution) */
        int stage = FU->latency-1;
        struct uop_t * uop = FU->pipe[stage].uop;
        
        /* For fused uops we need to know if all of them have completed to mark the FU->stage.uop as exec_complete */
        int all_complete=true;

        /* We need to iterate thru multiple uops if they are fused together, all_complete tracks 
           if all have completed and the uop is ready to commit */
        while(uop)
        {
          int squashed;
          if(uop->decode.in_fusion)
            squashed = (FU->pipe[stage].action_id != uop->decode.fusion_head->exec.action_id);
          else
            squashed = (FU->pipe[stage].action_id != uop->exec.action_id);
		
          if(uop->Mop->core->id != core->id)
          {
            squashed=true;	
          }

          if(uop->timing.when_completed != TICK_T_MAX)
          {
            if(uop->decode.in_fusion)
            {
              uop = uop->decode.fusion_next;
              continue;
            }
            else
            {
              if(uop->Mop->oracle.spec_mode == false)
                break;
            }
          } 

          if(uop->Mop->oracle.spec_mode==true) //&& uop->Mop->decode.op == NOP)
            squashed=true;

          /* there's a uop completing execution (that hasn't been squashed) */ 
          if(!squashed)// && (!needs_bypass || bypass_available))
          {
            #ifdef ZESTO_COUNTERS
            core->counters->latch.FU2ROB.read++;

            if((!uop->decode.in_fusion||(uop->decode.in_fusion&&(uop->decode.fusion_next == NULL)))\
               &&uop->exec.ovalue_valid)
            {
              core->counters->ROB.write++; // this uop is done
            }
            #endif

            if(!(uop->decode.is_sta||uop->decode.is_std||uop->decode.is_load||uop->decode.is_ctrl))
            {
              port[i].when_bypass_used = core->sim_cycle;

              #ifdef ZESTO_COUNTERS
              core->counters->exec_bypass.read++;
              #endif
            }

            if(uop->decode.is_load) /* loads need to be processed differently */ 
            {
              /*Check the store buffer for a matching store. If found the uop completes else it waits for load_writeback() function to get completed*/
              int j=STQ_head;
              while(j!=STQ_tail && STQ[j].sta!=NULL )
              {
                int st_mem_size = STQ[j].mem_size;
                int ld_mem_size = uop->decode.mem_size;
                md_addr_t st_addr1 = STQ[j].virt_addr; /* addr of first byte */
                md_addr_t st_addr2 = STQ[j].virt_addr + st_mem_size - 1; /* addr of last byte */
                md_addr_t ld_addr1 = uop->oracle.virt_addr;
                md_addr_t ld_addr2 = uop->oracle.virt_addr + ld_mem_size - 1;
           
                if((st_addr1 <= ld_addr1) && (st_addr2 >= ld_addr2)) /* match */ 
                {
                  if(uop->timing.when_completed != TICK_T_MAX)
                  {
                    uop->Mop->fetch.branch_mispred = false;
                    core->oracle->pipe_flush(uop->Mop);
                    ZESTO_STAT(core->stat.load_nukes++;)
                    if(uop->Mop->oracle.spec_mode)
                      ZESTO_STAT(core->stat.wp_load_nukes++;)
#ifdef ZTRACE
                    ztrace_print(uop,"e|order-violation|matching store found but load already executed");
#endif
                  }
                  else
                  {
                    #ifdef ZESTO_COUNTERS
                    core->counters->storeQ.read++;
                    core->counters->latch.SQ2LQ.read++; // STQ2ROB (no LDQ)

                    if((!uop->decode.in_fusion)||(uop->decode.in_fusion&&(uop->decode.fusion_next == NULL))) // this uop is done
                    {
                      core->counters->ROB.write++;
                    }
                    core->counters->load_bypass.read++;
                    #endif

                    int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;
                    uop->exec.ovalue = STQ[j].value;
                    uop->exec.ovalue_valid = true;
                    zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
                    uop->timing.when_completed = core->sim_cycle+fp_penalty;
                    uop->timing.when_exec = core->sim_cycle+fp_penalty;

                    if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC)) /* for RETN */
                    {
                      uop->Mop->fetch.branch_mispred = true;
                      core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                      ZESTO_STAT(core->stat.num_jeclear++;)
                      if(uop->Mop->oracle.spec_mode)
                        ZESTO_STAT(core->stat.num_wp_jeclear++;)
#ifdef ZTRACE
                      ztrace_print(uop,"e|jeclear|load/RETN mispred detected in STQ hit");
#endif
                    }

                    /* we thought this output would be ready later in the future, but    it's ready now! */
                    if(uop->timing.when_otag_ready > (core->sim_cycle+fp_penalty))
                      uop->timing.when_otag_ready = core->sim_cycle+fp_penalty;

                    struct odep_t * odep = uop->exec.odep_uop;

                    while(odep)
                    {
                      /* check scheduling info (tags) */
                      if(odep->uop->timing.when_itag_ready[odep->op_num] > core->sim_cycle)
                      {
                        int j;
                        tick_t when_ready = 0;

                        odep->uop->timing.when_itag_ready[odep->op_num] = core->sim_cycle+fp_penalty;

                        for(j=0;j<MAX_IDEPS;j++)
                          if(when_ready < odep->uop->timing.when_itag_ready[j])
                            when_ready = odep->uop->timing.when_itag_ready[j];

                        if(when_ready < TICK_T_MAX)
                        {
                          odep->uop->timing.when_ready = when_ready;
                        }
                      }

                      #ifdef ZESTO_COUNTERS
                      if((odep->uop->timing.when_allocated != TICK_T_MAX)&&(!odep->uop->exec.ivalue_valid[odep->op_num]))
                      {
                        core->counters->RS.write++;
                      }
                      #endif

                      /* bypass output value to dependents */
                      odep->uop->exec.ivalue_valid[odep->op_num] = true;
                      if(odep->aflags) /* shouldn't happen for loads? */
                      {
                        warn("load modified flags?");
                        odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
                      }
                      else
                        odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
                      odep->uop->timing.when_ival_ready[odep->op_num] = core->sim_cycle+fp_penalty;
                      odep = odep->next;
                    }
                  }
                  FU->pipe[stage].load_enqueued = false;
                  FU->pipe[stage].load_received = true;
                  uop->exec.exec_complete = true;
                  break;
                }/* End of code for matching store buffer entry */
                else 
                {
                  /* If it does not match the store buffer but is marked as load_received, it means the uop has completed execution. */
                  if(FU->pipe[stage].load_received==true)
                  {
                    FU->pipe[stage].completed=true;
                    uop->exec.exec_complete =true;
                  }
                }
                j=modinc(j,knobs->exec.STQ_size);
              }/* End of the loop that searches the Store buffer for possible hits */  
              /* update load queue entry */
              if(FU->pipe[stage].load_received==true)
              {
#ifdef ZDEBUG
                if(core->sim_cycle > PRINT_CYCLE )
                  fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d Load Received and marked completed  PORT %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,i,uop->decode.in_fusion);
#endif             
                FU->pipe[stage].completed=true;
                uop->exec.exec_complete =true;
              }
            }/*End of Handling of load uop*/
            else
            {
              int fp_penalty = ((REG_IS_FPR(uop->decode.odep_name) && !(uop->decode.opflags & F_FCOMP)) ||
                (!REG_IS_FPR(uop->decode.odep_name) && (uop->decode.opflags & F_FCOMP)))?knobs->exec.fp_penalty:0;
              /* TODO: real execute-at-execute of instruction */
              /* XXX for now just copy oracle value */
              uop->exec.ovalue_valid = true;
              uop->exec.ovalue = uop->oracle.ovalue;
#ifdef ZDEBUG
              if(core->sim_cycle > PRINT_CYCLE )
                fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d Reached last stage of execution  PORT %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,i,uop->decode.in_fusion);
#endif             
              /* alloc, uopQ, and decode all have to search for the
                recovery point (Mop) because a long flow may have
                uops that span multiple sections of the latch.  */
              if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC) && (uop->Mop->decode.is_intr != true) )
              {
#ifdef ZDEBUG
                if(core->sim_cycle > PRINT_CYCLE )
                  fprintf(stdout,"\n[%lld][Core%d]Branch mispredicted ",core->sim_cycle,core->id);
#endif
                /* in case this instruction gets flushed more than once (jeclear followed by load-nuke) */
                uop->Mop->fetch.pred_NPC = uop->Mop->oracle.NextPC; 
                uop->Mop->fetch.branch_mispred = true;
                core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                ZESTO_STAT(core->stat.num_jeclear++;)
                if(uop->Mop->oracle.spec_mode)
                 ZESTO_STAT(core->stat.num_wp_jeclear++;)
#ifdef ZTRACE
                ztrace_print(uop,"e|jeclear|branch mispred detected at execute");
#endif
                uop->exec.exec_complete =true;
              }
              else if(uop->decode.is_sta) /* STQ to be treated as a store buffer */
              {
                if(STQ_available())      
                {
                  STQ_insert_sta(uop);
                  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
                  zesto_assert(!STQ[uop->alloc.STQ_index].addr_valid,(void)0);
                  STQ[uop->alloc.STQ_index].virt_addr = uop->oracle.virt_addr;
                  STQ[uop->alloc.STQ_index].addr_valid = true;
                  uop->exec.exec_complete =true;

                  #ifdef ZESTO_COUNTERS
                  core->counters->storeQ.write_tag++; // addr tag
                  #endif
                }
              }
              else if(uop->decode.is_std) /* STQ to be treated as a store buffer */
              {
                STQ_insert_std(uop);
                STQ[uop->alloc.STQ_index].value = uop->exec.ovalue;
                STQ[uop->alloc.STQ_index].value_valid = true;
                uop->exec.exec_complete =true;

                #ifdef ZESTO_COUNTERS
                core->counters->storeQ.write++; // data
                #endif
              }
              else /* Everything that is not a load or a store or a mispredicted branch */
              {
                uop->exec.exec_complete =true;
              }
              fflush(stdout);
              zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
              uop->timing.when_completed = core->sim_cycle+fp_penalty;

              /* bypass output value to dependents */
              struct odep_t * odep = uop->exec.odep_uop;

              while(odep)
              {
                #ifdef ZESTO_COUNTERS
                if((odep->uop->timing.when_allocated != TICK_T_MAX)&&(!odep->uop->exec.ivalue_valid[odep->op_num]))
                  core->counters->RS.write++;
                #endif

                if(uop->decode.fusion_head != odep->uop->decode.fusion_head)
                  zesto_assert(!odep->uop->exec.ivalue_valid[odep->op_num],(void)0);
                odep->uop->exec.ivalue_valid[odep->op_num] = true;
                if(odep->aflags)
                  odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
                else
                  odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
                odep->uop->timing.when_ival_ready[odep->op_num] = core->sim_cycle+fp_penalty;
                odep = odep->next;
              }
            } /* End of handling all uops that were not loads */
          }
          else  /* uop and FU->pipe[stage] action id do not match */
          {
#ifdef ZDEBUG
            if(core->sim_cycle > PRINT_CYCLE )
              fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d Squashed in_fusion %d ",core->sim_cycle,core->id,FU->pipe[stage].uop->decode.Mop_seq,FU->pipe[stage].uop->decode.uop_seq,FU->pipe[stage].uop->decode.in_fusion);
#endif
            FU->pipe[stage].uop =NULL;
            FU->pipe[stage].completed = false;
            FU->pipe[stage].load_enqueued = false;
            FU->pipe[stage].load_received = false;
            FU->occupancy--;
          }
          /* all_complete is a variable that captures if the load has been completed by store to load transfer 
             or after being returned from the cache(set in load_writeback)*/
          all_complete &=uop->exec.exec_complete;

          if(uop->decode.in_fusion)
          {
            uop = uop->decode.fusion_next;                  		
          }
          else /* Single uop occupying the stage and thus it has only one iteration of the while loop */
            uop = NULL;
			
        } /* End of while loop which iterates through all the fused uops that occupy the stage */

        /* If all the uops have been completed all_complete should be true */
        if(all_complete)
          FU->pipe[stage].completed=true;
      }
    }
  } /* End of the loop that goes through the last stages of all the ports and all the FUs marking completed uops in that port */


  /* Loops through all the last stages again and retire the uops from the Mop that is at the head of the commit queue */
  seq_t commit_seq =last_Mop_seq;
  bool all_done=false;
  for(int width=2;width>0;width--)
  {
    for(i=0;i<knobs->exec.num_exec_ports;i++)
    {
      int j;
      for(j=0;j<port[i].num_FU_types;j++)
      {
        enum md_fu_class FU_type = port[i].FU_types[j];
        struct ALU_t * FU = port[i].FU[FU_type];
        if(FU && (FU->occupancy > 0))
        {
          int stage = FU->latency-1;
          if(FU->pipe[stage].uop)
          {
            if( FU->pipe[stage].uop->decode.Mop_seq <= commit_seq)
            {
              if(FU->pipe[stage].completed==false)
              {
                all_done &=false;
              }
              else
              {
                if(FU->pipe[stage].uop->decode.in_fusion)
                {
                  struct uop_t * fusion_uop = FU->pipe[stage].uop->decode.fusion_head;
                  while(fusion_uop)
                  {
                    fusion_uop->exec.commit_done=true;
                    fusion_uop = fusion_uop->decode.fusion_next;
                  }
                }
                else
                {
                  FU->pipe[stage].uop->exec.commit_done=true;
                }
#ifdef ZDEBUG
                if(core->sim_cycle > PRINT_CYCLE )
                  fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d in exiting exec stage in_fusion %d ",core->sim_cycle,core->id,FU->pipe[stage].uop->decode.Mop_seq,FU->pipe[stage].uop->decode.uop_seq,FU->pipe[stage].uop->decode.in_fusion);
#endif
                FU->pipe[stage].uop =NULL;
                FU->pipe[stage].completed = false;
                FU->pipe[stage].load_enqueued = false;
                FU->pipe[stage].load_received = false;
                FU->occupancy--;
                last_completed=core->sim_cycle;
              }
            }
          }
        }
      }
    }
    if(!all_done)
      break;
    commit_seq++; /*2-way superscalar so we try to commit two Mops per cycle whereever possible*/
  }


  /* shuffle the rest of the stages forward */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    int j;
    for(j=0;j<port[i].num_FU_types;j++)
    {
      enum md_fu_class FU_type = port[i].FU_types[j];
      struct ALU_t * FU = port[i].FU[FU_type];
      if(FU && (FU->occupancy > 0))
      {
        int stage = FU->latency-1;
        if(FU->occupancy > 0)
        {
          for( /*nada*/; stage > 0; stage--)
          {
            if(FU->pipe[stage-1].uop && ( FU->pipe[stage].uop == NULL ) )
            {
#ifdef ZDEBUG
              if(core->sim_cycle > PRINT_CYCLE )
                fprintf(stdout,"\n[%lld][Core%d]EXEC shuffling stages Port %d FU_class %d MOP=%d UOP=%d stage%d to stage%d in_fusion %d MopCore %d ",core->sim_cycle,core->id,i,FU_type,FU->pipe[stage-1].uop->decode.Mop_seq,FU->pipe[stage-1].uop->decode.uop_seq,stage-1,stage,FU->pipe[stage-1].uop->decode.in_fusion,FU->pipe[stage-1].uop->Mop->core->id);
              if(core->sim_cycle > PRINT_CYCLE )
                md_print_insn(FU->pipe[stage-1].uop->Mop,stdout);
#endif
              if(FU->pipe[stage-1].uop->Mop->core->id ==core->id)
              {   
                FU->pipe[stage] = FU->pipe[stage-1];
                FU->pipe[stage-1].uop = NULL;
                zesto_assert(FU->pipe[stage-1].completed == false,(void)0);
              }
              else
              {
                FU->pipe[stage-1].uop = NULL;
              }
            }
          }
        }
      }
    }
  }        


  /* Process Payload RAM pipestages */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    if(port[i].occupancy > 0)
    {
      int stage = knobs->exec.payload_depth-1;
      struct uop_t * uop = port[i].payload_pipe[stage].uop;
      work_found = true;

      /* uops leaving payload section go to their respective FU's */
      if(uop && (port[i].payload_pipe[stage].action_id == uop->exec.action_id)) /* uop is valid, hasn't been squashed */
      {
        int j;
        int all_ready = true;
        enum md_fu_class FU_class = uop->decode.FU_class;

        for(j=0;j<MAX_IDEPS;j++)
          all_ready &= uop->exec.ivalue_valid[j];
  
        if(uop->decode.in_fusion)  /*Only move forward if all the fused uops are ready to move*/
        {
          struct uop_t * fusion_uop = uop->decode.fusion_head;
          zesto_assert(fusion_uop == uop, (void)0);
          while(fusion_uop)
          {
            for(int k=0;k<MAX_IDEPS;k++)
            {
              if(fusion_uop->exec.idep_uop[k])
                if(fusion_uop->oracle.idep_uop[k]->decode.in_fusion)
                  if(fusion_uop->oracle.idep_uop[k]->decode.fusion_head == fusion_uop->decode.fusion_head )
                    fusion_uop->exec.ivalue_valid[k] = true;
              all_ready &= fusion_uop->exec.ivalue_valid[k];
            }
            fusion_uop = fusion_uop->decode.fusion_next;
          }
        }	
	
        /* have all input values arrived and FU available? */
        if((!all_ready) || (port[i].FU[FU_class]->pipe[0].uop != NULL) || (port[i].FU[FU_class]->when_executable > core->sim_cycle))
        {
          for(j=0;j<MAX_IDEPS;j++)
          {
            if((!uop->exec.ivalue_valid[j]) && uop->oracle.idep_uop[j] && (uop->oracle.idep_uop[j]->timing.when_otag_ready < core->sim_cycle))
            {
              uop->timing.when_ready = core->sim_cycle+BIG_LATENCY;
            }
          }
          continue;
        }
        else  /*We have a uop or a fused uop that is ready and can be dispatched to the excution units*/
        {
          if(uop->decode.in_fusion)
            uop->decode.fusion_head->exec.uops_in_RS--;
#ifdef ZTRACE
          ztrace_print_start(uop,"e|payload|uop goes to ALU");
#endif
	
#ifdef ZTRACE
          ztrace_print_cont(", deallocates from RS");
#endif

          uop->timing.when_exec = core->sim_cycle;
	
          /* this port has the proper FU and the first stage is free. */
          zesto_assert(port[i].FU[FU_class] && (port[i].FU[FU_class]->pipe[0].uop == NULL),(void)0);
	
          port[i].FU[FU_class]->pipe[0].uop = uop;
          port[i].FU[FU_class]->pipe[0].action_id = uop->exec.action_id;
          port[i].FU[FU_class]->occupancy++;  /*occupancy is not incremented for each uop in a fused set*/
          port[i].FU[FU_class]->when_executable = core->sim_cycle + port[i].FU[FU_class]->issue_rate;
          port[i].FU[FU_class]->pipe[0].load_enqueued = false;
          port[i].FU[FU_class]->pipe[0].load_received = false;
          port[i].FU[FU_class]->pipe[0].completed = false;
          check_for_work = true;
          port[i].occupancy--;
          zesto_assert(port[i].occupancy >= 0,(void)0);

          #ifdef ZESTO_COUNTERS
          core->counters->FU[i].read++;
          #endif

          #ifdef ZESTO_COUNTERS
          core->counters->payload.read++;
          core->counters->latch.PR2FU.read++;
          #endif
        }
      }
      else if(uop && (port[i].payload_pipe[stage].action_id != uop->exec.action_id)) /* uop has been squashed */
      {
#ifdef ZTRACE
        ztrace_print(uop,"e|payload|on exit from payload, uop discovered to have been squashed");
#endif
        port[i].payload_pipe[stage].uop = NULL;
        port[i].occupancy--;
        zesto_assert(port[i].occupancy >= 0,(void)0);

        #ifdef ZESTO_COUNTERS
        core->counters->payload.write++;
        #endif
      }
	
      /* shuffle the other uops through the payload pipeline */
	
      for(/*nada*/; stage > 0; stage--)
      {
        port[i].payload_pipe[stage] = port[i].payload_pipe[stage-1];
        port[i].payload_pipe[stage-1].uop = NULL;
      }
      port[i].payload_pipe[0].uop = NULL;
    } /* End of if loop that  is executed if occupancy is more than zero on the port */ 
  } /* End of the big loop that goes through all the ports */
  check_for_work = work_found;
}

void core_exec_atom_t::recover(const struct Mop_t * const Mop)
{
  /* most flushing/squashing is accomplished through assignments of new action_id's */
}

void core_exec_atom_t::recover(void)
{
  /* most flushing/squashing is accomplished through assignments of new action_id's */
}

bool core_exec_atom_t::RS_available(int port)
{
//  struct core_knobs_t * knobs = core->knobs;
  //return RS_num < knobs->exec.RS_size;
  return(RS[port]==NULL);
}

/* assumes you already called RS_available to check that
   an entry is available */
void core_exec_atom_t::RS_insert(struct uop_t * const uop)
{
//  struct core_knobs_t * knobs = core->knobs;
 
  zesto_assert(RS[uop->alloc.port_assignment] == NULL ,(void)0);
  RS[uop->alloc.port_assignment] = uop;
  RS_num++;
  RS_eff_num++;
  uop->alloc.RS_index = uop->alloc.port_assignment;

  #ifdef ZESTO_COUNTERS
  core->counters->RS.write++;
  #endif

  if(uop->decode.in_fusion)
    uop->exec.uops_in_RS ++; /* used to track when RS can be deallocated */
  uop->alloc.full_fusion_allocated = false;
}

/* add uops to an existing entry (where all uops in the same
   entry are assumed to be fused) */
void core_exec_atom_t::RS_fuse_insert(struct uop_t * const uop)
{
  /* fusion body shares same RS entry as fusion head */
  uop->alloc.RS_index = uop->decode.fusion_head->alloc.RS_index;
  RS_eff_num++;
  uop->decode.fusion_head->exec.uops_in_RS ++;
  if(uop->decode.fusion_next == NULL)
    uop->decode.fusion_head->alloc.full_fusion_allocated = true;

  #ifdef ZESTO_COUNTERS
  core->counters->RS.write++;
  #endif
}

void core_exec_atom_t::RS_deallocate(struct uop_t * const dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert(dead_uop->alloc.RS_index < knobs->exec.RS_size,(void)0);
  if(dead_uop->decode.in_fusion && (dead_uop->timing.when_exec == TICK_T_MAX))
  {
    dead_uop->decode.fusion_head->exec.uops_in_RS --;
    RS_eff_num--;
  }
  if(!dead_uop->decode.in_fusion)
  {
    RS_eff_num--;
    zesto_assert(RS_eff_num >= 0,(void)0);
  }

  if(dead_uop->decode.is_fusion_head)
    zesto_assert(dead_uop->exec.uops_in_RS == 0,(void)0);

  if((!dead_uop->decode.in_fusion) || dead_uop->decode.is_fusion_head) /* make head uop responsible for deallocating RS during recovery */
  {
    RS[dead_uop->alloc.RS_index] = NULL;
    RS_num --;
    zesto_assert(RS_num >= 0,(void)0);

    #ifdef ZESTO_COUNTERS
    core->counters->RS.write_tag++;
    #endif
  }
  dead_uop->alloc.RS_index = -1;
}

bool core_exec_atom_t::LDQ_available(void)
{
  return true;
}

void core_exec_atom_t::LDQ_insert(struct uop_t * const uop)
{

}

/* called by commit */
void core_exec_atom_t::LDQ_deallocate(struct uop_t * const uop)
{

}

void core_exec_atom_t::LDQ_squash(struct uop_t * const dead_uop)
{

}

bool core_exec_atom_t::STQ_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return STQ_num < knobs->exec.STQ_size;
}

void core_exec_atom_t::STQ_insert_sta(struct uop_t * const uop)
{
  struct core_knobs_t * knobs = core->knobs;
  memzero(&STQ[STQ_tail],sizeof(*STQ));
  STQ[STQ_tail].sta = uop;
  if(STQ[STQ_tail].sta != NULL)
    uop->decode.is_sta = true;
  STQ[STQ_tail].mem_size = uop->decode.mem_size;
  STQ[STQ_tail].uop_seq = uop->decode.uop_seq;
  STQ[STQ_tail].value_valid= true;
  uop->alloc.STQ_index = STQ_tail;
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
	fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d put store in SB  STA %llx in_fusion %d STQ entry %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uop->oracle.phys_addr,uop->decode.in_fusion,STQ_tail);
#endif
  STQ_num++;
  STQ_tail = modinc(STQ_tail,knobs->exec.STQ_size); //(STQ_tail+1) % knobs->exec.STQ_size;

  #ifdef ZESTO_COUNTERS
  core->counters->storeQ.write++;
  #endif
}

void core_exec_atom_t::STQ_insert_std(struct uop_t * const uop)
{
   struct core_knobs_t * knobs = core->knobs;
  /* STQ_tail already incremented from the STA.  Just add this uop to STQ->std */
  int index = moddec(STQ_tail,knobs->exec.STQ_size); //(STQ_tail - 1 + knobs->exec.STQ_size) % knobs->exec.STQ_size;
  uop->alloc.STQ_index = index;
  STQ[index].std = uop;
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
	fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d put store in SB  STD %llx in_fusion %d STQ entry %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uop->oracle.phys_addr,uop->decode.in_fusion,index);
#endif
}

void core_exec_atom_t::STQ_deallocate_sta(void)
{
}

/* returns true if successful */
bool core_exec_atom_t::STQ_deallocate_std(struct uop_t * const uop)
{
//  struct core_knobs_t * knobs = core->knobs;
  return true;
}

void core_exec_atom_t::STQ_deallocate_senior(void)
{
  struct core_knobs_t * knobs = core->knobs;


  struct uop_t * uop = STQ[STQ_head].sta;

  if(uop && STQ[STQ_head].addr_valid && STQ[STQ_head].value_valid)
  {
	    STQ[STQ_head].translation_complete = true; //TODO No support fot TLBs yet so we assume translation complete, otherwise this would be false
        STQ[STQ_head].write_complete = false;

       ///*The mem request to send the load to the cache*/
       cache_req* request = new cache_req(core->request_id, core->id, uop->oracle.phys_addr, OpMemSt, INITIAL);

       #ifdef ZESTO_COUNTERS
       core->counters->latch.SQ2L1.read++;
       core->counters->storeQ.read++;
       #endif

      /** Issue a new Preq and corresponding Mreq.  */
      core_t::Preq preq;
      preq.index = uop->alloc.STQ_index;
      preq.thread_id = core->current_thread->id;
      preq.action_id = STQ[STQ_head].action_id;
      core->my_map.insert(std::pair<int, core_t::Preq>(core->request_id, preq));
      core->request_id++;
      #ifdef ZDEBUG
      if(core->sim_cycle > PRINT_CYCLE )
	    fprintf(stdout,"\n[%lld][Core%d]EXEC MOP=%d UOP=%d store_enqueue %llx in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uop->oracle.phys_addr,uop->decode.in_fusion);
      #endif             
	    /*Actually enqueueing the request to cache*/
      core->cache_link->send(request); // sendtick? 


#ifdef ZTRACE
    ztrace_print(uop,"c|store|store enqueued to DL1/DTLB");
#endif
    STQ[STQ_head].sta=NULL;
    STQ[STQ_head].std=NULL;
    STQ[STQ_head].value_valid = false;
    STQ_num --;
    STQ_head = modinc(STQ_head,knobs->exec.STQ_size); //(STQ_head+1) % knobs->exec.STQ_size;

    #ifdef ZESTO_COUNTERS
    core->counters->storeQ.write_tag++;
    #endif
  }
}

void core_exec_atom_t::STQ_squash_sta(struct uop_t * const dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((dead_uop->alloc.STQ_index >= 0) && (dead_uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].std == NULL,(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].sta == dead_uop,(void)0);
  memzero(&STQ[dead_uop->alloc.STQ_index],sizeof(STQ[0]));
  STQ_num --;
  STQ_senior_num --;
  STQ_tail = moddec(STQ_tail,knobs->exec.STQ_size); //(STQ_tail - 1 + knobs->exec.STQ_size) % knobs->exec.STQ_size;
  zesto_assert(STQ_num >= 0,(void)0);
  zesto_assert(STQ_senior_num >= 0,(void)0);
  dead_uop->alloc.STQ_index = -1;

  #ifdef ZESTO_COUNTERS
  core->counters->storeQ.write_tag++;
  #endif
}

void core_exec_atom_t::STQ_squash_std(struct uop_t * const dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((dead_uop->alloc.STQ_index >= 0) && (dead_uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].std == dead_uop,(void)0);
  STQ[dead_uop->alloc.STQ_index].std = NULL;
  dead_uop->alloc.STQ_index = -1;
}

void core_exec_atom_t::STQ_squash_senior(void)
{
  struct core_knobs_t * knobs = core->knobs;

  while(STQ_senior_num > 0)
  {
    memzero(&STQ[STQ_senior_head],sizeof(*STQ));
    STQ[STQ_senior_head].action_id = core->new_action_id();

    if((STQ_senior_head == STQ_head) && (STQ_num>0))
    {
      STQ_head = modinc(STQ_head,knobs->exec.STQ_size); //(STQ_head + 1) % knobs->exec.STQ_size;
      STQ_num--;

      #ifdef ZESTO_COUNTERS
      core->counters->storeQ.write_tag++;
      #endif
    }
    STQ_senior_head = modinc(STQ_senior_head,knobs->exec.STQ_size); //(STQ_senior_head + 1) % knobs->exec.STQ_size;
    STQ_senior_num--;
  }
}

void core_exec_atom_t::recover_check_assertions(void)
{
  zesto_assert(STQ_senior_num == 0,(void)0);
  zesto_assert(STQ_num == 0,(void)0);
  zesto_assert(LDQ_num == 0,(void)0);
  zesto_assert(RS_num == 0,(void)0);
}

bool core_exec_atom_t::has_load_arrived(int LDQ_index)
{
	return(LDQ[LDQ_index].last_byte_arrived & LDQ[LDQ_index].first_byte_arrived);
}

unsigned long long int core_exec_atom_t::get_load_latency(int LDQ_index)
{
	return (LDQ[LDQ_index].when_issued );
}

/* Stores don't write back to cache/memory until commit.  When D$
   and DTLB accesses complete, these functions get called which
   update the status of the corresponding STQ entries.  The STQ
   entry cannot be deallocated until the store has completed. */
void core_exec_atom_t::store_dl1_callback(void * const op)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_exec_atom_t * E = (core_exec_atom_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;

#ifdef ZTRACE
  ztrace_print(uop,"c|store|written to cache/memory");
#endif

  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  if(uop->exec.action_id == E->STQ[uop->alloc.STQ_index].action_id)
  {
    E->STQ[uop->alloc.STQ_index].first_byte_written = true;
    if(E->STQ[uop->alloc.STQ_index].last_byte_written)
      E->STQ[uop->alloc.STQ_index].write_complete = true;
  }
  core->return_uop_array(uop);
}

/* only used for the 2nd part of a split write */
void core_exec_atom_t::store_dl1_split_callback(void * const op)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_exec_atom_t * E = (core_exec_atom_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;

#ifdef ZTRACE
  ztrace_print(uop,"c|store|written to cache/memory");
#endif

  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  if(uop->exec.action_id == E->STQ[uop->alloc.STQ_index].action_id)
  {
    E->STQ[uop->alloc.STQ_index].last_byte_written = true;
    if(E->STQ[uop->alloc.STQ_index].first_byte_written)
      E->STQ[uop->alloc.STQ_index].write_complete = true;
  }
  core->return_uop_array(uop);
}

void core_exec_atom_t::store_dtlb_callback(void * const op)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_exec_atom_t * E = (core_exec_atom_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;
  
#ifdef ZTRACE
  ztrace_print(uop,"c|store|translated");
#endif

  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  if(uop->exec.action_id == E->STQ[uop->alloc.STQ_index].action_id)
    E->STQ[uop->alloc.STQ_index].translation_complete = true;
  core->return_uop_array(uop);
}

bool core_exec_atom_t::store_translated_callback(void * const op, const seq_t action_id /* ignored */)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_knobs_t * knobs = core->knobs;
  struct core_exec_atom_t * E = (core_exec_atom_t*)core->exec;

  if((uop->alloc.STQ_index == -1) || (uop->exec.action_id != E->STQ[uop->alloc.STQ_index].action_id))
    return true;
  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),true);
  return E->STQ[uop->alloc.STQ_index].translation_complete;
}

void core_exec_atom_t::memory_callbacks(void)
{

  while (core->m_input_from_cache.size() != 0)
  {

    cache_req* request = core->m_input_from_cache.front();
    zesto_assert(core->my_map.find(request->req_id) != core->my_map.end(), (void)0);
    core_t::Preq preq = (core->my_map.find(request->req_id))->second;
    core->my_map.erase(core->my_map.find(request->req_id));

    /* Retire in that context */
    zesto_assert(preq.thread_id == core->current_thread->id && "Requests thread_id does not match the running thread", (void)0);
   
    if(request->op_type == OpMemLd) //Handling Loads
    {
      struct uop_t * uop = LDQ[preq.index].uop;

      if(uop != NULL)
      {
        if(preq.action_id == uop->exec.action_id) // a valid load
        {
          DL1_callback(uop);
          DL1_split_callback(uop);
          DTLB_callback(uop);
        }
      }
    }    

    delete(request);
    core->m_input_from_cache.pop_front();
  }

}


#endif
