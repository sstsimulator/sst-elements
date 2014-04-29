// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* sst_mcniagara.cpp
 *
 * Created on Feb 1, 2013
 * 	Author: Michael Scoggin - Sandia National Labs/org 1422
 *
 * Decription:
 * 	SST wrapper for mcniagara, a stochastic model for a multicore niagara processor.
 *
 * References:
 *
 * History:
 *	[Feb 1, 2013] - Created (Michael Scoggin)
 */

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "sst_mcniagara.h"

#include <iostream>

#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

using std::cout;
using std::endl;


using namespace SST;
using namespace SST::SST_McNiagara;

SSTMcNiagara::SSTMcNiagara(ComponentId_t id, Params& params):Component(id){
	//	the_cpu = new McNiagara();
	//cout<<"SSTMcNiagara(ComponentId_t id, Params& params) Begin"<<endl;
	the_cpu = new McNiagara::McNiagara();
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
	//Debug (default=100)
	//cout<<"  Reading Debug"<<endl;
	if ( params.find("Debug") == params.end() ) the_cpu->set_debug(0);
	else the_cpu->set_debug(strtol( params[ "Debug" ].c_str(), NULL, 0 ));
	//seed (default=100)
	//cout<<"  Reading seed"<<endl;
	if ( params.find("seed") == params.end() ) seed=100;
	else seed = strtol( params[ "seed" ].c_str(), NULL, 0 );
	//cycles (default=100000)
	//cout<<"  Reading cycles"<<endl;
	if ( params.find("cycles") == params.end() ) numSimCycles=100000;
	else numSimCycles = strtol( params[ "cycles" ].c_str(), NULL, 0 );
	//converge (default=false)
	//cout<<"  Reading converge"<<endl;
	if ( params.find("converge") == params.end() ) untilConvergence=false;
	else untilConvergence = (strtol( params[ "converge" ].c_str(), NULL, 0 ))!=0;
	//appDirectory (default=".")
	//cout<<"  Reading appDirectory"<<endl;
	if ( params.find("appDirectory") == params.end() ) appDirectory=".";
	else appDirectory = params[ "appDirectory" ];
	//inputHistogram (default="INPUT")
	//cout<<"  Reading inputhist"<<endl;
	if ( params.find("inputHistogram") == params.end() ) inputfile="INPUT";
	else inputfile = params[ "inputHistogram" ];
	//instprob (default="inst_prob.data")
	//cout<<"  Reading instprob"<<endl;
	if ( params.find("instructionProbabilityFile") == params.end() ) iprobfile="inst_prob.data";
	else iprobfile = params[ "instructionProbabilityFile" ];
	//performanceCounterFile (default="perf_cnt.data")
	//cout<<"  Reading performanceCounterFile"<<endl;
	if ( params.find("performanceCounterFile") == params.end() ) pcntfile="perf_cnt.data";
	else pcntfile = params[ "performanceCounterFile" ];
	//traceFile (default=null)
	//cout<<"  Reading traceFile"<<endl;
	if ( params.find("traceFile") == params.end() ) tracefile="";
	else tracefile = params[ "traceFile" ];
	//outputFile (default="")
	//cout<<"  Reading outputFile"<<endl;
	if ( params.find("outputFile") == params.end() ) outputfile="";
	else outputfile = params[ "outputFile" ];

	if(appDirectory!="."){
		inputfile=appDirectory+inputfile;
		iprobfile=appDirectory+iprobfile;
		pcntfile=appDirectory+pcntfile;
		if(tracefile.size())
			tracefile=appDirectory+tracefile;
		if(outputfile.size())
			outputfile=appDirectory+outputfile;
	}
	//cout<<" Creating Clock"<<endl;
	registerClock( "1GHz",
			new Clock::Handler<SSTMcNiagara>(this, &SSTMcNiagara::tic ) );
	//cout<<"SSTMcNiagara(ComponentId_t id, Params& params) End"<<endl;

}//SSTMcNiagara::SSTMcNiagara()

SSTMcNiagara::SSTMcNiagara():Component(-1){}
SSTMcNiagara::~SSTMcNiagara(){}

bool SSTMcNiagara::tic(Cycle_t){
	converged=the_cpu->convergence;
	if(untilConvergence)
		if(converged)
      primaryComponentOKToEndSim();
		
	if(cyclecount<numSimCycles){
		if(the_cpu->sim_cycle(cyclecount))
      primaryComponentOKToEndSim();
	}
	else
    primaryComponentOKToEndSim();
	++cyclecount;
	return 0;
}//SSTMcNiagara::tic(Cycle_t)

void SSTMcNiagara::setup(){
	McNiagara::NullIF* exitIf;
	exitIf=new McNiagara::NullIF();
	//external_if->memoryAccess(OffCpuIF::AM_READ, 0x1000, 9);
	the_cpu->init(inputfile, exitIf, iprobfile, pcntfile, tracefile, seed);
	cyclecount=0;
	converged=false;
}//SSTMcNiagara::setup()

void SSTMcNiagara::finish(){ 
	the_cpu->fini(outputfile);
	delete the_cpu;
	the_cpu=0;
}//SSTMcNiagara::finish()


bool SSTMcNiagara::Status(){
	return 0;
}

static Component* create_SSTMcNiagara(SST::ComponentId_t id,SST::Params& params){
	return new SSTMcNiagara(id,params);
}

static const ElementInfoParam mcniagara_params[] = {
  {"Debug","Set the debug level","0"},
  {"seed","Set RNG seed","100"},
  {"cycles","set number of cycles to simulate","100000"},
  {"converge","Run until CPI converges?","0"},
  {"appDirectory","What directory to look for files in",""},
  {"inputHistogram","The input histogram",""},
  {"instructionProbabilityFile","Instruction mix input file",""},
  {"traceFile","Use for instruction trace file, trumps instruction mix",""},
  {"performanceCounterFile","used as an instruction mix only file",""},
  {"outputFile","an output file",""},
  {NULL,NULL,NULL}
};

static const ElementInfoComponent components[] = {
	{ 	"SSTMcNiagara",
		"Multicore Niagara Component",
		NULL,
		create_SSTMcNiagara,
    mcniagara_params,
	},
	{ NULL, NULL, NULL, NULL }
};

ElementLibraryInfo sst_mcniagara_eli = {
	"SSTMcNiagara",
	"Multicore Niagara Component",
	components,
  NULL,
  NULL,
  NULL
};
