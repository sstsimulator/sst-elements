// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "rdmaNic.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

void RdmaNic::NetworkQueue::process()
{
    for ( int i =0; i < queues.size(); i++ ) {
        if ( ! queues[i].empty() ) {
            Entry& entry = queues[i].front();

            if( nic.m_linkControl->spaceToSend( i, entry.ev->getPayloadSizeBytes() * 8 ) ) {
                nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"send packet to node %d pktlen=%zu\n",entry.destNode, entry.ev->getData().size());

                Interfaces::SimpleNetwork::Request* req = new Interfaces::SimpleNetwork::Request();
                req->dest = entry.destNode;
                req->src = nic.m_nicId;
                req->size_in_bits = entry.ev->getPayloadSizeBytes() * 8;
                req->vn = i;
                req->givePayload( entry.ev );
                nic.m_linkControl->send( req, i );
                queues[i].pop();
            }
        }
    }
}
