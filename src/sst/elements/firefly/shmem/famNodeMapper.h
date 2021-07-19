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

#ifndef _H_FIREFLY_SHMEM_FAM_NODE_MAPPER
#define _H_FIREFLY_SHMEM_FAM_NODE_MAPPER

#include <sst/core/module.h>

namespace SST {
namespace Firefly {

class FamNodeMapper : public Module {
  public:
	virtual int calcNode( int node ) = 0;
	virtual void setDbg( Output* output ) { assert(9); }
  protected:
	Output* m_dbg;
};


class Group_FamNodeMapper : public FamNodeMapper {
  public:

    SST_ELI_REGISTER_MODULE(
        Group_FamNodeMapper,
        "firefly",
        "Group_FamNodeMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Firefly::Group_FamNodeMapper"
    )

	SST_ELI_DOCUMENT_PARAMS(
		{"firstNode","Sets first node of map","0"},
		{"nodeStride","Sets stride between groups of nodes","1"},
		{"nodesPerGroup","Set number of nodes per group","1"},
	)

  public:
	Group_FamNodeMapper( Params& params ) {
		m_firstNode = (int) params.find<int>("firstNode", 0);
		m_nodeStride = (int) params.find<int>("nodeStride",1);
		m_nodesPerGroup = (int) params.find<int>("nodesPerGroup",1);
	}

	void setDbg( Output* output ) {
		m_dbg = output;
		m_dbg->debug(CALL_INFO,3,0,"firstNode=%d nodeStride=%d nodesPerGroup %d\n", m_firstNode, m_nodeStride, m_nodesPerGroup );
	}

	virtual int calcNode( int node ) {
		int groupNum = node/m_nodesPerGroup;
		int newNode = m_firstNode + (groupNum * m_nodeStride) + (node % m_nodesPerGroup);
		m_dbg->debug(CALL_INFO,3,0,"node %d -> %d\n",node,newNode);
		return newNode;
	};

  private:
	int	m_firstNode;
	int	m_nodeStride;
	int m_nodesPerGroup;
};

}
}

#endif
