#ifndef ZESTO_QSIMCLIENT_CORE
#define ZESTO_QSIMCLIENT_CORE


#ifdef USE_QSIM

#include "../zesto-core.h"
#include "qsim-client.h"

struct QsimClient_Settings {
    QsimClient_Settings(const char* s, int p) : server(s), port(p) {}

    const char* server;
    int port;
};


class qsimclient_core_t: public core_t
{
public:
    qsimclient_core_t(SST::ComponentId_t core_id, SST::Component::Params_t& params);
//const int core_id, char* config, int cpuid, const QsimClient_Settings&);
    ~qsimclient_core_t();

protected:
    virtual bool fetch_next_pc(md_addr_t* nextPC, struct core_t* tcore);
 
    virtual void fetch_inst(md_inst_t* inst, struct mem_t* mem, const md_addr_t pc, core_t* const tcore);
 
private:
    Qsim::Client* m_Qsim_client;
    Qsim::ClientQueue* m_Qsim_queue;
    int m_Qsim_cpuid; //QSim cpu ID. In general this is different from core_id.
};


#endif //USE_QSIM

#endif
