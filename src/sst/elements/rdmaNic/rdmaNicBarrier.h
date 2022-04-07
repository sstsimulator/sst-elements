// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class Barrier {

	#include "rdmaNicTree.h"

	enum { SendFanout, SendFanin, WaitFanout, WaitFanin, Finished } m_state;
  public:

	
	Barrier( RdmaNic& nic, int vc, int degree, int myNode, int numNodes ) :
			m_nic( nic ), m_vc(vc), m_tree( degree, myNode, numNodes, 0 )
	{
		init();
	}	

	bool process() { 
		switch( m_state ) {
		  case SendFanout:
			// send a pkt to each child	
			if ( ! m_nic.m_networkQ->full( m_vc ) ) {
				m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"send pkt to child %d node %d\n",m_curIndex, m_tree.calcChild(m_curIndex));
				m_nic.m_networkQ->add( m_vc, m_tree.calcChild( m_curIndex++ ), createPkt() );
			}	
			// we have sent message to all of our children wait for their response
			if ( m_curIndex == m_tree.numChildren() ) {
				m_state = WaitFanin;
			}
			break;

		  case SendFanin:
			// send packet to parent
			if ( ! m_nic.m_networkQ->full( m_vc ) ) {
				m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"send pkt to parent %d\n",m_tree.parent());
				m_nic.m_networkQ->add( m_vc, m_tree.parent(), createPkt() );
				m_state = Finished;
			}	
			break;

		  case WaitFanin:
		  case WaitFanout:
			break;

		  case Finished:
			m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"barrier is finished\n");
			// reinitializd the starting state 
			init();
			return true;
		}
		return false; 
	}

	void givePkt( RdmaNicNetworkEvent* pkt ) { 
		if ( WaitFanout == m_state ) {
			m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"get message from parent srcNode=%d\n",pkt->getSrcNode());
			assert( pkt->getSrcNode() == m_tree.parent() );

			if ( m_tree.numChildren() ) {
				// if we have children we need to send messages to our children 
				m_state = SendFanout;
			} else {
				// if we are a leaf we send a response to our parent
				m_state = SendFanin;
			}
		} else if ( WaitFanin == m_state ) {
			m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"get message from child srcNode=%d\n",pkt->getSrcNode());
			++m_rcvdCnt;
			if ( m_rcvdCnt == m_tree.numChildren() ) {
				if ( 0 == m_tree.myRank() ) { 
					m_state = Finished;
				} else {
					m_state = SendFanin;
				}
			}
		} else {
			assert(0);
		}
		delete pkt;
	}

  private:
	void init() {
		m_curIndex = 0;
		m_rcvdCnt = 0;
		if ( 0 == m_tree.myRank() ) {
			m_state = SendFanout;
		} else {
			m_state = WaitFanout;
		}
	}

	RdmaNicNetworkEvent* createPkt() {
		RdmaNicNetworkEvent* pkt = new RdmaNicNetworkEvent( RdmaNicNetworkEvent::Barrier );
		pkt->setSrcNode( m_nic.m_nicId );
        return pkt; 
    }

	RdmaNic& m_nic;
	Tree m_tree;
	int m_curIndex;
	int m_vc;
	int m_rcvdCnt;
};
