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


#ifndef INSTRUCTIONQUEUE_H
#define INSTRUCTIONQUEUE_H
#include <string>
using std::string;
#include "OpteronDefs.h"
#include "FunctionalUnit.h"
#include "Token.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
//class FunctionalUnit;
//class Token;

#define MAXFUNITS 5

//-------------------------------------------------------------------
/// @brief Represents an instruction queue in the CPU
///
/// This class implements the instruction queue concept. One
/// object represents one queue. Each queue has some functional
/// units that it supervises. This class really is the heart of
/// simulating the execution of an instruction, since it assigns
/// instructions to functional units and retires them when they 
/// are done.
//-------------------------------------------------------------------
class InstructionQueue
{
 public:
   enum QType {UNKNOWN,INT, INTMUL, INTSP, FLOAT};
   InstructionQueue(QType type, string name, unsigned int id, unsigned int size, unsigned int acceptRate);
   ~InstructionQueue();
   void setNext(InstructionQueue *other);
   InstructionQueue* getNext();
   int addFunctionalUnit(FunctionalUnit *fu);
   int scheduleInstructions(CycleCount currentCycle);
   bool canHandleInstruction(Token *token, CycleCount atCycle);
   int assignInstruction(Token *token, CycleCount atCycle);
   bool isFull() {return (numInstructions >= size);};
   bool isEmpty() {return (numInstructions == 0);};
   void incFullStall() {fullStalls++;}
   void incAlreadyAssignedStall() {assignedStalls++;}
   bool alreadyAssigned(CycleCount currentCycle);
   double averageOccupancy(CycleCount cycles);
   int FlushAllInstructions(CycleCount currentCycle);
   unsigned int getAvailSlots() { return (size - numInstructions); } 
   unsigned int getAcceptRate() { return acceptRate; } 
   string getName() { return name.c_str(); }
	
 private:
   string name;         ///< Queue name
   QType type;         ///< Queue type
   unsigned int id;    ///< Unique queue ID
   unsigned int size;  ///< Number of entries in queue
   unsigned int acceptRate;  ///< Number of token that can be issued to this queue per cycle
   unsigned int assignedThisCycle;  ///< Number of tokens assigned to this queue at current cycle
   FunctionalUnit* myUnits[MAXFUNITS];  ///< Functional units this queue manages
   Token **queuedInstructions;    ///< Instructions in queue (of size 'size')
   InstructionCount totalInstructions;   ///< Total instructions this queue processed
   InstructionCount finishedInstructions; ///< Total instructions completed from queue so far
   unsigned long long fullStalls, assignedStalls;
   unsigned long long occupancyXCycles, totalCycles;
   unsigned int numInstructions;  ///< Number of instructions ???
   unsigned int nextAvailableSlot; 
   CycleCount lastAssignedCycle;  ///< Cycle that last instruction assignment occurred
   class InstructionQueue *next;  ///< Linked list ptr
};
}//end namespace McOpteron
#endif
