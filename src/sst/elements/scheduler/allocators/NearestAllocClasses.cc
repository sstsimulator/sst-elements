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
 *  to implement allocation algorithms of the family that
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

#include "sst_config.h"
#include "NearestAllocClasses.h"

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>  //for std::stable_sort

#include "StencilMachine.h"

using namespace std;
using namespace SST::Scheduler;

//Center Generators: 

vector<MeshLocation*>* FreeCenterGenerator::getCenters(vector<MeshLocation*>* available) 
{
    //returns vector containing contents of available (deep copy to match Intersection version)
    vector<MeshLocation*>* retVal = new vector<MeshLocation*>();
    for (unsigned int x = 0; x < available -> size(); x++) {
        retVal -> push_back(new MeshLocation(*((*available)[x])));
    }
    return retVal;
}

string FreeCenterGenerator::getSetupInfo(bool comment)
{
    string com;
    if (comment)  {
        com="# ";
    } else  {
        com="";
    }
    return com + "FreeCenterGenerator";
}

bool contains(vector<int>* vec, int i)
{
    bool ret = false;
    for (unsigned int x = 0; x < vec -> size() && !ret ; x++) {
        if (vec -> at(x) == i) {
            ret = true;
        }
    }
    return ret;
}


vector<MeshLocation*>* IntersectionCenterGen::getCenters(vector<MeshLocation*>* available) 
{ 
    vector<MeshLocation*>* retVal = new vector<MeshLocation*>();
    //Collect available X,Y and Z coordinates
    vector<int> X;
    vector<int> Y;
    vector<int> Z;

    //no duplicate values
    for (vector<MeshLocation*>::iterator loc = available -> begin(); loc != available -> end(); ++loc) {
        if (!contains(&X,(*loc) -> dims[0])) {
            X.push_back((*loc) -> dims[0]);
        }
        if (!contains(&Y,(*loc) -> dims[1])) {
            Y.push_back((*loc) -> dims[1]);
        }
        if (!contains(&Z,(*loc) -> dims[2])) {
            Z.push_back((*loc) -> dims[2]);
        }
    }

    //Make all possible intersections of the X,Y and Z coordinates
    for (vector<int>::iterator ind_x = X.begin(); ind_x != X.end(); ind_x++) { //ind_x is x val index
        for (vector<int>::iterator ind_y = Y.begin(); ind_y != Y.end(); ind_y++) { 
            for (vector<int>::iterator ind_z = Z.begin(); ind_z != Z.end(); ind_z++) {
                //Get an intersection with current x, y and z values
                vector<int> tempVec(3);
                tempVec[0] = *(ind_x);
                tempVec[1] = *(ind_y);
                tempVec[2] = *(ind_z);
                MeshLocation* val = new MeshLocation(tempVec);                                                                  
                retVal->push_back(val); //Add to the return value list
            }
        }
    }
    return retVal;
}

string IntersectionCenterGen::getSetupInfo(bool comment)
{
    string com;
    if (comment)  {
        com="# ";
    } else  {
        com="";
    }
    return com + "IntersectionCenterGen";
}


//returns List containing all locations in mesh
vector<MeshLocation*>* AllCenterGenerator::getCenters(vector<MeshLocation*>* available) {
    vector<MeshLocation*>* retVal = new vector<MeshLocation*>();

    int xdim = machine -> dims[0];
    int ydim = machine -> dims[1];
    int zdim = machine -> dims[2];
    for(int i = 0; i < xdim; i++) {
        for(int j = 0; j < ydim; j++) {
            for(int k = 0; k < zdim; k++) {
                vector<int> tempVec(3);
                tempVec[0] = i;
                tempVec[1] = j;
                tempVec[2] = k;
                retVal -> push_back(new MeshLocation(tempVec));
            }
        }
    }
    return retVal;
}

//Point Collectors:

vector<MeshLocation*>* L1PointCollector::getNearest(MeshLocation* center, int num, const StencilMachine & mach) 
{
    //get sufficient nodes
    int found = 1;
    int dist = 1;
    std::list<int>* nodeList = new std::list<int>();
    while(found < num && dist <= (mach.dims[0] + mach.dims[1] + mach.dims[2])){
        std::list<int>* tempList = mach.getFreeAtDistance(center->toInt(mach), dist);
        found += tempList->size();
        nodeList->insert(nodeList->end(), tempList->begin(), tempList->end());
        delete tempList;
        dist++;
    }
    
    //convert to MeshLocation
    vector<MeshLocation*>* retList = new vector<MeshLocation*>(num);
    retList->at(0) = center;
    int i = 1;
    for(std::list<int>::iterator it = nodeList->begin(); it != nodeList->end() && i < num; it++){
        retList->at(i) = new MeshLocation(*it, mach);
        i++;
    }
    delete nodeList;
    
    return retList;
}

string  L1PointCollector::getSetupInfo(bool comment)
{
    string com;
    if (comment)  {
        com="# ";
    } else  {
        com="";
    }
    return com + "L1PointCollector";
}

vector<MeshLocation*>* LInfPointCollector::getNearest(MeshLocation* center, int num, const StencilMachine & mach) 
{
    //get sufficient nodes
    int found = 1;
    int dist = 1;
    std::list<int>* nodeList = new std::list<int>();
    while(found < num && dist <= (mach.dims[0] + mach.dims[1] + mach.dims[2])){
        std::list<int>* tempList = mach.getFreeAtLInfDistance(center->toInt(mach), dist);
        found += tempList->size();
        nodeList->insert(nodeList->end(), tempList->begin(), tempList->end());
        delete tempList;
        dist++;
    }
    
    //convert to MeshLocation
    vector<MeshLocation*>* retList = new vector<MeshLocation*>(num);
    retList->at(0) = center;
    int i = 1;
    for(std::list<int>::iterator it = nodeList->begin(); it != nodeList->end() && i < num; it++){
        retList->at(i) = new MeshLocation(*it, mach);
        i++;
    }
    delete nodeList;
    
    return retList;
}

string LInfPointCollector::getSetupInfo(bool comment)
{
    string com;
    if (comment)  {
        com="# ";
    } else  {
        com="";
    }
    return com + "LInfPointCollector";
}

//Scorers:

string PairwiseL1DistScorer::getSetupInfo(bool comment)
{
    string com;
    if (comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    return com + "PairwiseL1DistScorer";
}

std::pair<long,long>* PairwiseL1DistScorer::valueOf(MeshLocation* center, std::vector<MeshLocation*>* procs, StencilMachine* mach) 
{
    long retVal = 0;
    for (unsigned int i = 0; i < procs->size(); i++) {
        for (unsigned int j = i + 1; j < procs->size(); j++) {
            retVal += procs->at(i)->L1DistanceTo(*(procs->at(j)));
        }
    }
    return new std::pair<long,long>(retVal,0);
}

//Takes mesh center, available processors sorted by correct comparator,
//and number of processors needed and returns tiebreak value.
long Tiebreaker::getTiebreak(MeshLocation* center, vector<MeshLocation*>* avail, StencilMachine* mesh)
{
    int num = avail->size();
    long ret = 0;

    lastTieInfo = "0\t0\t0";

    if (avail -> size() == (unsigned int)num) {
        return 0;
    }

    LInfComparator* lc = new LInfComparator(center -> dims[0],center -> dims[1],center -> dims[2]);
    stable_sort(avail -> begin(), avail -> end(),*lc);
    delete lc;
    if (maxshells == 0) {
        return 0;
    }

    long ascore = 0;
    long wscore = 0;
    long bscore = 0;

    long lastshell=center -> LInfDistanceTo(*((*avail)[num-1]));
    long lastlook=lastshell + maxshells;
    lastTieInfo="";
    int ydim = mesh -> dims[1];

    //Add to score for nearby available processors.
    if(availFactor != 0) {
        for (unsigned int i = num; i < avail -> size(); i++) {
            long dist = center -> LInfDistanceTo(*((*avail)[i]));
            if (dist > lastlook) {
                break;
            } else {
                ret += availFactor * (lastlook - dist + 1);
                ascore += availFactor * (lastlook - dist + 1);
            }
        }
    }

    //Subtract from score for nearby walls
    if (wallFactor != 0) {
        int xdim = mesh -> dims[0];
        int zdim = mesh -> dims[2];
        for (int i = 0; i < num; i++) {
            int dist = center -> LInfDistanceTo(*((*avail)[i]));
            //if(dist == lastshell)
            if( ((avail->at(i)->dims[0] == 0 || avail->at(i)->dims[0] == xdim-1) && xdim > 2) || 
                ((avail->at(i)->dims[1] == 0 || avail->at(i)->dims[1] == ydim-1) && ydim > 2) || 
                ((avail->at(i)->dims[2] == 0 || avail->at(i)->dims[2] == zdim-1) && zdim > 2)) {
                //NOTE: After removing if statement above, is this right?
                wscore -= wallFactor * (lastlook - dist + 1);
                ret -= wallFactor * (lastlook - dist + 1);
            }
        }
    }

    //Subtract from score for bordering allocated processors
    if (borderFactor != 0){
        std::vector<int>* usedNodes = mesh->getUsedNodes();
        std::vector<MeshLocation*>* used = new std::vector<MeshLocation*>(usedNodes->size());
        for(unsigned int i = 0; i < usedNodes->size(); i++){
            used->at(i) = new MeshLocation(usedNodes->at(i), *mesh);
        }   
        delete usedNodes;
        
        LInfComparator* lc = new LInfComparator(center -> dims[0],center -> dims[1],center -> dims[2]);
        stable_sort(used -> begin(), used -> end(),*lc);
        delete lc;
        for (vector<MeshLocation*>::iterator it = used -> begin(); it != used -> end(); ++it){
            MeshLocation* ml = *it;
            long dist = center -> LInfDistanceTo(*ml);
            if(dist > lastlook) {
                break;
            } else if(dist == lastshell+1) {
                ret -= borderFactor * (lastlook - dist + 1);
                bscore -= borderFactor * (lastlook - dist + 1);
            }
        }
        //get rid of everything in used as it was allocated during the function call
        while (!used -> empty()) {
            delete used -> back();
            used -> pop_back();
        }
        used -> clear();
        delete used;
    }

    //Add to score for being at a worse curve location
    //Only works for 2D now.
    long cscore = 0;
    if (curveFactor != 0) {
        long centerLine = center -> dims[0] / curveWidth;
        long tsc = ydim * centerLine;
        tsc += (centerLine % 2 == 0) ? (center->dims[1]) : (ydim - center -> dims[1]);
        cscore += curveFactor * tsc;
        ret += cscore;
    }

    stringstream sstemp;
    sstemp << lastTieInfo << ascore << "\t"<< wscore<< "\t" << bscore << "\t" <<cscore;
    lastTieInfo = sstemp.str();

    return ret;
}

Tiebreaker::Tiebreaker(long ms, long af, long wf, long bf) 
{
    maxshells=ms;
    availFactor = af;
    wallFactor = wf;
    borderFactor = bf;

    bordercheck = false;
    curveFactor = 0;
    curveWidth = 2;
}

string Tiebreaker::getInfo() 
{
    stringstream ret;
    ret << "(" <<maxshells << "," << availFactor << "," <<wallFactor << "," << borderFactor << "," <<curveFactor << "," <<curveWidth << ")";
    return ret.str();
}


pair<long,long>* LInfDistFromCenterScorer::valueOf(MeshLocation* center, vector<MeshLocation*>* procs, StencilMachine* mach) {
    //returns the sum of the LInf distances of the num closest processors

    long retVal = 0;
    for (unsigned int i = 0; i < procs->size(); i++) 
        retVal += center -> LInfDistanceTo(*((*procs)[i]));

    long tiebreak = tiebreaker -> getTiebreak(center,procs, mach);

    return new pair<long,long>(retVal,tiebreak);
}

LInfDistFromCenterScorer::LInfDistFromCenterScorer(Tiebreaker* tb)
{
    tiebreaker = tb;
}


string  LInfDistFromCenterScorer::getSetupInfo(bool comment)
{
    string com;
    stringstream ret;
    if (comment)  {
        com = "# ";
    } else  {
        com = "";
    }
    ret << com << "LInfDistFromCenterScorer (Tiebreaker: " << tiebreaker -> getInfo() << ")";
    return ret.str();
}

pair<long,long>* L1DistFromCenterScorer::valueOf(MeshLocation* center, vector<MeshLocation*>* procs, StencilMachine* mach) 
{
    //returns sum of L1 distances from center
    long retVal = 0;
    for (unsigned int i = 0; i < procs->size(); i++)
        retVal += center -> L1DistanceTo(*(procs->at(i)));
    return new pair<long,long>(retVal,0);
}

