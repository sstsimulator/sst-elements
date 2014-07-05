// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <cpu.h>

#include <stdio.h>
#include <stdlib.h>

#include "sst/core/serialization.h"
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/debug.h>
#include <sst/core/rng/mersenne.h>

using namespace SST;
using namespace SST::RNG;

cpu::cpu( ComponentId_t id, Params& params ) :
  IntrospectedComponent( id ), outstanding(0), memOps(0), inst(0)
{
  printf("making cpu\n");

  std::string frequency = "2.2 GHz";
  if ( params.find( "clock" ) != params.end() ) {
    frequency = params["clock"];
  }

  threads = params.find_integer( "threads", 0 );
  if (0 == threads) {
    _abort(cpu::cpu, "no <threads> tag defined for cpu\n");
  } else {
    memSet_t blank;
    for (int i = 0; i < threads; ++i) {
      thrOutstanding.push_back(blank);
      coreAddr.push_back(10000+i*100);
    }
  }

  app = params.find_integer( "app", -1 );
  if ( -1 == app ) {
    _abort(cpu::cpu, " no <app> tag defined for cpu\n");
  }

  bwlimit = params.find_integer( "bwlimit", 0 );
  if (0 == bwlimit) {
    _abort(cpu::cpu, " no <bwlimit> tag defined for cpu\n");
  }

  // connect chain
  toMem = configureLink( "toMem", frequency);

  registerClock( frequency, new Clock::Handler<cpu>(this, &cpu::clock) );

  //printf("made cpu %p\n", toMem);

  // init random number generator
  unsigned seed = params.find_integer("seed", 0);
  if (0 != seed) {
    rng = new MersenneRNG(seed);
  } else {
    rng = new MersenneRNG();
  } 
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
    SST::MemHierarchy::MemEvent *event = 
      dynamic_cast<SST::MemHierarchy::MemEvent*>(e);
    if (event == NULL) {
      _abort(cpu::clock, "CPU got bad event\n");
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
	  SST::MemHierarchy::MemEvent *event = getInst(1,app,c); // L1
	  inst++;
	  if (event) {
	    toMem->send( event );
	    outstanding++;
	    Missued++;
	    if (event->getCmd() == SST::MemHierarchy::GetS) {
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
    printf("%lld: out:%d ops/cycle:%0.2f\n", current, outstanding, 
	   double(memOps)/double(current));
  }

  return false;
}

extern "C" {
	Component* create_cpu( SST::ComponentId_t id,  SST::Params& params )
	{
		return new cpu( id, params );
	}
}

