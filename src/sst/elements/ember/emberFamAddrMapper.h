
#ifndef _H_EMBER_SHMEM_FAM_ADDR_MAPPER
#define _H_EMBER_SHMEM_FAM_ADDR_MAPPER

#include <sst/core/module.h>
#include <sst/core/elementinfo.h>

class FamAddrMapper : public SST::Module {
  public:
	virtual void getAddr( Output& output, uint64_t globalOffset, int& node, uint64_t& localOffset ) = 0;
  protected:
	size_t 	m_bytesPerNode;
	int	   	m_numNodes;
	int    	m_start;
	int	   	m_interval;
	int    	m_blockSize;
	int		m_totalBlocks;
};


class Group_FamAddrMapper : public FamAddrMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        Group_FamAddrMapper,
        "ember",
        "Group_FamAddrMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::Group_FamAddrMapper"
    )

  public:
	Group_FamAddrMapper( Params& params ) {
		m_bytesPerNode = (size_t) params.find<SST::UnitAlgebra>("bytesPerNode","16MiB").getRoundedValue();
		m_blockSize = (int) params.find<SST::UnitAlgebra>("blockSize","4KiB").getRoundedValue();
		m_start = (int) params.find<int>("start", 0);
		m_interval = (int) params.find<int>("interval",1);
		m_numGroups = (int) params.find<int>("numGroups",1);
		m_groupSize = (int) params.find<int>("groupSize",1);
		m_numNodes = m_groupSize * m_numGroups;

		m_totalBlocks = m_numNodes * m_bytesPerNode/m_blockSize;

#if 0 
		printf("numNodes=%d bytesPerNode=%zu blockSize=%d m_startNode=%d interval=%d groupSize=%d numGroups=%d\n",
					m_numNodes, m_bytesPerNode, m_blockSize, m_start, m_interval, m_groupSize, m_numGroups );
#endif
	}

	virtual void getAddr( Output& output, uint64_t globalOffset, int& node, uint64_t& localOffset ) {

		uint64_t block = globalOffset/m_blockSize;	
		
		if ( block >= m_totalBlocks ) {
			output.fatal(CALL_INFO, -1,"Group_FamAddrMapper addr 0x%" PRIx64 " is out of range maxAddr 0x%" PRIx64 "\n", globalOffset, (uint64_t ) m_blockSize* m_totalBlocks  );
		}

		int tmp = (block % m_numNodes);

		node = m_start;
		node += ( tmp / m_groupSize ) * m_interval;
		node += tmp % m_groupSize;

		localOffset = (block / m_numNodes) * m_blockSize;
		localOffset += globalOffset % m_blockSize;
		output.debug(CALL_INFO,3,0,"globalOffset=%#" PRIx64" blockNum=%" PRIu64 " node=%d localOffset=0x%" PRIx64 "\n", globalOffset, block, node, localOffset );
		node |= 1<<31;

	};

  private:

	int m_groupSize;
	int m_numGroups;

};

class RR_FamAddrMapper : public FamAddrMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        RR_FamAddrMapper,
        "ember",
        "RR_FamAddrMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::RR_FamAddrMapper"
    )

  public:
	RR_FamAddrMapper( Params& params ) {
		m_bytesPerNode = (size_t) params.find<SST::UnitAlgebra>("bytesPerNode","16MiB").getRoundedValue();
		m_blockSize = (int) params.find<SST::UnitAlgebra>("blockSize","4KiB").getRoundedValue();
		m_start = (int) params.find<int>("start", 0);
		m_interval = (int) params.find<int>("interval",1);
		m_numNodes = (int) params.find<int>("numNodes",0);

		m_totalBlocks = m_numNodes * m_bytesPerNode/m_blockSize;

		//printf("numNodes=%d bytesPerNode=%zu blockSize=%d m_startNode=%d interval=%d\n",m_numNodes, m_bytesPerNode, m_blockSize, m_start, m_interval );
	}

	virtual void getAddr( Output& output, uint64_t globalOffset, int& node, uint64_t& localOffset ) {

		uint64_t block = globalOffset/m_blockSize;	
		
		if ( block >= m_totalBlocks ) {
			output.fatal(CALL_INFO, -1,"RR_FamAddrMapper addr 0x%" PRIx64 " is out of range maxAddr 0x%" PRIx64 "\n", globalOffset, (uint64_t ) m_blockSize* m_totalBlocks  );
		}

		node = (block % m_numNodes ) * m_interval + m_start;
		node |= 1<<31;
		localOffset = (block / m_numNodes) * m_blockSize;
		localOffset += globalOffset % m_blockSize;

		output.debug(CALL_INFO,3,0,"globalOffset=%#" PRIx64" blockNum=%" PRIu64 " node=%#x localOffset=0x%" PRIx64 "\n", globalOffset, block, node, localOffset );
	};

  private:

};


class RandomNode_FamAddrMapper : public FamAddrMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        RandomNode_FamAddrMapper,
        "ember",
        "RandomNode_FamAddrMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::RandomNode_FamAddrMapper"
    )

  public:
	RandomNode_FamAddrMapper( Params& params ) {
		m_bytesPerNode = (size_t) params.find<SST::UnitAlgebra>("bytesPerNode","16MiB").getRoundedValue();
		m_blockSize = (int) params.find<SST::UnitAlgebra>("blockSize","4KiB").getRoundedValue();
		std::string nidList = (std::string)params.find<std::string>("nidList","");
		m_numNodes = (int) params.find<int>("numNodes",0);

		nidList += ',';

		m_nodeMap.reserve( m_numNodes );
		int pos = 0;

		size_t strPos = 0;
		while ( strPos < nidList.size() ) {
			size_t tmp = nidList.find(',',strPos); 

			m_nodeMap.push_back( atoi( nidList.substr( strPos, tmp - strPos ).c_str() ) );
			strPos = tmp + 1; 
		}	

		m_totalBlocks = m_numNodes * m_bytesPerNode/m_blockSize;
		//printf("numNodes=%d bytesPerNode=%zu blockSize=%d\n",m_numNodes, m_bytesPerNode, m_blockSize );
		assert( m_nodeMap.size() == m_numNodes );

	}

	virtual void getAddr( Output& output, uint64_t globalOffset, int& node, uint64_t& localOffset ) {

		uint64_t block = globalOffset/m_blockSize;	
		int tmp;
		
		if ( block >= m_totalBlocks ) {
			output.fatal(CALL_INFO, -1,"RR_FamAddrMapper addr 0x%" PRIx64 " is out of range maxAddr 0x%" PRIx64 "\n", globalOffset, (uint64_t ) m_blockSize* m_totalBlocks  );
		}
        node = block % m_numNodes;

		assert( node < m_nodeMap.size() ); 

		node = m_nodeMap[node];
		node |= 1<<31;
		localOffset = (block / m_numNodes) * m_blockSize;

		output.debug(CALL_INFO,3,0,"globalOffset=%" PRIx64" blockNum=%" PRIx64 " node=%#x localOffset=0x%" PRIx64 "\n", globalOffset, block, node, localOffset );
	};

  private:
	std::vector<int> m_nodeMap;

};

#endif
