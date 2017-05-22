// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
    m_notifySendPioDone(NULL),
    m_notifyRecvDmaDone(NULL),
    m_notifyNeedRecv(NULL)
{
    m_dbg.init("@t:VirtNic::@p():@l ", 
        params.find<uint32_t>("verboseLevel",0),
        0,
        Output::STDOUT );

    m_toNicLink = owner->configureLink( params.find<std::string>("portName","nic"), 
			"1 ns", new Event::Handler<VirtNic>(this,&VirtNic::handleEvent) );

    assert( m_toNicLink );
}

VirtNic::~VirtNic()
{
    if ( m_notifyGetDone ) delete m_notifyGetDone;
    if ( m_notifySendPioDone ) delete m_notifySendPioDone;
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
    case NicRespEvent::PioSend:
        (*m_notifySendPioDone)( event->key );
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

void VirtNic::regMem( int node, int tag, std::vector<IoVec>& vec, void* key )
{
    m_dbg.verbose(CALL_INFO,2,0,"node=%d\n",node);
    m_toNicLink->send(0, new NicCmdEvent( NicCmdEvent::RegMemRgn, 
			calcCoreId(node), calcRealNicId(node), tag, vec, key ) );
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

void VirtNic::setNotifyOnGetDone(VirtNic::HandlerBase<void*>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifyGetDone = functor;
}

void VirtNic::setNotifyNeedRecv(
                VirtNic::HandlerBase3Args<int,int,size_t>* functor) 
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_notifyNeedRecv = functor;
}
