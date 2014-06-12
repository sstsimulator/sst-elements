// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
/*
 * Class to implement allocation algorithms of the family that
 * includes Gen-Alg, MM, and MC1x1; from each candidate center,
 * consider the closest points, and return the set of closest points
 * that is best->  Members of the family are specified by giving the
 * way to find candidate centers, how to measure "closeness" of points
 * to these, and how to evaluate the sets of closest points->

 GenAlg - try centering at open places;
 select L_1 closest points;
 eval with sum of pairwise L1 distances
 MM - center on intersection of grid in each direction by open places;
 select L_1 closest points
 eval with sum of pairwise L1 distances
 MC1x1 - try centering at open places
 select L_inf closest points
 eval with L_inf distance from center

 This file in particular implements the comparators, point collectors,
 generators, and scorers for use with the nearest allocators
 */

#ifndef SST_SCHEDULER_MBSALLOCCLASSES_H__
#define SST_SCHEDULER_MBSALLOCCLASSES_H__

#include <set>
#include <sstream>

//#include "sst/core/serialization.h"

namespace SST {
    namespace Scheduler {
        class MeshLocation;

        class Block : public std::binary_function<Block*,Block*,bool>{
            public:
                std::set<Block*, Block>* children;
                Block* parent;
                MeshLocation* dimension;
                MeshLocation* location;

                Block()
                {
                    //empty constructor, only to be used for comparator
                }

                Block (MeshLocation* l, MeshLocation* d) 
                {
                    dimension = d;
                    location = l;
                    Block* BComp = new Block();
                    children = new std::set<Block*, Block>(*BComp);
                    parent = NULL;
                }
                Block (MeshLocation* l, MeshLocation* d, Block* p) 
                {
                    dimension = d;
                    location = l;
                    Block* BComp = new Block();
                    children = new std::set<Block*, Block>(*BComp);
                    parent = p;
                }

                /**
                 * Returns all processors in this block
                 */
                std::set<MeshLocation*, MeshLocation>* processors()
                {
                    MeshLocation* MLComp = new MeshLocation(0,0,0);
                    std::set<MeshLocation*, MeshLocation>* processors = new std::set<MeshLocation*, MeshLocation>(*MLComp);
                    for (int i = 0;i < dimension -> x;i++) {
                        for (int j = 0;j < dimension -> y;j++) {
                            for (int k = 0;k < dimension -> z;k++) {
                                processors -> insert(new MeshLocation(location -> x+i,
                                                                      location -> y+j,
                                                                      location -> z+k));
                            }
                        }
                    }
                    return processors;
                }

                /**
                 * Calculate the number of processors in this block
                 */
                int size()
                {
                    return dimension -> x*dimension -> y*dimension -> z;
                }

                std::set<Block*, Block>* getChildren()
                {
                    return children;
                }

                void addChild(Block* b)
                {
                    children -> insert(b);
                }

                /**
                 * Below are standard class methods: equals, toString()
                 */

                bool operator()(Block*  b1, Block*  b2) const
                {
                    if (b1 -> size() == b2 -> size()){
                        if (b1 -> location -> equals(b2 -> location)){
                            return (*b1 -> dimension)(b1 -> dimension, b2 -> dimension);
                        }
                        return (*b1 -> location)(b1 -> location, b2 -> location);
                    }
                    return b1 -> size() < b2 -> size();
                }

                /**
                 * This equals method is not truly equals.  It is more of similar enough to
                 * fool their mother
                 * The reason for this is the fact that most collections of blocks are TreeSets  To test for contains
                 * it would be almost unreasonably difficult to have to construct a tree of parent/children when
                 * checking location and dimension is enough
                 */
                bool equals (Block* b)
                {
                    return dimension -> equals(b -> dimension) && location -> equals(b -> location);
                }

                std::string toString()
                {
                    std::stringstream ret;
                    ret << "Block[" << dimension -> x << "x" <<dimension -> y << "x" << dimension -> z << "]@" << location -> toString();
                    return ret.str();
                }
        };

    }
}
#endif
