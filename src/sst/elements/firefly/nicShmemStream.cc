// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

Nic::RecvMachine::ShmemStream::ShmemStream( Output& output, Ctx* ctx,
        int srcNode, int srcPid, int destPid, FireflyNetworkEvent* ev) :
    StreamBase(output, ctx, srcNode, srcPid, destPid ), m_blocked(true)
{
    m_shmemHdr = *(ShmemMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    ev->bufPop(sizeof(MsgHdr) + sizeof(m_shmemHdr) );

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"core=%d %s srcNode=%d srcCore=%d this=%p\n",
            m_myPid, m_shmemHdr.getOpStr().c_str(), ev->getSrcNode(),m_srcPid, this);

    if ( m_shmemHdr.op != ShmemMsgHdr::Ack  ) {
        m_ctx->nic().schedCallback( std::bind( &Nic::RecvMachine::ShmemStream::processFirstPkt, this, ev ),
            m_ctx->nic().getShmemRxDelay_ns() );
    } else {
        processPktHdr(ev);
    }
}

void Nic::RecvMachine::ShmemStream::processOp( FireflyNetworkEvent* ev )
{
    if ( m_shmemHdr.op == ShmemMsgHdr::Ack ) {
        m_unit = m_ctx->allocAckUnit();
    } else {
        m_unit = m_ctx->allocRecvUnit();
    }

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"core=%d %s srcNode=%d srcCore=%d\n",
            m_myPid, m_shmemHdr.getOpStr().c_str(), ev->getSrcNode(),m_srcPid);

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
        processAck( m_shmemHdr, ev, m_myPid, m_srcPid );
        break;

      default:
        assert(0);
        break;
    }
}

void Nic::RecvMachine::ShmemStream::processAck( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int pid, int srcPid  )
{
    m_ctx->nic().shmemDecPendingPuts( pid );
    m_ctx->deleteStream(this);

    delete ev;
}

void Nic::RecvMachine::ShmemStream::processPut( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"myAddr=%#" PRIx64 " length=%u\n", m_shmemHdr.vaddr, m_shmemHdr.length);

    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );

    if ( hdr.op2 == Hermes::Shmem::MOVE ) {
        m_recvEntry = new ShmemRecvEntry( m_ctx->getShmem(), local_pid, addr, hdr.length );
    }else{
        m_recvEntry = new ShmemRecvEntry( m_ctx->getShmem(), local_pid, addr, hdr.length,
                            (Hermes::Shmem::ReduOp) hdr.op2,
                            (Hermes::Value::Type) hdr.dataType );
    }

	m_sendEntry = new ShmemAckSendEntry( local_pid, ev->getSrcNode(), dest_pid, m_ctx->nic().m_shmemAckVN );

	m_matched_len = hdr.length;

    ev->clearHdr();
    processPktBody( ev );
}

void Nic::RecvMachine::ShmemStream::processGetResp( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"respKey=%d\n", m_shmemHdr.respKey);

    ShmemRespSendEntry* entry = (ShmemRespSendEntry*) m_ctx->nic().getRespKeyValue(hdr.respKey);

    if ( entry->getCmd()->type == NicShmemCmdEvent::Getv ||
           entry->getCmd()->type == NicShmemCmdEvent::Fadd ||
           entry->getCmd()->type == NicShmemCmdEvent::Swap ||
           entry->getCmd()->type == NicShmemCmdEvent::Cswap ) {
        m_recvEntry = new ShmemGetvRespRecvEntry( m_ctx->getShmem(), hdr.length, static_cast<ShmemGetvSendEntry*>(entry) );
    } else {
        Hermes::Vaddr addr = static_cast<NicShmemGetCmdEvent*>(entry->getCmd())->getMyAddr();

        Hermes::MemAddr memAddr = m_ctx->findShmem( local_pid, addr, hdr.length );

        m_recvEntry = new ShmemGetbRespRecvEntry( m_ctx->getShmem(), local_pid, hdr.length, static_cast<ShmemGetbSendEntry*>(entry),
                memAddr.getBacking() );
    }

	m_matched_len = hdr.length;

    ev->clearHdr();
    processPktBody( ev );
}

void Nic::RecvMachine::ShmemStream::processGet( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);

    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

	void* backing = addr.getBacking();
	Hermes::Vaddr simVaddr = addr.getSimVAddr();
    int srcNode = ev->getSrcNode();

    int vn = m_ctx->nic().m_shmemGetSmallVN;
    if ( hdr.length > m_ctx->nic().m_shmemGetThresholdLength ) {
        vn = m_ctx->nic().m_shmemGetLargeVN;
    }
    m_ctx->calcNicMemDelay( m_unit, memOps,
			[=]() {
    			m_ctx->runSend(0, new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, backing, hdr.length, hdr.respKey, simVaddr, vn ) );
                m_ctx->deleteStream( this );
			}
    );

    delete ev;
}

void Nic::RecvMachine::ShmemStream::processAdd( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"srcNode=%d myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            ev->getSrcNode(), m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);

    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );
	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) );

    Hermes::Value local( (Hermes::Value::Type) hdr.dataType, addr.getBacking());

    if ( addr.getBacking() ) {
        Hermes::Value got( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );
        local += got;
    }

    size_t localLen=local.getLength();
	m_matched_len = hdr.length;

	Hermes::Vaddr tmpAddr = addr.getSimVAddr();
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore,
		[=]() {
			m_dbg.debug(CALL_INFO_LAMBDA, "processAdd",1,NIC_DBG_RECV_STREAM,"checkWaitOps\n");
			m_ctx->checkWaitOps( local_pid, tmpAddr, localLen );
		}
	) );

    int srcNode = ev->getSrcNode();
   	m_ctx->calcNicMemDelay( m_unit, memOps,
			[=]() {
				m_dbg.debug(CALL_INFO_LAMBDA, "processAdd",1,NIC_DBG_RECV_STREAM,"send Ack to %d\n",srcNode);
				m_sendEntry = new ShmemAckSendEntry( local_pid, srcNode, dest_pid, m_ctx->nic().m_shmemAckVN );
                m_ctx->deleteStream(this);
			}
		);
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processFadd( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);
    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );

	std::vector< MemOp >* memOps = new std::vector< MemOp >;

    assert( ev->bufSize() == Hermes::Value::getLength((Hermes::Value::Type)hdr.dataType) );

   	Hermes::Value* save = new Hermes::Value( (Hermes::Value::Type)hdr.dataType );
    Hermes::Value local( (Hermes::Value::Type) hdr.dataType, addr.getBacking());

    if ( addr.getBacking() ) {
        Hermes::Value got( (Hermes::Value::Type) hdr.dataType, ev->bufPtr() );

        *save = local;
        local += got;
    }

    size_t localLen=local.getLength();
	Hermes::Vaddr tmpAddr = addr.getSimVAddr();

	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusLoad ) );
	memOps->push_back( MemOp( addr.getSimVAddr(), local.getLength(), MemOp::Op::BusStore,
		[=]() {
			m_ctx->checkWaitOps( local_pid, tmpAddr, localLen );
		}
	) );

    int vn = m_ctx->nic().m_shmemGetSmallVN;
    if ( hdr.length > m_ctx->nic().m_shmemGetThresholdLength ) {
        vn = m_ctx->nic().m_shmemGetLargeVN;
    }
    int srcNode = ev->getSrcNode();
   	m_ctx->calcNicMemDelay( m_unit, memOps,
			[=]() {
    			m_ctx->runSend( 0, new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey, vn ) );
                m_ctx->deleteStream( this );
			}
	);
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processSwap( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);

    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );
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

    int vn = m_ctx->nic().m_shmemGetSmallVN;
    if ( hdr.length > m_ctx->nic().m_shmemGetThresholdLength ) {
        vn = m_ctx->nic().m_shmemGetLargeVN;
    }

    int srcNode = ev->getSrcNode();
   	m_ctx->calcNicMemDelay( m_unit, memOps,
			[=]() {
    			m_ctx->runSend( 0, new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey, vn ) );
                m_ctx->deleteStream( this );
			}
	);
    delete ev;
}

void Nic::RecvMachine::ShmemStream::processCswap( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_pid, int dest_pid )
{
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"myAddr=%#" PRIx64 " length=%u respKey=%d\n",
            m_shmemHdr.vaddr, m_shmemHdr.length, m_shmemHdr.respKey);

    Hermes::MemAddr addr = m_ctx->findShmem( local_pid, hdr.vaddr, hdr.length );
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
    size_t localLen=local.getLength();

    if ( local == cond ) {
        memOps->push_back( MemOp( addr.getSimVAddr(), localLen, MemOp::Op::BusStore ) );
        local = swap;
    }

    int vn = m_ctx->nic().m_shmemGetSmallVN;
    if ( hdr.length > m_ctx->nic().m_shmemGetThresholdLength ) {
        vn = m_ctx->nic().m_shmemGetLargeVN;
    }
    int srcNode = ev->getSrcNode();
   	m_ctx->calcNicMemDelay( m_unit, memOps,
		    [=]() {
    			m_ctx->runSend( 0, new ShmemPut2SendEntry( local_pid, srcNode, dest_pid, save, hdr.respKey, vn ) );
                m_ctx->deleteStream( this );
			}
	);
    delete ev;
}
