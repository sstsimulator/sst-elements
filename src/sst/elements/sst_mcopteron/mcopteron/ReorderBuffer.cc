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
#include <string.h>
#include <stdlib.h>

#include "ReorderBuffer.h"
#include "Token.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor: create and clear data
/// @param numSlots is the total number of instruction slots in the buffer
/// @param numRetireablePerCycle is the maximum # of instructions retireable per cycle
///
ReorderBuffer::ReorderBuffer(unsigned int numSlots, unsigned int numRetireablePerCycle)
{
   this->numSlots = numSlots;
   numPerCycle = numRetireablePerCycle;
   tokenBuffer = new Token*[numSlots];
   numTokens = 0;
   availSlot = 0;
   retireSlot = 0;
   totalRetired = totalAnulled = 0;
   fullStalls = 0;
   for (unsigned int i=0; i < numSlots; i++)
      tokenBuffer[i] = 0;
}

/// @brief Destructor: report stats
///
ReorderBuffer::~ReorderBuffer()
{
   delete[] tokenBuffer;
   fprintf(stdout, "ROB: Total instructions retired: %llu\n", totalRetired);
   fprintf(stdout, "ROB: Total instructions anulled: %llu\n", totalAnulled);
   fprintf(stdout, "ROB: full RO buffer stalls: %llu\n", fullStalls);
}

/// @brief Dispatch an instruction to the reorder buffer
///
/// This assumes that isFull() was already checked; it will
/// silently fail to add the instruction if the buffer is
/// full.
/// @param token is the instruction to add to the buffer
/// @param atCycle is the cycle count at which it is being added
///
bool ReorderBuffer::dispatch(Token *token, CycleCount atCycle)
{
   if (numTokens >= numSlots) {
      fprintf(stderr, "ROB: Error dispatching token %llu on cycle %llu, buffer full!(REG)\n",
              token->instructionNumber(), atCycle);
   if (Debug>1) 
         fprintf(stderr, "ROB: dispatching token %llu (%p) into slot %u\n", 
               token->instructionNumber(), token, availSlot);
      return false;
   }
   if (Debug>1) 
         fprintf(stderr, "ROB: dispatching token %llu (%p) into slot %u\n", 
               token->instructionNumber(), token, availSlot);
   
   tokenBuffer[availSlot] = token;
   availSlot = (availSlot + 1) % numSlots;
   numTokens++;
   return true;
}


bool ReorderBuffer::dispatchEmpty(CycleCount atCycle)
{
   if (numTokens >= numSlots) {
      fprintf(stderr, "ROB: Error dispatching EMPTY token on cycle %llu, buffer full (EMPTY)!\n", atCycle);
      return false;
   }
   if (Debug>1) fprintf(stderr, "ROB: dispatching EMPTY token into slot %u\n", availSlot);
   
   tokenBuffer[availSlot] = 0;
   availSlot = (availSlot + 1) % numSlots;
   numTokens++;
   return true;
}


/// @brief True if buffer is currently full
///
bool ReorderBuffer::isFull()
{
   if (numTokens >= numSlots)
      return true;
   else
      return false;
}

/// @brief Update: retire instructions in order, or cancel instructions
/// 
/// This retires instructions in order, only up to numPerCycle per cycle,
/// and if it hits a mispredicted branch instruction, it cancels all
/// instructions behind it. The counter 'retireSlot' is an index that is
/// always pointing to the next instruction in-order that should retire.
/// @param currentCycle is the current cycle count
///
// Waleed: Actually, the ROB is organized into 24 entries, each containing 3 lanes
// A lane is populated after each decode stage, if 3 MOPs are generated, then the 
// 3 lanes are occupied, if one MOP is generated, then only one spot in that lane is 
// occupied and ONLY that MOP need to retire when it reaches top of the ROB
// That is, each entry (containing 3 lanes/spots) is filled at the same cycle(decode) and
// should retire at the same cycle; not all the 3 lanes are occupied though.
// Therefore, I'll make some slight modifications to the code to make sure only 
// the instructions issued to the ROB at the same cycle can retire at the same cycle
// Something is still missing, which is that every entry(ie 3 lanes) can contain empty spots
// This should be done somewhere else and I'll get to it
int ReorderBuffer::updateStatus(CycleCount currentCycle)
{
   unsigned int i,ix, canRetire;
   CycleCount retireIssueCycle=0; 

   // return if the ROB has nothing in it?
   //if(!tokenBuffer[retireSlot])
   //   return 0; 

   // find the cycle at which the instruction at the top of the ROB was issued
   for (i=0; i < numPerCycle; i++) {
      ix = (retireSlot+i) % numSlots;
      if (tokenBuffer[ix]) { 
         retireIssueCycle = tokenBuffer[ix]->issuedAt(); 		    
         break; 
      } 
   }
   // top buffer entry has no valid tokens (maybe just empty)
   if(retireIssueCycle == 0) { 
      // don't show for fake buffer
      if(Debug>1)
      fprintf(stderr, "ROB: ROB Empty!\n");
      return 0;
   }			
	
   // first check to see if numPerCycle can be retired -- a whole
   // reorder buffer line can be retired together
   for (i=0; i < numPerCycle; i++) {
      ix = (retireSlot+i) % numSlots;
      if (tokenBuffer[ix] == 0) continue; // empty token
      if (!tokenBuffer[ix] || !tokenBuffer[ix]->isCompleted(currentCycle) 
          || tokenBuffer[ix]->issuedAt() != retireIssueCycle )
         break;
   }
   canRetire = i;
	//if(numPerCycle>1)
	//fprintf(stderr, "OKAY, canRetire = %d and numPercycle= %d \n", canRetire, numPerCycle);
   // there's nothing that can be retired now or not a whole line can retire
   if (canRetire == 0 || canRetire != numPerCycle)  
      return 0;

   // now retire the whole line, taking care if we encounter a
   // mispredicted branch
   for (i=0; i < canRetire; i++) {
      if (Debug>1 && tokenBuffer[retireSlot]!=0)
         fprintf(stderr, "ROB: retiring instruction %llu (%s) at cycle %llu in slot %u\n",
                           tokenBuffer[retireSlot]->instructionNumber(), 
                           tokenBuffer[retireSlot]->getType()->getName(), currentCycle, retireSlot);
		if(tokenBuffer[retireSlot] == 0) { // empty token
         numTokens--;
         retireSlot = (retireSlot+1) % numSlots;
      } else { 						
         if (tokenBuffer[retireSlot]->isMispredictedJump())
               break; // handle mispredicts outside of loop
         tokenBuffer[retireSlot]->retireInstruction(currentCycle);
         totalRetired++;
         tokenBuffer[retireSlot] = 0;
         numTokens--;
         retireSlot = (retireSlot+1) % numSlots;
      }
   }
   // if insn we stopped at is a mispredicted branch, then flush the buffer
   if (i < canRetire) {
      unsigned int nt = numTokens;
      for (i=0; i < nt; i++) {
         if (Debug>1 && tokenBuffer[retireSlot] !=0) 
            fprintf(stderr, "ROB: canceling instruction %llu in slot %u, numTokens now %u\n",
                           tokenBuffer[retireSlot]->instructionNumber(), retireSlot, numTokens);
         if(tokenBuffer[retireSlot]!=0) { 
            tokenBuffer[retireSlot]->cancelInstruction(currentCycle);
            totalAnulled++;
         } 
         tokenBuffer[retireSlot] = 0;
         numTokens--;
         retireSlot = (retireSlot+1) % numSlots;
         if (Debug>1) fprintf(stderr, "ROB: numTokens now %u and retireSlot %u \n",numTokens, retireSlot);

      }
      // Waleed: try to empty the whole buffer now
      for (unsigned int i=0; i < numSlots; i++) {
         if(tokenBuffer[i]) { 
           if (Debug>1) fprintf(stderr, "ROB: emptying left-over token after BR Mispredict, slot %u\n", i);
           tokenBuffer[i] = 0;
         }
         // reset indices
         retireSlot = 0;
         availSlot = 0;
      }

      return 1;
   } 
   else {
      return 0;
   }
}


// old function code && fake buffer code
int ReorderBuffer::updateStatusFake(CycleCount currentCycle)
{
   unsigned int i,ix;
  
   // first check to see if numPerCycle can be retired -- a whole
   // reorder buffer line must be retired together (we assume?)
   for (i=0; i < numPerCycle; i++) {
      ix = (retireSlot+i) % numSlots;
      if (!tokenBuffer[ix] || !tokenBuffer[ix]->isCompleted(currentCycle))
         break;
   }

   // if we can't retire a line, we're done, nothing to do
   if (i < numPerCycle) 
      return 0;
    
   // now retire the whole line, taking care if we encounter a
   for (i=0; i < numPerCycle; i++) {
      if (Debug>1) fprintf(stderr, "ROB: retiring FAKE instruction %llu (%p) in slot %u\n",
                           tokenBuffer[retireSlot]->instructionNumber(), 
                           tokenBuffer[retireSlot], retireSlot);
      tokenBuffer[retireSlot]->retireInstruction(currentCycle);
      tokenBuffer[retireSlot] = 0;
      totalRetired++;
      numTokens--;      
      retireSlot = (retireSlot+1) % numSlots;
   }

   return 0;
}

// cancel all entries, this should only be called by the fake buffer
int ReorderBuffer::cancelAllEntries(CycleCount currentCycle)
{
   for (unsigned int i=0; i < numSlots; i++) {
      if (Debug>1 && tokenBuffer[i]) 
         fprintf(stderr, "ROB: canceling FAKE instruction %llu in slot %u\n",
                        tokenBuffer[i]->instructionNumber(), i);
      if(tokenBuffer[i]) { 
         tokenBuffer[i]->cancelInstruction(currentCycle);
         totalAnulled++;
         numTokens--;
      } 
      tokenBuffer[i] = 0;
   }
   // reset indices
   retireSlot = 0;
   availSlot = 0;
	
   return 0;
}
}//End namespace McOpteron

