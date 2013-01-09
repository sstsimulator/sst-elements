#include "trace-core.h"
#include "zesto-opts.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"
#include "zesto-bpred.h"

trace_core_t::trace_core_t(SST::ComponentId_t cid, SST::Component::Params_t& params) : core_t(cid, params)
{

  if (params.find("traceFile") == params.end()) {
	_abort(zesto_core, "couldn't find zesto trace file\n");
  }
  sim.trace = params["traceFile"].c_str();

  current_thread->pin_trace = fopen(sim.trace,"r");

  if(!current_thread->pin_trace)
    fatal("could not open PIN trace file %s",sim.trace);

  use_stored_nextPC = false;

  md_addr_t nextPC = 0;
  fetch_next_pc(&nextPC,this);

  current_thread->regs.regs_PC = nextPC;
  current_thread->regs.regs_NPC = nextPC;
  fetch->PC = current_thread->regs.regs_PC;
  store_nextPC = nextPC;


}



bool trace_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * core)
{	
  trace_core_t* tcore = dynamic_cast<trace_core_t*>(core);
  assert(tcore != 0);

  if(tcore->use_stored_nextPC == true)
    *nextPC = tcore->store_nextPC;
  else
  {
    if(!feof(tcore->current_thread->pin_trace))
    {
      unsigned long nextPC64;
      fscanf(tcore->current_thread->pin_trace,"%llx", &nextPC64);
      *nextPC = nextPC64;
      tcore->store_nextPC = *nextPC;
    }
    else
    {
      if(tcore->current_thread->active)
      {
        fclose(tcore->current_thread->pin_trace);
        fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
        tcore->store_nextPC = *nextPC = NULL;
        tcore->current_thread->active = false;
        tcore->fetch->bpred->freeze_stats();
        tcore->exec->freeze_stats();
        tcore->print_stats();
      }
    }
  }
  return false;
}	


void trace_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core)
{
  unsigned  int i,len,memops;
  trace_core_t* tcore = dynamic_cast<trace_core_t*>(core);
  assert(tcore != 0);

  fscanf(tcore->current_thread->pin_trace,"%d ",&len);
  fscanf(tcore->current_thread->pin_trace,"%d ",&memops);

  inst->vaddr=pc;//store_nextPC[core_id];	
  inst->paddr=pc;//store_nextPC[core_id];
  inst->qemu_len=len;

#ifdef ZDEBUG
  fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",cores[core_id]->sim_cycle_per_core,core_id,inst->vaddr);
#endif

  for (i = 0; i < len; i++) 
  {
    if(feof(tcore->current_thread->pin_trace))
    {
      fclose(tcore->current_thread->pin_trace);
      fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
      tcore->store_nextPC = NULL;
      tcore->current_thread->active = false;
      tcore->fetch->bpred->freeze_stats();
      tcore->exec->freeze_stats();
      tcore->print_stats();
    }
    fscanf(tcore->current_thread->pin_trace,"%x ",&(inst->code[i]));
#ifdef ZDEBUG
    fprintf(stdout," %x", inst->code[i]);
#endif
  }

  inst->mem_ops.mem_vaddr_ld[0]=0;
  inst->mem_ops.mem_vaddr_ld[1]=0;
  inst->mem_ops.mem_vaddr_str[0]=0;
  inst->mem_ops.mem_vaddr_str[1]=0;
  if(memops)
  {
    unsigned long long ch;
    inst->mem_ops.memops=memops;
    fscanf(tcore->current_thread->pin_trace," %llx ",&ch);

    if(ch!=0x00 || ch!=0x01)
      tcore->store_nextPC = ch;
    while(true)
    {
      if(feof(tcore->current_thread->pin_trace))
      {
        fclose(tcore->current_thread->pin_trace);
         fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
        tcore->store_nextPC = NULL;
        tcore->current_thread->active = false;
        tcore->fetch->bpred->freeze_stats();
        tcore->exec->freeze_stats();
        tcore->print_stats();
      }
      if(ch==0)
      {
        if(inst->mem_ops.mem_vaddr_ld[0]==0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[0]), &(inst->mem_ops.ld_size[0]) );
          inst->mem_ops.mem_paddr_ld[0]= inst->mem_ops.mem_vaddr_ld[0];
          inst->mem_ops.ld_dequeued[0]=false;
          inst->mem_ops.memops++;
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[1]), &(inst->mem_ops.ld_size[1]) );
          inst->mem_ops.mem_paddr_ld[1]= inst->mem_ops.mem_vaddr_ld[1];
          inst->mem_ops.ld_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else if (ch == 1)
      {
        if(inst->mem_ops.mem_vaddr_str[0] == 0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[0]), &(inst->mem_ops.str_size[0]) );
          inst->mem_ops.mem_paddr_str[0]= inst->mem_ops.mem_vaddr_str[0];
          inst->mem_ops.str_dequeued[0]=false;
          inst->mem_ops.memops++;	
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[1]), &(inst->mem_ops.str_size[1]) );
          inst->mem_ops.mem_paddr_str[1]= inst->mem_ops.mem_vaddr_str[1];
          inst->mem_ops.str_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else
      {
        tcore->store_nextPC = ch;
        tcore->use_stored_nextPC = true;
        break;
      }
      fscanf(tcore->current_thread->pin_trace," %llx ",&ch);
    }
  }
  else
    tcore->use_stored_nextPC = false;
}




void trace_core_t::emergency_recovery(void)
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

  use_stored_nextPC = false;  //this line is the only difference from the base class version

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



