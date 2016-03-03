//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _COLLECTIVE_TOPOLOGY_H
#define _COLLECTIVE_TOPOLOGY_H

#include <stdio.h>
#include <stdint.h>
#include <list>
#if !defined(NOT_PART_OF_SIM)
#include "patterns.h"
#endif


typedef enum {TREE_BINARY, TREE_DEEP} tree_type_t;


class Collective_topology   {
    public:
	Collective_topology(int const rank, int const num_ranks, tree_type_t const tree_type) :
	    root(0),
	    this_rank(rank),
	    this_topology_size(num_ranks),
	    t(tree_type)

	{
	    switch (t)   {
		case TREE_BINARY:
		case TREE_DEEP:
		    break;
		default:
		    fprintf(stderr, "%s: Unkown tree type!\n", __FUNCTION__);
	    }

	    // Generate the children of each node up front
	    gen_children();
	}

        ~Collective_topology() {}

	bool is_root(void);
	bool is_leaf(void);
	int parent_rank(void);
	int num_children(void);
	int num_children(int rank);
	int num_descendants(int rank);
	int num_nodes(void);
	int my_rank(void)   {return this_rank;}

	const int root;
	std::list <int>children;


    private:
	const int this_rank;
	const int this_topology_size;
	const tree_type_t t;

	// Some utility functions we need
	int lsb(uint32_t v);
	void gen_children(void);
	uint32_t next_power2(uint32_t v);

};

#endif // _COLLECTIVE_TOPOLOGY_H
