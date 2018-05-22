// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_THORNHILL_MEMORY_HEAP
#define _H_THORNHILL_MEMORY_HEAP

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>

namespace SST {
namespace Thornhill {

class MemoryHeap : public Component {
  public:
    SST_ELI_REGISTER_COMPONENT(
        MemoryHeap,
        "thornhill",
        "MemoryHeap",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )
    SST_ELI_DOCUMENT_PORTS(
        {"detailed%(num_ports)d", "Port connected to Memory Heap client", {}},
    )

  public:
    MemoryHeap( ComponentId_t id, Params& params );
    ~MemoryHeap(){};

	void setup() {}
    void finish() {}
    void init( unsigned int phase ) {}

  private:
    void eventHandler( SST::Event* ev, int src ); 

    std::vector<Link*>  		m_links;
	uint64_t                    m_currentVaddr;
	Output						m_output;

	MemoryHeap() : Component(-1) {}

};

}
}
#endif
