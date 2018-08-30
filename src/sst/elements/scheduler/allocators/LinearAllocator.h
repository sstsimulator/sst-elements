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

/**
 * Abstract class to act as superclass for allocators based on curves.
 *
 * Format for file giving curve:
 * list of pairs of numbers (separated by whitespace)
 *   the first member of each pair is the processor 0-indexed
 *     rank if the coordinates are treated as a 3-digit number
 *     (z coord most significant, x coord least significant)
 *     these values should appear in order
 *   the second member of each pair gives its rank in the desired order
 *
 */
#ifndef SST_SCHEDULER__LINEARALLOCATOR_H__
#define SST_SCHEDULER__LINEARALLOCATOR_H__

#include <vector>
#include <stdio.h>

#include "Allocator.h"
#include "output.h"

namespace SST {
    namespace Scheduler {

        //forward declared classes
        class Machine;
        class StencilMachine;
        class Job;
        class MeshLocation;

        class LinearAllocator : public Allocator {

            protected:
                class MeshLocationOrdering : public std::binary_function<MeshLocation*, MeshLocation*, bool>{
                    //represent linear ordering

                    private:
                        int xpos,ypos,zpos; //helps because we don't know which is largest
                        int xdim;     //size of mesh in each dimension
                        int ydim;
                        int zdim;

                        int* rank;   //way to store ordering
                        //(x,y,z) has position rank[x+y*xdim+z*xdim*ydim] in ordering

                    public:
                        //helper structs and functions for Hilbert curve
                        int valtox(int x, int xoff, int y, int yoff, int rotate, int mirror);
                        int valtoy(int x, int xoff, int y, int yoff, int rotate, int mirror);
                        struct triple{
                            int d[3];
                            void operator=(triple in) {
                                d[0] = in.d[0];
                                d[1] = in.d[1];
                                d[2] = in.d[2];
                            }
                            void print()
                            {
                                schedout.output("(%d, %d, %d)\n", d[0], d[1], d[2]);
                            }
                        };
                        struct rotation{
                            int r[9];
                            rotation operator*(rotation in){
                                rotation ret;
                                ret.r[0] = r[0] * in.r[0] + r[1] * in.r[3] + r[2] * in.r[6];
                                ret.r[1] = r[0] * in.r[1] + r[1] * in.r[4] + r[2] * in.r[7];
                                ret.r[2] = r[0] * in.r[2] + r[1] * in.r[5] + r[2] * in.r[8];

                                ret.r[3] = r[3] * in.r[0] + r[4] * in.r[3] + r[5] * in.r[6];
                                ret.r[4] = r[3] * in.r[1] + r[4] * in.r[4] + r[5] * in.r[7];
                                ret.r[5] = r[3] * in.r[2] + r[4] * in.r[5] + r[5] * in.r[8];

                                ret.r[6] = r[6] * in.r[0] + r[7] * in.r[3] + r[8] * in.r[6];
                                ret.r[7] = r[6] * in.r[1] + r[7] * in.r[4] + r[8] * in.r[7];
                                ret.r[8] = r[6] * in.r[2] + r[7] * in.r[5] + r[8] * in.r[8];
                                return ret; 
                            }
                            void operator=(rotation in) {
                                for (int x = 0; x < 9; x++) {
                                    r[x] = in.r[x];
                                }
                            } 
                            void print()
                            {
                                schedout.output("[%2d %2d %2d\n %2d %2d %2d\n %2d %2d %2d]\n", r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8]);
                            }
                        };
                        int tripletoint(triple in) 
                        {
                            return (in.d[0] + in.d[1] * xdim + in.d[2] * xdim * ydim);
                        }
                        triple addwithrotate(triple origin, int coordinate, int offset, int mirror, rotation rot);
                        void threedmark(triple curpoint, rotation* rotation, int* rank, int curdim, triple* next, int* mirrors, int* inverse, int* rules);

                        MeshLocationOrdering(Machine* m, bool SORT, bool hilbert);

                        int rankOf(MeshLocation* L);

                        bool operator()(MeshLocation* L1, MeshLocation* L2){
                            return rankOf(L1) < rankOf(L2);
                        }

                        void print(){
                            schedout.output("%d %d %d", xpos, ypos, zpos);
                        }
                };

                std::vector<std::vector<MeshLocation*>*>* getIntervals();
                AllocInfo* minSpanAllocate(Job* job);
                MeshLocationOrdering* ordering;
                StencilMachine *mMachine;

            public:
                LinearAllocator(std::vector<std::string>* params, Machine* m);
        };

    }
}
#endif
