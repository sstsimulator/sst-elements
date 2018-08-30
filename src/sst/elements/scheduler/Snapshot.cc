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

#include "output.h"
#include "Snapshot.h"
//#include "events/SnapshotEvent.h"

using namespace SST::Scheduler;


Snapshot::Snapshot() : simFinished(true)
{

}

Snapshot::~Snapshot()
{
    runningJobs.clear();
}


void Snapshot::append(SimTime_t snapshotTime, unsigned long nextArrivalTime, std::map<int, ITMI> runningJobs)
{
    this->runningJobs = runningJobs;
    this->snapshotTime = snapshotTime;
    this->nextArrivalTime = nextArrivalTime;
    this->simFinished = false;
}


