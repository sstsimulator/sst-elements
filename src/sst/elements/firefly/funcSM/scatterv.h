// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_FUNCSM_SCATTERV_H
#define COMPONENTS_FIREFLY_FUNCSM_SCATTERV_H

#include "funcSM/api.h"
#include "funcSM/event.h"
#include "ctrlMsg.h"

namespace SST {
namespace Firefly {

class MaryTree {
  public:
    MaryTree( int radix, int myNode, int numNodes, int root ) : m_numNodes(numNodes), m_radix(radix), m_myNode(myNode), m_root(root) {
        initNode( -1, 0, m_numNodes-1, myNode );

		if ( root != 0 && ( myNode == 0 || myNode == root ) ) {
			int x;
			if ( myNode == 0  ) {
				x = root;
			} else if ( myNode == root ) {
				x = 0;
			}
			MaryTree tmp( radix, x, m_numNodes, 0 );

			m_parent = tmp.parent();
			m_numDes = tmp.numDes();

			m_children.clear();
			for ( int i = 0; i < tmp.numChildren(); i ++ ) {
				int x = tmp.calcChild(i);
				m_children.push_back( x );
			}
		}
		for ( int i = 0; i < m_children.size(); i ++ ) {
			if ( m_children[i] == root ) {
				m_children[i] = 0;
			}
		}

		int tmp = m_parent;
		if ( tmp == 0 ) {
			m_parent = root;
		} else if ( tmp == root ) {
			m_parent = 0;
		}
    }

	int numDes() {
		return m_numDes;
	}
	int size() {
		return m_numNodes;
	}
    int parent( ) {
        return m_parent;
    }

    int numChildren( ) {
        return m_children.size();
    }

    int calcChild( int n ) {
        return m_children[n];
    }

	int getOrig( int rank ) {
		int ret = rank;
		if ( rank == 0 ) {
			ret = m_root;
		} else if ( rank == m_root ) {
			ret = 0;
		}
		return ret;
	}

    int calcChildTreeSize( int n ) {
		int retval;
		if ( n < numChildren() -1 ) {
			retval = getOrig( calcChild(n+1) ) - getOrig( calcChild(n) );
		}  else {
			retval = numDes() + getOrig( myRank() ) + 1 - getOrig( calcChild(n) );
		}
		return retval;
    }

    int myRank() {
        return m_myNode;
    }

  private:

    int m_parent;
    std::vector<int> m_children;
    int m_numNodes;
    int m_radix;
    int m_myNode;
    int m_numDes;
    int m_root;
    void initNode( int parent, int root, int numDes, int want ) {
        int firstDes = root + 1;
        int size0 = numDes / m_radix;
        int size1 = size0 + 1;
        int numSize1 = numDes % m_radix;
        int numSize0 = m_radix - numSize1;
        int xfer = size0 * numSize0 + firstDes;

        assert ( size0 * numSize0 + size1 * numSize1 == numDes );

        if ( root == want ) {
            m_parent = parent;
			m_numDes = numDes;
            if ( numDes ) {
                if ( size0 ) {
                    for ( int i = 0; i < numSize0; i++ ) {
                        m_children.push_back( firstDes + i * size0 );
                    }
                }
                for ( int i = 0; i < numSize1; i++ ) {
                    m_children.push_back( xfer + i * size1 );
                }
            }
            return;
        }

        int nodesPerSubTree;
        int subTreeRoot;

        if ( want >= xfer ) {
            nodesPerSubTree = size1;
            subTreeRoot = xfer + ( (want - xfer) / nodesPerSubTree ) * nodesPerSubTree;
        } else {
            nodesPerSubTree = size0;
            subTreeRoot = firstDes + ( (want - firstDes) / nodesPerSubTree ) * nodesPerSubTree;
        }


        initNode( root, subTreeRoot, nodesPerSubTree-1, want );
    }

};

class ScattervFuncSM :  public FunctionSMInterface
{
 public:
    SST_ELI_REGISTER_MODULE(
        ScattervFuncSM,
        "firefly",
        "Scatterv",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"smallCollectiveVN","Sets the VN to use for small collectives","0"},
        {"smallCollectiveSize","Sets the size of small collectives","0"},
    )

  public:
    ScattervFuncSM( SST::Params& params ) :
        FunctionSMInterface( params ),
        m_event( NULL ),
        m_seq( 0 )
    {
        m_smallCollectiveVN = params.find<int>( "smallCollectiveVN", 0);
        m_smallCollectiveSize = params.find<int>( "smallCollectiveSize", 0);
   }

    virtual void handleStartEvent( SST::Event*, Retval& );

    virtual void handleEnterEvent( Retval& retval ) {
        Callback* tmp = m_callback;
		if ( (*tmp)()) {
		    m_dbg.debug(CALL_INFO,1,0,"Exit\n" );
        	retval.setExit( 0 );
			delete m_tree;
			delete m_event;
			m_event = NULL;
		}
		delete tmp;
	}
	virtual std::string protocolName() { return "CtrlMsgProtocol"; }

  private:

	struct RecvInfo {
		~RecvInfo( ) {
			if ( ioVec.size() > 1 ) {
				if ( ioVec[1].addr.getBacking() ) {
					free( ioVec[1].addr.getBacking() );
				}
			}
		}
		CtrlMsg::CommReq req;
    	std::vector<IoVec> ioVec;
		char* bufPos;
	};

	struct SendInfo {
		SendInfo( std::vector<int>* sizeBuf, int numChildren  ) : sizeBuf(sizeBuf), count(0), bufPos(1), reqs( numChildren )  {}
		~SendInfo() { delete sizeBuf; }
		int count;
		int bufPos;
		std::vector<int>* sizeBuf;
		std::vector< CtrlMsg::CommReq > reqs;
	};


	enum Phase { SizeMsg, DataMsg  };
    uint32_t    genTag( Phase phase) {
        return CtrlMsg::ScattervTag | (phase << 16 ) |  (m_seq & 0xffff);
    }

    CtrlMsg::API* proto() { return static_cast<CtrlMsg::API*>(m_proto); }

	bool recvSize( );
	bool recvSizeDone( std::vector<int>* );
	bool sendSize( SendInfo*, RecvInfo* );
	bool sendSizeWait( SendInfo*, RecvInfo* );
	bool dataWait( SendInfo*, RecvInfo* );
	bool dataSend( SendInfo*, RecvInfo* );
	bool dataSendWait( SendInfo*, RecvInfo* );
	bool exit( SendInfo*, RecvInfo* );

	typedef std::function<bool(void)> Callback;

	Callback* m_callback;

    ScattervStartEvent*	m_event;
    int                 m_seq;
    MaryTree*           m_tree;
    int m_smallCollectiveVN;
    int m_smallCollectiveSize;
};

}
}

#endif
