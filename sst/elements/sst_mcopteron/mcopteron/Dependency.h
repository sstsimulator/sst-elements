
#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include <stdio.h>

#include "OpteronDefs.h"

#include "Token.h"
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
class Token;

/// @brief Dependency record for a token
///
class Dependency{
 public:
   Dependency(Token *token, unsigned int *dist);
   ~Dependency();
   static Dependency *dependenciesHead;
   static Dependency *dependenciesTail;
   Token *tkn;				// token to which this record belongs
	unsigned int numInputs; // # of input registers for token
   unsigned int numReady;  // # of input registers that have become ready
   InstructionCount *producers; // instructionCount of instructions that produce each input register
   Dependency *next; // used to create a linked-list
};

}//End namespace McOpteron
#endif
