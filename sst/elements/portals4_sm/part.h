// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_ELEMENTS_PORTAL4_SM_PART_H
#define SST_ELEMENTS_PORTAL4_SM_PART_H

#include <sst/core/part/sstpart.h>

namespace SST {
namespace Portals4_sm {


    
/**
   Self partitioner actually does nothing.  It is simply a pass
   through for graphs which have been partitioned during graph
   creation.
*/
class Portals4Partition : public SST::Partition::SSTPartitioner {

	public:
		/**
			Creates a new self partition scheme.
		*/
		Portals4Partition(int total_ranks);

		/**
			Performs a partition of an SST simulation configuration
			\param graph The simulation configuration to partition
		*/
		void performPartition(ConfigGraph* graph);

        bool requiresConfigGraph() { return true; }
        bool spawnOnAllRanks() { return false; }

        static SSTPartitioner* allocate(int total_ranks, int my_rank, int verbosity) {
            return new Portals4Partition(total_ranks);
    }
        
	private:
        int ranks;
        
};

}
}

#endif
