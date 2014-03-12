#ifndef COMPONENTS_FIREFLY_NICTESTER_H
#define COMPONENTS_FIREFLY_NICTESTER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

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


class VirtNic;

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
    bool notifySendPioDone( void* );
    bool notifySendDmaDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    void postRecv();
    void postSend();
    void waitSend();

    SST::Link*  m_selfLink;
    SST::Link*  m_hostLink;
    VirtNic*    m_vNic;
    Output      m_dbg;
};


}
}
#endif
