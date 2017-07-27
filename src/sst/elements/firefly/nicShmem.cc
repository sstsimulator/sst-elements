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

void Nic::Shmem::regMem( NicShmemCmdEvent* event, int id )
{
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::RegMem, NULL );

    m_dbg.verbose(CALL_INFO,1,1,"Shmem: simVAddr=%" PRIx64 " backing=%p\n", 
            event->src.getSimVAddr(), event->src.getBacking() );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::RegMem, event->callback );
    m_regMem.push_back( std::make_pair(event->src, event->len) );
}

void Nic::Shmem::put( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: src=%" PRIx64 ",%p dest=%" PRIx64",%p len=%lu\n", 
            event->src.getSimVAddr(), event->src.getBacking(),
            event->dest.getSimVAddr(), event->dest.getBacking(), event->len );


    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::PutV, NULL );

    ShmemPutSendEntry* entry;
    
    if ( event->type == NicShmemCmdEvent::PutP ) {
        entry = new ShmemPutPSendEntry( id, event,
            [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::put complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::PutP, event->callback );
            } ); 
    } else {
        entry = new ShmemPutVSendEntry( id, event, [=]() {
                m_dbg.verbose(CALL_INFO,1,1,"Nic::Shmem::put complete\n");
                m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::PutV, event->callback );
            } ); 
    }
    m_nic.m_sendMachine[0]->run( entry );
}

void Nic::Shmem::get( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: src=%" PRIx64 ",%p dest=%" PRIx64",%p len=%lu\n", 
            event->src.getSimVAddr(), event->src.getBacking(),
            event->dest.getSimVAddr(), event->dest.getBacking(), event->len );

    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::GetV,  NULL );

    m_nic.m_sendMachine[0]->run( new ShmemGetSendEntry( id, event ) );
}

void Nic::Shmem::fence( NicShmemCmdEvent* event, int id )
{
    m_dbg.verbose(CALL_INFO,1,1,"Shmem: \n");
    assert(0);
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Fence, NULL );
    m_nic.getVirtNic(id)->notifyShmem( NicShmemRespEvent::Fence, event->callback );
}
