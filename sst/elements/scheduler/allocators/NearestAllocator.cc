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
 Cooling - use MC1x1 but use cooling information to obtain centers
 */

#include "sst_config.h"
#include "NearestAllocator.h"

#include <sstream>
#include <limits>
#include <vector>
#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "MeshMachine.h"
#include "MeshAllocInfo.h"
#include "output.h"
#include "NearestAllocClasses.h"
#include "EnergyAllocClasses.h"


using namespace SST::Scheduler;
using namespace std;

NearestAllocator::NearestAllocator(MeshMachine* m, CenterGenerator* cg,
                                   PointCollector* pc, Scorer* s, std::string name) 
{
    schedout.init("", 8, 0, Output::STDOUT);
    machine = m;
    centerGenerator = cg;
    pointCollector = pc;
    scorer = s;
    configName = name;
}

NearestAllocator::NearestAllocator(std::vector<std::string>* params, Machine* mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    MeshMachine* m = (MeshMachine*) mach;
    if (NULL == m) {
        //error("Nearest allocators require a Mesh machine");
        schedout.fatal(CALL_INFO, 1, "Nearest allocators require a Mesh machine");
    }


    if (params -> at(0) == "MM") {
        MMAllocator(m);
    } else if (params -> at(0) == "MC1x1") {
        MC1x1Allocator(m);
    } else if (params -> at(0) == "genAlg") {
        genAlgAllocator(m);
    } else if (params -> at(0) == "OldMC1x1") {
        OldMC1x1Allocator(m);
    } else if (params -> at(0) == "Hybrid") {
        HybridAllocator(m);
    } else {

        if (params -> size() < 4) {
            schedout.fatal(CALL_INFO, 1, "Not enough arguments for custom Nearest Allocator\n");
        }
        configName = "custom";
        //custom Nearest allocator
        CenterGenerator* cg = NULL;
        PointCollector* pc = NULL;
        Scorer* sc = NULL;
        machine = m;

        std::string cgstr = params -> at(1);

        if (cgstr == ("all")) {
            cg = new AllCenterGenerator(m);
        } else if (cgstr == ("free")) {
            cg = new FreeCenterGenerator(m);
        } else if (cgstr == ("intersect")) {
            cg = new IntersectionCenterGen(m);
        } else {
            //error("Unknown center generator " + cgstr);
            schedout.fatal(CALL_INFO, 1, "Unknown center generator %s", cgstr.c_str());
        }

        std::string pcstr=params -> at(2);

        if (pcstr == ("l1")) {
            pc = new L1PointCollector();
        } else if (pcstr == ("linf")) {
            pc = new LInfPointCollector();
        } else if (pcstr == ("greedylinf")) {
            pc = new GreedyLInfPointCollector();
        } else {
            //error("Unknown point collector " + pcstr);
            schedout.fatal(CALL_INFO, 1, "Unknown point collector %s", pcstr.c_str());
        }


        pcstr = params -> at(3);

        if (pcstr == ("l1")) {
            sc = new L1DistFromCenterScorer();
        } else if(pcstr == ("linf")) {
            if (m -> getXDim() > 1 && m -> getYDim() > 1 && m -> getZDim() > 1) {
                //error("\nTiebreaker (and therefore MC1x1 and LInf scorer) only implemented for 2D meshes");
                schedout.fatal(CALL_INFO, 1, "\nTiebreaker (and therefore MC1x1 and LInf scorer) only implemented for 2D meshes");
            }
            long TB = 0;
            long af = 1;
            long wf = 0;
            long bf = 0;
            long cf = 0;
            long cw = 2;
            if (params -> size() > 4) {
                if (params -> at(4)==("m")) {
                    TB=LONG_MAX;
                } else {
                    TB = atol(params -> at(4).c_str());
                }
            }
            if (params -> size() > 5) {
                af = atol(params -> at(5).c_str());
            }
            if (params -> size() > 6) {
                wf = atol(params -> at(6).c_str());
            }
            if (params -> size() > 7) {
                bf = atol(params -> at(7).c_str());
            }
            if (params -> size() > 8) {
                cf = atol(params -> at(8).c_str());
            }
            if (params -> size() > 9) {
                cw = atol(params -> at(9).c_str());
            }

            Tiebreaker* tb = new Tiebreaker(TB,af,wf,bf);
            tb -> setCurveFactor(cf);
            tb -> setCurveWidth(cw);
            sc = new LInfDistFromCenterScorer(tb);
        } else if(pcstr==("pairwise")) {
            sc = new PairwiseL1DistScorer();
        } else {
            //error("Unknown scorer " + pcstr);
            schedout.fatal(CALL_INFO, 1, "Unknown scorer %s", pcstr.c_str());
        }

        centerGenerator = cg;
        pointCollector = pc;
        scorer = sc;
    }

    if(params -> at(0) != "Hybrid" && (NULL == centerGenerator || NULL == pointCollector || NULL == scorer)) {
        //error("Nearest input not correctly parsed");
        schedout.fatal(CALL_INFO, 1, "Nearest input not correctly parsed");
    }
    delete params;
    params = NULL;
}

std::string NearestAllocator::getParamHelp()
{
    std::stringstream ret;
    ret << "[<center_gen>,<point_col>,<scorer>]\n"<<
        "\tcenter_gen: Choose center generator (all, free, intersect)\n"<<
        "\tpoint_col: Choose point collector (L1, LInf, GreedyLInf)\n"<<
        "\tscorer: Choose point scorer (L1, LInf, Pairwise)";
    return ret.str();
}

std::string NearestAllocator::getSetupInfo(bool comment)
{ 
    std::string com;
    if (comment) {
        com="# ";
    } else  {
        com="";
    }
    std::stringstream ret;
    ret <<com<<"Nearest Allocator ("<<configName<<")\n";
    if("Hybrid" != configName) {
        ret << com << "\tCenterGenerator: "<<centerGenerator -> getSetupInfo(false)<<"\n"<<com<<
        "\tPointCollector: "<<pointCollector -> getSetupInfo(false)<<"\n"<<com<<
        "\tScorer: "<<scorer -> getSetupInfo(false);
    }
    return ret.str();
}
AllocInfo* NearestAllocator::allocate(Job* job)
{
    return allocate(job,((MeshMachine*)machine) -> freeProcessors());
}

//Allocates job if possible.
//Returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation).
AllocInfo* NearestAllocator::allocate(Job* job, std::vector<MeshLocation*>* available) 
{
    if (!canAllocate(*job, available)) {
        return NULL;
    }
    
    MeshMachine* mMachine = static_cast<MeshMachine*>(machine);

    MeshAllocInfo* retVal = new MeshAllocInfo(job);

    int numProcs = job -> getProcsNeeded();

    //optimization: if exactly enough procs are free, just return them
    if ((unsigned int) numProcs == available -> size()) {
        for (int i = 0; i < numProcs; i++) {
            (*retVal -> processors)[i] = (*available)[i];
            retVal -> nodeIndices[i] = (*available)[i] -> toInt(*mMachine);
        }
        delete available;
        return retVal;
    }


    //score of best value found so far with it tie-break score:
    std::pair<long,long>* bestVal = new std::pair<long,long>(LONG_MAX,LONG_MAX);

    bool recordingTies = false; //Statistics.recordingTies();

    //stores allocations w/ best score (no tiebreaking) if ties being recorded:
    //(actual best value w/ tiebreaking stored in retVal.processors)
    std::vector<std::vector<MeshLocation*>*>* bestAllocs = new std::vector<std::vector<MeshLocation*> *>(); 
    std::vector<MeshLocation*>* possCenters;

    if ("Hybrid" == configName) {
        //need to call LP to get possCenters
        //if done using only energy, just return the LP results by copying possCenters
        //otherwise, we'll use it as the actual centers
        possCenters = EnergyHelpers::getEnergyNodes(available, numProcs, *mMachine);
    } else { 
        possCenters = centerGenerator -> getCenters(available);
    }
    
    std::vector<MeshLocation*>* nearest = NULL;
    std::vector<MeshLocation*>* alloc = new std::vector<MeshLocation*>();
    for (std::vector<MeshLocation*>::iterator center = possCenters -> begin(); center != possCenters -> end(); ++center) {
        nearest = pointCollector -> getNearest(*center, numProcs, available);

        std::pair<long,long>* val = scorer -> valueOf(*center, nearest, numProcs, (MeshMachine*) machine); 
        if (val -> first < bestVal -> first || 
            (val -> first == bestVal -> first && val -> second < bestVal -> second) ) {
            delete bestVal;
            bestVal = val;
            for (int i = 0; i < numProcs; i++) {
                (*(retVal -> processors))[i] = (*nearest)[i];
                retVal -> nodeIndices[i] = (*nearest)[i] -> toInt(*mMachine);
            }
            if (recordingTies) {
                bestAllocs -> clear();
            }
        }
        delete *center;
        *center = NULL;

        if (recordingTies && val -> first == bestVal -> first) {
            delete alloc;
            alloc = new std::vector<MeshLocation*>();
            for (int i = 0; i < numProcs; i++)
                alloc -> push_back((*nearest)[i]);
            bestAllocs -> push_back(alloc);
        }
    }
    //clear memory
    delete nearest;
    delete alloc;
    delete bestAllocs;
    possCenters -> clear();
    delete possCenters;
    delete bestVal;
    
    return retVal;
}

void NearestAllocator::genAlgAllocator(MeshMachine* m) {
    configName = "genAlg";
    machine = m;
    centerGenerator = new FreeCenterGenerator(m);
    pointCollector = new L1PointCollector();
    scorer = new PairwiseL1DistScorer();
}

void NearestAllocator::MMAllocator(MeshMachine* m) {
    configName = "MM";
    machine = m;
    centerGenerator = new IntersectionCenterGen(m);
    pointCollector = new L1PointCollector();
    scorer = new PairwiseL1DistScorer();
}

void NearestAllocator::OldMC1x1Allocator(MeshMachine* m) {
    configName = "MC1x1";
    machine = m;
    centerGenerator = new FreeCenterGenerator(m);
    pointCollector = new LInfPointCollector();
    scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
    //readnewcenter = false;
}

void NearestAllocator::MC1x1Allocator(MeshMachine* m) {
    configName = "MC1x1";
    machine = m;
    centerGenerator = new FreeCenterGenerator(m);
    pointCollector = new GreedyLInfPointCollector();
    scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
    //readnewcenter = false;
} 

void NearestAllocator::HybridAllocator(MeshMachine* m) {
    configName = "Hybrid";
    machine = m;
    centerGenerator = NULL;
    pointCollector = new GreedyLInfPointCollector();
    scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
    //readnewcenter = false;
} 




