
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
        node = block % m_numNodes;

		node = (block % m_numNodes ) * m_interval + m_start;
		node |= 1<<31;
		localOffset = (block / m_numNodes) * m_blockSize;

		output.debug(CALL_INFO,3,0,"globalOffset=%" PRIx64" blockNum=%" PRIx64 " node=%#x localOffset=0x%" PRIx64 "\n", globalOffset, block, node, localOffset );
	};

  private:

};

#endif
