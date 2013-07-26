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
#include "sst/core/serialization/element.h"

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

#include "ctrlMsg.h"

using namespace SST::Firefly;
using namespace Hermes;

FunctionSM::FunctionSM( int verboseLevel, Output::output_location_t loc,
                    SST::Component* obj, Info& info, ProtocolAPI* dm,
                     IO::Interface* io, ProtocolAPI* ctrlMsg ) :
    m_sm( NULL ),
    m_nodeId( -1 ),
    m_worldRank( -1 ),
    m_info( info )
{
    m_dbg.init("@t:FunctionSM::@p():@l ", verboseLevel,0, loc );

    m_smV.resize( NumFunctions );

#if 0
    m_funcLat.resize(FunctionCtx::NumFunctions);
    m_funcLat[FunctionCtx::Init] = 10;
    m_funcLat[FunctionCtx::Fini] = 10;
    m_funcLat[FunctionCtx::Rank] = 1;
    m_funcLat[FunctionCtx::Size] = 1;
    m_funcLat[FunctionCtx::Barrier] = 100;
    m_funcLat[FunctionCtx::Irecv] = 1;
    m_funcLat[FunctionCtx::Isend] = 1;
    m_funcLat[FunctionCtx::Send] = 1;
    m_funcLat[FunctionCtx::Recv] = 1;
    m_funcLat[FunctionCtx::Wait] = 1;
#endif

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
                                        m_toProgressLink, ctrlMsg, io );
    m_smV[Rank] = new RankFuncSM( verboseLevel, loc, &info );
    m_smV[Size] = new SizeFuncSM( verboseLevel, loc, &info );
    m_smV[Send] = new SendFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm, io );
    m_smV[Wait] = new WaitFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm, io );
    m_smV[Recv] = new RecvFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, dm, io, m_selfLink );
    m_smV[Barrier] = new BarrierFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg, io );
    m_smV[Allreduce] = new AllreduceFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg, io );
    m_smV[Reduce] = new AllreduceFuncSM( verboseLevel, loc, &info,
                                        m_toProgressLink, ctrlMsg, io );
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
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_sm->name());
    m_fromProgressLink->send( e );
}

void FunctionSM::start( SST::Event* e  )
{
    SMEnterEvent* event =  static_cast<SMEnterEvent*>(e);
    event->retLink = m_toDriverLink;
    m_sm = m_smV[ event->type ];
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_sm->name());
    m_fromDriverLink->send( e );
}

void FunctionSM::handleSelfEvent( SST::Event* e  )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_sm->name());
    m_sm->handleSelfEvent( e );
}


void FunctionSM::handleDriverEvent( SST::Event* e )
{
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_sm->name());
    m_sm->handleEnterEvent( e );
}

void FunctionSM::handleProgressEvent( SST::Event* e )
{
    assert( m_sm );
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_sm->name());
    m_sm->handleProgressEvent( e );
}

void FunctionSM::handleToDriver( Event* e )
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    DriverEvent* event = static_cast<DriverEvent*>(e);
    (*event->retFunc)( event->retval );
    delete e;
}

