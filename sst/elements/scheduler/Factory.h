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


/* Factory file helps parse the parameters in the sdl file
 * returns correct type of machine, allocator, and scheduler
 */

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <string>
#include "Scheduler.h"
#include "Machine.h"
#include "Allocator.h"
#include "sst/core/element.h"
#include "sst/core/sst_types.h"

using namespace std;

class Factory{
  public:
    Scheduler* getScheduler(SST::Component::Params_t& params, int numProcs);
    Machine* getMachine(SST::Component::Params_t& params, int numProcs, schedComponent* sc);
    Allocator* getAllocator(SST::Component::Params_t& params, Machine* m);
  private:
    vector<string>* parseparams(string inparam);

    //the following is for easy conversion from strings to ints
    //to add a new (say) scheduler, add its string to Schedulertype
    //and schedTable[], and increment numSchedTableEntries and the
    //size of schedTable.  Then add a case to getScheduler() that calls
    //its constructor
    enum SchedulerType{
      PQUEUE = 0,
      EASY = 1,
      CONS = 2,
      PRIORITIZE = 3,
      DELAYED = 4,
      ELC = 5,
    };
    enum MachineType{
      SIMPLEMACH = 0,
      MESH = 1,
    };
    enum AllocatorType{
      SIMPLEALLOC = 0,
      RANDOM = 1,
      NEAREST = 2,
      GENALG = 3,
      MM = 4,
      MC1X1 = 5,
      OLDMC1X1 = 6,
      MBS = 7,
      GRANULARMBS = 8,
      OCTETMBS = 9,
      FIRSTFIT = 10,
      BESTFIT = 11,
      SORTEDFREELIST = 12,
    };

    struct schedTableEntry{
      SchedulerType val;
      string name;
    };
    struct machTableEntry{
      MachineType val;
      string name;
    };
    struct allocTableEntry{
      AllocatorType val;
      string name;
    };

    static const schedTableEntry schedTable[6];
    static const machTableEntry machTable[2];
    static const allocTableEntry allocTable[13];

    SchedulerType schedulername(string inparam);
    MachineType machinename(string inparam);
    AllocatorType allocatorname(string inparam);
    static const int numSchedTableEntries;
    static const int numAllocTableEntries;
    static const int numMachTableEntries;
};

#endif
