#ifndef ZESTO_QSIMLIB_CORE
#define ZESTO_QSIMLIB_CORE

#ifdef USE_QSIM


#include "zesto-core.h"
#include "qsim.h"


class qsimlib_core_t: public core_t
{
public:
    qsimlib_core_t(const int core_id, char* config, Qsim::OSDomain* osd, int cpuid);
    ~qsimlib_core_t();

    //Since there should only be one OS domain, we use a static function to do the
    //fast forwarding of the OS domain.
    static void Start_qsim(Qsim::OSDomain*);

protected:
    virtual bool fetch_next_pc(md_addr_t* nextPC, struct core_t* tcore);
 
    virtual void fetch_inst(md_inst_t* inst, struct mem_t* mem, const md_addr_t pc, core_t* const tcore);
 
private:
    friend class AppStartCB;

    static std::vector<qsimlib_core_t*> Procs;

    void create_queue();
    void get_first_pc(); //can only be called after create_queue().

    Qsim::OSDomain* m_Qsim_osd;
    Qsim::Queue* m_Qsim_queue;
    const int m_Qsim_cpuid; //QSim cpu ID. In general this is different from core_id.

    bool m_got_first_pc;
};




#endif //USE_QSIM

#endif // ZESTO_QSIMLIB_CORE
