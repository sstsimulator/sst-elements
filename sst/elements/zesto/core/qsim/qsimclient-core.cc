#ifndef USE_QSIM

#include "qsimclient-core.h"
#include "zesto-fetch.h"
#include <assert.h>

namespace manifold {
namespace zesto {

//! @param \c core_id  ID of the core.
//! @param \c conifg  Name of the config file.
//! @param \c cpuid  QSim cpu ID assigned to this core. Note this may be different from core ID.
qsimclient_core_t::qsimclient_core_t(const int core_id, char * config, int cpuid, const QsimClient_Settings& settings) :
    core_t(core_id, config),
    m_Qsim_cpuid(cpuid)
{
    //client_socket() requires port to be passed as a string, so convert to string first.
    char port_str[20];
    sprintf(port_str, "%d", settings.port);
    m_Qsim_client = new Qsim::Client(client_socket(settings.server, port_str));
    m_Qsim_queue = new Qsim::ClientQueue(*m_Qsim_client, m_Qsim_cpuid);

    md_addr_t nextPC = 0;
    fetch_next_pc(&nextPC, this);

    current_thread->regs.regs_PC = nextPC;
    current_thread->regs.regs_NPC = nextPC;
    fetch->PC = current_thread->regs.regs_PC;
}



qsimclient_core_t:: ~qsimclient_core_t()
{
    delete m_Qsim_queue;
    delete m_Qsim_client;
}



static unsigned Count = 0; //This is used by cpu 0 to count the number of instructions requested so
                           //it can call timer_interrupt() periodically.


//! @return  True if interrupt is seen.
bool qsimclient_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * tcore)
{
    qsimclient_core_t* core = dynamic_cast<qsimclient_core_t*>(tcore);
    assert(core != 0);

    Qsim::ClientQueue *q_ptr = core->m_Qsim_queue; 
    bool interrupt=false;

    while(true) {
	if(!q_ptr->empty()) {
	    if (q_ptr->front().type == Qsim::QueueItem::INST) {
		*nextPC = q_ptr->front().data.inst.vaddr;
		break;
	    }
	    else if(q_ptr->front().type == Qsim::QueueItem::INTR) {
#ifdef ZDEBUG
if(sim_cycle> PRINT_CYCLE)
fprintf(stdout,"\n[%lld][Core%d]Interrupt seen in md_fetch_next_PC",core->sim_cycle,core->id);
#endif
		interrupt=true;
		q_ptr->pop();
	    }
	    else if(q_ptr->front().type == Qsim::QueueItem::MEM) {	
		//should this happen?
	        q_ptr->pop();
	    }
	}
        else { //queue empty
	    core->m_Qsim_client->run(core->m_Qsim_cpuid, 1);
	    if(m_Qsim_cpuid == 0) {
		Count += 1;
		if(Count >= 1000000) { //every 1M instructions, cpu 0 calls timer_interrupt().
		    m_Qsim_client->timer_interrupt(); //this is called after each proc has advanced 1M instructions
		    Count = 0; //reset count
		}
	    }
	}
    }//while

    return(interrupt);
}	







void qsimclient_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const tcore)
{
    qsimclient_core_t* core = dynamic_cast<qsimclient_core_t*>(tcore);
    assert(core != 0);

    Qsim::ClientQueue *q_ptr = core->m_Qsim_queue;

    while(true) {
	if(q_ptr->empty()) {
	    core->m_Qsim_client->run(core->m_Qsim_cpuid, 1);
	    if(m_Qsim_cpuid == 0) {
		Count += 1;
		if(Count >= 1000000) { //every 1M instructions, cpu 0 calls timer_interrupt().
		    m_Qsim_client->timer_interrupt(); //this is called after each proc has advanced 1M instructions
		    Count = 0; //reset count
		}
	    }
        }
	else  {
	    if (q_ptr->front().type == Qsim::QueueItem::INST) {
#ifdef ZDEBUG
fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,q_ptr->front().data.inst.vaddr);
#endif
		core->current_thread->insn_count++;
		assert(q_ptr->front().data.inst.len <= MD_MAX_ILEN); //size of inst->code[] is MD_MAX_ILEN

	        for (unsigned i = 0; i < q_ptr->front().data.inst.len; i++) {
		    inst->code[i] = q_ptr->front().data.inst.bytes[i]; 
	        }
	        inst->vaddr=q_ptr->front().data.inst.vaddr;
	        inst->paddr=q_ptr->front().data.inst.paddr;
	        inst->qemu_len=q_ptr->front().data.inst.len;
	        inst->mem_ops.mem_vaddr_ld[0]=0;
	        inst->mem_ops.mem_vaddr_ld[1]=0;
	        inst->mem_ops.mem_vaddr_str[0]=0;
	        inst->mem_ops.mem_vaddr_str[1]=0;
	        inst->mem_ops.memops=0;
	    }
	    else if (q_ptr->front().type == Qsim::QueueItem::INTR) { //interrupt
		q_ptr->pop(); //ignore interrupt
		continue;
	    }
	    else {
		fprintf(stdout, "Memory Op found while looking for an instruction!\n");
		q_ptr->pop(); //ignore this
		continue;
	    }

	    q_ptr->pop();

	    if(q_ptr->empty())
	        core->m_Qsim_client->run(core->m_Qsim_cpuid, 1);

	    //Process the memory ops following the instruction.
	    while (q_ptr->front().type == Qsim::QueueItem::MEM) {
    
	        if(q_ptr->front().data.mem.type==MEM_RD) { //read
		    if(inst->mem_ops.mem_vaddr_ld[0]==0) {
			inst->mem_ops.mem_vaddr_ld[0]=q_ptr->front().data.mem.vaddr;
			inst->mem_ops.mem_paddr_ld[0]=q_ptr->front().data.mem.paddr;
			inst->mem_ops.ld_dequeued[0]=false;
			inst->mem_ops.ld_size[0]=q_ptr->front().data.mem.size;
			inst->mem_ops.memops++;
		    }
		    else if(inst->mem_ops.mem_vaddr_ld[1]==0) {
		      inst->mem_ops.mem_vaddr_ld[1]=q_ptr->front().data.mem.vaddr;
		      inst->mem_ops.mem_paddr_ld[1]=q_ptr->front().data.mem.paddr;
		      inst->mem_ops.ld_dequeued[1]=false;
		      inst->mem_ops.ld_size[1]=q_ptr->front().data.mem.size;
		      inst->mem_ops.memops++;
		    }
		}
	        else if(q_ptr->front().data.mem.type==MEM_WR) {
		    if(inst->mem_ops.mem_vaddr_str[0]==0) {
		        inst->mem_ops.mem_vaddr_str[0]=q_ptr->front().data.mem.vaddr;
		        inst->mem_ops.mem_paddr_str[0]=q_ptr->front().data.mem.paddr;
		        inst->mem_ops.str_dequeued[0]=false;
		        inst->mem_ops.str_size[0]=q_ptr->front().data.mem.size;
		        inst->mem_ops.memops++;
		    }
		    else if(inst->mem_ops.mem_vaddr_str[1]==0) {
		        inst->mem_ops.mem_vaddr_str[1]=q_ptr->front().data.mem.vaddr;
		        inst->mem_ops.mem_paddr_str[1]=q_ptr->front().data.mem.paddr;
		        inst->mem_ops.str_dequeued[1]=false;
		        inst->mem_ops.str_size[1]=q_ptr->front().data.mem.size;
		        inst->mem_ops.memops++;
		    }
	        }
		else
		    assert(0);

	        q_ptr->pop();

	        if(q_ptr->empty())
		    core->m_Qsim_client->run(core->m_Qsim_cpuid, 1);
	    }//while MEM ops
	    break; //got one complete instruction
	}

    }//while(true)
}



} //namespace zesto
} //namespace manifold


#endif //USE_QSIM
