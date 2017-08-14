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


void Nic::ShmemSendMoveMem::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{

    assert( m_ptr );
    size_t space = numBytes - event.bufSize(); 
    size_t len = m_length > space ? space : m_length; 

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                vc, numBytes, event.bufSize(), space, len );

    m_offset += len; 
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %p %x\n",m_ptr, ((int*)m_ptr)[0]);

    event.bufAppend( m_ptr ,len );
}

void Nic::ShmemSendMoveValue::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    size_t space = numBytes - event.bufSize(); 
    size_t len = m_value.getLength() > space ? space : m_value.getLength(); 

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                vc, numBytes, event.bufSize(), space, len );

    m_offset += len; 
    std::stringstream tmp;
    tmp << m_value;
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value=%s\n",tmp.str().c_str());

    event.bufAppend( m_value.getPtr() ,len );
}

void Nic::ShmemSendMove2Value::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    size_t space = numBytes - event.bufSize(); 
    assert( getLength() <= space );

    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n",
                vc, numBytes, event.bufSize(), space, getLength() );

    m_offset += getLength(); 

    std::stringstream tmp1;
    tmp1 << m_value1;
    std::stringstream tmp2;
    tmp2 << m_value1;
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: value1=%s value2=%s\n",tmp1.str().c_str(), tmp2.str().c_str());

    event.bufAppend( m_value1.getPtr() , m_value1.getLength() );
    event.bufAppend( m_value2.getPtr() , m_value2.getLength() );
}

bool Nic::ShmemRecvMoveMem::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    size_t length = event.bufSize();
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize());
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: backing=%p addr=%#" PRIx64 "\n", m_ptr, m_addr );

    if ( m_ptr ) {
        memcpy(  m_ptr, event.bufPtr(), length);
    }

    if ( m_addr ) { 
        m_shmem->checkWaitOps( m_addr, length );
    }

    event.bufPop(length);
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");

    return true;
}
bool Nic::ShmemRecvMoveValue::copyIn( Output& dbg, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    size_t length = event.bufSize();
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize());
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: backing=%p\n", m_value.getPtr() );

    memcpy( m_value.getPtr(), event.bufPtr(), length);

    event.bufPop(length);
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");

    return true;
}
