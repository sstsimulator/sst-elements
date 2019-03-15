// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

class MsgTiming {
  public:

    MsgTiming( Component* comp, Params& params, Output& dbg );
    ~MsgTiming();

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
    Output& m_dbg;
    size_t m_shortMsgLength;
    int    m_sendAckDelay;
    LatencyMod* m_txSetupMod;
    LatencyMod* m_rxSetupMod;
    LatencyMod* m_rxPostMod;
    LatencyMod* m_txFiniMod;
    LatencyMod* m_rxFiniMod;

};

inline MsgTiming::MsgTiming( Component* comp, Params& params, Output& dbg ) :  m_dbg(dbg), m_rxPostMod(NULL) {

    m_shortMsgLength = params.find<size_t>( "shortMsgLength", 4096 );
    m_sendAckDelay = params.find<int>( "sendAckDelay_ns", 0 );

    std::string tmpName = params.find<std::string>("txSetupMod");
    Params tmpParams = params.find_prefix_params("txSetupModParams.");
    m_txSetupMod = dynamic_cast<LatencyMod*>(
            comp->loadModule( tmpName, tmpParams ) );
    assert( m_txSetupMod );

    tmpName = params.find<std::string>("rxSetupMod");
    tmpParams = params.find_prefix_params("rxSetupModParams.");
    m_rxSetupMod = dynamic_cast<LatencyMod*>(
            comp->loadModule( tmpName, tmpParams ) );
    assert( m_rxSetupMod );

    tmpName = params.find<std::string>("rxPostMod");
    if ( ! tmpName.empty() ) {
        tmpParams = params.find_prefix_params("rxPostModParams.");
        m_rxPostMod = dynamic_cast<LatencyMod*>(
            comp->loadModule( tmpName, tmpParams ) );
    }

    tmpName = params.find<std::string>("txFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("txFiniModParams.");
    m_txFiniMod = dynamic_cast<LatencyMod*>(
            comp->loadModule( tmpName, tmpParams ) );
    assert( m_txFiniMod );

    tmpName = params.find<std::string>("rxFiniMod","firefly.LatencyMod");
    tmpParams = params.find_prefix_params("rxFiniModParams.");
    m_rxFiniMod = dynamic_cast<LatencyMod*>(
            comp->loadModule( tmpName, tmpParams ) );
    assert( m_rxFiniMod );
}

inline MsgTiming::~MsgTiming()
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
