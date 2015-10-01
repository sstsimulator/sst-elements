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


#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "McSimDefs.h"
#include "CycleTracker.h"

namespace McNiagara{
//-------------------------------------------------------------------
/// @brief Dependency tracker class
///
//-------------------------------------------------------------------
class DependencyTracker
{
   /// Dependency list node type
   struct Dependency
   {
      InstructionNumber producer;
      InstructionNumber consumer;
      CycleCount availableCycle;
      CycleTracker::CycleReason reason;
      struct Dependency *next;
   };

 public:
   DependencyTracker();
   ~DependencyTracker();
   int addDependency(InstructionNumber instructionNum, CycleCount whenSatisfied,
                     CycleTracker::CycleReason reason);
   int adjustDependenceChain(CycleCount numCycles);
   CycleCount isDependent(InstructionNumber instructionNum, CycleTracker::CycleReason *reason);
 private:
   Dependency *DepQHead, ///< Dependency list head pointer
              *DepQTail; ///< Dependency list tail pointer
};
}//end namespace McNiagara
#endif
