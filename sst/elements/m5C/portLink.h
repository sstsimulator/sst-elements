// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __portLink_h
#define __portLink_h

#include <sst_config.h>
#include <sst/core/component.h>

#include <debug.h>
#include <m5.h>
#include <dll/gem5dll.hh>
#include <dll/memEvent.hh>
#include <rawEvent.h>
#include <sst/core/interfaces/memEvent.h>

namespace SST {
namespace M5 {

class PortLink {
  public:
    PortLink( M5&, Gem5Object_t&, const SST::Params& );
	void setup(void);

  private:
    inline void eventHandler( SST::Event* );

	MemPkt* findMatchingEvent(SST::Interfaces::MemEvent *sstev);
	MemPkt* convertSSTtoGEM5( SST::Event *e );
	SST::Interfaces::MemEvent* convertGEM5toSST( MemPkt *pkt );

    void poke() {
        DBGX(2,"\n");
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
        DBGX(2,"\n");

        // Note we are not throttling events
		if ( m_doTranslate ) {
			m_link->Send( convertGEM5toSST(static_cast<MemPkt*>(data)) );
		} else {
			m_link->Send( new RawEvent( data, len ) );
		}
        return true;
    }

    static inline void poke( void* obj ) {
        return ((PortLink*)obj)->poke( );
    }

    static inline bool recv( void *obj, void *data, size_t len ) {
        return ((PortLink*)obj)->recv( data, len );
    }

	bool            m_doTranslate;
    SST::Link*      m_link;
    M5&             m_comp;
    MsgEndPoint*    m_gem5EndPoint;
    MsgEndPoint     m_myEndPoint;
    std::deque<RawEvent*>  m_deferredQ;
	std::list<MemPkt*> m_g5events;
};


}
}

#endif
