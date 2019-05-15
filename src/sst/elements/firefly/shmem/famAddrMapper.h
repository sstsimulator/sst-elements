
#ifndef _H_FIREFLY_SHMEM_FAM_ADDR_MAPPER
#define _H_FIREFLY_SHMEM_FAM_ADDR_MAPPER

#include <sst/core/module.h>

namespace SST {
namespace Firefly {

class FamAddrMapper : public SST::Module {
  public:
	virtual void getAddr( uint64_t globalOffset, int& node, uint64_t& localOffset ) = 0;
	void setDbg( Output* output ) {
		m_dbg = output; 
		m_dbg->debug(CALL_INFO,3,0,"numNodes=%d bytesPerNode=%" PRIu64 " blockSize=%" PRIu64 "\n",
						m_numNodes, m_bytesPerNode, m_blockSize );
	}

	uint32_t blockSize() { return m_blockSize; }
  protected:
	int 	m_numNodes;
	uint64_t 	m_blockSize;
	uint64_t 	m_bytesPerNode;
	int		m_totalBlocks;
	Output* m_dbg;
};

class RR_FamAddrMapper : public FamAddrMapper {
  public:
    SST_ELI_REGISTER_MODULE(
        RR_FamAddrMapper,
        "firefly",
        "RR_FamAddrMapper",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Firefly::RR_FamAddrMapper"
    )

  public:
	RR_FamAddrMapper( Params& params ) {
		m_bytesPerNode = (size_t) params.find<SST::UnitAlgebra>("bytesPerNode","16MiB").getRoundedValue();
		m_blockSize = (size_t) params.find<SST::UnitAlgebra>("blockSize","4KiB").getRoundedValue();
		m_numNodes = (int) params.find<int>("numNodes",0);

		m_totalBlocks = m_numNodes * m_bytesPerNode/m_blockSize;

	}

	virtual void getAddr( uint64_t globalOffset, int& node, uint64_t& localOffset ) {

		uint64_t block = globalOffset/m_blockSize;	
		
		if ( block >= m_totalBlocks ) {
			m_dbg->fatal(CALL_INFO, -1,"RR_FamAddrMapper addr 0x%" PRIx64 " is out of range maxAddr 0x%" PRIx64 "\n",
						globalOffset, (uint64_t ) m_blockSize* m_totalBlocks  );
		}

		node = block % m_numNodes;
		localOffset = (block / m_numNodes) * m_blockSize;
		localOffset += globalOffset % m_blockSize;

		m_dbg->debug(CALL_INFO,3,0,"globalOffset=%#" PRIx64" blockNum=%" PRIu64 " node=%#x localOffset=0x%" PRIx64 "\n",
						globalOffset, block, node, localOffset );
	};

  private:

};

}
}

#endif
