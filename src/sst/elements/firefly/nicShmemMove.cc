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


void Nic::ShmemSendMoveMem::copyOut( Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{

    size_t bufSpace = numBytes - event.bufSize();
    size_t left = m_length - m_offset;
    size_t len;

    if ( left > bufSpace  ) {
        len = bufSpace & ~( m_alignment - 1);
    } else {
        len = left;
    }

    dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"pktSpace=%lu dataLeft=%lu xferSize=%lu addr=%" PRIx64 "\n",
                bufSpace, left, len, m_addr + m_offset  );

	vec.push_back( MemOp( m_addr + m_offset, len, MemOp::Op::BusDmaFromHost ));

	if ( m_ptr ) {
    	event.bufAppend( m_ptr + m_offset ,len );
	} else {
    	event.bufAppend( NULL, len );
	}

    m_offset += len;
}

void Nic::ShmemSendMoveValue::copyOut( Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t space = numBytes - event.bufSize();
    size_t len = m_value.getLength() > space ? space : m_value.getLength();

    dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                numBytes, event.bufSize(), space, len );

	vec.push_back( MemOp( 0, len, MemOp::Op::LocalLoad ));

    m_offset += len;
    std::stringstream tmp;
    tmp << m_value;
#if 0
    dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value=%s\n",tmp.str().c_str());
#endif

    event.bufAppend( m_value.getPtr() ,len );
}

void Nic::ShmemSendMove2Value::copyOut( Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t space = numBytes - event.bufSize();
    assert( getLength() <= space );

    dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                numBytes, event.bufSize(), space, getLength() );

    m_offset += getLength();
	vec.push_back( MemOp( 0, getLength(), MemOp::Op::LocalLoad ));

#if 0
    std::stringstream tmp1;
    tmp1 << m_value1;
    std::stringstream tmp2;
    tmp2 << m_value1;
    dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value1=%s value2=%s\n",tmp1.str().c_str(), tmp2.str().c_str());
#endif

    event.bufAppend( m_value1.getPtr() , m_value1.getLength() );
    event.bufAppend( m_value2.getPtr() , m_value2.getLength() );
}

bool Nic::ShmemRecvMoveMem::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();

    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"event.bufSize()=%lu\n",event.bufSize() );
    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"length=%lu offset=%lu\n",m_length, m_offset );
    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"backing=%p addr=%#" PRIx64 "\n", m_ptr + m_offset, m_addr + m_offset );

    assert( length <= m_length - m_offset );

    if ( m_ptr ) {
        memcpy(  m_ptr + m_offset, event.bufPtr(), length);
    }

	size_t tmpOffset = m_addr + m_offset;
	int tmpCore = m_core;

	vec.push_back( MemOp( m_addr + m_offset, length, MemOp::Op::BusDmaToHost,
		[=] () {
			m_shmem->checkWaitOps( tmpCore, tmpOffset, length );
		}
	));

    event.bufPop(length);
    m_offset += length;

    return m_offset == m_length;
}

bool Nic::ShmemRecvMoveMemOp::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();
    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"event.bufSize()=%lu\n",event.bufSize());
    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"backing=%p addr=%#" PRIx64 "\n", m_ptr, m_addr );

    assert ( m_ptr );
    size_t dataLength = Hermes::Value::getLength(m_dataType);

    while ( event.bufSize() >= dataLength ) {
        Hermes::Value src( m_dataType, event.bufPtr() );
        Hermes::Value dest( m_dataType, m_ptr + m_offset );

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
        dbg.debug(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: op=%d src=%s dest=%s result=%s\n",
                m_op,tmp1.str().c_str(), tmp2.str().c_str(), tmp3.str().c_str());
#endif

		size_t tmpOffset = m_addr + m_offset;
		int tmpCore = m_core;
		vec.push_back( MemOp( m_addr, dataLength, MemOp::Op::BusLoad ));
		vec.push_back( MemOp( m_addr, dataLength, MemOp::Op::BusStore,
			[=]() {
        		m_shmem->checkWaitOps( tmpCore, tmpOffset, dataLength );
			}
 		));

        event.bufPop(dataLength);
        m_offset += dataLength;
    }
    assert( event.bufSize() == 0 );

    dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MOVE,"\n");

    return m_offset == m_length;
}

bool Nic::ShmemRecvMoveValue::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<MemOp>& vec )
{
    size_t length = event.bufSize();
    dbg.debug(CALL_INFO,3,NIC_DBG_RECV_MOVE,"event.bufSize()=%lu\n",event.bufSize());

	vec.push_back( MemOp( 0, length, MemOp::Op::LocalStore ));
    if ( m_value.getPtr() ) {
        memcpy( m_value.getPtr(), event.bufPtr(), length);
    }

    event.bufPop(length);

    return true;
}
