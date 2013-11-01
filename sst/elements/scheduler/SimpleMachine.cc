// Copyright 2011 Sandia Corporation. Under the terms                          
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
#include "SimpleMachine.h"

#include <vector>
#include <string>
#include <stdio.h>

#include "AllocInfo.h"
#include "Job.h"
#include "Machine.h"
#include "misc.h"
#include "schedComponent.h"

using namespace SST::Scheduler;

//takes number of processors
SimpleMachine::SimpleMachine(int procs, schedComponent* sc, bool insimulationmachine) 
{  
    schedout.init("", 8, ~0, Output::STDOUT);
    numProcs = procs;
    this->sc = sc;
    simulationmachine = insimulationmachine;
    reset();
}

//return to beginning-of-simulation state
void SimpleMachine::reset() 
{  
    numAvail = numProcs;
    freeNodes.clear();
    for(int i = 0; i < numProcs; i++)
        freeNodes.push_back(i);
}

/*
   Machine* SimpleMachine::Make(std::vector<std::string>* params) { //Factory creation method
   argsAtMost(1, params);
   argsAtLeast(1, params);

   return new SimpleMachine(atoi((*params)[1].c_str()));
   }

   std::string SimpleMachine::getParamHelp() {
   return "[<num procs>]";
   }
   */

std::string SimpleMachine::getSetupInfo(bool comment)
{
    std::string com;
    if (comment) {
        com="# ";
    } else {
        com="";
    }
    char mesg[100];
    sprintf(mesg, "SimpleMachine with %d processors", numProcs);
    return com + mesg;
}

//Allocates processors
void SimpleMachine::allocate(AllocInfo* allocInfo) 
{  
    int num = allocInfo -> job -> getProcsNeeded();  //number of processors

    /*
    if(debug)
        std::cerr << "allocate(" << allocInfo -> job -> toString() << "); "
            << (numAvail - num) << " processors free" << std::endl;
            */

    if (allocInfo -> job -> hasStarted()) {
        schedout.debug(CALL_INFO, 7, 0, "%lu: allocate(%s) ending at %lu\n", allocInfo -> job -> getStartTime(), allocInfo -> job -> toString().c_str(), allocInfo -> job -> getStartTime() + allocInfo -> job -> getActualTime());
    } else {
        schedout.debug(CALL_INFO, 7, 0, "allocate(%s);\n", allocInfo -> job -> toString().c_str());
    }
    if(num > numAvail) {
        //char mesg[100];
        //sprintf(mesg, "Attempt to allocate %d processors when only %d are available", num, numAvail);
        //internal_error(std::string(mesg));
        schedout.fatal(CALL_INFO, 1, "Attempt to allocate %d processors when only %d are available", num, numAvail);
    }

    numAvail -= num;
    if (allocInfo->nodeIndices[0] == -1){ //default allocator
        for(int i=0; i<num; i++) {
            allocInfo->nodeIndices[i] = freeNodes.back();
            freeNodes.pop_back();
        }
    }
    else { //ConstraintAllocator; remove the indices given by allocInfo
        for(int i=0; i<num; i++){
            freeNodes.erase( remove(freeNodes.begin(), freeNodes.end(), allocInfo->nodeIndices[i]) , freeNodes.end() );
        }
    }

    //we don't want to tell schedComponent about the job
    //if this machine is only simulating the job
    //(such as for calculating FST)
    if (!simulationmachine) sc -> startJob(allocInfo);
}

//Deallocates processors
void SimpleMachine::deallocate(AllocInfo* allocInfo) 
{  
    int num = allocInfo ->job -> getProcsNeeded();  //number of processors
    //if(debug)
    //    std::cerr << "deallocate(" << allocInfo -> job << "); " 
    //        << (numAvail + num) << " processors free" << std::endl;

    schedout.debug(CALL_INFO, 7, 0, "deallocate(%s); %d processors free\n", allocInfo -> job ->toString().c_str(), numAvail + num);

    if(num > (numProcs - numAvail)) {
        schedout.fatal(CALL_INFO, 1, "Attempt to deallocate %d processors when only %d are busy", num, (numProcs-numAvail));
        //char mesg[100];
        //sprintf(mesg, "Attempt to deallocate %d processors when only %d are busy",
        //        num, (numProcs-numAvail));
        //internal_error(std::string(mesg));
    }

    numAvail += num;
    for(int i = 0; i < num; i++) {
        freeNodes.push_back(allocInfo -> nodeIndices[i]);
    }
}

std::vector<int>* SimpleMachine::freeProcessors(){
    std::vector<int>* retVal = new std::vector<int>();
    for (unsigned int i = 0; i < freeNodes.size(); i++)
        retVal->push_back(freeNodes[i]);
    return retVal;
}

std::string SimpleMachine::getNodeID(int i){
    return sc->getNodeID(i);
}


