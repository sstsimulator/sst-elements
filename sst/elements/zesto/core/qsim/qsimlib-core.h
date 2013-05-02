#ifndef ZESTO_QSIMLIB_CORE
#define ZESTO_QSIMLIB_CORE

#ifdef USE_QSIM


#include "../zesto-core.h"
#include <qsim.h>

class qsimlib_core_t: public core_t
{
public:
    qsimlib_core_t(SST::ComponentId_t core_id, SST::Component::Params_t& params);
	~qsimlib_core_t();

	virtual void setup(); 


protected:
    virtual bool fetch_next_pc(md_addr_t* nextPC, struct core_t* tcore);
 
    virtual void fetch_inst(md_inst_t* inst, struct mem_t* mem, const md_addr_t pc, core_t* const tcore);
 
private:
//    friend class AppStartCB;

    void create_queue();
    void get_first_pc(); //can only be called after create_queue().

    static std::vector<qsimlib_core_t*> Procs;
	//All qsim lib share one osdomain
    static Qsim::OSDomain* m_Qsim_osd;
	static int osd_users;
    Qsim::Queue* m_Qsim_queue;
    int m_Qsim_cpuid; //QSim cpu ID. In general this is different from core_id.

    bool m_got_first_pc;
};


#endif //USE_QSIM

#endif // ZESTO_QSIMLIB_CORE
