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

	SST_ELI_DOCUMENT_PARAMS(
		{"bytesPerNode","Sets bytes per FAM node","16MiB"},
		{"blockSize","Sets block size","4KiB"},
		{"numNodes","Sets number of FAM nodes","0"},
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
