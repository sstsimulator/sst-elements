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


#include "CycleTracker.h"
namespace McNiagara{
// These must match enum definitions exactly or else the
// names will be meaningless
/*string CycleTracker::cycleReasonNames[NUMCYCLEREASONS+1] = {
   "CPI-inh", "I Cache", "L1 Cache", "L2 Cache", "Memory", 
   "Int Dep", "Int-Use Dep", "Int-DSU Dep", 
   "FGU Dep", "Branch MisP", "Branch Stall", 
   "Pipe Flush", "STB Full", "Special Loads", 
   "Ld STB", "TLB Miss", "ITLB Miss", 0 };*/

CycleTracker::CycleTracker()
{
	cycleReasonNames.push_back("CPI-inh");
	cycleReasonNames.push_back("I Cache");
	cycleReasonNames.push_back("L1 Cache");
	cycleReasonNames.push_back("L2 Cache");
	cycleReasonNames.push_back("Memory"); 
	cycleReasonNames.push_back("Int Dep");
	cycleReasonNames.push_back("Int-Use Dep");
	cycleReasonNames.push_back("Int-DSU Dep"); 
	cycleReasonNames.push_back("FGU Dep");
	cycleReasonNames.push_back("Branch MisP");
	cycleReasonNames.push_back("Branch Stall"); 
	cycleReasonNames.push_back("Pipe Flush");
	cycleReasonNames.push_back("STB Full");
	cycleReasonNames.push_back("Special Loads"); 
	cycleReasonNames.push_back("Ld STB");
	cycleReasonNames.push_back("TLB Miss");
	cycleReasonNames.push_back("ITLB Miss");

	int i;
	categoryCycles = new CycleCount[NUMCYCLEREASONS+1];
	categoryCount = new unsigned long long[NUMCYCLEREASONS+1];
	for (i = 0; i <= NUMCYCLEREASONS; i++) {
		categoryCycles[i] = 0;
		categoryCount[i] = 0;
	}
}

CycleTracker::~CycleTracker()
{
   delete[] categoryCycles;
   delete[] categoryCount;
}

void CycleTracker::accountForCycles(CycleCount cycles, CycleReason reason)
{
   categoryCycles[reason] += cycles;
   categoryCount[reason]++;
   totalCycles += cycles;
}

CycleCount CycleTracker::currentCycles()
{
   return totalCycles;
}

CycleCount CycleTracker::cyclesForCategory(CycleReason reason)
{
   return categoryCycles[reason];
}

double CycleTracker::cyclePercentForCategory(CycleReason reason)
{
   return (double) categoryCycles[reason] * 100.0 / totalCycles;
}

unsigned long long CycleTracker::eventCountForCategory(CycleReason reason)
{
   return categoryCount[reason];
}

string CycleTracker::categoryName(CycleReason reason)
{
   return cycleReasonNames[reason];
}
}//end namespace McNiagara
