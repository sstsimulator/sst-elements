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


#include <stdio.h>
#include <stdlib.h>
#include <string>
using std::string;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include "InstructionQueue.h"
//#include "FunctionalUnit.h"
//#include "Token.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
/// @brief Constructor
///
InstructionQueue::InstructionQueue(QType Type, string Name,
                                   unsigned int Id, unsigned int Size, unsigned int AcceptRate)
{
   unsigned int i;
   type=Type;//this->type = type;
   name = Name;
   id=Id;//this->id = id;
   next=0;//this->next = 0;
   assignedThisCycle=0;//this->assignedThisCycle=0; 
   size=Size;//this->size = size;	
   //this->acceptRate = acceptRate;
   acceptRate=AcceptRate;	
   nextAvailableSlot=0;//this->nextAvailableSlot = 0; 
   lastAssignedCycle=0;//this->lastAssignedCycle = 0; 
   queuedInstructions = new Token*[size];
   for (i=0; i < size; i++)
      queuedInstructions[i] = 0;
   for (i=0; i < MAXFUNITS; i++)
      myUnits[i] = 0;
   totalInstructions = finishedInstructions = 0;
   numInstructions = 0;
   fullStalls = assignedStalls = 0;
   occupancyXCycles = totalCycles = 0;
}


/// @brief Destructor
///
InstructionQueue::~InstructionQueue()
{
 //fprintf(stdout, "IQ%d %s: total   instructions: %llu\n", id, name, totalInstructions);
   cout<<"IQ"<<id<<" "<<name<<": total instructions: "<<totalInstructions<<endl;
 //fprintf(stdout, "IQ%d %s: finished instructions: %llu\n", id, name, finishedInstructions);
   cout<<"IQ"<<id<<" "<<name<<": finished instructions: "<<finishedInstructions<<endl;
 //fprintf(stdout, "IQ%d %s: average occupancy: %g\n", id, name,(double) occupancyXCycles / totalCycles);
   cout<<"IQ"<<id<<" "<<name<<": average occupancy: "<<(double(occupancyXCycles)/totalCycles)<<endl;
 //fprintf(stdout, "IQ%d %s: full     stalls: %llu\n", id, name, fullStalls);
   cout<<"IQ"<<id<<" "<<name<<": full stalls: "<<fullStalls<<endl;
 //fprintf(stdout, "IQ%d %s: assigned stalls: %llu\n", id, name, assignedStalls);
   cout<<"IQ"<<id<<" "<<name<<": assigned stalls: "<<assignedStalls<<endl;
   delete[] queuedInstructions;
}


/// @brief Create list link
///
void InstructionQueue::setNext(InstructionQueue *other)
{
   next = other;
}


/// @brief Retrieve list link
///
InstructionQueue* InstructionQueue::getNext()
{
   return next;
}


/// @brief Attach a functional unit to this queue
///
/// Functional units should only belong to one queue; this
/// attaches a unit to the queue it is called on.
///
int InstructionQueue::addFunctionalUnit(FunctionalUnit *fu)
{
   int i;
   for (i=0; i < MAXFUNITS; i++)
      if (myUnits[i] == 0)
         break;
   if (i >= MAXFUNITS) {
      //fprintf(stderr, "IQ%d %s: Error, can't handle any more FUs!\n", id, name);
      cerr<<"IQ"<<id<<" "<<name<<": Error, can't handle any more FUs!"<<endl;
      return -1;
   }
   myUnits[i] = fu;
   if (Debug) cerr<<"IQ"<<id<<" "<<name<<": added functional unit at "<<i<<endl;
     //fprintf(stderr, "IQ%d %s: added functional unit at %d\n", id, name, i);
   return 0;
}


/// @brief Schedule instructions onto functional units
///
/// This is the main functionality of a queue; it iterates 
/// through the instructions in the queue and assigns them to
/// functional units if they are available and can be used.
///
int InstructionQueue::scheduleInstructions(CycleCount currentCycle)
{
   unsigned int f, i;
   FunctionalUnit *fu;
   Token *t; 
   static unsigned int stuckCount = 0;
   //
   occupancyXCycles += numInstructions;
   totalCycles++;
   // LOTS of stuff here!!
   if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": scheduling instructions"<<endl;
     //fprintf(stderr, "IQ%d %s: scheduling instructions\n", id, name);
   for (f=0; f < MAXFUNITS; f++) {
      if (myUnits[f] == 0)
         continue; // no unit here, so skip index
      if (!myUnits[f]->isAvailable(currentCycle))
         continue; // unit is still busy, so skip
      // now we have an available unit, lets try to 
      // find an instruction that can use it
      fu = myUnits[f];
      for (i=0; i < size; i++) {
         t = queuedInstructions[i];
         if (!t) // empty slot
            continue;
         // debugging check to see if we are leaving tokens stranded
         if (currentCycle > 3000 && t->issuedAt() < currentCycle - 3000) {
            cerr<<"IQ"<<id<<" "<<name<<": Token likely stuck!"<<endl;
            //fprintf(stderr, "IQ%d %s: Token likely stuck!\n", id, name);
            t->dumpDebugInfo();
            t->getType()->dumpDebugInfo();
            t->cancelInstruction(currentCycle); // JEC: not the right fix!
            stuckCount++;
            exit(0);
            if (stuckCount >= 20) exit(0);
         }
         if (t->isExecuting(currentCycle))
            // skip already executing token
            continue;
         if (t->wasRetired() || t->wasCanceled()) {
            if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": retiring "<<t->instructionNumber()<<" "<<t<<endl; 
              //fprintf(stderr, "IQ%d %s: retiring %llu %p\n", id, name, t->instructionNumber(), t);
            queuedInstructions[i] = 0;  // forget pointer
            numInstructions--;          // remove from queue count
            finishedInstructions++;     // add to retired count
            delete t;
            continue;
            // TODO: only allow retire of one insn per cycle???
         }
         if (t->isCompleted(currentCycle))
            // insn is just waiting to be retired
            continue;
         // first check to see if insn needs to use an AGU for address
         // generation (what about FP insns that need addressing?)
         if (t->aguOperandsReady(currentCycle) && t->needsAddressGeneration() && 
             ((fu->getType() == AGU && fu->isAvailable(currentCycle)))) {
            //  || type == FLOAT)) 
            // assign AGU to token
            // token must keep place in queue since it will do another
            // op too (probably)
            if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": schedule "<<t->instructionNumber()<<" ("<<t->getType()->getName()<<")"<<endl; 
              //fprintf(stderr, "IQ%d %s: schedule %llu (%s) to AGU\n", id, name, t->instructionNumber(), t->getType()->getName());
            // JEC: try just not occupying anything for FLOAT queue address gen
            // TODO: Of course we need to occupy an AGU, but we were occupying the
            // ADDER and this is bad; 
            // if (fu->getType() == AGU)
               fu->occupy(currentCycle, AGU_LATENCY);
            t->executionStart(currentCycle);
            continue;
         } 
         // now check to see if insn can use current FU and if all is ready
         else if (t->allOperandsReady(currentCycle) && 
                  t->needsFunctionalUnit(fu) && fu->isAvailable(currentCycle)) {
            // assign other FU to token
            if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": schedule "<<t->instructionNumber()<<" ("<<t->getType()->getName()<<") to other"<<endl;
              //fprintf(stderr, "IQ%d %s: schedule %llu (%s) to other\n", id, name, t->instructionNumber(), t->getType()->getName());
            // occupy functional unit; we use the throughput denominator as the amount
            // of time to occupy the unit. Even though the instruction will take longer
            // to finish, the functional units are pipelined and the throughput determines
            // how fast instructions can issue, which our unit occuptation is simulating
            fu->occupy(currentCycle, t->getType()->throughput());
            t->executionStart(currentCycle);
            continue;
         }
         // TODO: check to see if insn needs to fire off a memory load
         // and then let it wait for load
      }
   }
   // repack queue so older insns stay at top (taking into account reserved spots for MOPS)
   unsigned slotIndex = 0; 
   for (f=0; f <size; ) {
      if(queuedInstructions[f]) { // move it 
         unsigned mops = queuedInstructions[f]->getType()->getMops();
         for (i=0; i<mops; i++) {
            slotIndex +=i;
            if(slotIndex != (f+i)) { 
               // move
               queuedInstructions[slotIndex] = queuedInstructions[f+i]; 
               // clear old spot
               queuedInstructions[f+i] = 0;
           }
         }
         slotIndex++; // point to the next might-be-empty slot
         f+=mops;
      }
      else { 
         f++;
      }
   }
   nextAvailableSlot = (slotIndex<size)?slotIndex:size;
					
   //fprintf(stderr, "\tAfter Schedule: nextAvailableSlot:%u and availableSlots:%u\n",nextAvailableSlot, (size-nextAvailableSlot));
 
   return 0;
}


/// @brief Check if instruction can be assigned to this queue
///
bool InstructionQueue::canHandleInstruction(Token *token, CycleCount atCycle)
{
   if (numInstructions >= size ) {// full queue
        if(Debug>=5) cerr<<"IQ: full queue >= size"<<endl;
	return false; 
   }
   if ((size-nextAvailableSlot) < token->getType()->getMops()){ // full queue
      if(Debug>=5) cerr<<"IQ: full queue > size-nextAvailableSlot"<<endl;
      return false;
   } 
   // easy int instructions can use any int queue
   if(Debug>=5) cerr<<"IQ: token=getType()->getCategory()="<<token->getType()->getCategory()<<endl;
   if ((type == INT || type == INTMUL || type == INTSP) &&
       token->getType()->getCategory() == GENERICINT)
       return true;
   // int multiplies need the intmul queue
   else if (type == INTMUL && token->getType()->getCategory() == MULTINT)
       return true;
   // int multiplies need the intmul queue
   else if (type == INTSP && token->getType()->getCategory() == SPECIALINT)
       return true;
   // float instructions need the float queue
   else if (type == FLOAT && token->getType()->getCategory() == McOpteron::FLOAT)
       return true;
   else{
      if(Debug>=5)cerr<<"IQ: No Type Match"<<endl;
      return false;
   }
}


/// @brief Check if instruction was already assigned this cycle
///
/// Only one acceptRate instructions can be placed on a queue per cycle, so
/// if it already happened on this queue we need to block others.
///
bool InstructionQueue::alreadyAssigned(CycleCount currentCycle)
{
   if (lastAssignedCycle == currentCycle && assignedThisCycle >= acceptRate )
      return true;
   else
      return false;
}


/// @brief Assign an instruction to this queue
///
/// This assumes canHandleInstruction() has already been called and
/// it has been verified that the instruction can be placed on this queue
///
int InstructionQueue::assignInstruction(Token *token, CycleCount atCycle)
{
   unsigned int i;
   unsigned int mops = token->getType()->getMops();
   // quick sanity check
   if ((size-nextAvailableSlot)< mops) {
      //fprintf(stderr, "IQ%d %s: Error assigning, queue does not enough space!\n", id, name);
      cerr<<"IQ"<<id<<" "<<name<<": Error assigning, queue does not enough space!"<<endl;
      return -1; 
   }

   // find empty slot(s) for this instruction 
   for (i=nextAvailableSlot; i <size; ) {
      bool slotsEmpty = true; 
      // check if all slots in one line are empty
      for(unsigned s=0; s<mops; s++)
         slotsEmpty = slotsEmpty && !queuedInstructions[i+s];   
      if (slotsEmpty)
         break;
      // update counter
      if(queuedInstructions[i])
         i+=queuedInstructions[i]->getType()->getMops();
      else
        i++;
    }

   // sanity checks
   if (i >= size || (lastAssignedCycle == atCycle && assignedThisCycle >= acceptRate) ) {
      //fprintf(stderr, "IQ%d %s: Error assigning, queue full or not accepting new assignments this cylce!\n", id, name);
      cerr<<"IQ"<<id<<" "<<name<<": Error assigning, queue full or not accepting new assignments this cylce!"<<endl;
      return -1; 
   }
   queuedInstructions[i] = token;
   numInstructions++;
   totalInstructions++;
   if(lastAssignedCycle == atCycle)
      assignedThisCycle+=mops;
   else		
      assignedThisCycle=mops;
	lastAssignedCycle = atCycle;
   nextAvailableSlot = i+mops; 
   if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": assign insn at "<<i<<" total "<<numInstructions<<endl;
     //fprintf(stderr, "IQ%d %s: assign insn at %u total %u\n", id, name, i, numInstructions);
   if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": nextAvailable Slot:"<<nextAvailableSlot<<" availabe slots:"<<(size-nextAvailableSlot)<<endl;
     //fprintf(stderr, "IQ%d %s: nextAvailable Slot:%u availabe slots:%u\n", id, name, nextAvailableSlot, size-nextAvailableSlot);

   return 0;
}


/// @brief Return average occupancy of this queue
///
double InstructionQueue::averageOccupancy(CycleCount cycles)
{
   return (double) totalInstructions / cycles;
}

/// @brief Flush instructions from isntruction queues 
///
///
int InstructionQueue::FlushAllInstructions(CycleCount currentCycle)
{
   unsigned int f, i;
   Token *t; 

   if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": flusing instructions"<<endl;
     //fprintf(stderr, "IQ%d %s: flusing instructions\n", id, name);
   for (f=0; f < MAXFUNITS; f++) {
      if (myUnits[f] == 0)
         continue; // no unit here, so skip index
      for (i=0; i < size; i++) {
         t = queuedInstructions[i];
         if (!t) // empty slot
            continue;
         else {
            if (Debug>1) cerr<<"IQ"<<id<<" "<<name<<": flushing/retiring "<<t->instructionNumber()<<" "<<t<<endl;
              //fprintf(stderr, "IQ%d %s: flushing/retiring %llu %p\n", id, name, t->instructionNumber(), t);
            queuedInstructions[i] = 0;  // forget pointer
            numInstructions--;          // remove from queue count
            finishedInstructions++;     // add to retired count
            delete t;
         }
      }
   }
   return 0;
}
}//end namespace McOpteron

