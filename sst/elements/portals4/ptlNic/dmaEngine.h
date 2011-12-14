#ifndef _dmaEngine_h
#define _dmaEngine_h

#include <sst/core/component.h>

#include "callback.h"
#include "ptlNicTypes.h"
#include "dmaEvent.h"

class NicMmu;
class DmaEngine {

  public:

    DmaEngine( SST::Component&, int nid ); 
    bool write( Addr, uint8_t*, size_t, CallbackBase* );
    bool read( Addr, uint8_t*, size_t, CallbackBase* );
    size_t  maxXfer( ) { return 0x2000; }
    
  private:
    struct XYZ {
        Addr    addr;
        size_t length;
    };
    typedef std::deque<XYZ> xyzList_t;

    struct DmaEntry {
        size_t doneLength;
        size_t length;
        CallbackBase* callback;
    };

    bool xfer( DmaEvent::Type, Addr, uint8_t*, size_t, CallbackBase* );
    void eventHandler( SST::Event* );

    void lookup( Addr, size_t, xyzList_t& );

    SST::Component& m_comp;
    SST::Link*      m_link;
    int             m_nid;
    NicMmu         *m_nicMmu;
};

#endif
