
#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include <stdio.h>

#include "OpteronDefs.h"
#include "Random.h"

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
  void setRandFunc(RandomFunc_t _R){rand.setrand_function(_R);}
  void setSeedFunc(SeedFunc_t _S){rand.setseed_function(_S);}
  void seedRandom(uint32_t _s){rand.seed(_s);}
 private:
  double genRandomProbability(){return rand.next();}
  Random rand;
};

}//End namespace McOpteron
#endif
