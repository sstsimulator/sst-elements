#ifdef USE_QSIM

#include "qsimlib-core.h"
#include "qsim-load.h"
#include "../zesto-fetch.h"
#include <assert.h>

Qsim::OSDomain * qsimlib_core_t::m_Qsim_osd = NULL;
int qsimlib_core_t::osd_users = 0;
std::vector<qsimlib_core_t*> qsimlib_core_t :: Procs; //static variable.

qsimlib_core_t::qsimlib_core_t(SST::ComponentId_t cid, SST::Component::Params_t & params) : core_t(cid, params)
{
  m_Qsim_cpuid = id;

  if(params.find("stateFile") == params.end())
    _abort(zesto_core, "couldn't find zesto qsim state file\n");
  if(params.find("appFile") == params.end())
    _abort(zesto_core, "couldn't find zesto qsim benchmark file\n");

  if(m_Qsim_osd == NULL) {
    std::cout << "Loading state...\n";
    m_Qsim_osd = new Qsim::OSDomain(params["stateFile"].c_str());
    std::cout << "Loading app...\n";
    m_Qsim_osd->connect_console(std::cout);
    Qsim::load_file(*m_Qsim_osd, params["appFile"].c_str());
    std::cout << "Finished loading app.\n";
  }

  osd_users++;
  m_Qsim_queue = 0;
  m_got_first_pc = false;
  Procs.push_back(this);
}



qsimlib_core_t:: ~qsimlib_core_t()
{
  delete m_Qsim_queue;
  osd_users--;
  if(m_Qsim_osd && osd_users == 0) {
    delete m_Qsim_osd;
    m_Qsim_osd = NULL;
  }

}

//int qsimlib_core_t::Setup() {  // Renamed per Issue 70 - ALevine
void qsimlib_core_t::setup() { 
	create_queue();
	get_first_pc();
//	return core_t::Setup();  // Renamed per Issue 70 - ALevine
	core_t::setup();  
}

//! Qsim queue should be created at app start.
void qsimlib_core_t :: create_queue()
{
    m_Qsim_queue = new Qsim::Queue(*m_Qsim_osd, m_Qsim_cpuid);
}

//! Get the first PC to get things going; should be called before 1st tick.
void qsimlib_core_t :: get_first_pc()
{
    if(m_got_first_pc == false) {
        m_got_first_pc = true;
        md_addr_t nextPC = 0;
        fetch_next_pc(&nextPC,this);

        current_thread->regs.regs_PC = nextPC;
        current_thread->regs.regs_NPC = nextPC;
        fetch->PC = current_thread->regs.regs_PC;
    }
}


//for Qsim: app start callback
static unsigned Count = 0; //this records number of instructions % 1M at the time when app_start_cb is called.
                           //we need this because we need to call timer_interrupt() roughly after each proc
			   //has advanced 1M instructions.




//! @return  True if interrupt is seen.
bool qsimlib_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * tcore)
{
    qsimlib_core_t* core = dynamic_cast<qsimlib_core_t*>(tcore);
    assert(core != 0);

    Qsim::Queue *q_ptr = core->m_Qsim_queue; 
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
	        q_ptr->pop();
	    }
	}
        else { //queue empty
	    core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    if(m_Qsim_cpuid == Procs.size() - 1) { //processor with largest cpuid calls timer_interrupt
		Count += 1;
		if(Count >= 1000000) {
		    m_Qsim_osd->timer_interrupt(); //this is called after the designated proc has advanced 1M instructions
		    Count = 0;
		}
	    }

	}
    }

    return(interrupt);
}

void qsimlib_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const tcore)
{
    qsimlib_core_t* core = dynamic_cast<qsimlib_core_t*>(tcore);
    assert(core != 0);

    Qsim::Queue *q_ptr = core->m_Qsim_queue;

    while(true) {
	if(q_ptr->empty()) {
	    core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    if(m_Qsim_cpuid == Procs.size() - 1) { //processor with largest cpuid calls timer_interrupt
		Count += 1;
		if(Count >= 1000000) {
		    m_Qsim_osd->timer_interrupt(); //this is called after the designated proc has advanced 1M instructions
		    Count = 0;
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
	        core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);

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
		    core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    }//while MEM ops
	    break; //got one complete instruction
	}

    }//while(true)

}





#endif //USE_QSIM
