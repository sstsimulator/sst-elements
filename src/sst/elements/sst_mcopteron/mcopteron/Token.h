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


#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

//class Dependency; 
#include "InstructionInfo.h"
#include "Listener.h"
#include "Dependency.h"
#include "FunctionalUnit.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
//class FunctionalUnit;
class Dependency;
/// @brief Simulated instruction class
///
class Token
{
 public:
   Token(InstructionInfo *type, InstructionCount number,
         CycleCount atCycle, bool isFake);
   Token(CycleCount atCycle);	// create empty token
   ~Token();
   void dumpDebugInfo();
   void dumpTokenTrace(FILE *f);
   void setMemoryLoadInfo(Address address, unsigned int numBytes);
   void setMemoryStoreInfo(Address address, unsigned int numBytes);
   void setInDependency(Dependency *dep);
   bool checkDependency();
   void removeDependency(); 
   void addListener(Listener *l) {deleteListener = l;}
   void setOptionalProb(double p) {optionalProb = p;}
   InstructionInfo* getType() {return type;}
   InstructionCount instructionNumber() {return number;}
   void fixupInstructionInfo();
   bool needsAddressGeneration();
   bool addressIsReady();
   bool needsFunctionalUnit(FunctionalUnit *fu);
   bool aguOperandsReady(CycleCount atCycle);
   bool allOperandsReady(CycleCount atCycle);
   bool isExecuting(CycleCount currentCycle);
   bool isCompleted(CycleCount currentCycle);
   bool isLoad() {return hasLoad;}
   bool isStore() {return hasStore;}
   bool isFake() {return fake;}
   bool wasRetired() {return retired;}
   bool wasCanceled() {return canceled;}
   CycleCount issuedAt() {return issueCycle;}
   //Waleed: added following two methods to set issue and exec cycles
   void setIssueCycle(CycleCount atCycle) {issueCycle=atCycle;}
   void setExecCycle(CycleCount atCycle) {execStartCycle=atCycle;}
   void setBranchMispredict() {wasMispredicted = true;}
   void executionStart(CycleCount currentCycle);
   void executionContinue(CycleCount currentCycle);
   void loadSatisfiedAt(CycleCount atCycle);
   void storeSatisfiedAt(CycleCount atCycle);
   bool isMispredictedJump();
   void retireInstruction(CycleCount atCycle);
   void cancelInstruction(CycleCount atCycle);
   static unsigned int totalTokensCreated, totalTokensDeleted;
   static InstructionCount lastTokenDone; 
 private:
   InstructionInfo *type;    ///< pointer to instruction info
   double optionalProb;      ///< option probability for sim to use
   InstructionCount number;  ///< issue number of this instruction
   CycleCount issueCycle;    ///< cycle at which issued
   CycleCount retiredCycle;  ///< cycle at which retired (will be computed)
   CycleCount currentCycle;  ///< current cycle in instruction's progress
   FunctionalUnit *atUnit;   ///< Ptr to unit this instruction is at
   CycleCount execStartCycle; ///< cycle of start of func unit use
   CycleCount execEndCycle;  ///< cycle of end of func unit use
   Listener *deleteListener; ///< item that needs notified of token delete
   bool fake;
   bool canceled;         ///< True if was canceled
   bool retired;             ///< True if was retired
   bool loadSatisfied;
   bool hasAddressOperand;   ///< True if insn needs address generated
   bool addressGenerated;    ///< True if address has already been generated
   bool hasLoad;             ///< True if insn does a memory load
   bool hasStore;            ///< True if insn does a memory store
   bool completed;           ///< True if instruction has finished
   Dependency *inDependency;  ///< record for input dependencies
   bool wasMispredicted;     ///< True if this is a branch and it was mispredicted
};
}//end namespace McOpteron
#endif
