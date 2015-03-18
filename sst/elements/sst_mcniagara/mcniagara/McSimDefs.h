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


#ifndef MCSIMDEFS_H
#define MCSIMDEFS_H
namespace McNiagara{
typedef unsigned long long InstructionNumber;
//typedef unsigned long long CycleCount;
typedef double CycleCount;
typedef unsigned long long Address;


// Stall reasons
/***
enum StallReason {
   CPII, I_CACHE, L1_CACHE, L2_CACHE, MEMORY, 
   INT_DEP, INT_USE_DEP, INT_DSU_DEP, 
   FGU_DEP, BRANCH_MP, BRANCH_ST,
   P_FLUSH, STB_FULL, SPCL_LOAD, LD_STB,
   TLB_MISS, ITLB_MISS, NUMSTALLREASONS
};
***/

double my_rand(void);
}//end namespace McNiagara
#endif
