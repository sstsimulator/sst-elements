// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_MEMORY_H
#define COMPONENTS_FIREFLY_CTRL_MSG_MEMORY_H

#include "ctrlMsgMemoryBase.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class Memory : public MemoryBase {

   typedef std::function<void()> Callback;
    class DelayEvent : public SST::Event {
      public:

        DelayEvent( Callback _callback ) :
            Event(),
            callback( _callback )
        {}

        Callback                callback;

        NotSerializable(DelayEvent)
    };

  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::CtrlMsg::Memory)
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        Memory,
        "firefly",
        "ctrlMsgMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Firefly::CtrlMsg::Memory
    )
	SST_ELI_DOCUMENT_PARAMS(
		{"matchDelay_ns","","1"},
		{"txMemcpyMod","Sets the TX memory copy module",""},
		{"rxMemcpyMod","Sets the RX memory copy module",""},
		{"regRegionBaseDelay_ns","Sets the default delay for a memory region pin","0"},
		{"regRegionPerPageDelay_ns","Sets the per page delay for memory region pin","0"},
		{"regRegionXoverLength","Sets the page size fo memory pin","4096"},
	)
	/* PARAMS
		txMemcpyModParams.*
		rxMemcpyModParams.*
	*/

    Memory( ComponentId_t id, Params& params );
    ~Memory();
    void setOutput( Output* output ) {
        m_dbg = output;
    }

    virtual void copy( Callback callback, MemAddr to, MemAddr from, size_t length ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        uint64_t delay;
        if ( from ) {
            delay = txMemcpyDelay( length );
        } else if ( to ) {
            delay = rxMemcpyDelay( length );
        } else {
            assert(0);
        }
        schedCallback( callback, delay );
    }

    virtual void write( Callback callback, MemAddr to, size_t length ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        schedCallback( callback, txMemcpyDelay( length ) );
    }

    virtual void read( Callback callback, MemAddr to, size_t length ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        schedCallback( callback, txMemcpyDelay( length ) );
    }

    virtual void pin( Callback callback, MemAddr, size_t length ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        schedCallback( callback, regRegionDelay( length ) );
    }

    virtual void unpin( Callback callback, MemAddr, size_t  length ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        schedCallback( callback, regRegionDelay( length ) );
    }

    virtual void walk( Callback callback, int count ) {
        m_dbg->debug(CALL_INFO,1,1,"\n");
        schedCallback( callback, matchDelay( count ) );
    }

    int txMemcpyDelay( int bytes ) {
        return m_txMemcpyMod->getLatency( bytes );
    }

    int rxMemcpyDelay( int bytes ) {
        return m_rxMemcpyMod->getLatency( bytes );
    }


    int regRegionDelay( int nbytes ) {
        double tmp = 0;

        if ( nbytes ) {
            tmp = m_regRegionBaseDelay_ns;
        }

        if ( nbytes > m_regRegionXoverLength  ) {
            tmp += (nbytes/4096) * m_regRegionPerPageDelay_ns;
        }
        return tmp;
    }

    int matchDelay( int i ) {
        return i * m_matchDelay_ns;
    }

  private:

    void schedCallback( Callback callback, uint64_t delay ) {
        m_delayLink->send( delay, new DelayEvent(callback) );
    }

    void delayHandler( SST::Event* e )
    {
        DelayEvent* event = static_cast<DelayEvent*>(e);

        m_dbg->debug(CALL_INFO,2,1,"execute callback\n");

        event->callback();
        delete e;
    }

    int m_matchDelay_ns;
    int m_regRegionBaseDelay_ns;
    int m_regRegionPerPageDelay_ns;
    int m_regRegionXoverLength;

    Output* m_dbg;

    LatencyMod* m_txMemcpyMod;
    LatencyMod* m_rxMemcpyMod;
    Link*       m_delayLink;
};

inline Memory::Memory( ComponentId_t id, Params& params ) : MemoryBase(id)
{
    std::stringstream ss;
    ss << this;
    m_delayLink = configureSelfLink( "ProcessQueuesStateSelfLink." + ss.str(), "1 ns",
                           new Event::Handler<Memory>(this,&Memory::delayHandler));

    m_matchDelay_ns = params.find<int>( "matchDelay_ns", 1 );

    std::string tmpName = params.find<std::string>("txMemcpyMod");
    Params tmpParams = params.get_scoped_params("txMemcpyModParams");
    m_txMemcpyMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_txMemcpyMod );

    tmpName = params.find<std::string>("rxMemcpyMod");
    tmpParams = params.get_scoped_params("rxMemcpyModParams");
    m_rxMemcpyMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_rxMemcpyMod );

    m_regRegionBaseDelay_ns = params.find<int>( "regRegionBaseDelay_ns", 0 );
    m_regRegionPerPageDelay_ns = params.find<int>( "regRegionPerPageDelay_ns", 0 );
    m_regRegionXoverLength = params.find<int>( "regRegionXoverLength", 4096 );
}
inline Memory::~Memory( ) {
    delete m_txMemcpyMod;
    delete m_rxMemcpyMod;
}

}
}
}

#endif
