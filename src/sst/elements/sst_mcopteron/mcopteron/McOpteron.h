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


#ifndef MCOPTERON_H
#define MCOPTERON_H

//EXTERN int TraceTokens; //Scoggin: Moved inside class

#include "Dependency.h"
#include "Token.h"
#include "FunctionalUnit.h"
#include "InstructionQueue.h"
#include "LoadStoreUnit.h"
#include "MemoryModel.h"
#include "ReorderBuffer.h"
#include "ConfigVars.h"
#include "MarkovModel.h"

//class FunctionalUnit;
//class InstructionQueue;
//class LoadStoreUnit;
//class MemoryModel;
//class ReorderBuffer;
//class ConfigVars;
//class MarkovModel;

namespace McOpteron{		//Scoggin: Added a namespace to reduce possible conflicts as library 
///-------------------------------------------------------------------
/// @brief Main Monte Carlo Opteron Simulation Class
///
/// This class drives the entire simulation. It instantiates the CPU model,
/// reads instruction definition and mix information from files, and then 
/// runs the simulation cycle by cycle. The simulation is generally composed
/// of working backwards up the architectural pipeline each cycle, from
/// retiring instructions out of the reorder buffer and load-store queue,
/// to updating the progress of the functional units, to allowing the 
/// reservation queues to send new instructions to the functional units,
/// to fetching and dispatching new instructions to the reservation queues.
/// Instruction fetch can also be done from a trace file. 
///
/// Each instruction is represented by a token object. This object has a 
/// pointer to an InstructionInfo record that holds the data about the type
/// of instruction it is. The token moves to the reservation queues, and
/// the reorder buffer and possibly the load-store queue also hold a reference
/// to it. Once the token recognizes it is completed, the reorder buffer will
/// tell it it is retired, and then the reservation queue can delete it. Token
/// objects are always deleted by the reservation queue that holds it.
///-------------------------------------------------------------------
class McOpteron
{
 public:
   McOpteron();
   ~McOpteron();
   int readConfigFile(string filename);
   //int init(const char* definitionFilename, const char* mixFilename, 
   //         const char *traceFilename=0, const char* newIMixFilename=0);
   int init(string appDirectory, string definitionFilename, 
            string mixFilename, string traceFilename, bool repeatTrace,
            string newIMixFilename, string instrSizeFile, string fetchSizeFile, 
				string transFile);
   int finish(bool printInstMix);
   int simCycle();
   CycleCount currentCycles();
   double currentCPI();
   void printStaticIMix();
   bool treatImmAsNone;	//Scoggin: Removed Static, life is simpler
   int TraceTokens;	//Scoggin: relocated here from global
   int local_debug;	//Scoggin: Added to pass Debug level around in library form
 private:
   Token* generateToken();
   Token* getNextTraceToken();
   bool allQueuesEmpty();
   int updateFunctionalUnits();
   int scheduleNewInstructions();
   int flushInstructions();
   // Waleed: old fetch/decode method
   //int refillInstructionQueues();
   // Waleed: added following three methods	
   int fetchInstructions();
   int decodeInstructions();
   int dispatchInstructions();
   int readIMixFile(string filename, string newIMixFilename);
   int readNewIMixFile(string filename);
   int readIDefFile(string filename, bool newImix);
   void createInstructionMixCDF();
   //Dependency* checkForDependencies(InstructionCount insn);
   //Dependency* addNewDependency(Token *t, unsigned int *);

   MarkovModel *markovModel;                 ///< Markov-based token generator
   FunctionalUnit *functionalUnitsHead;      ///< Functional units list
   InstructionQueue *instructionQueuesHead;  ///< Instruction queues list
   CycleCount currentCycle;                  ///< Current simulation cycle
   InstructionCount totalInstructions;       ///< Total instructions so far
   double* instructionClassProbabilities;    ///< Instruction type CDF
   InstructionInfo **instructionClasses;     ///< Instruction type ptrs
   InstructionInfo *instructionClassesHead,  ///< Instruction type list
                   *instructionClassesTail;  ///< Instruction type list tail
   unsigned int numInstructionClasses;       ///< Number of instruction types
   // Waleed: added following 
	double * instructionSizeProbabilities;    ///< Instruction size CDF
	double * fetchSizeProbabilities;          ///< Fetch size CDF
	int maxInstrSize;
   int fetchBuffSize;
	int fetchBufferIndex;
	int usedFetchBuffBytes; 
   int decodeWidth;  
	void createInstrSizeCDF(string filename);
	int  generateInstrSize(); 
	void createFetchSizeCDF(string filename);
	int  generateFetchSize(); 
   InstructionInfo *infoLEA;  ///< direct ptr to LEA insn for fast access
   //Dependency *dependenciesHead; ///< inter-insn operand dependency list
   //Dependency *dependenciesTail;
   LoadStoreUnit *loadStoreUnit;
   MemoryModel *memoryModel;
   ReorderBuffer *reorderBuffer;
   ReorderBuffer *fakeIBuffer;
   double lastCPI, measuredCPI;
   Token *lastToken; ///< Must remember last token generated but not used
   double probBranchMispredict;
   double probTakenBranch; 
   int instructionsPerCycle, instructionsPerFetch, branchMissPenalty;
   CycleCount nextAvailableFetch;
   unsigned long long fetchStallCycles;
   FILE *traceF;
   Address fakeAddress;
   ConfigVars *config;
   bool repeatTrace;
   //Waleed: fetchedBuffer contains the tokens newly fetched from i-cache and still not decoded
   Token** fetchedBuffer;
   //Waleed: decodeBuffer contains tokens that have been decoded; upto decodeWidth
   Token** decodeBuffer; 
};
}//End namespace McOpteron

#define READIntConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = atoi(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }
#define READStringConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = strdup(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }
#define READRealConfigVar(name, variable) \
      if ((v = config->findVariable(name))) \
         variable = atof(v); \
      else { \
         fprintf(stderr, "Error: must specify var (%s)...quitting\n", name); \
         exit(1); \
      }
#define READRealOptionalVar(name, variable) \
      if ((v = config->findVariable(name))) {\
         variable = atof(v); \
      }


#endif
