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


#ifndef FUNCTIONALUNIT_H
#define FUNCTIONALUNIT_H

#include "OpteronDefs.h"

namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
//-------------------------------------------------------------------
/// @brief Represents a functional unit in the CPU
///
/// This keeps track of when a functional unit is occupied, and
/// lets an instruction occupy it for a length of time. One object
/// of this class will represent one functional unit.
//
// Basic idea: simulator will inspect instruction to see which FUs
// it needs or can use, then will iterate functional units and for
// each type that matches, it will check occupiedUntil() to find out
// when it is available; if none are available then will hold that
// instruction and try another (out of order); if no current 
// instructions can be satisfied, then stall. When an instruction 
// can be executed, sim will call occupy() on corresponding FUs to
// occupy them for as long as needed.
//
// At end, FU will be able to report its duty cycle (cycles used vs.
// idle)
//-------------------------------------------------------------------
class FunctionalUnit
{
 public:
   FunctionalUnit(FunctionalUnitTypes type, const char *name, unsigned int id);
   ~FunctionalUnit();
   void setNext(FunctionalUnit *other);
   FunctionalUnit* getNext();
   FunctionalUnitTypes getType();
   CycleCount occupiedUntil(CycleCount atCycle);
   int occupy(CycleCount atCycle, CycleCount numCycles);
   int updateStatus(CycleCount currentCycle);
   void flush(CycleCount atCycle);
   bool isAvailable(CycleCount atCycle);
   double dutyCycle();
 private:
   const char *name;              ///< Name of this unit
   FunctionalUnitTypes type;      ///< Type of this unit
   unsigned int id;               ///< Unique unit ID
   CycleCount occupiedUntilCycle; ///< Cycle that current insn is occupying this until
   bool occupied;                 ///< True if currently occupied
   CycleCount numOccupiedCycles;  ///< Total # of occupied cycles
   CycleCount numFreeCycles;      ///< Total # of free cycles
   CycleCount latestCycle;        ///< Latest known cycle
   class FunctionalUnit *next;    ///< Linked list ptr
};

}//end namespace McOpteron
#endif
