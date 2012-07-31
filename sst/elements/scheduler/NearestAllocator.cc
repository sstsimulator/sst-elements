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
 */

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <time.h>

#include "sst/core/serialization/element.h"

#include "NearestAllocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "AllocInfo.h"
#include "MeshAllocInfo.h"
#include "Job.h"
#include "misc.h"



NearestAllocator::NearestAllocator(MachineMesh* m, CenterGenerator* cg,
    PointCollector* pc, Scorer* s,string name) {
  machine = m;
  centerGenerator = cg;
  pointCollector = pc;
  scorer = s;
  configName = name;
}

NearestAllocator::NearestAllocator(vector<string>* params, Machine* mach){

  MachineMesh* m = (MachineMesh*) mach;
  if(m == NULL)
    error("Nearest allocators require a MachineMesh machine");

  if(params->at(0) == "MM")
    MMAllocator(m);
  else if(params->at(0) == "MC1x1")
    MC1x1Allocator(m);
  else if(params->at(0) == "genAlg")
    genAlgAllocator(m);
  else if(params->at(0) == "OldMC1x1")
    OldMC1x1Allocator(m);
  else
    error("Custom nearest allocator not implemented yet");
/*
  CenterGenerator* cg=NULL;
  PointCollector* pc=NULL;
  Scorer* sc=NULL;

  string cgstr=params.at(1);

  if(cgstr.equals("all"))
    cg=new AllCenterGenerator(m);
  else if(cgstr.equals("free"))
    cg=new FreeCenterGenerator(m);
  else if(cgstr.equals("intersect"))
    cg=new IntersectionCenterGen(m);
  else
    Main.error("Unknown center generator "+cgstr);

  string pcstr=params.at(2);

  if(pcstr.equals("L1"))
    pc=new L1PointCollector();
  else if(pcstr.equals("LInf"))
    pc=new LInfPointCollector();
  else if(pcstr.equals("GreedyLInf"))
    pc=new GreedyLInfPointCollector();
  else
    Main.error("Unknown point collector "+pcstr);


  vector<string> sclist = Factory.ParseInput(params.at(3));
  string scstr=sclist.at(0);

  if(scstr.equals("L1"))
    sc=new L1DistFromCenterScorer();
  else if(scstr.equals("LInf")){
    Factory.argsAtMost(6,sclist);
    long TB=0;
    long af=1;
    long wf=0;
    long bf=0;
    long cf=0;
    long cw=2;
    if(sclist.size()-1 >= 1)
      if(sclist.at(1).equals("M"))
        TB=long.MAX_VALUE;
      else
        TB=long.parselong(sclist.at(1));
    if(sclist.size()-1 >= 2)
      af=long.parselong(sclist.at(2));
    if(sclist.size()-1 >= 3)
      wf=long.parselong(sclist.at(3));
    if(sclist.size()-1 >= 4)
      bf=long.parselong(sclist.at(4));
    if(sclist.size()-1 >= 5)
      cf=long.parselong(sclist.at(5));
    if(sclist.size()-1 >= 6)
      cw=long.parselong(sclist.at(6));

    Tiebreaker tb = new Tiebreaker(TB,af,wf,bf);
    tb.setCurveFactor(cf);
    tb.setCurveWidth(cw);
    sc=new LInfDistFromCenterScorer(tb);
  }
  else if(scstr.equals("Pairwise"))
    sc=new PairwiseL1DistScorer();
  else
    Main.error("Unknown scorer "+scstr);

  return new NearestAllocator(m,cg,pc,sc,"custom");
  return null;
*/
}

string NearestAllocator::getParamHelp(){
  stringstream ret;
  ret << "[<center_gen>,<point_col>,<scorer>]\n"<<
    "\tcenter_gen: Choose center generator (all, free, intersect)\n"<<
    "\tpoint_col: Choose point collector (L1, LInf, GreedyLInf)\n"<<
    "\tscorer: Choose point scorer (L1, LInf, Pairwise)";
  return ret.str();
}

string NearestAllocator::getSetupInfo(bool comment){
  
  string com;
  if(comment) com="# ";
  else com="";
  stringstream ret;
  ret <<com<<"Nearest Allocator ("<<configName<<")\n"<<com<<
    "\tCenterGenerator: "<<centerGenerator->getSetupInfo(false)<<"\n"<<com<<
    "\tPointCollector: "<<pointCollector->getSetupInfo(false)<<"\n"<<com<<
    "\tScorer: "<<scorer->getSetupInfo(false);
  return ret.str();
}

AllocInfo* NearestAllocator::allocate(Job* job){
  return allocate(job,((MachineMesh*)machine)->freeProcessors());
}

AllocInfo* NearestAllocator::allocate(Job* job, vector<MeshLocation*>* available) {
  //allocates job if possible
  //returns information on the allocation or null if it wasn't possible
  //(doesn't make allocation; merely returns info on possible allocation)

  if(!canAllocate(job, available))
    return NULL;

  MeshAllocInfo* retVal = new MeshAllocInfo(job);

  int numProcs = job->getProcsNeeded();

  //optimization: if exactly enough procs are free, just return them
  if((unsigned int) numProcs == available->size()) {
    for(int i=0; i < numProcs; i++)
    {
      (*retVal->processors)[i] = (*available)[i];
      retVal->nodeIndices[i] = (*available)[i]->toInt((MachineMesh*)machine);
    }
    return retVal;
  }

  //score of best value found so far with it tie-break score:
  pair<long,long>* bestVal = new pair<long,long>(-1,-1);

  bool recordingTies = false;//Statistics.recordingTies();
  //stores allocations w/ best score (no tiebreaking) if ties being recorded:
  //(actual best value w/ tiebreaking stored in retVal.processors)
  vector<vector<MeshLocation*>*>* bestAllocs = NULL;
  if(recordingTies)
    bestAllocs = new vector<vector<MeshLocation*> *>(); 
  vector<MeshLocation*>* possCenters = centerGenerator->getCenters(available);
  for(vector<MeshLocation*>::iterator center = possCenters->begin(); center != possCenters->end(); ++center) {
    vector<MeshLocation*>* nearest = pointCollector->getNearest(*center, numProcs, available);

    pair<long,long>* val = scorer->valueOf(*center, nearest, numProcs, (MachineMesh*) machine); 
    if(val->first < bestVal->first || (val->first == bestVal->first && val->second < bestVal->second) || bestVal->first == -1 ) {
      delete bestVal;
      bestVal = val;
      for(int i=0; i<numProcs; i++)
      {
        (*(retVal->processors))[i] = (*nearest)[i];
        retVal->nodeIndices[i] = (*nearest)[i]->toInt((MachineMesh*) machine);
      }
      if(recordingTies)
        bestAllocs->clear();
    }
    delete *center;
    *center = NULL;

    if(recordingTies && val->first == bestVal->first) {
      vector<MeshLocation*>* alloc = new vector<MeshLocation*>();
      for(int i=0; i<numProcs; i++)
        alloc->push_back((*nearest)[i]);
      bestAllocs->push_back(alloc);
    }
  }
  possCenters->clear();
  delete possCenters;
  delete bestVal;
  return retVal;
}

void NearestAllocator::genAlgAllocator(MachineMesh* m) {
  configName = "genAlg";
  machine = m;
  centerGenerator = new FreeCenterGenerator(m);
  pointCollector = new L1PointCollector();
  scorer = new PairwiseL1DistScorer();

}

void NearestAllocator::MMAllocator(MachineMesh* m) {
  configName = "MM";
  machine = m;
  centerGenerator = new IntersectionCenterGen(m);
  pointCollector = new L1PointCollector();
  scorer = new PairwiseL1DistScorer();
}

void NearestAllocator::OldMC1x1Allocator(MachineMesh* m) {
  configName = "MC1x1";
  machine = m;
  centerGenerator = new FreeCenterGenerator(m);
  pointCollector = new LInfPointCollector();
  scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
}

void NearestAllocator::MC1x1Allocator(MachineMesh* m) {
  configName = "MC1x1";
  machine = m;
  centerGenerator = new FreeCenterGenerator(m);
  pointCollector = new GreedyLInfPointCollector();
  scorer = new LInfDistFromCenterScorer(new Tiebreaker(0,0,0,0));
}

