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


#ifndef CYCLETRACKER_H
#define CYCLETRACKER_H

#include <string>
using std::string;
#include <vector>
using std::vector;
#include "McSimDefs.h"

namespace McNiagara{
/// @brief Keeps track of cycles accumulated and the reasons for them
//
//
class CycleTracker
{
 public:
   enum CycleReason { 
      CPII, I_CACHE, L1_CACHE, L2_CACHE, MEMORY, 
	   INT_DEP, INT_USE_DEP, INT_DSU_DEP, 
   	FGU_DEP, BRANCH_MP, BRANCH_ST,
	   P_FLUSH, STB_FULL, SPCL_LOAD, LD_STB,
   	TLB_MISS, ITLB_MISS, NUMCYCLEREASONS
   };
   CycleTracker();
   ~CycleTracker();
   void accountForCycles(CycleCount cycles, CycleReason reason);
   CycleCount currentCycles();
   CycleCount cyclesForCategory(CycleReason reason);
   double cyclePercentForCategory(CycleReason reason);
   unsigned long long eventCountForCategory(CycleReason reason);
   string categoryName(CycleReason reason);
 private:
   vector<string> cycleReasonNames;
   CycleCount totalCycles;
   CycleCount *categoryCycles;
   unsigned long long *categoryCount;
};
}//end Namespace McNiagara
#endif
