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
#include "funcSM/api.h"

#include "info.h"

namespace SST {
namespace Firefly {

class FunctionSMInterface;
class ProtocolAPI;


class FunctionSM  {

    typedef FunctionSMInterface::Retval Retval;
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
        Alltoall,
        Alltoallv,
        Irecv,
        Isend,
        Send,
        Recv,
        Wait,
        NumFunctions
    };

    FunctionSM( SST::Params& params, SST::Component*, Info&, SST::Link*, 
                                        ProtocolAPI*, ProtocolAPI* ); 
    ~FunctionSM();

    void setup();
    void start(int type, Hermes::Functor* retFunc,  SST::Event* );
    void enter( );

  private:
    void handleSelfEvent( SST::Event* );
    void handleStartEvent( SST::Event* );
    void handleToDriver(SST::Event*);
    void handleEnterEvent( SST::Event* );
    void processRetval( Retval& );
    int myNodeId() { return m_info.nodeId(); }
    int myWorldRank() { return m_info.worldRank(); }

    std::vector<FunctionSMInterface*>  m_smV; 
    FunctionSMInterface*    m_sm; 
    int                 m_type;
    Hermes::Functor*    m_retFunc;

    SST::Link*          m_fromDriverLink;    
    SST::Link*          m_toDriverLink;    
    SST::Link*          m_selfLink;
    SST::Link*          m_fromProgressLink;
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
