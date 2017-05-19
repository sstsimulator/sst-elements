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

#ifndef _H_THORNHILL_MEMORY_HEAP
#define _H_THORNHILL_MEMORY_HEAP

#include <sst/core/component.h>

namespace SST {
namespace Thornhill {

class MemoryHeap : public Component {

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
