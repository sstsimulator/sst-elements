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
 * sst_mcopteron.cpp
 *
 *  Created on: Dec 19, 2012
 *      Author: Michael Scoggin - Sandia National Labs/org 1422
 *
 *  Description:
 *
 *  References:
 *
 * History:
 *  [Dec 19, 2012] - Created (Michael Scoggin)
 */
#define NOTSTANDALONE 1

#include <sst_config.h>
#include "sst_mcopteron.h"

#include <string>
#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/params.h>


//extern int TraceTokens;
using namespace SST;
using namespace SST::SST_McOpteron;
using std::cout;
using std::endl;
SSTMcOpteron::SSTMcOpteron(ComponentId_t id, Params& params):Component(id){
	//	the_cpu = new McOpteron();
	//cout<<"SSTMcOpteron(ComponentId_t id, Params& params) Begin"<<endl;
	the_cpu = new McOpteron::McOpteron();
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
	//debugLevel (default=0)
	//cout<<"  Reading debugLevel"<<endl;
        debug = params.find_integer("debugLevel", 0);
	//cycles (default=100000)
	//cout<<"  Reading cycles"<<endl;
        numSimCycles = params.find_integer("cycles", 100000);
	//converge (default=false)
	//cout<<"  Reading converge"<<endl;
	untilConvergence = (params.find_integer("converge", 0)) !=0;
	//defFile (default=("opteron-insn.txt")
	//cout<<"  Reading defFile"<<endl;
        defFile = params.find_string("defFile","opteron-insn.txt");
	//debugCycles (default=0)
	//cout<<"  Reading debugCycles"<<endl;
        debugCycle = params.find_integer("debugCycles", 0);
	//printStaticMix (default=false)
	//cout<<"  Reading printStaticMix"<<endl;
        printStaticIMix = (params.find_integer("printStaticMix", 0)) !=0;
	//printInstructionMix (default=false)
	//cout<<"  Reading printInstructionMix"<<endl;
        printIMix = (params.find_integer("printInstructionMix", 0)) !=0;
	//mixFile (default="usedist_new.all")
	//cout<<"  Reading mixfile"<<endl;
        mixFile = params.find_string("mixFile", "usedist_new.all");
	//appDirectory (default=".")
	//cout<<"  Reading appDirectory"<<endl;
	appDirectory = params.find_string("appDirectory", ".");
	//seed (default=100)
	//cout<<"  Reading seed"<<endl;
	seed = params.find_integer("seed", 100);
	//traceFile (default=null)
	//cout<<"  Reading traceFile"<<endl;
        traceFile = params.find_string("traceFile", "");
	//traceOut (default=false)
	//cout<<"  Reading traceOut"<<endl;
        the_cpu->TraceTokens = params.find_integer("traceOut", 0);
	//seperateSize (default=false)
	//cout<<"  Reading sepearateSize"<<endl;
        McOpteron::InstructionInfo::separateSizeRecords = (params.find_integer("seperateSize", 0)) !=0;
	//newMixFile (default=null)
	//cout<<"  Reading newMixFile"<<endl;
        newIMixFile = params.find_string("newMixFile", "instrMix.txt");
	//instructionSizeFile (default=null)
	//cout<<"  Reading instructionSizeFile"<<endl;
        instrSizeFile = params.find_string("instructionSizeFile", "");
	//fetchSizeFile (default=null)
	//cout<<"  Reading fetchSizeFile"<<endl;
        fetchSizeFile = params.find_string("fetchSizeFile", "");
	//transFile (default=null)
	//cout<<"  Reading transFile"<<endl;
        transFile = params.find_string("transFile", "");
	//repeatTrace (default=false)
	//cout<<"  Reading repeatTrace"<<endl;
        repeatTrace = (params.find_integer("repeatTrace", 0)) !=0;
	//treateImmediateAsNone (default=false)
	//cout<<"  Reading treatImmediateAsNone"<<endl;
	//if ( params.find("treatImmediateAsNone") == params.end() ) the_cpu->treatImmAsNone = false;
	//else the_cpu->treatImmAsNone = strtol( params[ "treatImmediateAsNone" ].c_str(), NULL, 0 )!=0;
        //JC: This was already commented out when I went to update deprecated params functions. 

	//cout<<" Creating Clock"<<endl;
	registerClock( "1GHz",
			new Clock::Handler<SSTMcOpteron>(this, &SSTMcOpteron::tic ) );
	//cout<<"SSTMcOpteron(ComponentId_t id, Params& params) End"<<endl;

}//SSTMcOpteron::SSTMcOpteron()

SSTMcOpteron::SSTMcOpteron():Component(-1){}
SSTMcOpteron::~SSTMcOpteron(){}

bool SSTMcOpteron::tic(Cycle_t){
	if(cyclecount==debugCycle)
		the_cpu->local_debug=debug;
	++cyclecount;
	if(untilConvergence)
		if(!converged)
			converged=the_cpu->simCycle();
		else
      primaryComponentOKToEndSim();
	else
		if(cyclecount<numSimCycles)
			converged=the_cpu->simCycle();
		else
      primaryComponentOKToEndSim();
	return 0;
}//SSTMcOpteron::tic(Cycle_t)

void SSTMcOpteron::setup(){  
	the_cpu->local_debug=debug;
	the_cpu->init(appDirectory, defFile, mixFile, traceFile, repeatTrace, newIMixFile, instrSizeFile, fetchSizeFile, transFile);
	McOpteron::seedRandom(seed);
	if (printStaticIMix)
		the_cpu->printStaticIMix();
	cyclecount=0;
	converged=false;
}//SSTMcOpteron:sSetup()

void SSTMcOpteron::finish(){  
	the_cpu->finish(printIMix);
	delete the_cpu;
	the_cpu=0;
}//SSTMcOpteron::finish()

bool SSTMcOpteron::Status(){
	return 0;
}

static Component* create_SSTMcOpteron(SST::ComponentId_t id,SST::Params& params){
	return new SSTMcOpteron(id,params);
}

static const ElementInfoParam mcopteron_params[] = {
  {"debugLevel","Print differen levels of debugging info (1-3)","1"},
  {"cycles","Number of cycles to simulate","100000"},
  {"converge","Run until CPI converges?","0"},
  {"defFile","Instruction definition file","opteron-insn.txt"},
  {"debugCycles","What cycle to start debug output","0"},
  {"printStaticMix","Print out static input instruction mix at begining of simulation","0"},
  {"printInstructionMix","Print out simulated instruction mix at end","0"},
  {"mixFile","Instruction mix input file",""},
  {"appDirectory","What directory to look for files in","./mcopteron"},
  {"seed","Seed for random number generator","100"},
  {"traceFile","Instruction Trace file, trumps instruction mix",""},
  {"traceOut","Generate a token trace to stderr?",""},
  {"instructionSizeFile","Instruction size distribution file","instrSizeDistr.txt"},
  {"fetchSizeFile","Fetch size distribution file","fetchSizeDistr.txt"},
  {"transFile","Markov transition file, trumps instruction mix",""},
  {"repeatTrace","Loop tracefile?","0"},
  {"seperateSize","keep instruction types seperate on operand size","0"},
  {"newMixFile","use as an i-mix-only file (new format)",""},
  {NULL,NULL,NULL}
};

static const ElementInfoComponent components[] = {
	{ 	"SSTMcOpteron",
		"Multicore Opteron Component",
		NULL,
		create_SSTMcOpteron,
    mcopteron_params,
    NULL,
    COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL,0 }
};

//extern "C" {
	ElementLibraryInfo sst_mcopteron_eli = {
		"SSTMcOpteron",
		"Multicore Opteron Component",
		components,
    NULL,
    NULL,
    NULL
	};
//}

