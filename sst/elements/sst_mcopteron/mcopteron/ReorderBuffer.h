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


#ifndef REORDERBUFFER_H
#define REORDERBUFFER_H

#include "OpteronDefs.h"
#include "Token.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library

//class Token;

/// @brief Simulated instruction re-order buffer
///
/// This class is responsible for retiring instructions in order
/// and for canceling all outstanding instructions in case of a
/// branch mispredict. It was expected to be a singleton class, but
/// we are currently instantiating a second object which acts as
/// a "fake" retirement buffer for fake LEA instructions that are
/// created for FP instructions that have a memop on them. This way
/// the fake LEAs are not counted, but are retired and cleaned up
/// properly (ooh, except for mispredicted branches! -- does it matter?)
///
class ReorderBuffer
{
 public:
   ReorderBuffer(unsigned int numSlots, unsigned int numRetireablePerCycle);
   ~ReorderBuffer();
   bool dispatch(Token *token, CycleCount atCycle); 
   bool dispatchEmpty(CycleCount atCycle);
   bool isFull();
   void incFullStall() {fullStalls++;}
   int updateStatus(CycleCount currentCycle); // returns 1 if canceled insns
   int updateStatusFake(CycleCount currentCycle); // only for fake buffer
   int cancelAllEntries(CycleCount currentCycle); // only for fake buffer
 private:
   Token **tokenBuffer; ///< circular queue of Token pointers of size numSlots
   unsigned int numSlots; ///< number of instructions the buffer can hold
   unsigned int numTokens; ///< number of instructions currently in buffer
   unsigned int numPerCycle; ///< number of instructions accepted and retired per cycle
   unsigned int availSlot; ///< index of next available slot (if not full);
   unsigned int retireSlot; ///< index of next retireable token
   unsigned long long totalRetired; ///< statistic: total retired instructions
   unsigned long long totalAnulled; ///< statistic: total canceled instructions
   unsigned long long fullStalls; ///< statistic: total stalls due to full buffer
};
}//End namespace McOpteron
#endif
