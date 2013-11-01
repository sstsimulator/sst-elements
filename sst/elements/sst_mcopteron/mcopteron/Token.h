
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
         CycleCount atCycle, bool isFake, CPIStack *cpiStack);
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
   void updateCPIStack();
   void setDTLB1(bool c) { dtlb1 = c; }
   void setDTLB2(bool c) { dtlb2 = c; }
   void setL1(bool c) { L1 = c; }
   void setL2(bool c) { L2 = c; }
   void setL3(bool c) { L3 = c; }
   void setMem(bool c) { mem = c; }
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
   bool loadSatisfied;        ///< True if load data has been fetched from cache or memory
   bool storeSatisfied;       ///< True if store has been issued from the LSQ
   bool hasAddressOperand;   ///< True if insn needs address generated
   bool addressGenerated;    ///< True if address has already been generated
   bool hasLoad;             ///< True if insn does a memory load
   bool hasStore;            ///< True if insn does a memory store
   bool completed;           ///< True if instruction has finished
   Dependency *inDependency;  ///< record for input dependencies
   bool wasMispredicted;     ///< True if this is a branch and it was mispredicted
   CPIStack *cpiStack;       ///< A pointer to the CPI stack variable in McOpteron
   // variables for CPI stack
   bool dtlb1;
   bool dtlb2;
   bool L1;
   bool L2;
   bool L3;
   bool mem;
};
}//end namespace McOpteron
#endif
