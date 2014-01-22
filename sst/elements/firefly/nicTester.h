#ifndef COMPONENTS_FIREFLY_NICTESTER_H
#define COMPONENTS_FIREFLY_NICTESTER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "./nic.h"
#include <sst/core/output.h>

class DmaEntry;

namespace SST {
namespace Firefly {

#define FOREACH_STATE(NAME) \
    NAME(PostRecv)\
    NAME(PostSend)\
    NAME(WaitSend)\
    NAME(WaitRecv)\

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,


class NicTester : public SST::Component {

  public:
    NicTester( SST::ComponentId_t id, SST::Params &params );
    ~NicTester() {};
    void setup( void );
    void init( unsigned int phase );
    void printStatus( Output& ) {};
    void handleSelfEvent( SST::Event* );

  private:
    enum {
        FOREACH_STATE(GENERATE_ENUM)
    } m_state;

    static const char* m_stateName[];
    bool notifySendDmaDone( Nic::XXX* );
    bool notifyRecvDmaDone( Nic::XXX* );
    bool notifySendPioDone( Nic::XXX* );
    bool notifyNeedRecv( Nic::XXX* );

    void postRecv();
    void postSend();
    void waitSend();
    void waitRecv( Nic::XXX* );

    SST::Link*  m_selfLink;
    SST::Link*  m_hostLink;
    Nic*        m_nic;
    Output      m_dbg;
};


}
}
#endif
