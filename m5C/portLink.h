// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __portLink_h
#define __portLink_h

#include <debug.h>
#include <m5.h>
#include <dll/gem5dll.hh>
#include <dll/memEvent.hh>
#include <rawEvent.h>
#include <sst/core/output.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

namespace SST {

namespace M5 {

using namespace SST;
using namespace SST::Interfaces;

class PortLink {
  public:
    PortLink( M5&, Gem5Object_t&, const SST::Params& );
	void setup(void);
    void printQueueSize();
    void setOutput(Output* _dbg){
        dbg = _dbg;
    }

    SimpleMem* getSSTInterface() { return m_sstInterface; }
  private:
    void eventHandler( SimpleMem::Request *e );

	MemPkt* findMatchingEvent( SimpleMem::Request *e);
	MemPkt* convertSSTtoGEM5( SimpleMem::Request *e );
    SimpleMem::Request* convertGEM5toSST( MemPkt *pkt );
    Output* dbg;

    void poke() {
        DBGX(2,"PorkLink::poke()\n");
        assert( ! m_deferredQ.empty() );

        while ( ! m_deferredQ.empty() ) {
            RawEvent *event = m_deferredQ.front();

            if ( ! m_gem5EndPoint->send( event->data(), event->len() ) ) {
                DBGX(2,"busy\n");
                return;
            }

            m_deferredQ.pop_front();
            delete event;
        }
    }

    bool recv( void* data, size_t len ) {
        // Note we are not throttling events
        DBGX(2, "PortLink::recv\n");
        m_sstInterface->sendRequest( convertGEM5toSST(static_cast<MemPkt*>(data)) );
        return true;
    }

    static inline void poke( void* obj ) {
        return ((PortLink*)obj)->poke( );
    }

    static inline bool recv( void *obj, void *data, size_t len ) {
        return ((PortLink*)obj)->recv( data, len );
    }

    SimpleMem *m_sstInterface;
    std::string     m_name;
    M5&             m_comp;
    MsgEndPoint*    m_gem5EndPoint;
    MsgEndPoint     m_myEndPoint;
    std::deque<RawEvent*>  m_deferredQ;
	std::map<uint64_t, MemPkt*> m_g5events;
    int sent;
    int received;
    static const uint32_t LOCKED                      = 0x00100000;
    static const uint32_t UNCACHED                    = 0x00001000;
};


}
}

#endif
