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

#ifndef COMPONENTS_FIREFLY_CTRL_MSG_TIMING_H
#define COMPONENTS_FIREFLY_CTRL_MSG_TIMING_H

#include "ctrlMsgTimingBase.h"
#include "latencyMod.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class MsgTiming : public SubComponent {
  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::CtrlMsg::MsgTiming)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        MsgTiming,
        "firefly",
        "msgTiming",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Firefly::CtrlMsg::MsgTiming
    )

	SST_ELI_DOCUMENT_PARAMS( 
        {"shortMsgLength","Sets the short to long message transition point", "16000"},
        {"sendAckDelay_ns","", "0"},
        {"txSetupMod","Set the module used to calculate TX setup latency", ""},
        {"rxSetupMod","Set the module used to calculate RX setup latency", ""},
        {"rxPostMod","Set the module used to calculate RX post latency", ""},
        {"txFiniMod","Set the module used to calculate TX fini latency", ""},
        {"rxFiniMod","Set the module used to calculate RX fini latency", ""},
	)
	/* PARAMS
		txSetupModParams.*
		rxSetupModParams.*
		rxPostModParams.*,
		txFiniModParams.*,
		rxFiniModParams.*
	*/

    inline MsgTiming( ComponentId_t id, Params& params );
    inline ~MsgTiming();

    virtual uint64_t txDelay( size_t bytes ) {
        return m_txSetupMod->getLatency( bytes );
    }
    virtual uint64_t rxDelay( size_t bytes ) {
        return m_rxSetupMod->getLatency( bytes );
    }
    virtual uint64_t sendReqFiniDelay( size_t bytes ) {
        return m_txFiniMod->getLatency( bytes );
    }
    virtual uint64_t recvReqFiniDelay( size_t bytes ) {
        return m_rxFiniMod->getLatency( bytes );
    }
    virtual uint64_t rxPostDelay_ns( size_t bytes ) {
        if ( m_rxPostMod ) {
            return m_rxPostMod->getLatency( bytes );
        } else {
            return 0;
        }
    }
    virtual uint64_t sendAckDelay() {
        return m_sendAckDelay;
    }
    virtual uint64_t shortMsgLength() {
        return m_shortMsgLength;
    }

  private:
    size_t m_shortMsgLength;
    int    m_sendAckDelay;
    LatencyMod* m_txSetupMod;
    LatencyMod* m_rxSetupMod;
    LatencyMod* m_rxPostMod;
    LatencyMod* m_txFiniMod;
    LatencyMod* m_rxFiniMod;

};

MsgTiming::MsgTiming( ComponentId_t id, Params& params ) :
			SubComponent(id), m_rxPostMod(NULL) {

    m_shortMsgLength = params.find<size_t>( "shortMsgLength", 4096 );
    m_sendAckDelay = params.find<int>( "sendAckDelay_ns", 0 );

    std::string tmpName = params.find<std::string>("txSetupMod");
    Params tmpParams = params.get_scoped_params("txSetupModParams");
    m_txSetupMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_txSetupMod );

    tmpName = params.find<std::string>("rxSetupMod");
    tmpParams = params.get_scoped_params("rxSetupModParams");
    m_rxSetupMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_rxSetupMod );

    tmpName = params.find<std::string>("rxPostMod");
    if ( ! tmpName.empty() ) {
        tmpParams = params.get_scoped_params("rxPostModParams");
        m_rxPostMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    }

    tmpName = params.find<std::string>("txFiniMod","firefly.LatencyMod");
    tmpParams = params.get_scoped_params("txFiniModParams");
    m_txFiniMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_txFiniMod );

    tmpName = params.find<std::string>("rxFiniMod","firefly.LatencyMod");
    tmpParams = params.get_scoped_params("rxFiniModParams");
    m_rxFiniMod = dynamic_cast<LatencyMod*>( loadModule( tmpName, tmpParams ) );
    assert( m_rxFiniMod );
}

MsgTiming::~MsgTiming()
{
    delete m_txSetupMod;
    delete m_rxSetupMod;
    delete m_rxFiniMod;
    delete m_txFiniMod;

    if ( m_rxPostMod ) {
        delete m_rxPostMod;
    }
}

}
}
}

#endif
