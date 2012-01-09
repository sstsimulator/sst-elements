/* decode-atom.cpp - Detailed Pipeline Model */
/*
 * __COPYRIGHT__ GT
 */

#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(decode_opt_string,"ATOM"))
    return new core_decode_atom_t(core);
#else

class core_decode_atom_t:public core_decode_t
{
  enum decode_stall_t {DSTALL_NONE,   /* no stall */
                       DSTALL_FULL,   /* first decode stage is full */
                       DSTALL_REP,    /* REP inst waiting for microsequencer */
                       DSTALL_UROM,   /* big flow waiting for UROM */
                       DSTALL_SMALL,  /* available decoder is too small */
                       DSTALL_EMPTY,  /* no insts to decode */
                       DSTALL_PHANTOM,/* front-end predicted taken when no branch was present */
                       DSTALL_TARGET, /* target correction flushed remaining insts (only occurs when target_stage == crack_stage-1) */
                       DSTALL_MAX_BRANCHES, /* exceeded maximum number of branches decoded per cycle */
                       DSTALL_num
                      };

  public:

  /* constructor, stats registration */
  core_decode_atom_t(struct core_t * const core);
  virtual void reg_stats(struct stat_sdb_t * const sdb);
  virtual void update_occupancy(void);

  virtual void step(void);
  virtual void recover(void);
  virtual void recover(struct Mop_t * const Mop);

  /* interface functions for alloc stage */
  virtual bool uop_available(void);
  virtual struct uop_t * uop_peek(void);
  virtual void uop_consume(void);

  protected:

  struct Mop_t *** pipe; /* the actual decode pipe */
  int * occupancy;

  struct uop_t ** uopQ; /* sits between decode and alloc */
  int uopQ_head;
  int uopQ_tail;
  int uopQ_num;
  int uopQ_eff_num;

  static const char * decode_stall_str[DSTALL_num];

  enum decode_stall_t check_target(struct Mop_t * const Mop);
  bool check_flush(const int stage, const int idx);
  void recover_decode_pipe(const struct Mop_t * const Mop);
  void recover_decode_pipe(void);
  void recover_uopQ(const struct Mop_t * const Mop);
  void recover_uopQ(void);
};

const char * core_decode_atom_t::decode_stall_str[DSTALL_num] = {
  "no stall                   ",
  "next decode stage is full  ",
  "REP inst waiting for MS    ",
  "long flow waiting for UROM ",
  "available decoder too small",
  "no insts to decode         ",
  "phantom taken branch       ",
  "target correction          ",
  "branch decode limit        ",
};

core_decode_atom_t::core_decode_atom_t(struct core_t * const arg_core):
  uopQ_head(0), uopQ_tail(0), uopQ_num(0), uopQ_eff_num(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  if(knobs->decode.depth <= 0)
    fatal("decode pipe depth must be > 0");

  if((knobs->decode.width <= 0) || (knobs->decode.width > MAX_DECODE_WIDTH))
    fatal("decode pipe width must be > 0 and < %d (change MAX_DECODE_WIDTH if you want more)",MAX_DECODE_WIDTH);

  if(knobs->decode.target_stage <= 0 || knobs->decode.target_stage >= knobs->decode.depth)
    fatal("decode target resteer stage (%d) must be > 0, and less than decode pipe depth (currently set to %d)",knobs->decode.target_stage,knobs->decode.depth);

  /* if the pipe is N wide, we assume there are N decoders */
  pipe = (struct Mop_t***) calloc(knobs->decode.depth,sizeof(*pipe));
  if(!pipe)
    fatal("couldn't calloc decode pipe");

  for(int i=0;i<knobs->decode.depth;i++)
  {
    pipe[i] = (struct Mop_t**) calloc(knobs->decode.width,sizeof(**pipe));
    if(!pipe[i])
      fatal("couldn't calloc decode pipe stage");
  }

  occupancy = (int*) calloc(knobs->decode.depth,sizeof(*occupancy));
  if(!occupancy)
    fatal("couldn't calloc decode pipe occupancy array");

  knobs->decode.max_uops = (int*) calloc(knobs->decode.width,sizeof(*knobs->decode.max_uops));
  if(!knobs->decode.max_uops)
    fatal("couldn't calloc decode.max_uops");
  if(knobs->decode.width != knobs->decode.num_decoder_specs)
    fatal("number of decoder specifications must be equal to decode pipeline width");
  for(int i=0;i<knobs->decode.width;i++)
    knobs->decode.max_uops[i] = knobs->decode.decoders[i];

  uopQ = (struct uop_t**) calloc(knobs->decode.uopQ_size,sizeof(*uopQ));
  if(!uopQ)
    fatal("couldn't calloc uopQ");

  if(knobs->decode.fusion_none)
    knobs->decode.fusion_mode = 0x00000000;
  else if(knobs->decode.fusion_all)
    knobs->decode.fusion_mode = 0xffffffff;
  else
  {
    knobs->decode.fusion_mode = FUSION_NONE;
    if(knobs->decode.fusion_load_op)
      knobs->decode.fusion_mode |= FUSION_LOAD_OP;
    if(knobs->decode.fusion_sta_std)
      knobs->decode.fusion_mode |= FUSION_STA_STD;
    if(knobs->decode.fusion_partial)
      knobs->decode.fusion_mode |= FUSION_PARTIAL;
  }
}

void
core_decode_atom_t::reg_stats(struct stat_sdb_t * const sdb)
{
  char buf[1024];
  char buf2[1024];

  sprintf(buf,"\n#### Zesto Core#%d Decode Stats ####",core->id);
  stat_reg_note(sdb,buf);
  sprintf(buf,"c%d.target_resteers",core->id);
  stat_reg_counter(sdb, true, buf, "decode-time target resteers", &core->stat.target_resteers, core->stat.target_resteers, NULL);
  sprintf(buf,"c%d.phantom_resteers",core->id);
  stat_reg_counter(sdb, true, buf, "decode-time phantom resteers", &core->stat.phantom_resteers, core->stat.phantom_resteers, NULL);
  sprintf(buf,"c%d.decode_insn",core->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions decodeed", &core->stat.decode_insn, core->stat.decode_insn, NULL);
  sprintf(buf,"c%d.decode_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of uops decodeed", &core->stat.decode_uops, core->stat.decode_uops, NULL);
  sprintf(buf,"c%d.decode_eff_uops",core->id);
  stat_reg_counter(sdb, true, buf, "total number of effective uops decodeed", &core->stat.decode_eff_uops, core->stat.decode_eff_uops, NULL);
  sprintf(buf,"c%d.decode_IPC",core->id);
  sprintf(buf2,"c%d.decode_insn/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "IPC at decode", buf2, NULL);
  sprintf(buf,"c%d.decode_uPC",core->id);
  sprintf(buf2,"c%d.decode_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "uPC at decode", buf2, NULL);
  sprintf(buf,"c%d.decode_euPC",core->id);
  sprintf(buf2,"c%d.decode_eff_uops/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "effective uPC at decode", buf2, NULL);
  sprintf(buf,"c%d.uopQ_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total uopQ occupancy", &core->stat.uopQ_occupancy, core->stat.uopQ_occupancy, NULL);
  sprintf(buf,"c%d.uopQ_eff_occupancy",core->id);
  stat_reg_counter(sdb, false, buf, "total uopQ effective occupancy", &core->stat.uopQ_eff_occupancy, core->stat.uopQ_eff_occupancy, NULL);
  sprintf(buf,"c%d.uopQ_empty",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles uopQ was empty", &core->stat.uopQ_empty_cycles, core->stat.uopQ_empty_cycles, NULL);
  sprintf(buf,"c%d.uopQ_full",core->id);
  stat_reg_counter(sdb, false, buf, "total cycles uopQ was full", &core->stat.uopQ_full_cycles, core->stat.uopQ_full_cycles, NULL);
  sprintf(buf,"c%d.uopQ_avg",core->id);
  sprintf(buf2,"c%d.uopQ_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average uopQ occupancy", buf2, NULL);
  sprintf(buf,"c%d.uopQ_eff_avg",core->id);
  sprintf(buf2,"c%d.uopQ_eff_occupancy/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "average uopQ effective occupancy", buf2, NULL);
  sprintf(buf,"c%d.uopQ_frac_empty",core->id);
  sprintf(buf2,"c%d.uopQ_empty/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles uopQ was empty", buf2, NULL);
  sprintf(buf,"c%d.uopQ_frac_full",core->id);
  sprintf(buf2,"c%d.uopQ_full/c%d.sim_cycle",core->id,core->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles uopQ was full", buf2, NULL);

  sprintf(buf,"c%d.decode_stall",core->current_thread->id);
  core->stat.decode_stall = stat_reg_dist(sdb, buf,
                                           "breakdown of stalls at decode",
                                           /* initial value */0,
                                           /* array size */DSTALL_num,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */decode_stall_str,
                                           /* print fn */NULL);
  
}

void core_decode_atom_t::update_occupancy(void)
{
    /* uopQ */
  core->stat.uopQ_occupancy += uopQ_num;
  core->stat.uopQ_eff_occupancy += uopQ_eff_num;
  if(uopQ_num >= core->knobs->decode.uopQ_size)
    core->stat.uopQ_full_cycles++;
  if(uopQ_num <= 0)
    core->stat.uopQ_empty_cycles++;
}


/*************************/
/* MAIN DECODE FUNCTIONS */
/*************************/

/* Helper functions to check for BAC-related resteers */

/* Check an individual Mop to see if it's next target is correct/reasonable, and
   recover if necessary. */
enum core_decode_atom_t::decode_stall_t
core_decode_atom_t::check_target(struct Mop_t * const Mop)
{
  if(!Mop->decode.is_ctrl)
  {
    /* next-PC had better be the next sequential inst , IN case its a trap instruction we let it pass because the pipeline is at it is drained */
    if( (Mop->fetch.pred_NPC != (Mop->fetch.PC + Mop->fetch.inst.qemu_len)) && !(Mop->decode.is_trap) && !(Mop->decode.is_intr)  )
    {
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
      fprintf(stdout,"\n[%lld][Core%d] ",core->sim_cycle,core->id);
    if(core->sim_cycle > PRINT_CYCLE )
      md_print_insn(Mop,stdout);
    if(core->sim_cycle > PRINT_CYCLE )
      fprintf(stdout,"Non control instruction has wrong Target in Decode Stage FetchPC = %llx Pred_NPC = %llx ", Mop->fetch.PC, Mop->fetch.pred_NPC);
#endif
      /* it's not (phantom taken branch), so resteer */
      ZESTO_STAT(core->stat.phantom_resteers++;)
      core->oracle->recover(Mop);
      recover_decode_pipe(Mop);
      Mop->fetch.pred_NPC = Mop->fetch.PC + Mop->fetch.inst.len;
      core->fetch->recover(Mop->fetch.pred_NPC);
      return DSTALL_PHANTOM;
    }
  }
  else /* branch or REP */
  {
    if((Mop->fetch.pred_NPC != (Mop->fetch.PC + Mop->fetch.inst.len)) /* branch is predicted taken */
        && (Mop->decode.opflags | F_UNCOND) && !(Mop->decode.is_intr) )
    {
      /* We assume that even for control instructions whose target addresses are dependent on immediate offsets and PC 
         are only resolved during execute rather than decode. */
      if(Mop->fetch.pred_NPC != Mop->decode.targetPC) // wrong target 
      {
        if(Mop->fetch.bpred_update)
          core->fetch->bpred->recover(Mop->fetch.bpred_update,1);//1=taken
#ifdef ZDEBUG
        fprintf(stdout,"\n[%lld][Core%d] ",core->sim_cycle,core->id);
        md_print_insn(Mop,stdout);
        fprintf(stdout," Wrong Target in Decode Stage PredNPC = %llx TargetPC= 0x%llx \n", Mop->fetch.pred_NPC, Mop->decode.targetPC);
#endif
        Mop->fetch.branch_mispred = true; 
        ZESTO_STAT(core->stat.target_resteers++;)
        core->oracle->recover(Mop);
        recover_decode_pipe(Mop);
        core->fetch->recover(Mop->oracle.NextPC);
        Mop->fetch.pred_NPC = Mop->oracle.NextPC;
        return DSTALL_TARGET;
      }
    }
  }
  return DSTALL_NONE;
}

/* Perform branch-address calculation check for a given decode-pipe stage
   and Mop position. */
bool core_decode_atom_t::check_flush(const int stage, const int idx)
{
  struct core_knobs_t * knobs = core->knobs;
  /* stage-1 because we assume the following checks are acted upon only after
     the instruction has spent a cycle in the corresponding pipeline stage. */
  struct Mop_t * Mop = pipe[stage-1][idx];
  enum decode_stall_t stall_reason;

  if(Mop)
  {
    if((stage-1) == knobs->decode.target_stage)
    {
      if((stall_reason = check_target(Mop)))
      {
        pipe[stage][idx] = pipe[stage-1][idx];
        pipe[stage-1][idx] = NULL;
        if(pipe[stage][idx]) {
          occupancy[stage]++;
          occupancy[stage-1]--;
          zesto_assert(occupancy[stage] <= knobs->decode.width,false);
          zesto_assert(occupancy[stage-1] >= 0,false);
        }
        ZESTO_STAT(stat_add_sample(core->stat.decode_stall, (int)stall_reason);)
#ifdef ZTRACE
        ztrace_print(Mop,"d|target-check|mispred target resteer");
#endif
        return true;
      }
    }
  }

  return false;
}


/* attempt to send uops to the uopQ, shuffle other Mops down the decode pipe, read Mops from IQ */
void core_decode_atom_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage, i;
  enum decode_stall_t stall_reason = DSTALL_NONE;

  /* move decoded uops to uopQ */
  stage = knobs->decode.depth-1;
  if(occupancy[stage])
    for(i=0;i<knobs->decode.width;i++)
    {
      if(pipe[stage][i])
      {
        while(uopQ_num < knobs->decode.uopQ_size)   /* while uopQ is not full */
        {
          struct Mop_t * Mop = pipe[stage][i];      /* Mop in current decoder */
          struct uop_t * uop = &Mop->uop[Mop->decode.last_stage_index]; /* first non-queued uop */
          Mop->decode.last_stage_index += uop->decode.has_imm ? 3 : 1;  /* increment the uop pointer to next uop */
          
          zesto_assert((!(uop->decode.opflags & F_STORE)) || uop->decode.is_std,(void)0);
          zesto_assert((!(uop->decode.opflags & F_LOAD)) || uop->decode.is_load,(void)0);

          if((!uop->decode.in_fusion) || uop->decode.is_fusion_head) /* don't enqueue fusion body */
          {
            uopQ[uopQ_tail] = uop;      /* queue the uop */
#ifdef ZDEBUG
    if(core->sim_cycle > PRINT_CYCLE )
            fprintf(stdout,"\n[%lld][Core%d]DECODE MOP=%d UOP=%d in uopQ_Enqueue TAIL %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,uopQ_tail,uop->decode.in_fusion);
#endif
            uopQ_tail = modinc(uopQ_tail,knobs->decode.uopQ_size); //(uopQ_tail+1) % knobs->decode.uopQ_size;
            uopQ_num++;

            #ifdef ZESTO_COUNTERS
            core->counters->uopQ.write++; 
            core->counters->latch.ID2uQ.read++;
            #endif

            if(uop->decode.is_fusion_head)
              uopQ_eff_num += uop->decode.fusion_size;
            else
              uopQ_eff_num++;
            ZESTO_STAT(core->stat.decode_uops++;)
          }
#ifdef ZDEBUG
          else
	  {
	    int index=	moddec(uopQ_tail,knobs->decode.uopQ_size);
    if(core->sim_cycle > PRINT_CYCLE )
            fprintf(stdout,"\n[%lld][Core%d]DECODE MOP=%d UOP=%d in uopQ_Enqueue_fused TAIL %d in_fusion %d ",core->sim_cycle,core->id,uop->decode.Mop_seq,uop->decode.uop_seq,index,uop->decode.in_fusion);
	  }
#endif
          ZESTO_STAT(core->stat.decode_eff_uops++;)
          uop->timing.when_decoded = core->sim_cycle;
          if(uop->decode.BOM)
            uop->Mop->timing.when_decode_started = core->sim_cycle;
          if(uop->decode.EOM)
            uop->Mop->timing.when_decode_finished = core->sim_cycle;

          if(Mop->decode.last_stage_index >= Mop->decode.flow_length)
          {
            if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
              ZESTO_STAT(core->stat.decode_insn++;)
            /* all uops dispatched, remove from decoder */
            pipe[stage][i] = NULL;
            /* all the uops of the Mops have been dispatched to uopQ 
               = remove the last entry of the decode pipe */
            occupancy[stage]--;
            zesto_assert(occupancy[stage] >= 0,(void)0);
#ifdef ZTRACE
            ztrace_print(Mop,"d|decode-pipe|dequeue");
#endif
            break;
          }
        }
        if(uopQ_num >= knobs->decode.uopQ_size)
          break; /* uopQ is full */
      }
    }

  /* walk pipe backwards up to and but not including the first stage*/
  for(stage=knobs->decode.depth-1; stage > 0; stage--)
  {
    if(0 == occupancy[stage]) /* implementing non-serpentine pipe (no compressing) - can't advance until stage is empty */
    {
      zesto_assert(occupancy[stage] == 0,(void)0);
      tick_t when_MS_started = pipe[stage-1][0] ? pipe[stage-1][0]->timing.when_MS_started : 0;
      if((when_MS_started != TICK_T_MAX) && (when_MS_started >= core->sim_cycle)) /* checks for non-zero MS/uROM delay */
        break;

      /* move everyone from previous stage forward */
      if(occupancy[stage-1])
        for(i=0;i<knobs->decode.width;i++)
        {
          if(check_flush(stage,i))
            return;

          pipe[stage][i] = pipe[stage-1][i];
          pipe[stage-1][i] = NULL;
          if(pipe[stage][i]) {
            occupancy[stage]++;
            occupancy[stage-1]--;
            zesto_assert(occupancy[stage] <= knobs->decode.width,(void)0);
            zesto_assert(occupancy[stage-1] >= 0,(void)0);
          }
        }
    }
  }

  /* process the first stage, which reads Mops from the IQ */
  /* we're moving instructions into the decoder.  decoder must be able to handle the Mop */
  {
    if(!core->fetch->Mop_available())
    {
      stall_reason = DSTALL_EMPTY;
    }
    else
    {
      int Mops_decoded = 0;
      int branches_decoded = 0;
      for(i=0;(i<knobs->decode.width) && core->fetch->Mop_available();i++)
      {
        if(pipe[0][i] == NULL) /* decoder available */
        {
          struct Mop_t * IQ_Mop = core->fetch->Mop_peek();

          if(IQ_Mop->decode.is_ctrl && knobs->decode.branch_decode_limit && (branches_decoded >= knobs->decode.branch_decode_limit))
          {
            stall_reason = DSTALL_MAX_BRANCHES;
            break;
          }

          if(0==i) /* special case first decoder since it can access the uROM and MS */
          {
            /* consume the Mop from the IQ */
            pipe[0][i] = IQ_Mop;
            occupancy[0]++;
            zesto_assert(occupancy[0] <= knobs->decode.width,(void)0);
            core->fetch->Mop_consume();
            Mops_decoded++;

#ifdef ZTRACE
            ztrace_print(IQ_Mop,"d|decode-pipe|enqueue");
#endif

            #ifdef ZESTO_COUNTERS
            /* instruction decode */
            core->counters->decoder.instruction_decoder.read += pipe[0][i]->stat.num_uops;

            int uop_index = 0;
            for(int uop_count = 0; uop_count < pipe[0][i]->stat.num_uops; uop_count++)
            {
              /* input operands */
              for(int idep_index = 0; idep_index < MAX_IDEPS; idep_index++)
              {
                if((pipe[0][i]->uop[uop_index].decode.idep_name[idep_index]!=DNA)\
                   &&(pipe[0][i]->uop[uop_index].decode.idep_name[idep_index]!=MD_REG_ZERO))
                {
                  core->counters->decoder.operand_decoder.read++;
                }
              }
              /* output operand */
              if((pipe[0][i]->uop[uop_index].decode.odep_name!=DNA)\
                 &&(pipe[0][i]->uop[uop_index].decode.odep_name!=MD_REG_ZERO))
              {
                core->counters->decoder.operand_decoder.read++;
              }
              uop_index += pipe[0][i]->uop[uop_index].decode.has_imm?3:1;
            }
            #endif

            /* does this Mop need help from the MS? */
            if((knobs->decode.max_uops[i] && (pipe[0][i]->stat.num_uops > knobs->decode.max_uops[i])) ||
                pipe[0][i]->fetch.inst.rep) {
              pipe[0][i]->timing.when_MS_started = core->sim_cycle + knobs->decode.MS_latency; /* all other insts (non-MS) have this timestamp default to TICK_T_MAX */

              #ifdef ZESTO_COUNTERS
              core->counters->decoder.uop_sequencer.read++;
              #endif
            }
            if(IQ_Mop->decode.is_ctrl)
              branches_decoded++;
          }
          else /* other decoders must check uop limits */
          {
            if((!knobs->decode.max_uops[i] || (IQ_Mop->stat.num_uops <= knobs->decode.max_uops[i])) &&
                !IQ_Mop->fetch.inst.rep) /* decoder0 handles reps */
            {
              /* consume the Mop from the IQ */
              pipe[0][i] = IQ_Mop;
              occupancy[0]++;
              zesto_assert(occupancy[0] <= knobs->decode.width,(void)0);
              core->fetch->Mop_consume();
              Mops_decoded++;
              if(IQ_Mop->decode.is_ctrl)
                branches_decoded++;
#ifdef ZTRACE
              ztrace_print(IQ_Mop,"d|decode-pipe|enqueue");
#endif

              #ifdef ZESTO_COUNTERS
              /* instruction decode */
              core->counters->decoder.instruction_decoder.read += pipe[0][i]->stat.num_uops;

              int uop_index = 0;
              for(int uop_count = 0; uop_count < pipe[0][i]->stat.num_uops; uop_count++)
              {
                /* input operands */
                for(int idep_index = 0; idep_index < MAX_IDEPS; idep_index++)
                {
                  if((pipe[0][i]->uop[uop_index].decode.idep_name[idep_index]!=DNA)\
                     &&(pipe[0][i]->uop[uop_index].decode.idep_name[idep_index]!=MD_REG_ZERO))
                  {
                    core->counters->decoder.operand_decoder.read++;
                  }
                }
                /* output operand */
                if((pipe[0][i]->uop[uop_index].decode.odep_name!=DNA)\
                   &&(pipe[0][i]->uop[uop_index].decode.odep_name!=MD_REG_ZERO))
                {
                  core->counters->decoder.operand_decoder.read++;
                }
                uop_index += pipe[0][i]->uop[uop_index].decode.has_imm?3:1;
              }
              #endif
            }
            else
            {
              if(IQ_Mop->fetch.inst.rep)
                stall_reason = DSTALL_REP;
              else if((IQ_Mop->stat.num_uops < knobs->decode.max_uops[0]) && knobs->decode.max_uops[0])
                stall_reason = DSTALL_SMALL; /* flow too big for current decoder, but small enough for complex decoder */
              else
                stall_reason = DSTALL_UROM;
              break; /* can't advance Mop, wait 'til next cycle to try again */
            }
          }
        }
        if(Mops_decoded == 0)
          stall_reason = DSTALL_FULL;
      }
    }
  }

  ZESTO_STAT(stat_add_sample(core->stat.decode_stall, (int)stall_reason);)
}

void
core_decode_atom_t::recover(void)
{
  recover_uopQ();
  recover_decode_pipe();
}

void
core_decode_atom_t::recover(struct Mop_t * const Mop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* walk pipe from youngest uop blowing everything away,
     stop if we encounter the recover-Mop */
  for(int stage=0;stage<knobs->decode.depth;stage++)
  {
    if(occupancy[stage])
      for(int i=knobs->decode.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          if(pipe[stage][i] == Mop)
            return;
          else
          {
            #ifdef ZESTO_COUNTERS
            core->counters->decoder.instruction_decoder.read++;
            #endif
            pipe[stage][i] = NULL;
            occupancy[stage]--;
            zesto_assert(occupancy[stage] >= 0,(void)0);
          }
        }
      }
  }
  recover_uopQ(Mop);
}

/* start from most speculative; stop if we find the Mop */
void
core_decode_atom_t::recover_uopQ(const struct Mop_t * const Mop)
{
  struct core_knobs_t * knobs = core->knobs;

  while(uopQ_num)
  {
    int index = moddec(uopQ_tail,knobs->decode.uopQ_size); //(uopQ_tail-1+knobs->decode.uopQ_size) % knobs->decode.uopQ_size;
    if(uopQ[index]->Mop == Mop)
      return;
    if(uopQ[index]->decode.is_fusion_head)
      uopQ_eff_num -= uopQ[index]->decode.fusion_size;
    else
      uopQ_eff_num --;
    uopQ_num--;
    uopQ[index] = NULL;
    uopQ_tail = index;

    #ifdef ZESTO_COUNTERS
    core->counters->uopQ.write++;
    #endif
  }
  uopQ_eff_num = 0;
}

/* blow away the entire uopQ */
void
core_decode_atom_t::recover_uopQ(void)
{
  struct core_knobs_t * knobs = core->knobs;

  #ifdef ZESTO_COUNTERS
  core->counters->uopQ.write += uopQ_num;
  #endif

  memzero(uopQ,sizeof(*uopQ)*knobs->decode.uopQ_size);
  uopQ_head = 0;
  uopQ_tail = 0;
  uopQ_num = 0;
  uopQ_eff_num = 0;
}

void
core_decode_atom_t::recover_decode_pipe(const struct Mop_t * const Mop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* walk pipe from youngest uop blowing everything away,
     stop if we encounter the recover-Mop */
  for(int stage=0;stage<knobs->decode.depth;stage++)
  {
    if(occupancy[stage])
      for(int i=knobs->decode.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          if(pipe[stage][i] == Mop)
            return;
          else
          {
            #ifdef ZESTO_COUNTERS
            core->counters->decoder.instruction_decoder.read++;
            #endif
            pipe[stage][i] = NULL;
            occupancy[stage]--;
            zesto_assert(occupancy[stage] >= 0,(void)0);
          }
        }
      }
  }
}

/* same as above, but blow away the entire decode pipeline */
void
core_decode_atom_t::recover_decode_pipe(void)
{
  struct core_knobs_t * knobs = core->knobs;
  for(int stage=0;stage<knobs->decode.depth;stage++)
  {
    if(occupancy[stage])
      for(int i=knobs->decode.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          #ifdef ZESTO_COUNTERS
          core->counters->decoder.instruction_decoder.read++;
          #endif
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
    zesto_assert(occupancy[stage] == 0,(void)0);
  }
}

bool core_decode_atom_t::uop_available(void)
{
  return uopQ_num > 0;
}

struct uop_t * core_decode_atom_t::uop_peek(void)
{
  return uopQ[uopQ_head];
}

void core_decode_atom_t::uop_consume(void)
{
  struct core_knobs_t * knobs = core->knobs;
  struct uop_t * uop = uopQ[uopQ_head];
  uopQ[uopQ_head] = NULL;
  uopQ_num --;
  if(uop->decode.is_fusion_head)
    uopQ_eff_num -= uop->decode.fusion_size;
  else
    uopQ_eff_num --;
  uopQ_head = modinc(uopQ_head,knobs->decode.uopQ_size); //(uopQ_head+1)%knobs->decode.uopQ_size;

  #ifdef ZESTO_COUNTERS
  core->counters->uopQ.read++;
  core->counters->latch.uQ2RR.read++;
  #endif
}

#endif
