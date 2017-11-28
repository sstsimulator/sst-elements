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

class MsgStream : public StreamBase {
  public:
    MsgStream( Output&, FireflyNetworkEvent*, RecvMachine& );
    virtual void processPkt( FireflyNetworkEvent* ev, DmaRecvEntry* entry = NULL ) {
			
		if ( ! m_recvEntry ) {
			if ( entry ) {
				m_recvEntry = entry;
			} else { 
				m_recvEntry = static_cast<DmaRecvEntry *>( m_rm.nic().findRecv( m_src, m_hdr, m_tag ) );
			}
			
			assert(m_recvEntry);

       		ev->bufPop( sizeof(MsgHdr) );
        	ev->bufPop( sizeof(m_tag) );
		}

    	m_rm.state_move_0( ev, this );
    }
};
