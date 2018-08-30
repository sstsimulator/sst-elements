// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2014, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <cpu.h>

#include <stdio.h>
#include <stdlib.h>

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/mersenne.h>

#include <sst/elements/VaultSimC/memReqEvent.h>

using namespace SST;
using namespace SST::VaultSim;
using namespace SST::RNG;

cpu::cpu( ComponentId_t id, Params& params ) :
  Component( id ), outstanding(0), memOps(0), inst(0),
    out(Simulation::getSimulation()->getSimulationOutput())
{
  printf("making cpu\n");

  std::string frequency = params.find<std::string>("clock", "2.2 Ghz");

  threads = params.find( "threads", 0 );
  if (0 == threads) {
    out.fatal(CALL_INFO, -1, "no <threads> tag defined for cpu\n");
  } else {
    memSet_t blank;
    for (int i = 0; i < threads; ++i) {
      thrOutstanding.push_back(blank);
      coreAddr.push_back(10000+i*100);
    }
  }

  app = params.find( "app", -1 );
  if ( -1 == app ) {
    out.fatal(CALL_INFO, -1, " no <app> tag defined for cpu\n");
  }

  bwlimit = params.find( "bwlimit", 0 );
  if (0 == bwlimit) {
    out.fatal(CALL_INFO, -1, " no <bwlimit> tag defined for cpu\n");
  }

  // connect chain
  toMem = configureLink( "toMem", frequency);

  registerClock( frequency, new Clock::Handler<cpu>(this, &cpu::clock) );

  //printf("made cpu %p\n", toMem);

  // init random number generator
  unsigned seed = params.find("seed", 0);
  if (0 != seed) {
    rng = new MersenneRNG(seed);
  } else {
    rng = new MersenneRNG();
  } 

  // register as a primary component
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

void cpu::finish() 
{
  printf("CPU completed %lld memOps\n", memOps);
  printf("CPU issued %lld inst\n", inst);
}

bool cpu::clock( Cycle_t current )
{
  SST::Event* e;

  // check for events from the memory chain
  while((e = toMem->recv())) {
    MemRespEvent *event = 
      dynamic_cast<MemRespEvent*>(e);
    if (event == NULL) {
      out.fatal(CALL_INFO, -1, "CPU got bad event\n");
    }
    //printf("CPU got event %lld\n", current);
    memOps++;
    outstanding--;
    const thrSet_t::iterator e = thrOutstanding.end();
    for (thrSet_t::iterator i = thrOutstanding.begin(); i != e; ++i) {
      i->erase(event->getAddr());
    }
    delete event;
  }

  const unsigned int MAX_OUT_THR = 2;
  unsigned int MAX_OUT = threads * MAX_OUT_THR * 4;

  int Missued=0;
  // send a reference
  for (int w = 0; w < 4; ++w) { //issue width
    for (int c = 0; c < threads; ++c) {
      if (outstanding < MAX_OUT && Missued <= bwlimit) {    
	if (thrOutstanding[c].size() <= MAX_OUT_THR) {
	  MemReqEvent *event = getInst(1,app,c); // L1
	  inst++;
	  if (event) {
	    toMem->send( event );
	    outstanding++;
	    Missued++;
	    if (! event->getIsWrite()) {
	      thrOutstanding[c].insert(event->getAddr());
	    }
	  }
	}
      } else {
	break; // reached MAX_OUT
      }
    }
  }

  if ((current & 0x3ff) == 0) {
//      printf("%lld: out:%d ops/cycle:%0.2f\n", current, outstanding,
      printf("%" PRIu64 ": out:%d ops/cycle:%0.2f\n", current, outstanding, 
	   double(memOps)/double(current));
  }

  return false;
}


