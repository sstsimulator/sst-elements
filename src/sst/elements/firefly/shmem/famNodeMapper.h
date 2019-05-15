
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
