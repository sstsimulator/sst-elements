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

 
//#include <malloc.h>
#include <memory.h>

#include "Dependency.h"
//#include "Token.h"


namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
Dependency * Dependency::dependenciesHead = 0;
Dependency * Dependency::dependenciesTail = 0;

/// @brief Constructor
///
Dependency::Dependency(Token *t, unsigned int *distances)
{
   InstructionCount insn = t->instructionNumber();
   unsigned int sourceOps = t->getType()->getSourceOps();
   if (Debug>=4) fprintf(stderr,"creating dependency for token %llu with %u source regs\n", insn, sourceOps); 
	tkn = t;
	numInputs = sourceOps;
   numReady = 0;
   producers = new InstructionCount[sourceOps];
   for(unsigned int i=0; i<sourceOps; i++) { // get distance for each op
      unsigned int distance;
      if(distances)
         distance = distances[i];
      else
         distance = t->getType()->getUseDistance(genRandomProbability(), i); 
      if (Debug>=4) fprintf(stderr,"Distance for source %u is %u\n", i, distance); 
      if(distance>=insn || distance==0) {
         producers[i] = 0;
         numReady++;
         if (Debug>=4) fprintf(stderr,"Producer is too far in the past\n"); 
      }
      else
         producers[i]= insn-distance; 
   }
   next = 0;
   if (dependenciesHead) {
      dependenciesTail->next = this;
      dependenciesTail = this;
   } else {
      dependenciesHead = dependenciesTail = this;
   }
   if (Debug>=4) { 
		fprintf(stderr,"dependency created for token: %llu numInputs: %u numReady: %u\n", 
				  insn, this->numInputs, this->numReady ); 
		for(unsigned i=0; i<sourceOps; i++)
			fprintf(stderr,"Input: %u Producer: %llu\n", i, this->producers[i]); 
	}	
	
}

/// @brief: Destructor
///
Dependency::~Dependency()
{
   tkn->setInDependency(NULL);	
   delete producers; 
   memset(this,0,sizeof(Dependency));
}

}//End namespace McOpteron

