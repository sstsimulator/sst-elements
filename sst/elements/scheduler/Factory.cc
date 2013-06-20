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

#include "sst_config.h"
#include "Factory.h"

#include <fstream>
#include <sstream>

//#include <QFileSystemWatcher>

#include "BestFitAllocator.h"
#include "ConstraintAllocator.h"
#include "EASYScheduler.h"
#include "FirstFitAllocator.h"
#include "GranularMBSAllocator.h"
#include "LinearAllocator.h"
#include "MBSAllocator.h"
#include "Machine.h"
#include "MachineMesh.h"
#include "NearestAllocator.h"
#include "OctetMBSAllocator.h"
#include "PQScheduler.h"
#include "RandomAllocator.h"
#include "RoundUpMBSAllocator.h"
#include "schedComponent.h"
#include "SimpleAllocator.h"
#include "SimpleMachine.h"
#include "SortedFreeListAllocator.h"
#include "StatefulScheduler.h"
#include "misc.h"

#define DEBUG false

using namespace SST::Scheduler;
using namespace std;

/* 
 * Factory file helps parse the parameters in the sdl file
 * returns correct type of machine, allocator, and scheduler
 */

const Factory::schedTableEntry Factory::schedTable[] = {
  {PQUEUE, "pqueue"},
  {EASY, "easy"},
  {CONS, "cons"},
  {PRIORITIZE, "prioritize"},
  {DELAYED, "delayed"},
  {ELC, "elc"},
};

const Factory::machTableEntry Factory::machTable[] = {
  {SIMPLEMACH, "simple"},
  {MESH, "mesh"},
};

const Factory::allocTableEntry Factory::allocTable[] = {
  {SIMPLEALLOC, "simple"},
  {RANDOM, "random"},
  {NEAREST, "nearest"},
  {GENALG, "genalg"},
  {MM, "mm"},
  {MC1X1, "mc1x1"},
  {OLDMC1X1, "oldmc1x1"},
  {MBS, "mbs"},
  {GRANULARMBS, "granularmbs"},
  {OCTETMBS, "octetmbs"},
  {FIRSTFIT, "firstfit"},
  {BESTFIT, "bestfit"},
  {SORTEDFREELIST, "sortedfreelist"},
  {CONSTRAINT, "constraint"},
};

const int Factory::numSchedTableEntries = 6;
const int Factory::numAllocTableEntries = 14;
const int Factory::numMachTableEntries = 2;

Scheduler* Factory::getScheduler(SST::Component::Params_t& params, int numProcs){
  if(params.find("scheduler") == params.end()){
    //default: FIFO queue priority scheduler
    printf("Defaulting to Priority Scheduler with FIFO queue\n");
    return new PQScheduler(PQScheduler::JobComparator::Make("fifo"));
  }
  else
  {
    int filltimes = 0;
    vector<string>* schedparams = parseparams(params["scheduler"]);
    if(schedparams->size() == 0)
      error("Error in parsing scheduler parameter");
    switch(schedulername(schedparams->at(0)))
    {
      //Priority Queue Scheduler
      case PQUEUE:
        if(DEBUG) printf("Priority Queue Scheduler\n");
        if(schedparams->size() == 1)
          return new PQScheduler(PQScheduler::JobComparator::Make("fifo"));
        else
          return new PQScheduler(PQScheduler::JobComparator::Make(schedparams->at(1)));
        break;

        //EASY Scheduler
      case EASY:
        if(DEBUG) printf("Easy Scheduler\n");
        if(schedparams->size() == 1)
          return new EASYScheduler(EASYScheduler::JobComparator::Make("fifo"));
        else if(schedparams->size() == 2)
        {
          EASYScheduler::JobComparator* comp = EASYScheduler::JobComparator::Make(schedparams->at(1));
          if(comp == NULL)
            error("Argument to Easy Scheduler parameter not found:" + schedparams->at(1));
          return new EASYScheduler(comp);
        }
        else
          error("EASY Scheduler requires 1 or 0 parameters (determines type of queue or defaults to FIFO");
        break;

        //Stateful Scheduler with Convervative Manager
      case CONS:
        if(DEBUG) printf("Conservative Scheduler\n");
        if(schedparams->size() == 1)
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make("fifo"), true);
        else
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make(schedparams->at(1)), true);
        break;

        //Stateful Scheduler with PrioritizeCompression Manager
      case PRIORITIZE:
        if(DEBUG) printf("Prioritize Scheduler\n");
        if(schedparams->size() == 1)
          error("PrioritizeCompression scheduler requires number of backfill times as an argument");
        filltimes = strtol(schedparams->at(1).c_str(),NULL,0);
        if(schedparams->size() == 2)
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make("fifo"),filltimes);
        else
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make(schedparams->at(2)), filltimes);
        break;

        //Stateful Scheduler with Delayed Compression Manager
      case DELAYED:
        if(DEBUG) printf("Delayed Compression Scheduler\n");
        if(schedparams->size() == 1)
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make("fifo"));
        else
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make(schedparams->at(1)));
        break;

        //Stateful Scheduler with Even Less Conservative Manager
      case ELC:
        if(DEBUG) printf("Even Less Convervative Scheduler\n");
        if(schedparams->size() == 1)
          error("PrioritizeCompression scheduler requires number of backfill times as an argument");
        filltimes = strtol(schedparams->at(1).c_str(),NULL,0);
        if(schedparams->size() == 2)
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make("fifo"),filltimes, true);
        else
          return new StatefulScheduler(numProcs, StatefulScheduler::JobComparator::Make(schedparams->at(2)), filltimes, true);
        break;

        //Default: scheduler name not matched
      default:
        error("Could not parse name of scheduler");
    }

  }
  return NULL; //control never reaches here
}

Machine* Factory::getMachine(SST::Component::Params_t& params, int numProcs, schedComponent* sc){
  if(params.find("machine") == params.end()){
    //default: FIFO queue priority scheduler
    printf("Defaulting to Simple Machine\n");
    return new SimpleMachine(numProcs, sc);
  }
  else
  {
    vector<string>* schedparams = parseparams(params["machine"]);
    switch(machinename(schedparams->at(0)))
    {
      //simple machine
      case SIMPLEMACH:
        if(DEBUG) printf("Simple Machine\n");
        return new SimpleMachine(numProcs, sc);
        break;

        //Mesh Machine
      case MESH:
        if(DEBUG) printf("Mesh Machine\n");
        {
          if(schedparams->size() != 3 && schedparams->size() != 4)
            error("Wrong number of arguments for Mesh Machine:\nNeed 3 (x, y, and z dimensions) or 2 (z defaults to 1)");
          int x = strtol(schedparams->at(1).c_str(),NULL,0); 
          int y = strtol(schedparams->at(2).c_str(),NULL,0); 
          int z;
          if(schedparams->size() == 4)
            z = strtol(schedparams->at(3).c_str(),NULL,0); 
          else
            z = 1;
          if(x*y*z != numProcs)
            error("The dimensions of the mesh do not correspond to the number of processors");
          return new MachineMesh(x,y,z,sc);
          break;
        }

      default:
        error("Could not parse name of machine");
    }
  }
  return NULL; //control never reaches here
}


Allocator* Factory::getAllocator(SST::Component::Params_t& params, Machine* m){
  if(params.find("allocator") == params.end()){
    //default: FIFO queue priority scheduler
    printf("Defaulting to Simple Allocator\n");
    SimpleMachine* mach = dynamic_cast<SimpleMachine*>(m);
    if(mach == NULL)
      error("Simple Allocator requires SimpleMachine");
    return new SimpleAllocator(mach);
  }
  else
  {
    vector<string>* schedparams = parseparams(params["allocator"]);
    vector<string>* nearestparams = NULL;
    switch(allocatorname(schedparams->at(0)))
    {
      //Simple Allocator for use with simple machine
      case SIMPLEALLOC:
        if(DEBUG) printf("Simple Allocator\n");
        {
          SimpleMachine* mach = dynamic_cast<SimpleMachine*>(m);
          if(mach == NULL)
            error("SimpleAllocator requires SimpleMachine");
          return new SimpleAllocator(mach);
          break;
        }

        //Random Allocator, allocates procs randomly from a mesh
      case RANDOM:
        if(DEBUG) printf("Random Allocator\n");
        return new RandomAllocator(m);
        break;

        //Nearest Allocators try to minimize distance between processors
        //according to various metrics
      case NEAREST:
        if(DEBUG) printf("Nearest Allocator\n");
        nearestparams = new vector<string>;
        for(int x = 0; x < (int)schedparams->size(); x++)
          nearestparams->push_back(schedparams->at(x));
        return new NearestAllocator(nearestparams, m);
        break;
      case GENALG:
        if(DEBUG) printf("General Algorithm Nearest Allocator\n");
        nearestparams = new vector<string>;
        nearestparams->push_back("genAlg");
        return new NearestAllocator(nearestparams, m);
        break;
      case MM:
        if(DEBUG) printf("MM Allocator\n");
        nearestparams = new vector<string>;
        nearestparams->push_back("MM");
        return new NearestAllocator(nearestparams, m);
        break;
      case MC1X1:
        if(DEBUG) printf("MC1x1 Allocator\n");
        nearestparams = new vector<string>;
        nearestparams->push_back("MC1x1");
        return new NearestAllocator(nearestparams, m);
        break;
      case OLDMC1X1:
        if(DEBUG) printf("Old MC1x1 Allocator\n");
        nearestparams = new vector<string>;
        nearestparams->push_back("OldMC1x1");
        return new NearestAllocator(nearestparams, m);
        break;

        //MBS Allocators use a block-based approach
      case MBS:
        if(DEBUG) printf("MBS Allocator\n");
        return new MBSAllocator(nearestparams, m);
      case GRANULARMBS:
        if(DEBUG) printf("Granular MBS Allocator\n");
        return new GranularMBSAllocator(nearestparams, m);
      case OCTETMBS: 
        if(DEBUG) printf("Octet MBS Allocator\n");
        return new OctetMBSAllocator(nearestparams, m);

        //Linear Allocators allocate in a linear fashion
        //along a curve
      case FIRSTFIT:
        if(DEBUG) printf("First Fit Allocator\n");
        nearestparams = new vector<string>;
        for(int x = 1; x < (int)schedparams->size(); x++)
          nearestparams->push_back(schedparams->at(x));
        return new FirstFitAllocator(nearestparams, m);
      case BESTFIT:
        if(DEBUG) printf("Best Fit Allocator\n");
        nearestparams = new vector<string>;
        for(int x = 1; x < (int)schedparams->size(); x++)
          nearestparams->push_back(schedparams->at(x));
        return new BestFitAllocator(nearestparams, m);
      case SORTEDFREELIST:
        if(DEBUG) printf("Sorted Free List Allocator\n");
        nearestparams = new vector<string>;
        for(int x = 1; x < (int)schedparams->size(); x++)
          nearestparams->push_back(schedparams->at(x));
        return new SortedFreeListAllocator(nearestparams, m);

    //Constraint Allocator tries to separate nodes whose estimated failure rates are close
      case CONSTRAINT:
        if( params.find( "ConstraintAllocatorDependencies" ) == params.end() )
	  error( "Constraint Allocator requires ConstraintAllocatorDependencies scheduler parameter" );
	if( params.find( "ConstraintAllocatorConstraints" ) == params.end() )
	  error( "Constraint Allocator requires ConstraintAllocatorConstraints scheduler parameter" );
        {SimpleMachine* mach = dynamic_cast<SimpleMachine*>(m);
        if(mach == NULL)
            error("ConstraintAllocator requires SimpleMachine");
        // will get these file names from schedparams eventually
        return new ConstraintAllocator(mach, params.find( "ConstraintAllocatorDependencies" )->second, params.find( "ConstraintAllocatorConstraints" )->second );
        break;
        }
      default:
        error("Could not parse name of allocator");
    }
  }
  return NULL; //control never reaches here
}

vector<string>* Factory::parseparams(string inparam){
  //takes in a parameter and breaks it down from class[arg,arg,...]
  //into {class, arg, arg}
  vector<string>* ret = new vector<string>;
  stringstream ss;
  ss.str(inparam);
  string str;
  getline(ss,str,'[');
  transform(str.begin(), str.end(), str.begin(), ::tolower);
  ret->push_back(str);
  while(getline(ss, str, ',') && str != "]")
  {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    if(*(str.rbegin()) == ']')
      str = str.substr(0,str.size()-1);
    ret->push_back(str);
  }
  return ret;
}

Factory::SchedulerType Factory::schedulername(string inparam){
  for(int i = 0; i < numSchedTableEntries; i++)
    if(inparam == schedTable[i].name)
      return schedTable[i].val;
  error("Scheduler name not found:" + inparam);
  exit(0); // control never reaches here
}
Factory::MachineType Factory::machinename(string inparam){
  for(int i = 0; i < numMachTableEntries; i++)
    if(inparam == machTable[i].name)
      return machTable[i].val;
  error("Machine name not found:" + inparam);
  exit(0);
}
Factory::AllocatorType Factory::allocatorname(string inparam){
  for(int i = 0; i < numAllocTableEntries; i++)
    if(inparam == allocTable[i].name)
      return allocTable[i].val;
  error("Allocator name not found:" + inparam);
  exit(0);
}
