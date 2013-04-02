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
	SSTMcNiagara(SST::ComponentId_t id, SST::Component::Params_t& params);
	SSTMcNiagara();
	~SSTMcNiagara();
	McNiagara::McNiagara *the_cpu;
//	int TraceTokens;
	bool tic(SST::Cycle_t);
	int Setup( );
	int Finish( );
	bool Status( );

	//Simulation Parameters
	long numSimCycles;
	long seed;
	bool untilConvergence;	

	string appDirectory;//="."
	string inputfile;// = "INPUT";
	string outputfile;
	string iprobfile;//= "inst_prob.h";
	string pcntfile;//= "perf_cnt.h";
	string tracefile;

	//bookkeeping
	long cyclecount;
	bool converged;

};

}
}
#endif /* SST_MCOPTERON_H_ */
