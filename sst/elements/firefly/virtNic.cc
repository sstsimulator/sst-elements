// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include <sst/core/link.h>
#include <sst/core/params.h>
#include "virtNic.h"
#include "nic.h"

using namespace SST::Firefly;
using namespace SST;

VirtNic::VirtNic( Component* owner, Params& params ) :
    m_realNicId(-1),
    m_notifyGetDone(NULL),
    m_notifyPutDone(NULL),
    m_notifySendPioDone(NULL),
    m_notifySendDmaDone(NULL),
    m_notifyRecvDmaDone(NULL),
    m_notifyNeedRecv(NULL)
{
    m_dbg_level = params.find_integer("debugLevel",0);
    m_dbg_loc = (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:VirtNic::@p():@l ", m_dbg_level, 0, m_dbg_loc );

    m_toNicLink = owner->configureLink( params.find_string("portName","nic"), 
			"1 ns", new Event::Handler<VirtNic>(this,&VirtNic::handleEvent) );

    assert( m_toNicLink );
}

VirtNic::~VirtNic()
{
    if ( m_notifyPutDone ) delete m_notifyPutDone;
    if ( m_notifyGetDone ) delete m_notifyGetDone;
    if ( m_notifySendPioDone ) delete m_notifySendPioDone;
    if ( m_notifySendDmaDone ) delete m_notifySendDmaDone;
    if ( m_notifyRecvDmaDone ) delete m_notifyRecvDmaDone;
    if ( m_notifyNeedRecv ) delete m_notifyNeedRecv;
}

void VirtNic::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"phase=%d\n",phase);

    if ( 1 == phase ) {
        NicInitEvent* ev = 
                        static_cast<NicInitEvent*>(m_toNicLink->recvInitData());
        assert( ev );
        m_realNicId = ev->node;
        m_coreId = ev->vNic;
        m_numCores = ev->num_vNics;
        delete ev;
    
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:VirtNic::@p():@l ", m_realNicId,
							m_coreId );
        m_dbg.setPrefix( buffer );

        m_dbg.verbose(CALL_INFO,1,0,"we are nic=%d core=%d\n",
                        m_realNicId, m_coreId );
    }
}

void VirtNic::handleEvent( Event* ev )
{
    NicRespEvent* event = static_cast<NicRespEvent*>(ev);

    m_dbg.verbose(CALL_INFO,2,0,"%d\n",event->type);

    switch( event->type ) {
    case NicRespEvent::Get:
        (*m_notifyGetDone)( event->key );
        break;
    case NicRespEvent::Put:
        (*m_notifyPutDone)( event->key );
        break;
    case NicRespEvent::PioSend:
        (*m_notifySendPioDone)( event->key );
        break;
    case NicRespEvent::DmaSend:
        (*m_notifySendDmaDone)( event->key );
        break;
    case NicRespEvent::DmaRecv:
        (*m_notifyRecvDmaDone)( calcNodeId( event->node, event->src_vNic ),
                    event->tag, event->len, event->key  );
        break;
    case NicRespEvent::NeedRecv:
        (*m_notifyNeedRecv)( calcNodeId( event->node, event->src_vNic),
                    event->tag, event->len );
        break;
    default:
        assert(0);
    }
    delete ev;
}

bool VirtNic::canDmaSend()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    //return m_nic.canDmaSend( this ); 
    return true;
}


bool VirtNic::canDmaRecv()
{ 
    m_dbg.verbose(CALL_INFO,1,0,"\n");
//    return m_nic.canDmaRecv( this ); 
    return true;
}

void VirtNic::dmaSend( int dest, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"dest=%d\n",dest);

    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::DmaSend, 
            calcCoreId(dest), calcRealNicId(dest), tag, vec, key ) );
}
void VirtNic::dmaRecv( int src, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"src=%d\n",src);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::DmaRecv,
            calcCoreId(src), calcRealNicId(src), tag, vec, key ) );
}

void VirtNic::pioSend( int dest, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"dest=%d\n",dest);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::PioSend, 
			calcCoreId(dest), calcRealNicId(dest), tag, vec, key ) );
}

void VirtNic::get( int node, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"node=%d\n",node);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::Get, 
			calcCoreId(node), calcRealNicId(node), tag, vec, key ) );
}

void VirtNic::put( int node, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"node=%d\n",node);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::Get, 
			calcCoreId(node), calcRealNicId(node), tag, vec, key ) );
}

void VirtNic::regMem( int node, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"node=%d\n",node);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::RegMemRgn, 
			calcCoreId(node), calcRealNicId(node), tag, vec, key ) );
}

void VirtNic::setNotifyOnSendDmaDone(VirtNic::HandlerBase<void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifySendDmaDone = functor;
}

void VirtNic::setNotifyOnRecvDmaDone(
                VirtNic::HandlerBase4Args<int,int,size_t,void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifyRecvDmaDone = functor;
}

void VirtNic::setNotifyOnSendPioDone(VirtNic::HandlerBase<void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifySendPioDone = functor;
}

void VirtNic::setNotifyOnPutDone(VirtNic::HandlerBase<void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifyPutDone = functor;
}

void VirtNic::setNotifyOnGetDone(VirtNic::HandlerBase<void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifyGetDone = functor;
}

void VirtNic::setNotifyNeedRecv(
                VirtNic::HandlerBase3Args<int,int,size_t>* functor) 
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_notifyNeedRecv = functor;
}
