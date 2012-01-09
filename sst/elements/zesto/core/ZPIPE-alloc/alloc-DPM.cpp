/* alloc-DPM.cpp - Detailed Pipeline Model */
/*
 * __COPYRIGHT__ GT
 */

#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(alloc_opt_string,"DPM"))
    return new core_alloc_DPM_t(core);
#else

class core_alloc_DPM_t:public core_alloc_t
{
  enum alloc_stall_t {ASTALL_NONE,   /* no stall */
                      ASTALL_EMPTY,
                      ASTALL_ROB,
                      ASTALL_LDQ,
                      ASTALL_STQ,
                      ASTALL_RS,
                      ASTALL_DRAIN,
                      ASTALL_num
                     };

  public:

  core_alloc_DPM_t(struct core_t * const core);
  virtual void reg_stats(struct stat_sdb_t * const sdb);

  virtual void step(void);
  virtual void recover(void);
  virtual void recover(const struct Mop_t * const Mop);

  virtual void RS_deallocate(const struct uop_t * const uop);
  virtual void start_drain(void); /* prevent allocation from proceeding to exec */

  protected:

  struct uop_t *** pipe;
  int * occupancy;
  /* for load-balancing port binding */
  int * port_loading;

  static const char *alloc_stall_str[ASTALL_num];
};

const char *core_alloc_DPM_t::alloc_stall_str[ASTALL_num] = {
  "no stall                   ",
  "no uops to allocate        ",
  "ROB is full                ",
  "LDQ is full                ",
  "STQ is full                ",
  "RS is full                 ",
  "OOO core draining          "
};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_alloc_DPM_t::core_alloc_DPM_t(struct core_t * const arg_core)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;
  int i;

  if(knobs->alloc.depth <= 0)
    fatal("allocation pipeline depth must be > 0");
  if(knobs->alloc.width <= 0)
    fatal("allocation pipeline width must be > 0");

  pipe = (struct uop_t***) calloc(knobs->alloc.depth,sizeof(*pipe));
  if(!pipe)
    fatal("couldn't calloc alloc pipe");

  for(i=0;i<knobs->alloc.depth;i++)
  {
    pipe[i] = (struct uop_t**) calloc(knobs->alloc.width,sizeof(**pipe));
    if(!pipe[i])
      fatal("couldn't calloc alloc pipe stage");
  }

  occupancy = (int*) calloc(knobs->alloc.depth,sizeof(*occupancy));
  if(!occupancy)
    fatal("couldn't calloc alloc occupancy array");

  port_loading = (int*) calloc(knobs->exec.num_exec_ports,sizeof(*port_loading));
  if(!port_loading)
    fatal("couldn't calloc allocation port-loading scoreboard");
}

void
core_alloc_DPM_t::reg_stats(struct stat_sdb_t * const sdb)
{
  char buf[1024];
  char buf2[1024];

  sprintf(buf,"\n#### Zesto Core#%d Alloc Stats ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.alloc_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions alloced", &core->stat.alloc_insn, core->stat.alloc_insn, NULL);
  sprintf(buf,"c%d.alloc_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of uops alloced", &core->stat.alloc_uops, core->stat.alloc_uops, NULL);
  sprintf(buf,"c%d.alloc_eff_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of effective uops alloced", &core->stat.alloc_eff_uops, core->stat.alloc_eff_uops, NULL);
  sprintf(buf,"c%d.alloc_IPC",core->id);
  sprintf(buf2,"c%d.alloc_insn/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "IPC at alloc", buf2, NULL);
  sprintf(buf,"c%d.alloc_uPC",core->id);
  sprintf(buf2,"c%d.alloc_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uPC at alloc", buf2, NULL);
  sprintf(buf,"c%d.alloc_euPC",core->id);
  sprintf(buf2,"c%d.alloc_eff_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "effective uPC at alloc", buf2, NULL);

  sprintf(buf,"c%d.alloc_stall",core->current_thread->id);
  core->stat.alloc_stall = stat_reg_dist(sdb, buf,
                                          "breakdown of stalls at alloc",
                                          /* initial value */0,
                                          /* array size */ASTALL_num,
                                          /* bucket size */1,
                                          /* print format */(PF_COUNT|PF_PDF),
                                          /* format */NULL,
                                          /* index map */alloc_stall_str,
                                          /* print fn */NULL);
}

/************************/
/* MAIN ALLOC FUNCTIONS */
/************************/

void core_alloc_DPM_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage, i;
  enum alloc_stall_t stall_reason = ASTALL_NONE;

  /*========================================================================*/
  /*== Dispatch insts if ROB, RS, and LQ/SQ entries available (as needed) ==*/
  stage = knobs->alloc.depth-1;
  if(occupancy[stage]) /* are there uops in the last stage of the alloc pipe? */
  {
    for(i=0; i < knobs->alloc.width; i++) /* if so, scan all slots (width) of this stage */
    {
      struct uop_t * uop = pipe[stage][i];
      int abort_alloc = false;

      /* if using drain flush: */
      /* is the back-end still draining from a misprediction recovery? */
      if(knobs->alloc.drain_flush && drain_in_progress)
      {
        if(!core->commit->ROB_empty())
        {
          stall_reason = ASTALL_DRAIN;
          break;
        }
        else
          drain_in_progress = false;
      }

      if(uop)
      {
        while(uop) /* this while loop is to handle multiple uops fused together into the same slot */
        {
          if(uop->timing.when_allocated == TICK_T_MAX)
          {
            /* is the ROB full? */
            if((!uop->decode.in_fusion||uop->decode.fusion_head) && !core->commit->ROB_available())
            {
              stall_reason = ASTALL_ROB;
              abort_alloc = true;
              break;
            }

            /* for loads, is the LDQ full? */
            if(uop->decode.is_load && !core->exec->LDQ_available())
            {
              stall_reason = ASTALL_LDQ;
              abort_alloc = true;
              break;
            }
            /* for stores, allocate STQ entry on STA.  NOTE: This is different from 
               Bob Colwell's description in Shen&Lipasti Chap 7 where he describes
               allocation on STD.  We emit STA uops first since the oracle needs to
               use the STA result to feed the following STD uop. */
            if(uop->decode.is_sta && !core->exec->STQ_available())
            {
              stall_reason = ASTALL_STQ;
              abort_alloc = true;
              break;
            }

            /* is the RS full? -- don't need to alloc for NOP's */
            if(!core->exec->RS_available(0) && !uop->decode.is_nop)
            {
              stall_reason = ASTALL_RS;
              abort_alloc = true;
              break;
            }

            /* ALL ALLOC STALL CONDITIONS PASSED */

            /* place in ROB */
            if((!uop->decode.in_fusion) || uop->decode.is_fusion_head)
              core->commit->ROB_insert(uop);
            else /* fusion body doesn't occupy additional ROB entries */
              core->commit->ROB_fuse_insert(uop);

            /* place in LDQ/STQ if needed */
            if(uop->decode.is_load)
              core->exec->LDQ_insert(uop);
            else if(uop->decode.is_sta)
              core->exec->STQ_insert_sta(uop);
            else if(uop->decode.is_std)
              core->exec->STQ_insert_std(uop);

            /* all store uops had better be marked is_std */
            zesto_assert((!(uop->decode.opflags & F_STORE)) || uop->decode.is_std,(void)0);
            zesto_assert((!(uop->decode.opflags & F_LOAD)) || uop->decode.is_load,(void)0);

            /* port bindings */
            if(!uop->decode.is_nop && !uop->Mop->decode.is_trap)
            {
              /* port-binding is trivial when there's only one valid port */
              if(knobs->exec.port_binding[uop->decode.FU_class].num_FUs == 1)
              {
                uop->alloc.port_assignment = knobs->exec.port_binding[uop->decode.FU_class].ports[0];
              }
              else /* else assign uop to least loaded port */
              {
                int min_load = INT_MAX;
                 int index = -1;
                for(int j=0;j<knobs->exec.port_binding[uop->decode.FU_class].num_FUs;j++)
                {
                  int port = knobs->exec.port_binding[uop->decode.FU_class].ports[j];
                  if(port_loading[port] < min_load)
                  {
                    min_load = port_loading[port];
                    index = port;
                  }
                }
                uop->alloc.port_assignment = index;
              }
              port_loading[uop->alloc.port_assignment]++;

              /* only allocate for non-fused or fusion-head */
              if((!uop->decode.in_fusion) || uop->decode.is_fusion_head)
                core->exec->RS_insert(uop);
              else
                core->exec->RS_fuse_insert(uop);

              /* Get input mappings - this is a proxy for explicit register numbers, which
                 you can always get from idep_uop->alloc.ROB_index */
              for(int j=0;j<MAX_IDEPS;j++)
              {
                /* This use of oracle info is valid: at this point the processor would be
                   looking up this information in the RAT, but this saves us having to
                   explicitly store/track the RAT state. */
                uop->exec.idep_uop[j] = uop->oracle.idep_uop[j];

                /* Add self onto parent's output list.  This output list doesn't
                   have a real microarchitectural counter part, but it makes the
                   simulation faster by not having to perform a whole mess of
                   associative searches each time any sort of broadcast is needed.
                   The parent's odep list only points to uops which have dispatched
                   into the OOO core (i.e. has left the alloc pipe). */
                if(uop->exec.idep_uop[j])
                {
                  struct odep_t * odep = core->get_odep_link();
                  odep->next = uop->exec.idep_uop[j]->exec.odep_uop;
                  uop->exec.idep_uop[j]->exec.odep_uop = odep;
                  odep->uop = uop;
                  odep->aflags = (uop->decode.idep_name[j] == DCREG(MD_REG_AFLAGS));
                  odep->op_num = j;

                  #ifdef ZESTO_COUNTERS
                  // input registers renaming
                  if((uop->decode.idep_name[j] != DNA)&&(uop->decode.idep_name[j] != MD_REG_ZERO))
                  {
                    core->counters->RAT.read++;
                    core->counters->operand_dependency.read++;
                  }
                  #endif
                }
              }

              #ifdef ZESTO_COUNTERS
              // output register renaming
              if((uop->decode.odep_name != DNA)&&(uop->decode.odep_name != MD_REG_ZERO))
              {
                core->counters->freelist.read++;
                core->counters->RAT.write++;
              }
              core->counters->latch.RR2RS.read++;
              #endif

              /* check "scoreboard" for operand readiness (we're not actually
                 explicitly implementing a scoreboard); if value is ready, read
                 it into data-capture window or payload RAM. */
              tick_t when_ready = 0;
              for(int j=0;j<MAX_IDEPS;j++) /* for possible input argument */
              {
                if(uop->exec.idep_uop[j]) /* if the parent uop exists (i.e., still in the processor) */
                {
                  uop->timing.when_itag_ready[j] = uop->exec.idep_uop[j]->timing.when_otag_ready;
                  if(uop->exec.idep_uop[j]->exec.ovalue_valid)
                  {
                    #ifdef ZESTO_COUNTERS
                    if(!uop->exec.ivalue_valid[j])
                    {
                      core->counters->RS.write++;
                      core->counters->ROB.read++;
                      core->counters->latch.ROB2RS.read++;
                    }
                    #endif

                    uop->timing.when_ival_ready[j] = uop->exec.idep_uop[j]->timing.when_completed;
                    uop->exec.ivalue_valid[j] = true;
                    if(uop->decode.idep_name[j] == DCREG(MD_REG_AFLAGS))
                      uop->exec.ivalue[j].dw = uop->exec.idep_uop[j]->exec.oflags;
                    else
                      uop->exec.ivalue[j] = uop->exec.idep_uop[j]->exec.ovalue;
                  }
                }
                else /* read from ARF */
                {
                  uop->timing.when_itag_ready[j] = core->sim_cycle;
                  uop->timing.when_ival_ready[j] = core->sim_cycle;

                  if(uop->decode.idep_name[j] != DNA)
                  {
                    uop->exec.ivalue[j] = uop->oracle.ivalue[j]; /* oracle value == architected value */

                    #ifdef ZESTO_COUNTERS
                    if((uop->decode.idep_name[j] != MD_REG_ZERO)&&(!uop->exec.ivalue_valid[j]))
                    {
                      core->counters->registers.read++;
                      if(REG_IS_GPR(uop->decode.idep_name[j]))
                        core->counters->registers.integer.read++;
                      else if(REG_IS_FPR(uop->decode.idep_name[j]))
                        core->counters->registers.floating.read++;
                      else if(REG_IS_SEG(uop->decode.idep_name[j]))
                        core->counters->registers.segment.read++;
                      else if(REG_IS_CREG(uop->decode.idep_name[j]))
                        core->counters->registers.control.read++;

                      core->counters->latch.ARF2RS.read++;
                    }
                    if(uop->decode.iflags && uop->decode.iflags != DNA)
                      core->counters->registers.flag.read++;
                    #endif
                  }
                  uop->exec.ivalue_valid[j] = true; /* applies to invalid (DNA) inputs as well */
                }
                if(when_ready < uop->timing.when_itag_ready[j])
                  when_ready = uop->timing.when_itag_ready[j];
              }
              uop->timing.when_ready = when_ready;
              if(when_ready < TICK_T_MAX) /* add to readyQ if appropriate */
	      {
		uop->timing.when_issued = TICK_T_MAX;
                core->exec->insert_ready_uop(uop);
	      }

            }
            else /* is_nop || is_trap */
            {
              /* NOP's don't go through exec pipeline; they go straight to the
                 ROB and are immediately marked as completed (they still take
                 up space in the ROB though). */
              /* Since traps/interrupts aren't really properly modeled in SimpleScalar, we just let
                 it go through without doing anything. */
              uop->timing.when_ready = core->sim_cycle;
              uop->timing.when_issued = core->sim_cycle;
              uop->timing.when_completed = core->sim_cycle;
            }

            uop->timing.when_allocated = core->sim_cycle;

#ifdef ZTRACE
            ztrace_print_start(uop,"a|alloc:ROB=%d,",uop->alloc.ROB_index);
            if(uop->alloc.RS_index == -1) // nop
              ztrace_print_cont("RS=.");
            else
              ztrace_print_cont("RS=%d",uop->alloc.RS_index);
            if(uop->decode.in_fusion && !uop->decode.is_fusion_head)
              ztrace_print_cont("f");
            if(uop->alloc.LDQ_index == -1)
              ztrace_print_cont(":LDQ=.");
            else
              ztrace_print_cont(":LDQ=%d",uop->alloc.LDQ_index);
            if(uop->alloc.STQ_index == -1)
              ztrace_print_cont(":STQ=.");
            else
              ztrace_print_cont(":STQ=%d",uop->alloc.STQ_index);
            ztrace_print_cont(":pb=%d",uop->alloc.port_assignment);
            ztrace_print_finish("|uop alloc'd and dispatched");
#endif

          }

          if(uop->decode.in_fusion)
            uop = uop->decode.fusion_next;
          else
            uop = NULL;
        }

        if(abort_alloc)
          break;

        if((!pipe[stage][i]->decode.in_fusion) || !uop) /* either not fused, or complete fused uops alloc'd */
        {
          uop = pipe[stage][i]; /* may be NULL if we just finished a fused set */

          /* update stats */
          if(uop->decode.EOM)
            ZESTO_STAT(core->stat.alloc_insn++;)

          ZESTO_STAT(core->stat.alloc_uops++;)
          if(uop->decode.in_fusion)
            ZESTO_STAT(core->stat.alloc_eff_uops += uop->decode.fusion_size;)
          else
            ZESTO_STAT(core->stat.alloc_eff_uops++;)

          /* remove from alloc pipe */
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
    }
  }
  else
    stall_reason = ASTALL_EMPTY;


  /*=============================================*/
  /*== Shuffle uops down the rename/alloc pipe ==*/

  /* walk pipe backwards */
  for(stage=knobs->alloc.depth-1; stage > 0; stage--)
  {
    if(0 == occupancy[stage]) /* implementing non-serpentine pipe (no compressing) - can't advance until stage is empty */
    {
      /* move everyone from previous stage forward */
      for(i=0;i<knobs->alloc.width;i++)
      {
        pipe[stage][i] = pipe[stage-1][i];
        pipe[stage-1][i] = NULL;
        if(pipe[stage][i])
        {
          occupancy[stage]++;
          occupancy[stage-1]--;
          zesto_assert(occupancy[stage] <= knobs->alloc.width,(void)0);
          zesto_assert(occupancy[stage-1] >= 0,(void)0);
        }
      }
    }
  }

  /*==============================================*/
  /*== fill first alloc stage from decode stage ==*/
  if(0 == occupancy[0])
  {
    /* while the uopQ sitll has uops in it, allocate up to alloc.width uops per cycle */
    for(i=0;(i<knobs->alloc.width) && core->decode->uop_available();i++)
    {
      pipe[0][i] = core->decode->uop_peek(); core->decode->uop_consume();
      occupancy[0]++;
      zesto_assert(occupancy[0] <= knobs->alloc.width,(void)0);
#ifdef ZTRACE
      ztrace_print(pipe[0][i],"a|alloc-pipe|enqueue");
#endif
    }
  }

  ZESTO_STAT(stat_add_sample(core->stat.alloc_stall, (int)stall_reason);)
}

/* start from most recently fetched, blow away everything until
   we find the Mop */
void
core_alloc_DPM_t::recover(const struct Mop_t * const Mop)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage,i;
  for(stage=0;stage<knobs->alloc.depth;stage++)
  {
    /* slot N-1 is most speculative, start from there */
    if(occupancy[stage])
      for(i=knobs->alloc.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          if(pipe[stage][i]->Mop == Mop)
            return;
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
  }
}

/* clean up on pipeline flush (blow everything away) */
void
core_alloc_DPM_t::recover(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage,i;
  for(stage=0;stage<knobs->alloc.depth;stage++)
  {
    /* slot N-1 is most speculative, start from there */
    if(occupancy[stage])
      for(i=knobs->alloc.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
    zesto_assert(occupancy[stage] == 0,(void)0);
  }
}

void core_alloc_DPM_t::RS_deallocate(const struct uop_t * const uop)
{
  zesto_assert(port_loading[uop->alloc.port_assignment] > 0,(void)0);
  port_loading[uop->alloc.port_assignment]--;
}

void core_alloc_DPM_t::start_drain(void)
{
  drain_in_progress = true;
}

#endif
