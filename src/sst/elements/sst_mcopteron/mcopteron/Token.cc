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

 
#include <stdio.h>
//#include <malloc.h>
#include <memory.h>

#include "Token.h"
//#include "FunctionalUnit.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
unsigned int Token::totalTokensCreated = 0;
unsigned int Token::totalTokensDeleted = 0;
InstructionCount Token::lastTokenDone = 0; // last token retired/canceled

/// @brief Constructor
///
Token::Token(InstructionInfo *Type, InstructionCount Number, 
             CycleCount atCycle, bool isFake)
{
   number=Number;//this->number = number;
   issueCycle = atCycle;
   currentCycle = atCycle;
   retiredCycle = 0;
   type=Type;//this->type = type;
   fake = isFake;
   if (!fake)
      type->incSimulationCount();//this->type->incSimulationCount();
   execStartCycle = atCycle;
   execEndCycle = 0;
   optionalProb = 0.0;
   hasAddressOperand = false; //( number % 2 == 0 );
   hasLoad = hasStore = false;
   loadSatisfied = false;
   addressGenerated = false;
   completed = false;
   retired = false;
   wasMispredicted = false;
   canceled = false;
   atUnit = 0;
   inDependency = 0;
   deleteListener = 0;
   totalTokensCreated++;
}

/// @brief: Destructor
///
Token::~Token()
{
   removeDependency();
   if (Debug>=3) fprintf(stderr, "Tk: destructing %llu at %p\n", number, this);
   if (deleteListener)
      deleteListener->notify(this);
   memset(this, 0, sizeof(Token));
   totalTokensDeleted++;
}

void Token::dumpDebugInfo()
{
	Dependency *dep = inDependency;
   fprintf(stderr, "Tk: number %llu issued %llu indeps %u numReady %u addrOp %s %s "
           "load %s %s store %s compl/ret/canc %s%s%s\n",
           number, issueCycle, dep?dep->numInputs:0,
           dep?dep->numReady:0,
           hasAddressOperand?"T":"F", addressGenerated?"T":"F", 
           hasLoad?"T":"F", loadSatisfied?"T":"F", hasStore?"T":"F", 
           completed?"T":"F", retired?"T":"F", canceled?"T":"F");
   fprintf(stderr, "Tk: Token %llu (%s) is starting at %llu till %llu\n",
              number, this->getType()->getName(), execStartCycle, execEndCycle);  
}

void Token::dumpTokenTrace(FILE *f)
{
   //Waleed: use a new method to print token trace
   fprintf(f, "%llu %s\n", number, type->printInfo());
/* // old code
   fprintf(f, "%llu %s %u ", number, type->getName(), type->getMaxOpSize());
   if (hasStore)
      fprintf(f,"mem, ");
   else
      fprintf(f,"reg, ");
   if (hasLoad)
      fprintf(f,"mem\n");
   else
      fprintf(f,"reg\n");
*/	  
   return;
}

/// @brief Set memory load operation info
///
void Token::setMemoryLoadInfo(Address address, unsigned int numBytes)
{
   // if stack op and loadProb is 1, then is a pop and doesn't need AGU,
   // otherwise it is a mem op and needs AGU
   if (type->needsLoadAddress())
      hasAddressOperand = true;
   hasLoad = true;
   loadSatisfied = false;
}

/// @brief Set memory store operation info
///
void Token::setMemoryStoreInfo(Address address, unsigned int numBytes)
{
   // if stack op and storeProb is 1, then is a push and doesn't need AGU,
   // otherwise it is a mem op and needs AGU
   if (type->needsStoreAddress())
      hasAddressOperand = true;
   hasStore = true;
}


/// @brief Adjust instruction info record if necessary.
///
/// Once the token has load/store's possibly generated, we
/// might need to point at a different instruction info record,
/// because multiple variants of an instruction are handled 
/// differently.
///
void Token::fixupInstructionInfo()
{
   InstructionInfo *ii = type;
   const char *origName = ii->getName();
   if (hasLoad && !ii->handlesLoad()) {
      ii = ii->getNext();
      while (ii && !strcmp(ii->getName(), origName)) {
         if (ii->handlesLoad()) break;
         ii = ii->getNext();
      }
   } else if (hasStore && !ii->handlesStore()) {
      ii = ii->getNext();
      while (ii && !strcmp(ii->getName(), origName)) {
         if (ii->handlesStore()) break;
         ii = ii->getNext();
      }
   }
   if (ii && ii != type && !strcmp(ii->getName(), origName)) {
      // we've found a different instruction record of the same 
      // instruction name and it supports the necessary data direction,
      // so change the token's record ptr.
      type = ii;
      if (Debug>=3) fprintf(stderr,"TOK %llu: switching Inst Infos\n", number);
   }
}


/// @brief Set link to input dependency record
///
void Token::setInDependency(Dependency *dep)
{
   inDependency = dep;
}

/// @brief Check if instruction needs an address generated
///
bool Token::needsAddressGeneration()
{
   if (type && type->isFPUInstruction() && hasAddressOperand) {
      // Waleed: for now, assume address is always redy for fp instructions
      // waleed: we are doing this for now because we are ignoring FAKE LEA
      addressGenerated = true; 
      // rely on fake LEA to indicate address is generated
      // - it will increment the dependency ready count,
      //   this is not quite accurate but should be close
      //if (inDependency && inDependency->numReady > 0)
      //   addressGenerated = true;
   }
   if (hasAddressOperand && !addressGenerated)
      return true;
   else
      return false;
}

/// @brief Check if address is ready for memory op
///
bool Token::addressIsReady()
{
   return !needsAddressGeneration();
}


/// @brief Check if instruction can use functional unit now
///
bool Token::needsFunctionalUnit(FunctionalUnit *fu)
{
   // TODO: compare instruction info and see if it can execute on
   // the FU. this must also consider sequencing, such as ALU after
   // AGU 
   if (hasAddressOperand && !addressGenerated) {
      if (fu->getType() == AGU) // || fu->getType() == FADD)
         return true;
      else
         return false;
   }
   if (type->needsFunctionalUnit(fu->getType()))
      return true;
   else
      return false;
}

/// @brief Check if AGU operands are ready
///
/// TODO: We may need separate use-distance tables for AGU operands and
/// ALU operands, since they can execute independently and are quite 
/// different. For now we assume AGU operands are ready always
///
bool Token::aguOperandsReady(CycleCount atCycle)
{
   return true;
}

/// @brief Check if all operands are available for instruction
///
bool Token::allOperandsReady(CycleCount atCycle)
{
   Dependency *dep = inDependency;
	if(!checkDependency()) { 
      if (Debug>=3) 
         fprintf(stderr, "Token %llu still waiting for dependencies %u %u\n",
                 number, dep->numReady, dep->numInputs);
      return false;
   } else if (hasLoad && !loadSatisfied) {
      if (Debug>=3) 
         fprintf(stderr, "Token %llu still waiting for memory load\n", number);
      return false;
   } else {
      return true;
   }
}


/// @brief Mark the beginning of execution on a functional unit
///
void Token::executionStart(CycleCount currentCycle)
{
   execStartCycle = currentCycle;
   if (hasAddressOperand && !addressGenerated) {
      // assume we are generating an address, finishes in one cycle
      execEndCycle = currentCycle + AGU_LATENCY - 1;
   } else {
      execEndCycle = currentCycle + type->getLatency() - 1;
   }
   if (Debug>=2) fprintf(stderr, "Token %llu is starting at %llu till %llu\n",
              number, currentCycle, execEndCycle);
}

void Token::loadSatisfiedAt(CycleCount atCycle)
{
   loadSatisfied = true;
}

void Token::storeSatisfiedAt(CycleCount atCycle)
{
}


/// @brief True if instruction is executing on functional unit now
///
bool Token::isExecuting(CycleCount currentCycle)
{
   bool needsAddress = needsAddressGeneration(); // side effect for FP insns
   if (execEndCycle == 0)
      return false;
   if (execEndCycle >= currentCycle)
      // still executing
      return true;
   else {
      // has finished some exec step
      if (needsAddress) //hasAddressOperand && !addressGenerated) 
         // we assume the first step must have been to generate an address
         // - maybe we should check the unit (AGU or FADD??)
         addressGenerated = true;
      else {
         completed = true;
         if (Debug>=3)
           fprintf(stderr,"Tk %llu: completed\n", number);
      }
      execEndCycle = 0; // clear exec step
      return false;
   }
}


/// @brief True if instruction has been completed
///
bool Token::isCompleted(CycleCount currentCycle)
{
   return completed; // allow token to check itself for this cycle

}

bool Token::isMispredictedJump()
{
   return wasMispredicted;
}

/// @brief Mark instruction as retired
///
void Token::retireInstruction(CycleCount atCycle)
{
   completed = true; // should already be set, but...
   retired = true;
	lastTokenDone = number;
   if (Debug>=3)
      fprintf(stderr,"Tk %llu: retired\n", number);
  	removeDependency(); 
}

/// @brief Mark instruction as canceled
///
void Token::cancelInstruction(CycleCount atCycle)
{
   completed = true;
   canceled = true;
	lastTokenDone = number;
   if (Debug>=4) fprintf(stderr,"Token: %llu is being canceled now", this->number);	
	removeDependency(); 
}

/// @brief check if dependencies for current token are satisfied
///
/// This walks the current dependency list and update records accordingly
/// It returns true if operands for current token have been generated
bool Token::checkDependency()
{
   Dependency *d, *n, *d2;
   d = Dependency::dependenciesHead; 
   if (Debug>=4) fprintf(stderr,"Checking dependencies for token: %llu\n", number);
   // first walk the list and update if needed
   while (d) {
      if (d->tkn->isCompleted(number)) {
         d2 = Dependency::dependenciesHead;
			while(d2){ 
				if (d2->numReady < d2->numInputs) {
					for(unsigned int i=0; i<d2->numInputs; i++) { 
						// update all those inputs waiting for d->tkn
						if(d->tkn->instructionNumber() == d2->producers[i]) { 
							d2->numReady++;
							d2->producers[i] = 0; 
						}
					}
				}
			   d2 = d2->next;
			}			
      }
      else if (d->numReady < d->numInputs) {
         for(unsigned int i=0; i<d->numInputs; i++) { 
            // also check if the producing token had been retired/canceled...etc
            if(d->producers[i] <= lastTokenDone && d->producers[i] != 0) { 
               d->numReady++;
               d->producers[i] = 0; 
            }
         }
      }
      n = d->next;
      d = n;
   }
   // now check if operands are ready
   d = inDependency;
	if(d && d->numReady != d->numInputs) { 
      if (Debug>=3) 
         fprintf(stderr, "Token %llu still waiting for dependencies Ready:%u Inputs:%u\n",
                 number, d->numReady, d->numInputs);
      return false;
   } 
	return true; 				  

}

/// @brief remove dependency record for this token
///
void Token::removeDependency()
{
   Dependency *d, *n, *prev = 0;
   d = Dependency::dependenciesHead; 
   if (Debug>=4) fprintf(stderr,"Removing dependency for token: %llu\n", number);
   // if it's been removed already
   if(!inDependency) 
      return; 
   // search and remove t
   while (d) {
      if (d!=inDependency) {
        n = d->next;
        prev = d;			   				
      }
      // remove
      else {
         n = d->next;
         if (prev)
            prev->next = d->next;
         else 
            Dependency::dependenciesHead = d->next;
         if (Dependency::dependenciesTail == d) {
            if (d->next)
               Dependency::dependenciesTail = d->next;
            else if (prev)
               Dependency::dependenciesTail = prev;
            else
               Dependency::dependenciesTail = Dependency::dependenciesHead;
         }
         delete d; 
         inDependency = 0;
         break;
      }
      d = n;
   }
   if (Debug>=4) fprintf(stderr,"Done updating dependencies\n");
}

}//end namespace McOpteron
