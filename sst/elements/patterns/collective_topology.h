//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _COLLECTIVE_TOPOLOGY_H
#define _COLLECTIVE_TOPOLOGY_H

#include <list>
#include <stdint.h>



class Collective_topology   {
    public:
	Collective_topology(int const rank, int const num_ranks) :
	    root(0),
	    this_rank(rank),
	    this_topology_size(num_ranks)

	{
	    // Generate the children of each node up front
	    gen_children();
	}

        ~Collective_topology() {}

	bool is_root(void);
	bool is_leaf(void);
	int parent_rank(void);

	const int root;
	std::list <int>children;


    private:
	const int this_rank;
	const int this_topology_size;

	// Some utility functions we need
	int lsb(uint32_t v);
	uint32_t next_power2(uint32_t v);
	void gen_children(void);

};

#endif // _COLLECTIVE_TOPOLOGY_H
