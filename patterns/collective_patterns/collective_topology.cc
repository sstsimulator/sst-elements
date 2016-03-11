//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

This object overlays (maps) a virtual topology onto the underlying
topology provided by the simulation. It doesn't know what the
underlying topology is, and simply uses a given rank and number of
ranks to create a new virtual topology.

The virtual topology is that of a tree. It is intended to implement
collective operations and has some characteristics aimed toward
that purpose. The root of the tree, placed at rank 0 in this
implementation, has it's first child num_ranks / 2 distance away.
The second child is at rank distance 1/4, then 1/8, etc.

The child at rank distance 1/2 from root has as its own first child
the rank 3 * num_ranks / 4, then 3 * num_ranks / 8, and so on. The
idea is to send messages, for a broadcast for example, a far distance
away first, so that further forking of the message stream does not
interfere with later, more local traffic.  Of course, this assumes
that the ranking provided by the underlying network resembles some
sort of physcial distance in the hardware topology.

If TREE_BINARY is selected, then a simple binary tree is created.

*/
#include <sst_config.h>

#include "collective_topology.h"



// Am I the root of the tree?
bool
Collective_topology::is_root(void)
{
    if (this_rank == root)   {
	return true;
    } else   {
	return false;
    }

}  // end of is_root()



// Am I a leaf in the tree?
// The last rank and all odd ranks are leaves
bool
Collective_topology::is_leaf(void)
{

    if (t == TREE_BINARY)   {
	if (this_rank >= (this_topology_size / 2))   {
	    return true;
	} else   {
	    return false;
	}

    } else   {
	// TREE_DEEP)
	if (this_rank == (this_topology_size - 1))   {
	    return true;
	} else   {
	    return this_rank & 1;
	}
    }

}  // end of is_leaf()



// Who is my parent?
// Flip the least significant bit that is set to 0! ;-)
int
Collective_topology::parent_rank(void)
{

    if (t == TREE_BINARY)   {
	return (this_rank - 1) / 2;
    } else    {
	// TREE_DEEP
	return this_rank & ~(1 << lsb((uint32_t)this_rank));
    }

}  // end of parent_rank()



// How many children do I have?
int
Collective_topology::num_children(void)
{

    return children.size();

}  // end of num_children()



// How many vertices are there?
int
Collective_topology::num_nodes(void)
{

    return this_topology_size;

}  // end of num_nodes()



// Generate my children
void
Collective_topology::gen_children(void)
{

int child;
int pos;


    if (t == TREE_BINARY)   {
	child= 2 * (this_rank + 1) - 1;
	if (child < this_topology_size)   {
	    children.push_front(child);
	}

	child= 2 * (this_rank + 1);
	if (child < this_topology_size)   {
	    children.push_front(child);
	}

    } else if (t == TREE_DEEP)   {
	if (this_rank == 0)   {
	    pos= lsb(next_power2((uint32_t)this_topology_size));
	} else   {
	    pos= lsb((uint32_t)this_rank);
	}
	for (int i= 0; i < pos; i++)   {
	    child= this_rank | (1 << i);
	    if (child >= this_topology_size) break;
	    children.push_front(child);
	}
    }

}  // end of gen_children()



// Count children of a rank
int
Collective_topology::num_children(int rank)
{

int child;
int pos;
int cnt;


    cnt= 0;
    if (t == TREE_BINARY)   {
	child= 2 * (rank + 1) - 1;
	if (child < this_topology_size)   {
	    cnt++;
	}

	child= 2 * (rank + 1);
	if (child < this_topology_size)   {
	    cnt++;
	}

    } else if (t == TREE_DEEP)   {
	if (rank == 0)   {
	    pos= lsb(next_power2((uint32_t)this_topology_size));
	} else   {
	    pos= lsb((uint32_t)rank);
	}
	for (int i= 0; i < pos; i++)   {
	    child= rank | (1 << i);
	    if (child >= this_topology_size) break;
	    cnt++;
	}
    }

    return cnt;

}  // end of num_children()



// Recursively count all children and children of children of a rank
int
Collective_topology::num_descendants(int rank)
{

int child;
int pos;
int cnt;


    cnt= 0;
    if (t == TREE_BINARY)   {
	child= 2 * (rank + 1) - 1;
	if (child < this_topology_size)   {
	    cnt++;
	    cnt= cnt + num_descendants(child);
	}

	child= 2 * (rank + 1);
	if (child < this_topology_size)   {
	    cnt++;
	    cnt= cnt + num_descendants(child);
	}

    } else if (t == TREE_DEEP)   {
	if (rank == 0)   {
	    pos= lsb(next_power2((uint32_t)this_topology_size));
	} else   {
	    pos= lsb((uint32_t)rank);
	}
	for (int i= 0; i < pos; i++)   {
	    child= rank | (1 << i);
	    if (child >= this_topology_size) break;
	    cnt++;
	    cnt= cnt + num_descendants(child);
	}
    }

    return cnt;

}  // end of num_descendants()



//
// A couple of bit-twidling utility functions we need
// Algorithms from http://graphics.stanford.edu/~seander/bithacks.html
//

// Find the next higher power of 2 for v
uint32_t
Collective_topology::next_power2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;

}  // end of next_power2()



// Find the number of trailing zeros in 32-bit v 
int
Collective_topology::lsb(uint32_t v)
{

    static const int MultiplyDeBruijnBitPosition[32] = 
    {
	  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
	    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];

}  // end of lsb

