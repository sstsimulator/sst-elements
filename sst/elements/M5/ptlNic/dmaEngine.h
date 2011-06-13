#ifndef _dmaEngine_h
#define _dmaEngine_h

#include <sst/core/component.h>

#include "callback.h"
#include "ptlNicTypes.h"

class NicMmu;
class DmaEngine {

  public:

    DmaEngine( SST::Component&, int nid ); 
    bool write( Addr, uint8_t*, size_t, CallbackBase* );
    bool read( Addr, uint8_t*, size_t, CallbackBase* );
    size_t  maxXfer( ) { return 0x2000; }
    
  private:
    void eventHandler( SST::Event* );
    Addr lookup( Addr );
    SST::Component& m_comp;
    SST::Link*      m_link;
    int             m_nid;
    NicMmu         *m_nicMmu;
};

#endif
