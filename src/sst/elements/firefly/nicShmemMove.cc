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


void Nic::ShmemSendMoveMem::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{

    size_t space = numBytes - event.bufSize(); 
    size_t len = (m_length - m_offset) > space ? space : (m_length - m_offset); 

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: pktSpace=%lu dataLeft=%lu xferSize=%lu\n",
                vc, space, m_length - m_offset, len  );

	vec.push_back( MemOp( m_addr, len, MemOp::Op::BusDmaFromHost ));

	if ( m_ptr ) {
    	event.bufAppend( m_ptr + m_offset ,len );
	} else {
    	event.bufAppend( NULL, len );
	}

    m_offset += len; 
}

void Nic::ShmemSendMoveValue::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t space = numBytes - event.bufSize(); 
    size_t len = m_value.getLength() > space ? space : m_value.getLength(); 

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                vc, numBytes, event.bufSize(), space, len );

	vec.push_back( MemOp( 0, len, MemOp::Op::LocalLoad ));

    m_offset += len; 
    std::stringstream tmp;
    tmp << m_value;
#if 0
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value=%s\n",tmp.str().c_str());
#endif

    event.bufAppend( m_value.getPtr() ,len );
}

void Nic::ShmemSendMove2Value::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t space = numBytes - event.bufSize(); 
    assert( getLength() <= space );

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                vc, numBytes, event.bufSize(), space, getLength() );

    m_offset += getLength(); 
	vec.push_back( MemOp( 0, getLength(), MemOp::Op::LocalLoad ));

#if 0
    std::stringstream tmp1;
    tmp1 << m_value1;
    std::stringstream tmp2;
    tmp2 << m_value1;
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value1=%s value2=%s\n",tmp1.str().c_str(), tmp2.str().c_str());
#endif

    event.bufAppend( m_value1.getPtr() , m_value1.getLength() );
    event.bufAppend( m_value2.getPtr() , m_value2.getLength() );
}

bool Nic::ShmemRecvMoveMem::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();

    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize() );
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: length=%lu offset=%lu\n",m_length, m_offset );
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: backing=%p addr=%#" PRIx64 "\n", m_ptr, m_addr );

    assert( length <= m_length - m_offset );

    if ( m_ptr ) {
        memcpy(  m_ptr + m_offset, event.bufPtr(), length);
    }

    m_shmem->checkWaitOps( m_core, m_addr + m_offset, length, true );

	vec.push_back( MemOp( m_addr, length, MemOp::Op::BusDmaToHost ));

    event.bufPop(length);
    m_offset += length;

    return m_offset == m_length;
}

bool Nic::ShmemRecvMoveMemOp::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize());
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: backing=%p addr=%#" PRIx64 "\n", m_ptr, m_addr );

    assert ( m_ptr );
    size_t dataLength = Hermes::Value::getLength(m_dataType);

    while ( event.bufSize() >= dataLength ) {
        Hermes::Value src( m_dataType, event.bufPtr() );
        Hermes::Value dest( m_dataType, m_ptr + m_offset );

		vec.push_back( MemOp( m_addr, dataLength, MemOp::Op::BusLoad ));
		vec.push_back( MemOp( m_addr, dataLength, MemOp::Op::BusStore ));
#if 0
        std::stringstream tmp1;
        tmp1 << src;
        std::stringstream tmp2;
        tmp2 << dest;
#endif

        switch ( m_op ) {
          case Hermes::Shmem::AND:
            dest &= src;
            break;
          case Hermes::Shmem::OR:
            dest |= src;
            break;
          case Hermes::Shmem::XOR:
            dest ^= src;
            break;
          case Hermes::Shmem::SUM:
            dest += src;
            break;
          case Hermes::Shmem::PROD:
            dest *= src;
            break;
          case Hermes::Shmem::MIN:
            if ( src < dest ) dest = src;
            break;
          case Hermes::Shmem::MAX:
            if ( src > dest ) dest = src;
            break;
          default:
            assert(0);
        } 

#if 0
        std::stringstream tmp3;
        tmp3 << dest;
        dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: op=%d src=%s dest=%s result=%s\n",
                m_op,tmp1.str().c_str(), tmp2.str().c_str(), tmp3.str().c_str());
#endif

        m_shmem->checkWaitOps( m_core, m_addr + m_offset, dataLength, true );
        event.bufPop(dataLength);
        m_offset += dataLength;
    }
    assert( event.bufSize() == 0 );

    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");

    return m_offset == m_length;
}

bool Nic::ShmemRecvMoveValue::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize());

	vec.push_back( MemOp( 0, length, MemOp::Op::LocalStore ));
    if ( m_value.getPtr() ) {
        memcpy( m_value.getPtr(), event.bufPtr(), length);
    }

    event.bufPop(length);

    return true;
}
