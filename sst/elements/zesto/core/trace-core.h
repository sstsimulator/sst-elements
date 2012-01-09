#ifndef ZESTO_TRACE_CORE
#define ZESTO_TRACE_CORE

#include "zesto-core.h"


class trace_core_t: public core_t
{
  public:
  trace_core_t(SST::ComponentId_t core_id, SST::Component::Params_t& params);

  protected:
  virtual bool fetch_next_pc(md_addr_t *nextPC, struct core_t * core);
 
  virtual void fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core);
 
  virtual void pipe_flush_helper(md_addr_t& pc, core_t* c)
  {
    pc = this->store_nextPC;
  }

  /* pipeline flush and recovery */
  virtual void emergency_recovery(void);

  private:
  bool use_stored_nextPC;
  md_addr_t store_nextPC;
    
};



#endif // ZESTO_TRACE_CORE
