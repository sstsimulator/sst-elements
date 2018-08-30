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
#include "StencilMachine.h"
#include "output.h"
#include "NearestAllocClasses.h"
#include "EnergyAllocClasses.h"

using namespace SST::Scheduler;
using namespace std;

NearestAllocator::NearestAllocator(std::vector<std::string>* params, Machine* mach) : Allocator(*mach)
{
    schedout.init("", 8, 0, Output::STDOUT);
    mMachine = (StencilMachine*) mach;
    if (NULL == mMachine || mMachine->numDims() != 3) {
        schedout.fatal(CALL_INFO, 1, "Nearest allocators require a 3D mesh or torus machine");
    }

    if (params -> at(0) == "MM") {
        MMAllocator(mMachine);
    } else if (params -> at(0) == "MC1x1") {
        MC1x1Allocator(mMachine);
    } else if (params -> at(0) == "genAlg") {
        genAlgAllocator(mMachine);
    } else if (params -> at(0) == "Hybrid") {
        HybridAllocator(mMachine);
    } else {

        if (params -> size() < 4) {
            schedout.fatal(CALL_INFO, 1, "Not enough arguments for custom Nearest Allocator\n");
        }
        configName = "custom";
        //custom Nearest allocator
        CenterGenerator* cg = NULL;
        PointCollector* pc = NULL;
        Scorer* sc = NULL;

        std::string cgstr = params -> at(1);

        if (cgstr == ("all")) {
            cg = new AllCenterGenerator(mMachine);
        } else if (cgstr == ("free")) {
            cg = new FreeCenterGenerator(mMachine);
        } else if (cgstr == ("intersect")) {
            cg = new IntersectionCenterGen(mMachine);
        } else {
            schedout.fatal(CALL_INFO, 1, "Unknown center generator %s", cgstr.c_str());
        }

        std::string pcstr=params -> at(2);

        if (pcstr == ("l1")) {
            pc = new L1PointCollector();
        } else if (pcstr == ("linf")) {
            pc = new LInfPointCollector();
        } else {
            schedout.fatal(CALL_INFO, 1, "Unknown point collector %s", pcstr.c_str());
        }


        pcstr = params -> at(3);

        if (pcstr == ("l1")) {
            sc = new L1DistFromCenterScorer();
        } else if(pcstr == ("linf")) {
            if (mMachine -> dims[0] > 1 && mMachine -> dims[1] > 1 && mMachine -> dims[2] > 1) {
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
            schedout.fatal(CALL_INFO, 1, "Unknown scorer %s", pcstr.c_str());
        }

        centerGenerator = cg;
        pointCollector = pc;
        scorer = sc;
    }

    if(params -> at(0) != "Hybrid" && (NULL == centerGenerator || NULL == pointCollector || NULL == scorer)) {
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
        "\tscorer: Choose point scorer (L1, LInf, Pairwise)";
    return ret.str();
}

std::string NearestAllocator::getSetupInfo(bool comment) const
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
    std::vector<int>* freeNodes = mMachine->getFreeNodes();
    std::vector<MeshLocation*>* available = new std::vector<MeshLocation*>(freeNodes->size());
    for(unsigned int i = 0; i < freeNodes->size(); i++){
        available->at(i) = new MeshLocation(freeNodes->at(i), *mMachine);
    }
    delete freeNodes;
    
    return allocate(job, available);
}

//Allocates job if possible.
//Returns information on the allocation or null if it wasn't possible
//(doesn't make allocation; merely returns info on possible allocation).
AllocInfo* NearestAllocator::allocate(Job* job, std::vector<MeshLocation*>* available) 
{
    if (!canAllocate(*job, available)) {
        return NULL;
    }
    
    AllocInfo* retVal = new AllocInfo(job, machine);
    int nodesNeeded = ceil((double) job->getProcsNeeded() / machine.coresPerNode);

    //optimization: if exactly enough procs are free, just return them
    if ((unsigned int) nodesNeeded == available -> size()) {
        for (int i = 0; i < nodesNeeded; i++) {
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

        //convert to machine indices and back
        std::vector<int> availableInds(available->size());
        for(unsigned int i = 0; i < available->size(); i++){
            availableInds[i] = available->at(i)->toInt(*mMachine);
        }
        std::vector<int>* possCenterInds = EnergyHelpers::getEnergyNodes( & availableInds, nodesNeeded, *mMachine);
        possCenters = new std::vector<MeshLocation*>(possCenterInds->size());
        for(unsigned int i = 0; i < possCenterInds->size(); i++){
            possCenters->at(i) = new MeshLocation(possCenterInds->at(i), *mMachine);
        }
        delete possCenterInds;
    } else { 
        possCenters = centerGenerator -> getCenters(available);
    }
    delete available;
    
    std::vector<MeshLocation*>* nearest = NULL;
    std::vector<MeshLocation*>* alloc = new std::vector<MeshLocation*>();
    for (std::vector<MeshLocation*>::iterator center = possCenters -> begin(); center != possCenters -> end(); ++center) {        
        nearest = pointCollector -> getNearest(*center, nodesNeeded, *mMachine);        
        std::pair<long,long>* val = scorer -> valueOf(*center, nearest, mMachine); 
        if (val -> first < bestVal -> first || 
            (val -> first == bestVal -> first && val -> second < bestVal -> second) ) {
            delete bestVal;
            bestVal = val;
            for (int i = 0; i < nodesNeeded; i++) {
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
            for (int i = 0; i < nodesNeeded; i++)
                alloc -> push_back((*nearest)[i]);
            bestAllocs -> push_back(alloc);
        }
        delete nearest;
    }
    //clear memory
    delete alloc;
    delete bestAllocs;
    delete possCenters;
    delete bestVal;
    
    return retVal;
}

void NearestAllocator::genAlgAllocator(StencilMachine* m) {
    configName = "genAlg";
    mMachine = m;
    centerGenerator = new FreeCenterGenerator(m);
    pointCollector = new L1PointCollector();
    scorer = new PairwiseL1DistScorer();
}

void NearestAllocator::MMAllocator(StencilMachine* m) {
    configName = "MM";
    mMachine = m;
    centerGenerator = new IntersectionCenterGen(m);
    pointCollector = new L1PointCollector();
    scorer = new PairwiseL1DistScorer();
}

void NearestAllocator::MC1x1Allocator(StencilMachine* m) {
    configName = "MC1x1";
    mMachine = m;
    centerGenerator = new FreeCenterGenerator(m);
    pointCollector = new LInfPointCollector();
    scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
} 

void NearestAllocator::HybridAllocator(StencilMachine* m) {
    configName = "Hybrid";
    mMachine = m;
    centerGenerator = NULL;
    pointCollector = new LInfPointCollector();
    scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
} 
