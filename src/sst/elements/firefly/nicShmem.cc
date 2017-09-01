// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

void Nic::Shmem::regMem( NicShmemRegMemCmdEvent* event, int id )
{
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    m_dbg.verbose(CALL_INFO,1,1,"Shmem: simVAddr=%" PRIx64 " backing=%p len=%lu\n", 
            event->addr.getSimVAddr(), event->addr.getBacking(), event->len );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::RegMem, event->callback );

    m_regMem.push_back( std::make_pair(event->addr, event->len) );
    delete event;
}

void Nic::Shmem::wait( NicShmemOpCmdEvent* event, int id )
{
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    std::stringstream tmp;
    tmp << event->value;
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: addr=%" PRIx64 " value=%s\n", event->addr, tmp.str().c_str() );

    Op* op = new WaitOp(  event, getBacking( event->addr, event->value.getLength() ),
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::wait complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Wait, event->callback );
            } 
        );

    if ( ! op->checkOp( ) ) { 
        m_pendingOps.push_back( op); 
    } else {
        delete op;
    }
}

void Nic::Shmem::checkWaitOps( Hermes::Vaddr addr, size_t length )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: addr=%" PRIx64" len=%lu\n", addr, length );

    std::list<Op*>::iterator iter = m_pendingOps.begin();

    while ( iter != m_pendingOps.end() ) {


        m_dbg.verbose(CALL_INFO,1,1,"Shmem: check op\n" );
        Op* op = *iter;
        if ( op->inRange( addr, length  ) && op->checkOp() ) {
            delete op; 
            iter = m_pendingOps.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Nic::Shmem::put( NicShmemPutCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    if( event->getNode() == m_nic.getNodeId() ) {

        assert( event->getOp() == Hermes::Shmem::ReduOp::MOVE );

        void* dest = getBacking( event->getFarAddr(), event->getLength() );
        void* src = getBacking( event->getMyAddr(), event->getLength() );
        memcpy( dest, src, event->getLength() );

        m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Put, event->getCallback() );
    } else {

        ShmemPutSendEntry* entry = new ShmemPutbSendEntry( id, event, getBacking( event->getMyAddr(), event->getLength() ), 
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::put complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Put, event->getCallback() );
            } 
        ); 

        m_nic.m_sendMachine[0]->run( entry );
    }
}

void Nic::Shmem::putv( NicShmemPutvCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    if( event->getNode() == m_nic.getNodeId() ) {

        Hermes::Value local( event->getDataType(), 
                getBacking( event->getFarAddr(), event->getLength() ) );

        m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Putv, event->getCallback() );
        local = event->getValue();
        std::stringstream tmp;
        tmp << local << " " << event->getValue();

    } else {

        ShmemPutSendEntry* entry = new ShmemPutvSendEntry( id, event,
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::put complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Putv, event->getCallback() );
            } 
        ); 

        m_nic.m_sendMachine[0]->run( entry );
    }
}

void Nic::Shmem::getv( NicShmemGetvCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    assert( event->getNode() != m_nic.getNodeId() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    ShmemGetvSendEntry* entry = new ShmemGetvSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::getv complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Getv, event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::get( NicShmemGetCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    assert( event->getNode() != m_nic.getNodeId() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    ShmemGetbSendEntry* entry = new ShmemGetbSendEntry( id, event, 
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::getv complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Get, event->getCallback() );
            } 
    );  

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::add( NicShmemAddCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    if( event->getNode() == m_nic.getNodeId() ) {

		assert(0);
#if 0
        Hermes::Value local( event->getDataType(), 
                getBacking( event->getFarAddr(), event->getLength() ) );

        m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Putv, event->getCallback() );
        local = event->getValue();
        std::stringstream tmp;
        tmp << local << " " << event->getValue();
#endif

	} else {
    	ShmemAddSendEntry* entry = new ShmemAddSendEntry( id, event, 
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::add complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Add, event->getCallback() );
            } 
    	); 

    	m_nic.m_sendMachine[0]->run( entry );
	}
}

void Nic::Shmem::fadd( NicShmemFaddCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    assert( event->getNode() != m_nic.getNodeId() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    ShmemFaddSendEntry* entry = new ShmemFaddSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::fadd complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Fadd, event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::cswap( NicShmemCswapCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    assert( event->getNode() != m_nic.getNodeId() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    ShmemCswapSendEntry* entry = new ShmemCswapSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::cswap complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Cswap, event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::swap( NicShmemSwapCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: far=%" PRIx64" len=%lu\n", event->getFarAddr(), event->getLength() );

    assert( event->getNode() != m_nic.getNodeId() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::FreeCmd, NULL );

    ShmemSwapSendEntry* entry = new ShmemSwapSendEntry( id, event, 
            [=](Hermes::Value& value) {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::swap complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Swap, event->callback, value );
            } 
    ); 

    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::fence( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: \n");
    assert(0);
#if 0
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Fence, NULL );
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Fence, event->callback );
#endif
}
