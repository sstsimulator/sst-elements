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

#include "Mesh3DMachine.h"

namespace SST {
    namespace Scheduler {

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
                    std::vector<int> tempLoc(3,0);
                    MeshLocation* MLComp = new MeshLocation(tempLoc);
                    std::set<MeshLocation*, MeshLocation>* processors = new std::set<MeshLocation*, MeshLocation>(*MLComp);
                    for (int i = 0; i < dimension->dims[0]; i++) {
                        for (int j = 0; j < dimension->dims[1]; j++) {
                            for (int k = 0; k < dimension->dims[2]; k++) {
                                tempLoc[0] = location->dims[0] + i;
                                tempLoc[1] = location->dims[1] + j;
                                tempLoc[2] = location->dims[2] + k;
                                processors -> insert(new MeshLocation(tempLoc));
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
                    return dimension->dims[0] * dimension->dims[1] * dimension->dims[2];
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
                        if (b1 -> location -> equals(*(b2->location))){
                            return (*b1->dimension)(b1->dimension,b2->dimension);
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
                    return dimension -> equals(*(b->dimension)) && location -> equals(*(b->location));
                }

                std::string toString()
                {
                    std::stringstream ret;
                    ret << "Block[" << dimension->dims[0] << "x" << dimension->dims[1] << "x" << dimension->dims[2] << "]@" << location -> toString();
                    return ret.str();
                }
        };

    }
}
#endif
