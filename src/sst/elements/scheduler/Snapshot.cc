// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
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


