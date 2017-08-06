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


void Nic::ShmemMove::copyOut( Output& dbg, int vc, int numBytes, FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{

    size_t space = numBytes - event.bufSize(); 
    size_t len = m_length > space ? space : m_length; 
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %d: PacketSize=%d event.bufSpace()=%lu space=%lu len=%lu\n", vc, numBytes, event.bufSize(), space, len );

    m_offset += len; 
    dbg.verbose(CALL_INFO,3,NIC_DBG_SEND_MACHINE,"Shmem: %p %x\n",m_ptr, ((int*)m_ptr)[0]);

    event.bufAppend( m_ptr ,len );
}
