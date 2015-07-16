// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "output.h"
#include "Snapshot.h"
//#include "events/SnapshotEvent.h"

using namespace SST::Scheduler;

Snapshot::Snapshot()
{

}

Snapshot::~Snapshot()
{
    runningJobs.clear();
}


void Snapshot::append(SimTime_t snapshotTime, unsigned long nextArrivalTime, int jobNum, ITMI itmi)
{
    this->runningJobs[jobNum] = itmi;
    this->snapshotTime = snapshotTime;
    this->nextArrivalTime = nextArrivalTime;
}


