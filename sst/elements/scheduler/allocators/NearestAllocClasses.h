// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
/*
 * Class to implement allocation algorithms of the family that
 * includes Gen-Alg, MM, and MC1x1; from each candidate center,
 * consider the closest points, and return the set of closest points
 * that is best.  Members of the family are specified by giving the
 * way to find candidate centers, how to measure "closeness" of points
 * to these, and how to evaluate the sets of closest points.

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

#ifndef SST_SCHEDULER_NEARESTALLOCCLASSES_H__
#define SST_SCHEDULER_NEARESTALLOCCLASSES_H__

#include <string>
#include <sstream>
#include <vector>

//#include "sst/core/serialization.h"

#include "MeshMachine.h"
#include "MeshAllocInfo.h"

namespace SST {
    namespace Scheduler {

        class CenterGenerator;
        class PointCollector;
        class Scorer;

        //Comparators:

        class L1Comparator: public std::binary_function<MeshLocation*, MeshLocation*, bool> {
            //compares points by L1 distance to a reference point
            //Warning: not consistent with equals (will call diff pts equal)

            private:
                MeshLocation* pt;  //point we measure distance from

            public:
                L1Comparator(int x, int y, int z) 
                {
                    //constructor that takes coordinates of reference point
                    pt = new MeshLocation(x, y, z);
                }

                bool operator()(MeshLocation* L1, MeshLocation* L2) 
                {
                    return pt -> L1DistanceTo(*L1) < pt -> L1DistanceTo(*L2);
                }
                std::string toString()
                {
                    return "L1Comparator";
                }
        };


        class LInfComparator : public std::binary_function<MeshLocation*, MeshLocation*, bool> {
            //compares points by L infinity distance to a reference point
            //Warning: not consistent with equals (will call diff pts equal)

            private:
                MeshLocation* pt;  //point we measure distance from

            public:
                LInfComparator(int x, int y, int z) 
                {
                    //constructor that takes coordinates of reference point
                    pt = new MeshLocation(x, y, z);
                }

                bool operator()(MeshLocation* L1, MeshLocation* L2) 
                {
                    return pt -> LInfDistanceTo(*L1) < pt -> LInfDistanceTo(*L2);
                }

                //bool operator==(LInfComparator* other); 

                std::string toString()
                {
                    return "LInfComparator";
                }
        };

        //Center Generators: 

        class CenterGenerator {
            //a way to generate possible center points

            protected:
                MeshMachine* machine;  //the machine we're generating for
                CenterGenerator(MeshMachine* m) 
                {
                    machine = m;
                }

            public :
                virtual  std::vector<MeshLocation*>* getCenters(std::vector<MeshLocation*>* available) = 0;
                virtual std::string getSetupInfo(bool comment) = 0;
                //returns centers to try given the current free processors
        };

        class FreeCenterGenerator : public CenterGenerator {
            //generated list is all free locations

            public:
                FreeCenterGenerator(MeshMachine* m) : CenterGenerator(m) { }

                std::vector<MeshLocation*>* getCenters(std::vector<MeshLocation*>* available); 

                std::string getSetupInfo(bool comment);

        };

        class CoolingGenerator : public CenterGenerator {
            //find the free nodes with best cooling properties

            public:
                CoolingGenerator(MeshMachine* m) : CenterGenerator(m) { } 
                std::vector<MeshLocation*>* getCenters(std::vector<MeshLocation*>* available);//, MeshMachine* m); 
                std::string getSetupInfo(bool comment); 
        };



        class IntersectionCenterGen : public CenterGenerator {
            //guaranted list contains all intersections of free locations
            //(including the free locations themselves)

            public:
                IntersectionCenterGen(MeshMachine* m) : CenterGenerator(m)
            {
            }

                std::vector<MeshLocation*>* getCenters(std::vector<MeshLocation*>* available); 

                std::string getSetupInfo(bool comment);
        };


        class AllCenterGenerator : public CenterGenerator {
            //generated list is all locations 
            public:

                AllCenterGenerator(MeshMachine* m) : CenterGenerator(m)
            {
            }

                std::vector<MeshLocation*>* getCenters(std::vector<MeshLocation*>* available);

                std::string getSetupInfo(bool comment)
                {
                    std::string com;
                    if (comment)  {
                        com = "# ";
                    } else  {
                        com = "";
                    }
                    return com + "AllCenterGenerator";
                }

        };


        //Point Collectors:

        class PointCollector {
            //a way to gather nearest free processors to a given center

            public:
                virtual std::vector<MeshLocation*>* getNearest(MeshLocation* center, int num,
                                                          std::vector<MeshLocation*>* available) = 0;
                virtual std::string getSetupInfo(bool comment) = 0;
                //returns num nearest locations to center from available
                //may reorder available and return it
        };


        class L1PointCollector : public PointCollector {
            //collects points nearest to center in terms of L1 distance
            public:
                std::vector<MeshLocation*>* getNearest(MeshLocation* center, int num,
                                                  std::vector<MeshLocation*>* available); 

                std::string getSetupInfo(bool comment);

        };

        class LInfPointCollector : public PointCollector{

            public:
                std::vector<MeshLocation*>* getNearest(MeshLocation* center, int num,
                                                  std::vector<MeshLocation*>* available); 

                std::string getSetupInfo(bool comment);

        };

        /**
         * GreedyLInf uses the same initial strategy as the regular LInf point
         * collector, but differs in how selects points from the outer shell.
         * GreedyLInf will pick a point in the outer shell that is closest (in terms of
         * L1 distance to the rest of the group).  The next point it picks will be
         * closest to the inner+newly selected points.  In the case of a tie, between
         * two points equally close to the group of points selected so far, a comparison
         * is made between how close the point is to the center.  This method is mainly
         * useful in preventing an allocation from looking like this:
         * 
         *   *
         *    C
         *    **
         * 
         * when it could look like this:
         * 
         *   **
         *   *C
         *   
         * Both equal in terms of the regular LInf point collector, but one is definitely more correct.
         * 
         * @author Peter Walker
         */
        class GreedyLInfPointCollector : public PointCollector{

            /**
             * PointInfo will keep locations, and distances together
             */
            private:
                class PointInfo : public std::binary_function<PointInfo*,PointInfo*, bool>{
                    public:
                        MeshLocation* point;
                        int L1toGroup;
                        long tieBreaker;	//This is used as the tie breaker before MeshLocation ordering
                        PointInfo(MeshLocation* point, int L1toGroup);
                        bool operator()(PointInfo* const& pi1, PointInfo* const& pi2);

                        std::string toString();
                };

            public:

                std::vector<MeshLocation*>* getNearest(MeshLocation* center, int num,
                                                  std::vector<MeshLocation*>* available) ;

                //loc shouldn't be in innerProcs
                int L1toInner(MeshLocation* outer, std::vector<MeshLocation*>* innerProcs) ;

                std::string getSetupInfo(bool comment);
        };

        //Scorers:

        class Scorer {
            //a way to evaluate a possible allocation; low is better
            public:
                virtual std::pair<long,long>* valueOf(MeshLocation* center, std::vector<MeshLocation*>* procs,int num, MeshMachine* mach) = 0;
                virtual std::string getSetupInfo(bool comment) = 0;
                //returns score associated with first num members of procs
                //center is the center point used to select these

        };

        class PairwiseL1DistScorer : public Scorer {
            //evaluates by sum of pairwise L1 distances

            public:
                std::pair<long,long>* valueOf(MeshLocation* center, std::vector<MeshLocation*>* procs, int num, MeshMachine* mach) 
                {
                    //returns pairwise L1 dist between first num members of procs
                    return new std::pair<long,long>(mach -> pairwiseL1Distance(procs, num),0);
                }
                std::string getSetupInfo(bool comment);
        };

        //needed for LInfDistFromCentScorer: 
        class Tiebreaker {
            private:
                bool bordercheck;
                long maxshells;

                long availFactor;
                long wallFactor;
                long borderFactor;

                long curveFactor;
                long curveWidth;

            public:
                std::string lastTieInfo;

                //Takes mesh center, available processors sorted by correct comparator,
                //and number of processors needed and returns tiebreak value.
                long getTiebreak(MeshLocation* center, std::vector<MeshLocation*>* avail, int num, MeshMachine* mesh);

                Tiebreaker(long ms, long af, long wf, long bf) ;

                void setCurveFactor(long cf)
                {
                    curveFactor = cf;
                }
                void setCurveWidth(long cw)
                {
                    curveWidth = cw;
                }

                std::string getInfo() ;

                long getMaxShells() 
                {
                    return maxshells;
                }
        };

        //Finds the sum of LInf distances of num nearest processors from center

        class LInfDistFromCenterScorer : public Scorer {

            private:
                Tiebreaker* tiebreaker;

            public:
                std::pair<long,long>* valueOf(MeshLocation* center, std::vector<MeshLocation*>* procs, int num, MeshMachine* mach) ;

                LInfDistFromCenterScorer(Tiebreaker* tb);

                std::string getLastTieInfo()
                {
                    return tiebreaker -> lastTieInfo;
                }

                std::string getSetupInfo(bool comment);
        }; 


        class L1DistFromCenterScorer : public Scorer {
            //evaluates by sum of L1 distance from center
            public:
                L1DistFromCenterScorer()
                {
                }

                std::pair<long,long>* valueOf(MeshLocation* center, std::vector<MeshLocation*>* procs, int num, MeshMachine* mach);

                std::string getSetupInfo(bool comment)
                {
                    std::string com;
                    if (comment)  {
                        com = "# ";
                    } else  {
                        com = "";
                    }
                    return com + "L1DistFromCenterScorer";
                }

        };

    }
}
#endif
