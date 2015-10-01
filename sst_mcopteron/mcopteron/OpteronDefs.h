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


#ifndef OPTERONDEFS_H
#define OPTERONDEFS_H

#include <string>
using std::string;
#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef MEMDEBUG
#include <duma.h>
#endif
namespace McOpteron{ //Scoggin: Added a namespace to reduce possible conflicts as library
	/// High level instruction categories
	enum Category { UNKNOWN, GENERICINT, SPECIALINT, MULTINT, FLOAT };

	/// Functional Unit Designators
	enum FunctionalUnitTypes { AGU = 1, ALU0 = 2, ALU1 = 4, ALU2 = 8,
		FADD = 16, FMUL = 32, FSTORE = 64 };
	// Access byte bitmask for up to four instruction steps
#define STEP1(v) ((v)&0xff)
#define STEP2(v) (((v)>>8)&0xff)
#define STEP3(v) (((v)>>16)&0xff)
#define STEP4(v) (((v)>>24)&0xff)

#define HISTOGRAMSIZE 128
#define MAXCANASSIGN 3  ///< Opteron decode path allows 3 insns/cycle
#define AGU_LATENCY 1   ///< # of cycles to generate address in AGU

	typedef unsigned long long InstructionCount;
	typedef unsigned long long CycleCount;
	typedef unsigned long long Address;

	/// Records of inter-instruction data dependencies
	/*
	   struct Dependency
	   {
	   InstructionCount consumer;
	   unsigned int numProducers;
	   unsigned int numReady;
	   bool consumed;
	   struct Dependency *next;
	   };


	/// Records of inter-instruction data dependencies
	struct Dependency
	{
	Token *tkn;				// token to which this record belongs
	unsigned int numInputs; // # of input registers for token
	unsigned int numReady;  // # of input registers that have become ready
	InstructionCount *producers; // instructionCount of instructions that produce each input register
	struct Dependency *next;
	};

*/

	EXTERN unsigned int Debug;


	inline string tokenize(string *str,string& aFormat);

	double genRandomProbability();
	void seedRandom(unsigned long seed);
}//end namespace McOpteron
#endif
