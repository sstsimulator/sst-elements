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
 * sst_mcniagara.h
 *
 *  Created on: Feb 1, 2013
 *      Author: Michael Scoggin - Sandia National Labs/org 1422
 *
 *  Description:
 *
 *  References:
 *
 * History:
 *  [Feb 1, 2013] - Created (Michael Scoggin)
 */

#ifndef SST_MCOPTERON_H_
#define SST_MCOPTERON_H_

#include <sst/core/component.h>

#include "mcniagara/McNiagara.h"
namespace SST {
namespace SST_McNiagara {
class SSTMcNiagara : public SST::Component{
public:
	SSTMcNiagara(SST::ComponentId_t id, SST::Params& params);
	SSTMcNiagara();
	~SSTMcNiagara();
	McNiagara::McNiagara *the_cpu;
//	int TraceTokens;
	bool tic(SST::Cycle_t);
	void setup( ); 
	void finish( );
	bool Status( );

	//Simulation Parameters
	long numSimCycles;
	long seed;
	bool untilConvergence;	

	string appDirectory;//="."
	string inputfile;// = "INPUT";
	string outputfile;
	string iprobfile;//= "inst_prob.data";
	string pcntfile;//= "perf_cnt.data";
	string tracefile;

	//bookkeeping
	long cyclecount;
	bool converged;

};

}
}
#endif /* SST_MCOPTERON_H_ */
