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

#include <sst_config.h>
#include "sst/core/serialization.h"

#include "functionSM.h"
#include "funcSM/init.h"
#include "funcSM/fini.h"
#include "funcSM/rank.h"
#include "funcSM/size.h"
#include "funcSM/send.h"
#include "funcSM/recv.h"
#include "funcSM/wait.h"
#include "funcSM/barrier.h"
#include "funcSM/allreduce.h"
#include "funcSM/gatherv.h"
#include "funcSM/allgather.h"

#include "ctrlMsg.h"

using namespace SST::Firefly;
using namespace Hermes;

FunctionSM::FunctionSM( SST::Params& params,
                        SST::Component* obj, Info& info, ProtocolAPI* dm,
                        IO::Interface* io, ProtocolAPI* ctrlMsg ) :
    m_sm( NULL ),
    m_nodeId( -1 ),
    m_worldRank( -1 ),
    m_info( info )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc = 
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:FunctionSM::@p():@l ", verboseLevel, 0, loc );

    m_smV.resize( NumFunctions );

    m_funcLat.resize(NumFunctions);

    Params timeParams = params.find_prefix_params("times.");
    
    int defaultTime = timeParams.find_integer("default",0);

    setFunctionTimes( Init, timeParams.find_integer("Init", defaultTime ) );
    setFunctionTimes( Fini, timeParams.find_integer("Fini", defaultTime ) );
    setFunctionTimes( Rank, timeParams.find_integer("Rank", defaultTime ) );
    setFunctionTimes( Size, timeParams.find_integer("Size", defaultTime ) );
    setFunctionTimes( Send, timeParams.find_integer("Send", defaultTime ));
    setFunctionTimes( Recv, timeParams.find_integer("Recv", defaultTime ) );
    setFunctionTimes( Wait, timeParams.find_integer("Wait", defaultTime ) );
    setFunctionTimes( Barrier, timeParams.find_integer("Barrier", defaultTime ) );
    setFunctionTimes( Allreduce, timeParams.find_integer("Allreduce", defaultTime ) );
    setFunctionTimes( Reduce, timeParams.find_integer("Reduce", defaultTime ) );
    setFunctionTimes( Allgather, timeParams.find_integer("Allgather", defaultTime ) );
    setFunctionTimes( Allgatherv, timeParams.find_integer("Allgatherv", defaultTime ) );
    setFunctionTimes( Gather, timeParams.find_integer("Gather", defaultTime ) );
    setFunctionTimes( Gatherv, timeParams.find_integer("Gatherv",defaultTime ));

    m_toDriverLink = obj->configureSelfLink("ToDriver", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleToDriver));

    m_fromDriverLink = obj->configureSelfLink("FromDriver", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleDriverEvent));
    assert( m_fromDriverLink );

    m_fromProgressLink = obj->configureSelfLink("FromProgress", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleProgressEvent));
    assert( m_fromProgressLink );

    m_selfLink = obj->configureSelfLink("funtionSMselfLink", "1 ps",
        new Event::Handler<FunctionSM>(this,&FunctionSM::handleSelfEvent));
    assert( m_selfLink );

    m_smV[Init] = new InitFuncSM( verboseLevel, loc, &info );
    m_smV[Fini] = new FiniFuncSM( verboseLevel, loc, &info, 
                                        m_toProgressLink, ctrlMsg);
    m_smV[Rank] = new RankFuncSM( verboseLevel, loc, &info );
    m_smV[Size] = new SizeFuncSM( verboseLevel, loc, &info );
    m_smV[Send] = new SendFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm );
    m_smV[Wait] = new WaitFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm );
    m_smV[Recv] = new RecvFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm, m_selfLink );
    m_smV[Barrier] = new BarrierFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Allreduce] = new AllreduceFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Reduce] = new AllreduceFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Allgather] = new AllgatherFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Allgatherv] = new AllgatherFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Gather] = new GathervFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
    m_smV[Gatherv] = new GathervFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg );
}

FunctionSM::~FunctionSM()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    for ( unsigned int i=0; i < m_smV.size(); i++ ) {
        delete m_smV[i];
    }
    delete m_fromDriverLink;
}

void FunctionSM::sendProgressEvent( SST::Event* e  )
{
    m_dbg.verbose(CALL_INFO,3,0,"%s\n",m_sm->name());
    m_fromProgressLink->send( e );
}

void FunctionSM::start( SST::Event* e  )
{
    SMEnterEvent* event =  static_cast<SMEnterEvent*>(e);
    event->retLink = m_toDriverLink;
    m_sm = m_smV[ event->type ];
    m_dbg.verbose(CALL_INFO,3,0,"%s\n",m_sm->name());
    m_fromDriverLink->send( m_funcLat[event->type].enterTime, e );
}

void FunctionSM::handleSelfEvent( SST::Event* e  )
{
    m_dbg.verbose(CALL_INFO,3,0,"\n");
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,3,0,"%s\n",m_sm->name());
    m_sm->handleSelfEvent( e );
}


void FunctionSM::handleDriverEvent( SST::Event* e )
{
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,3,0,"%s\n",m_sm->name());
    m_sm->handleEnterEvent( e );
}

void FunctionSM::handleProgressEvent( SST::Event* e )
{
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,3,0,"%s\n",m_sm->name());
    m_sm->handleProgressEvent( e );
}

void FunctionSM::handleToDriver( Event* e )
{
    m_dbg.verbose(CALL_INFO,3,0,"\n");
    DriverEvent* event = static_cast<DriverEvent*>(e);
    (*event->retFunc)( event->retval );
    delete e;
}

