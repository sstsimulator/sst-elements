// Copyright 2011 Sandia Corporation. Under the terms                          
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.             
// Government retains certain rights in this software.                         
//                                                                             
// Copyright (c) 2011, Sandia Corporation                                      
// All rights reserved.                                                        
//                                                                             
// This file is part of the SST software package. For license                  
// information, see the LICENSE file in the top level directory of the         
// distribution.                                                               

/**
 * By default the MBSAllocator provides a layered 2D mesh approach to
 * the Multi Buddy Strategy
 * A Note on Extending:  The only thing you need to do is override the initialize method,
 * create complete blocks, and make sure the "root" blocks are in the FBR.
 */

#ifndef __MBSALLOCATOR_H__
#define __MBSALLOCATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "MBSAllocClasses.h" 
#include "MachineMesh.h"
#include "MBSAllocInfo.h"
#include "Job.h"
#include "misc.h"


class MBSAllocator : public Allocator {

  protected:
    vector<set<Block*, Block>*>* FBR;
    vector<int>* ordering;

    //We know it must be a mesh, so make it one so we can access the goods.
    MachineMesh* meshMachine;


  public:
    MBSAllocator(Machine* mach);
    MBSAllocator(MachineMesh* m, int x, int y, int z);

    MBSAllocator(vector<string>* params, Machine* mach);

    string getSetupInfo(bool comment);

    string getParamHelp();

    /**
     * Initialize will fill in the FBR with z number of blocks (1 for
     * each layer) that fit into the given x,y dimensions.  It is
     * assumed that those dimensions are non-zero.
     */
    void initialize(MeshLocation* dim, MeshLocation* off);

    /**
     * Creates a rank in both the FBR, and in the ordering.
     * If a rank already exists, it does not create a new rank,
     * it just returns the one already there
     */
    int createRank(int size);

    /**
     *  Essentially this will reinitialize a block, except add the
     *  children to the b.children, then recurse
     */
    void createChildren(Block* b);

    set<Block*, Block>* splitBlock (Block* b) ;

    MBSMeshAllocInfo* allocate(Job* job);

    /**
     * Calculates the RBR, which is a map of ranks to number of blocks at that rank
     */
    map<int,int>* factorRequest(Job* j);

    /**
     * Breaks up a request for a block with a given rank into smaller request if able.
     */
    void splitRequest(map<int,int>* RBR, int rank);

    /**
     * Determines whether a split up of a possible larger block was
     * successful.  It begins looking at one larger than rank.
     */
    bool splitLarger(int rank);

    void deallocate(AllocInfo* alloc);

    void unallocate(MBSMeshAllocInfo* info);

    void mergeBlock(Block* p);

    void printRBR(map<int,int>* RBR);

    void printFBR(string msg);

    string stringFBR();

};


#endif
