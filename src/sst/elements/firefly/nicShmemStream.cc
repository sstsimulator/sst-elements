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

Nic::RecvMachine::ShmemStream::ShmemStream( Output& output, FireflyNetworkEvent* ev,
       RecvMachine& rm, int unit, int srcNode, int srcPid, int destPid ) : 
    StreamBase(output, rm, unit, srcNode, srcPid, destPid )
{
    m_hdr = *(MsgHdr*) ev->bufPtr();
    m_shmemHdr = *(ShmemMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    ev->bufPop(sizeof(MsgHdr) + sizeof(m_shmemHdr) );

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"srcNode=%d srcPid=%d\n", m_srcNode, m_srcPid );

    m_rm.nic().schedCallback( 
            std::bind( &Nic::RecvMachine::ShmemStream::processOp, this, m_shmemHdr, ev, m_myPid, m_srcPid ),
            m_rm.nic().getShmemRxDelay_ns() ); 
}

void Nic::RecvMachine::ShmemStream::processOp( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid ) 
{
    switch ( m_shmemHdr.op ) { 
        
      case ShmemMsgHdr::Put: 
    	if ( ! m_shmemHdr.respKey ) { 
        	processPut( m_shmemHdr, ev, m_myPid, m_srcPid );
		} else {
        	processGetResp( m_shmemHdr, ev, m_myPid, m_srcPid );
		}
        break;

      case ShmemMsgHdr::Get: 
        processGet( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      case ShmemMsgHdr::Add: 
        processAdd( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      case ShmemMsgHdr::Fadd: 
        processFadd( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      case ShmemMsgHdr::Cswap: 
        processCswap( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      case ShmemMsgHdr::Swap: 
        processSwap( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      case ShmemMsgHdr::Ack: 
        processAck( m_shmemHdr, ev, m_myPid );
        break;

      default:
        assert(0);
        break;
    }
}

void Nic::RecvMachine::ShmemStream::processAck( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Ack srcNode=%d\n",ev->getSrcNode());

    m_rm.m_nic.shmemDecPending( pid );
    m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, pid, this ) );  

    delete ev;
}

void Nic::RecvMachine::ShmemStream::processPut( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation opNum=%d myAddr=%#" PRIx64 " length=%u\n",
            m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length);
        
    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 

    if ( hdr.op2 == Hermes::Shmem::MOVE ) {
        m_recvEntry = new ShmemRecvEntry( m_rm.m_nic.m_shmem, local_pid, addr, hdr.length );
    }else{
        m_recvEntry = new ShmemRecvEntry( m_rm.m_nic.m_shmem, local_pid, addr, hdr.length,
                            (Hermes::Shmem::ReduOp) hdr.op2, 
                            (Hermes::Value::Type) hdr.dataType );
    } 

	m_sendEntry = new ShmemAckSendEntry( local_pid, ev->getSrcNode(), dest_pid );

	m_matched_len = hdr.length;

    processPkt( ev );
}

void Nic::RecvMachine::ShmemStream::processGetResp( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d respKey=%d\n",
            m_shmemHdr.op, m_shmemHdr.respKey);

    ShmemRespSendEntry* entry = (ShmemRespSendEntry*) m_rm.nic().getRespKeyValue(hdr.respKey);

    if ( entry->getCmd()->type == NicShmemCmdEvent::Getv || 
           entry->getCmd()->type == NicShmemCmdEvent::Fadd || 
           entry->getCmd()->type == NicShmemCmdEvent::Swap || 
           entry->getCmd()->type == NicShmemCmdEvent::Cswap ) {
        m_recvEntry = new ShmemGetvRespRecvEntry( m_rm.m_nic.m_shmem, hdr.length, static_cast<ShmemGetvSendEntry*>(entry) );
    } else {
        Hermes::Vaddr addr = static_cast<NicShmemGetCmdEvent*>(entry->getCmd())->getMyAddr();

        Hermes::MemAddr memAddr = m_rm.nic().findShmem( local_pid, addr, hdr.length ); 

        if ( ! static_cast<NicShmemGetCmdEvent*>(entry->getCmd())->isBlocking() ) {
	        m_rm.m_nic.shmemDecPending( local_pid );
        }

        m_recvEntry = new ShmemGetbRespRecvEntry( m_rm.m_nic.m_shmem, local_pid, hdr.length, static_cast<ShmemGetbSendEntry*>(entry), 
                memAddr.getBacking() );
    }

	m_matched_len = hdr.length;

    processPkt( ev );
}

void Nic::RecvMachine::ShmemStream::processGet( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);

    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

	void* backing = addr.getBacking();
	Hermes::Vaddr simVaddr = addr.getSimVAddr();
    int srcNode = ev->getSrcNode();

    m_rm.nic().calcNicMemDelay( m_unit, memOps, 
			[=]() {
    			m_rm.nic().m_sendMachine[0]->run( new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, backing, 
                	hdr.length, hdr.respKey, simVaddr ) );
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, local_pid, this ) );  
			}
    );	

    delete ev;
}

void Nic::RecvMachine::ShmemStream::processAdd( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation srcNode=%d op=%d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            ev->getSrcNode(), m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);
    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) );

    Hermes::Value local( (Hermes::Value::Type) hdr.dataType, addr.getBacking());

    if ( addr.getBacking() ) {
        Hermes::Value got( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );
        local += got;
    }

	m_matched_len = hdr.length;

	Hermes::Vaddr tmpAddr = addr.getSimVAddr(); 
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore, 
		[=]() {
			m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"processAdd() checkWaitOps\n");
			m_rm.m_nic.m_shmem->checkWaitOps( local_pid, tmpAddr, local.getLength() );
		}
	) ); 

    int srcNode = ev->getSrcNode();
   	m_rm.nic().calcNicMemDelay( m_unit, memOps,
			[=]() {
				m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"processAdd() send Ack to %d\n",srcNode);
				m_sendEntry = new ShmemAckSendEntry( local_pid, srcNode, dest_pid );
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::deleteStream, &m_rm, this ) );  
			}
		);
    m_rm.clearStreamMap( local_pid, getSrcKey() );
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processFadd( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);
    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 

	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) );

   	Hermes::Value* save = new Hermes::Value( (Hermes::Value::Type)hdr.dataType );
    Hermes::Value local( (Hermes::Value::Type) hdr.dataType, addr.getBacking());

    if ( addr.getBacking() ) {
        Hermes::Value got( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );

        *save = local;
        local += got;
    }

	Hermes::Vaddr tmpAddr = addr.getSimVAddr();

	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore,
		[=]() {
			m_rm.m_nic.m_shmem->checkWaitOps( local_pid, tmpAddr, local.getLength() );
		}
	) ); 

    int srcNode = ev->getSrcNode();
   	m_rm.nic().calcNicMemDelay( m_unit, memOps, 
			[=]() {
    			m_rm.nic().m_sendMachine[0]->run( new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey ) );
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, local_pid, this ) ); 
			}
	);	
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processSwap( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);
    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) );

    Hermes::Value local( (Hermes::Value::Type)hdr.dataType, addr.getBacking());
    Hermes::Value* save = new Hermes::Value( (Hermes::Value::Type)hdr.dataType );
    if ( addr.getBacking() ) {
        Hermes::Value swap( (Hermes::Value::Type)hdr.dataType, ev->bufPtr() );
        *save = local;
        local = swap;
   }

	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore ) );

    int srcNode = ev->getSrcNode();
   	m_rm.nic().calcNicMemDelay( m_unit, memOps,
			[=]() {
    			m_rm.nic().m_sendMachine[0]->run( new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey ) );
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, local_pid, this ) );  
			}	
	);
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processCswap( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.op, m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);
    Hermes::MemAddr addr = m_rm.nic().findShmem( local_pid, hdr.vaddr, hdr.length ); 
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) * 2 );

	assert ( addr.getBacking() );

    Hermes::Value local( (Hermes::Value::Type) hdr.dataType, addr.getBacking());
    Hermes::Value* save = new Hermes::Value( (Hermes::Value::Type) hdr.dataType );
    *save = local;
    Hermes::Value swap( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );
    ev->bufPop(swap.getLength());
    Hermes::Value cond( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );

	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );

    if ( local == cond ) {
        memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore ) );
        local = swap;
    }

    int srcNode = ev->getSrcNode();
   	m_rm.nic().calcNicMemDelay( m_unit, memOps,
		    [=]() {
    			m_rm.nic().m_sendMachine[0]->run( new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey ) );
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, local_pid, this ) );  
			}
	);
    delete ev;
}
