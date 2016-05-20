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

#include "LoadStoreUnit.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor: create slots and initialize as empty
/// @param numSlots is the total number of instruction slots
/// @param maxOpsPerCycle is the total number of memory ops allowed per cycle
/// @param memModel is a ptr to the memory model object, used to perform memops
///
LoadStoreUnit::LoadStoreUnit(unsigned int numberSlots, unsigned int maxOpsPerCycle,
                             MemoryModel *memModel)
{
   unsigned int i;
   //this->numSlots = numSlots;
   numSlots=numberSlots;
   slots = new LSSlot[numSlots];
   numFilled = 0;
   fullStalls = 0;
   for (i=0; i < numSlots; i++) {
      slots[i].type = EMPTY;
      slots[i].token = 0;
      slots[i].satisfiedCycle = 0;
   }
   memoryModel = memModel;
   maxMemOpsPerCycle = maxOpsPerCycle;
}

/// @brief Destructor: delete slot array
///
LoadStoreUnit::~LoadStoreUnit()
{
   fprintf(stdout, "LSQ: stalls from full: %llu\n", fullStalls);
   delete[] slots;
}

void LoadStoreUnit::notify(void *obj)
{
   unsigned int i;
   Token *t = (Token *) obj;
   for (i=0; i < numSlots; i++) {
      if (slots[i].token == t) break;
   }
   if (i < numSlots) {
      if (slots[i].type == LOAD)
         slots[i].token->cancelInstruction(0);
      // 128-bit stores take up two slots
      if(slots[i].type == STORE && slots[i].token->getType()->getMaxOpSize() == 128 )
         numFilled-=2;
      else 
         numFilled--;
      // clear slot
      slots[i].type = EMPTY;
      slots[i].token = 0;
      slots[i].satisfiedCycle = 0;
   }
}

/// @brief Add a load or store token to the LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return 0 if instruction could not be added, 1 on success
///
unsigned int LoadStoreUnit::add(Token *token, CycleCount atCycle)
{
   if (!token->isLoad() && !token->isStore()) {
      // shouldn't be here since this token doesn't do a memop, 
      // but fake success anyways
      fprintf(stderr, "LSQ: trying to add a non memop!\n");
      return 1;
   }
   if (token->isLoad() && token->isStore()) {
      // 128-bit stores need two spots in the queue
      if(token->getType()->getMaxOpSize() == 128 &&  numFilled > numSlots-3) { 
         fullStalls++;
         return 0;
      }
      if (numFilled > numSlots-2) {
         // need at least two slots for a load and store, so fail
         fullStalls++;
         return 0;
      }
   }
   if(token->isStore() && token->getType()->getMaxOpSize() == 128 &&  numFilled > numSlots-2) { 
      fullStalls++;
      return 0;
   }
   if (numFilled > numSlots-1) {
      // need at least one slot for a load or store, so fail
      fullStalls++;
      return 0;
   }
   if (token->isLoad())
      addLoad(token, atCycle);
   if (token->isStore())
      addStore(token, atCycle);
   token->addListener(this);
   return 1;
}


/// @brief Internal add-load token to LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return the slot ID (starting at 1), not used anywhere
///
/// Waleed: this should ONLY be called after add() to check numFilled
unsigned int LoadStoreUnit::addLoad(Token *token, CycleCount atCycle)
{
   unsigned int i;
   for (i=0; i < numSlots; i++ ) { 
      if (slots[i].token == 0) 
         break;
   }
   if (i >= numSlots) {
      fprintf(stderr, "LSQ: Error trying to add load: queue full!\n");
      return 0;
   }
   slots[i].token = token;
   slots[i].type = LOAD;
   slots[i].startCycle = atCycle;
   slots[i].satisfiedCycle = 0;
   numFilled++;
   if (Debug>1)
      fprintf(stderr, "LSQ: load token %llu added at %llu\n",
                    slots[i].token->instructionNumber(), slots[i].startCycle);
   // TODO: check if a load is satisfied by a store in LSQ
   return i+1; // avoid ID of 0
}

/// @brief Internal add-store token to LSQ
/// @param token is the instruction token to add
/// @param atCycle is the cycle count at which it is being added
/// @return the slot ID (starting at 1), not used anywhere
///
unsigned int LoadStoreUnit::addStore(Token *token, CycleCount atCycle)
{
   unsigned int i;
   for (i=0; i < numSlots; i++ ) {
      if (slots[i].token == 0) 
         break;      
   }
   if (i >= numSlots) {
      fprintf(stderr, "LSQ: Error trying to add store: queue full!\n");
      return 0;
   }
   slots[i].token = token;
   slots[i].type = STORE;
   slots[i].startCycle = atCycle;
   slots[i].satisfiedCycle = 0;
   // reserve two spots for 128-bit stores
   if(slots[i].token->getType()->getMaxOpSize() == 128) 
      numFilled+=2;
   else
      numFilled++;
   if (Debug>1)
      fprintf(stderr, "LSQ: store token %llu added at %llu\n",
                    slots[i].token->instructionNumber(), slots[i].startCycle);
   return i+1; // avoid ID of 0
}


/// @brief Update LSQ status (called each cycle)
///
/// This does two things: 1) cycles through the LSQ and
/// for any memop that has an address newly ready it
/// asks the memory model to serve that memop (just
/// calculate cycles to serve); and 2) it purges memops
/// that have finished; stores just go away quietly but
/// loads have a token callback that indicates the load
/// is satisfied (so that the instruction can continue).
///
/// @param currentCycle is the active cycle 
///
void LoadStoreUnit::updateStatus(CycleCount currentCycle)
{
   unsigned int i;
   unsigned int numOps = 0;
// Waleed: loop thru the queue 2 times
// 1st time: update by removing finished/canceled slots and notify  satisfied loads/stores
// 2nd time: schedule loads out of order if they are ready
// 2nd time: schedule stores in-order if they are ready 
// 2nd time: only upto maxMemOpsPerCycle ops can be served per cycle
// 2nd tiem: 128-bit stores are considered two ops
// finally: repack the queue so that older tokens stay atop (this helps serve stores in order)

   // first loop
   for (i=0; i < numSlots; i++) {
      if (slots[i].type == EMPTY )
         continue;
      if (slots[i].token && slots[i].token->wasCanceled()) {
         // release slots
         if(slots[i].token->getType()->getMaxOpSize() == 128 &&  slots[i].type==STORE)
            numFilled-=2;
         else
            numFilled--;
         // throw instruction away
         slots[i].type   = EMPTY;
         slots[i].token  = 0;
         continue;
      }
      if (slots[i].token)
          // allow token to update status
         slots[i].token->isExecuting(currentCycle);
      
		
		// following condition is: this memop has just been satisfied (it set its
      // satisfied cycle previously and the current cycle is now >= it's sat-cycle)
      if (slots[i].satisfiedCycle > 0 && slots[i].satisfiedCycle <= currentCycle) {
         // if token is a load, then notify it with the callback
         if (slots[i].type == LOAD)
            slots[i].token->loadSatisfiedAt(currentCycle);
         if(slots[i].token && slots[i].token->getType()->getMaxOpSize() == 128 &&  slots[i].type == STORE)
            numFilled-=2;
         else
            numFilled--;
         // we don't report stores since they might be long gone and token deleted
         // Waleed: actually this ^ should not happen since we assume stores to be satisfied immediately
         // Waleed: there's no way the store token will be long gone the cycle after it's been issued
         slots[i].type   = EMPTY; // clear this record
         slots[i].token  = 0; // clear this record
      }
   }

#if 0
   // 2nd loop : serve loads out of order (only look at 12  loads)
   unsigned counter = 0; 
	for (i=0; i < numSlots; i++) {
      if (slots[i].type != LOAD)
         continue;
      if(counter >= 12)
        break;
      counter++; 		
      if (Debug>2) 
         fprintf(stderr,"LSQ slot %d: token %llu satCyc %llu addrRdy %u\n",
                 i, slots[i].token->instructionNumber(), slots[i].satisfiedCycle,
                 slots[i].token->addressIsReady());
      // following condition is: if this token hasn't yet generated a satisfy
      // cycle (it is still 0) and this token's address is now read and we 
      // haven't already performed the maximum memory ops this cycle, then serve it
      if (slots[i].token && slots[i].satisfiedCycle == 0 && 
          slots[i].token->addressIsReady() && numOps < maxMemOpsPerCycle) {
           
         slots[i].satisfiedCycle = memoryModel->serveLoad(currentCycle,0,0);
         if (Debug>1)
            fprintf(stderr, "LSQ: token %llu will be satisfied at %llu\n",
                    slots[i].token?slots[i].token->instructionNumber():0, 
                    slots[i].satisfiedCycle);
         numOps++;
      }
   }
#endif

   // this variable tells us if we can serve anymore stores
   bool canServeStores = true; 

   // 2nd loop : serve loads out-of-order and stores in-order
	for (i=0; i < numSlots && numOps < maxMemOpsPerCycle ; i++) {
      if (slots[i].type != STORE && slots[i].type != LOAD)
         continue;
      if (Debug>2) 
         fprintf(stderr,"LSQ slot %d: token %llu satCyc %llu addrRdy %u\n",
                 i, slots[i].token->instructionNumber(), slots[i].satisfiedCycle,
                 slots[i].token->addressIsReady());
      // following condition is: if this token hasn't yet generated a satisfy
      // cycle (it is still 0) and we 
      // haven't already performed the maximum memory ops this cycle, then serve it
      // if the address is not ready for a store, don't serve anymore stores
      if (slots[i].token && slots[i].type == STORE && slots[i].satisfiedCycle == 0 && canServeStores) {
         if( slots[i].token->addressIsReady() ) { 
            // 128-bit stores are processed in two ops
            if(slots[i].token->getType()->getMaxOpSize() == 128) { 
               if((maxMemOpsPerCycle-numOps)>=2) {
                  slots[i].satisfiedCycle = memoryModel->serveStore(currentCycle,0,0);
                  // store latency does not cause delay in the CPU, so assume they're always done immediately
                  slots[i].satisfiedCycle = 1; // should never happen
                  //slots[i].token = 0; // make sure we don't access store token again
                  numOps +=2; 
                } 
                else
                   canServeStores = false;
             }
            else { // other stores
               slots[i].satisfiedCycle = memoryModel->serveStore(currentCycle,0,0);
               // store latency does not cause delay in the CPU, so assume they're always done immediately
               slots[i].satisfiedCycle = 1; // should never happen
               //slots[i].token = 0; // make sure we don't access store token again
               numOps ++; 
            }
         if (Debug>1)
            fprintf(stderr, "LSQ: token %llu will be satisfied at %llu\n",
                    slots[i].token?slots[i].token->instructionNumber():0, 
                    slots[i].satisfiedCycle);
         }
         else // address not ready yet
            canServeStores = false; 
      }
      // following condition is: if this token hasn't yet generated a satisfy
      // cycle (it is still 0) and this token's address is now read and we 
      // haven't already performed the maximum memory ops this cycle, then serve it
      if (slots[i].token && slots[i].type == LOAD && slots[i].satisfiedCycle == 0 && 
          slots[i].token->addressIsReady() && numOps < maxMemOpsPerCycle) {
      
         slots[i].satisfiedCycle = memoryModel->serveLoad(currentCycle,0,0);
         if (Debug>1)
            fprintf(stderr, "LSQ: token %llu will be satisfied at %llu\n",
                    slots[i].token?slots[i].token->instructionNumber():0, 
                    slots[i].satisfiedCycle);
         numOps++;
      }
   }
   //fprintf(stderr, "before final\n");
   // finally, repack the queue so that older tokens stay atop
   for (i=0; i < numSlots; i++) {
      // vacant spot?
      if (slots[i].token == 0) { 
         // find the next non-empty slot and move it here
         for(unsigned int n=i+1; n<numSlots; n++) { 
			    if(slots[n].token !=0) { 
                // move it
                slots[i].token = slots[n].token;
                slots[i].type = slots[n].type;
                slots[i].satisfiedCycle = slots[n].satisfiedCycle;
                // now clear old spot
                slots[n].token = 0;
                slots[n].type = EMPTY;
                slots[n].satisfiedCycle = 0;
                // now break
                break;
             }
         }   
      } 
    }

}


/// @brief Flush LSQ (called after branch misprediction)
///
/// @param currentCycle is the active cycle 
///
void LoadStoreUnit::flush(CycleCount currentCycle)
{
   unsigned int i;
   numFilled = 0;
   for (i=0; i < numSlots; i++) {
      slots[i].type = EMPTY;
      slots[i].token = 0;
      slots[i].satisfiedCycle = 0;
   }

}
}//end namespace McOpteron
