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

#include <stdio.h>
#include <mpi.h>
#include <string>
#include <set>

#include "collective_topology.h"

using namespace std;


#include <sstream>

// From http://notfaq.wordpress.com/2006/08/30/c-convert-int-to-string/
template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

int
main(int argc, char *argv[])
{

int my_rank;
int num_ranks;
Collective_topology *ctopo;
string root;
string leaf;
int parent;
string children ("");


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    ctopo= new Collective_topology(my_rank, num_ranks);
    if (ctopo->is_root())   {
	root= "yes";
    } else   {
	root= " no";
    }

    if (ctopo->is_leaf())   {
	leaf= "yes";
    } else   {
	leaf= " no";
    }

    parent= ctopo->parent_rank();

    list<int>::iterator it;
    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	children += to_string(*it);
	children += ", ";
    }

    printf("Rank %3d, root, %s, leaf %s, parent %3d, num children: %3d: %s\n",
	my_rank, root.c_str(), leaf.c_str(), parent, ctopo->num_children(), children.c_str());

    delete ctopo;


    // Some more rigourous testing
    if (my_rank == 0)   {
	for (num_ranks= 1; num_ranks < 1024; num_ranks++)   {
	    int roots;
	    int leaves;
	    set <int>nodes;

	    roots= 0;
	    leaves= 0;
	    nodes.insert(0);

	    // Each rank adds its children to the list "nodes". At the end, each
	    // rank should be represented in there once and only once
	    for (my_rank= 0; my_rank < num_ranks; my_rank++)   {
		ctopo= new Collective_topology(my_rank, num_ranks);
		if (ctopo->is_root()) roots++;
		if (ctopo->is_leaf()) leaves++;

		for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		    nodes.insert(*it);
		}
		delete ctopo;
	    }

	    for (int i= 0; i < num_ranks; i++)   {
		nodes.erase(nodes.find(i));
	    }
	    // Must be empty now!
	    if (nodes.size() != 0)   {
		fprintf(stderr, "ERROR: children test failed!\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }

	    if (roots != 1)   {
		fprintf(stderr, "ERROR: Num_ranks = %d: Number of roots != 1 (%d)!\n", num_ranks, roots);
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }

	    if (leaves != (num_ranks + 1) / 2)   {
		fprintf(stderr, "ERROR: Num_ranks = %d: Number of leaves != %d (%d)!\n",
		    num_ranks, (num_ranks + 1) / 2, leaves);
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	}
	printf("All testing concluded\n");
    }

    MPI_Finalize();
}
