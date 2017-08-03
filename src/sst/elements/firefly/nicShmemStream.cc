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
       RecvMachine& rm  ) : 
    StreamBase(output, rm )
{
    m_hdr = *(MsgHdr*) ev->bufPtr();
    m_shmemHdr = *(ShmemMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );


    ev->bufPop(sizeof(MsgHdr) + sizeof(m_shmemHdr) );

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"SHMEM Operation %d simVAddrSrc=%#" PRIx64 " simVaddrDest=%#" PRIx64 " length=%lu key=%#" PRIx64 "\n",
            m_shmemHdr.op, m_shmemHdr.simVAddrSrc, m_shmemHdr.simVAddrDest, m_shmemHdr.length, m_shmemHdr.key);

    Callback callback;
    switch ( m_shmemHdr.op ) { 
        
      case ShmemMsgHdr::Put: 
        callback = processPut( m_shmemHdr, ev );
        break;

      case ShmemMsgHdr::Get: 
        callback = processGet( m_shmemHdr, ev, m_hdr.dst_vNicId, m_hdr.src_vNicId );
        break;

      default:
        assert(0);
        break;
    }
    m_rm.nic().schedCallback( callback, 0 );
}

Nic::RecvMachine::ShmemStream::Callback Nic::RecvMachine::ShmemStream::processPut( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"destSimVAddr=%#" PRIx64 "\n", hdr.simVAddrDest);

    if ( hdr.simVAddrDest ) { 
    std::pair<Hermes::MemAddr, size_t> region = m_rm.nic().findShmem( hdr.simVAddrDest );

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"found region simVAddr=%#" PRIx64 " length=%lu\n",
            region.first.getSimVAddr(), region.second );

    uint64_t offset =  hdr.simVAddrDest - region.first.getSimVAddr();

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"simVaddr=%#" PRIx64 " backing=%p length=%lu\n",
            region.first.getSimVAddr(), region.first.getBacking(), region.second ); 

    assert(  hdr.simVAddrDest + hdr.length <= region.first.getSimVAddr() + region.second );


    m_recvEntry = new ShmemRecvEntry( region.first.offset(offset), hdr.length, hdr.key );
    } else {
    m_recvEntry = new ShmemRecvEntry( hdr.length, hdr.key );
    }

    return std::bind( &Nic::RecvMachine::state_move_0, &m_rm, ev, this );
}

Nic::RecvMachine::ShmemStream::Callback Nic::RecvMachine::ShmemStream::processGet( ShmemMsgHdr& hdr, FireflyNetworkEvent* ev, int local_vNic, int dest_vNic )
{
    std::pair<Hermes::MemAddr, size_t> region = m_rm.nic().findShmem( hdr.simVAddrSrc );

    uint64_t offset =  hdr.simVAddrSrc - region.first.getSimVAddr();

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"simVaddr=%#" PRIx64 " backing=%p length=%lu\n",
            region.first.getSimVAddr(), region.first.getBacking(), region.second ); 

    assert(  hdr.simVAddrSrc + hdr.length < region.first.getSimVAddr() + region.second );
    void* ptr =  region.first.offset(offset).getBacking();

    m_rm.nic().m_sendMachine[0]->run( new ShmemPut2SendEntry( local_vNic, ev->src, dest_vNic, hdr.simVAddrDest, ptr, hdr.length, hdr.key ) );

    return std::bind( &Nic::RecvMachine::state_move_2, &m_rm, ev );
}

bool Nic::ShmemRecvEntry::copyIn( Output& dbg,
                  FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    size_t length = event.bufSize();

    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: event.bufSize()=%lu\n",event.bufSize());

    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Shmem: backing=%p simVAddr=%#" PRIx64 "\n",
            m_addr.getBacking(), m_addr.getSimVAddr()  );

    if ( m_addr.getBacking() ) {
        memcpy(  m_addr.getBacking() , event.bufPtr(), length);
    }

    event.bufPop(length);
    dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");

    return true;
}
