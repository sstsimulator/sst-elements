// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCTIONSM_H
#define COMPONENTS_FIREFLY_FUNCTIONSM_H

#include "sst/core/output.h"

#include "sst/elements/hermes/msgapi.h"
#include "ioapi.h"

#include "info.h"

namespace SST {
namespace Firefly {

class FunctionSMInterface;
class ProtocolAPI;

class FunctionSM {

  public:
    enum FunctionType {
        Init,
        Fini,
        Rank,
        Size,
        Barrier,
        Allreduce,
        Reduce,
        Allgather,
        Allgatherv,
        Gather,
        Gatherv,
        Irecv,
        Isend,
        Send,
        Recv,
        Wait,
        NumFunctions
    };

    FunctionSM( SST::Params& params,
                SST::Component*, Info&, ProtocolAPI*, IO::Interface*,
                ProtocolAPI* ); 
    ~FunctionSM();

    void start( SST::Event* );
    void sendProgressEvent( SST::Event* );

    void setProgressLink( SST::Link* link ) {
        m_toProgressLink = link;
    }

  private:
    void handleSelfEvent( SST::Event* );
    void handleDriverEvent( SST::Event* );
    void handleToDriver(SST::Event*);
    void handleProgressEvent( SST::Event* );
    int myNodeId() { return m_info.nodeId(); }
    int myWorldRank() { return m_info.worldRank(); }

    std::vector<FunctionSMInterface*>  m_smV; 
    FunctionSMInterface*  m_sm; 
    SST::Link*          m_fromDriverLink;    
    SST::Link*          m_toDriverLink;    
    SST::Link*          m_selfLink;
    SST::Link*          m_fromProgressLink;
    SST::Link*          m_toProgressLink;
    int                 m_nodeId;
    int                 m_worldRank;

    Info&               m_info;

    Output              m_dbg;

    struct FunctionTimes {
        int enterTime;
    };

    std::vector<FunctionTimes>    m_funcLat;

    void setFunctionTimes( FunctionType type, int enterTime ) {
        FunctionTimes tmp;
        tmp.enterTime = enterTime;
        m_funcLat[type] = tmp;
    }
};

}
}

#endif
