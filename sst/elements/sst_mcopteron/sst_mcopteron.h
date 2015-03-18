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


/*
 * sst_mcopteron.h
 *
 *  Created on: Dec 21, 2012
 *      Author: Michael Scoggin - Sandia National Labs/org 1422
 *
 *  Description:
 *
 *  References:
 *
 * History:
 *  [Dec 21, 2012] - Created (Michael Scoggin)
 */

#ifndef SST_MCOPTERON_H_
#define SST_MCOPTERON_H_

#include <sst/core/component.h>

#include "mcopteron/McOpteron.h"
namespace SST {
namespace SST_McOpteron {
class SSTMcOpteron : public SST::Component{
public:
	SSTMcOpteron(SST::ComponentId_t id, SST::Params& params);
	SSTMcOpteron();
	~SSTMcOpteron();
	McOpteron::McOpteron *the_cpu;
//	int TraceTokens;
	bool tic(SST::Cycle_t);
	void setup( );
	void finish( );
	bool Status( );

	//Simulation Parameters
	long numSimCycles;
	long debugCycle;
	long seed;

	int debug;
	bool untilConvergence;
	bool printIMix;
	bool printStaticIMix;
	bool repeatTrace;


	string mixFile;// = "usedist_new.all";
	string defFile;// = "opteron-insn.txt";
	string traceFile;// = 0;
	string appDirectory;// = ".";
	string newIMixFile;// = "instrMix.txt";
	string instrSizeFile;// = 0;
	string transFile;//=0;
	string fetchSizeFile;//=0;
	//bookkeeping
	long cyclecount;
	bool converged;

};
}
}
#endif /* SST_MCOPTERON_H_ */
