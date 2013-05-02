// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
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

#include <string>
#include <assert.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

//#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "sst_mcopteron.h"

//extern int TraceTokens;
using namespace SST;
using namespace SST::SST_McOpteron;
using std::cout;
using std::endl;
SSTMcOpteron::SSTMcOpteron(ComponentId_t id, Params_t& params):Component(id){
	//	the_cpu = new McOpteron();
	//cout<<"SSTMcOpteron(ComponentId_t id, Params_t& params) Begin"<<endl;
	the_cpu = new McOpteron::McOpteron();
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
	//debugLevel (default=0)
	//cout<<"  Reading debugLevel"<<endl;
	if ( params.find("debugLevel") == params.end() ) debug=0;
	else debug = strtol( params[ "debugLevel" ].c_str(), NULL, 0 );
	//cycles (default=100000)
	//cout<<"  Reading cycles"<<endl;
	if ( params.find("cycles") == params.end() ) numSimCycles=100000;
	else numSimCycles = strtol( params[ "cycles" ].c_str(), NULL, 0 );
	//converge (default=false)
	//cout<<"  Reading converge"<<endl;
	if ( params.find("converge") == params.end() ) untilConvergence=false;
	else untilConvergence = (strtol( params[ "converge" ].c_str(), NULL, 0 ))!=0;
	//defFile (default=("opteron-insn.txt")
	//cout<<"  Reading defFile"<<endl;
	if ( params.find("defFile") == params.end() ) defFile="opteron-insn.txt";
	else defFile = params[ "defFile" ];
	//debugCycles (default=0)
	//cout<<"  Reading debugCycles"<<endl;
	if ( params.find("debugCycles") == params.end() ) debugCycle=0;
	else debugCycle = strtol( params[ "debugCycles" ].c_str(), NULL, 0 );
	//printStaticMix (default=false)
	//cout<<"  Reading printStaticMix"<<endl;
	if ( params.find("printStaticMix") == params.end() ) printStaticIMix=false;
	else printStaticIMix = strtol( params[ "printStaticMix" ].c_str(), NULL, 0 )!=0;
	//printInstructionMix (default=false)
	//cout<<"  Reading printInstructionMix"<<endl;
	if ( params.find("printInstructionMix") == params.end() ) printIMix=false;
	else printIMix = strtol( params[ "printInstructionMix" ].c_str(), NULL, 0 )!=0;
	//mixFile (default="usedist_new.all")
	//cout<<"  Reading mixfile"<<endl;
	if ( params.find("mixFile") == params.end() ) mixFile="usedist_new.all";
	else mixFile = params[ "mixFile" ];
	//appDirectory (default=".")
	//cout<<"  Reading appDirectory"<<endl;
	if ( params.find("appDirectory") == params.end() ) appDirectory=".";
	else appDirectory = params[ "appDirectory" ];
	//seed (default=100)
	//cout<<"  Reading seed"<<endl;
	if ( params.find("seed") == params.end() ) seed=100;
	else seed = strtol( params[ "seed" ].c_str(), NULL, 0 );
	//traceFile (default=null)
	//cout<<"  Reading traceFile"<<endl;
	if ( params.find("traceFile") == params.end() ) traceFile="";
	else traceFile = params[ "traceFile" ];
	//traceOut (default=false)
	//cout<<"  Reading traceOut"<<endl;
	if ( params.find("traceOut") == params.end() ) the_cpu->TraceTokens=0;
	else the_cpu->TraceTokens = strtol( params[ "traceOut" ].c_str(), NULL, 0 )!=0;
	//seperateSize (default=false)
	//cout<<"  Reading sepearateSize"<<endl;
	if ( params.find("seperateSize") == params.end() ) McOpteron::InstructionInfo::separateSizeRecords = false;
	else McOpteron::InstructionInfo::separateSizeRecords = strtol( params[ "seperateSize" ].c_str(), NULL, 0 )!=0;
	//newMixFile (default=null)
	//cout<<"  Reading newMixFile"<<endl;
	if ( params.find("newMixFile") == params.end() ) newIMixFile="instrMix.txt";
	else newIMixFile = params[ "newMixFile" ];
	//instructionSizeFile (default=null)
	//cout<<"  Reading instructionSizeFile"<<endl;
	if ( params.find("instructionSizeFile") == params.end() ) instrSizeFile="";
	else instrSizeFile = params[ "instructionSizeFile" ];
	//fetchSizeFile (default=null)
	//cout<<"  Reading fetchSizeFile"<<endl;
	if ( params.find("fetchSizeFile") == params.end() ) fetchSizeFile="";
	else {fetchSizeFile = params[ "fetchSizeFile" ];}
	//transFile (default=null)
	//cout<<"  Reading transFile"<<endl;
	if ( params.find("transFile") == params.end() ) transFile="";
	else transFile = params[ "transFile" ];
	//repeatTrace (default=false)
	//cout<<"  Reading repeatTrace"<<endl;
	if ( params.find("repeatTrace") == params.end() ) repeatTrace=false;
	else repeatTrace = strtol( params[ "repeatTrace" ].c_str(), NULL, 0 )!=0;
	//treateImmediateAsNone (default=false)
	//cout<<"  Reading treatImmediateAsNone"<<endl;
	//if ( params.find("treatImmediateAsNone") == params.end() ) the_cpu->treatImmAsNone = false;
	//else the_cpu->treatImmAsNone = strtol( params[ "treatImmediateAsNone" ].c_str(), NULL, 0 )!=0;

	//cout<<" Creating Clock"<<endl;
	registerClock( "1GHz",
			new Clock::Handler<SSTMcOpteron>(this, &SSTMcOpteron::tic ) );
	//cout<<"SSTMcOpteron(ComponentId_t id, Params_t& params) End"<<endl;

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

static Component* create_SSTMcOpteron(SST::ComponentId_t id,SST::Component::Params_t& params){
	return new SSTMcOpteron(id,params);
}

static const ElementInfoComponent components[] = {
	{ 	"SSTMcOpteron",
		"Multicore Opteron Component",
		NULL,
		create_SSTMcOpteron
	},
	{ NULL, NULL, NULL, NULL }
};

//extern "C" {
	ElementLibraryInfo sst_mcopteron_eli = {
		"SSTMcOpteron",
		"Multicore Opteron Component",
		components,
	};
//}

