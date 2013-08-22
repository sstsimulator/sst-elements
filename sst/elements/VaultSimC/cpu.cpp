// Copyright 2009-2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"
#include <cpu.h>

#include <sstream> // for stringstream() so I don't have to use atoi()
#include <stdio.h>
#include <stdlib.h>

#include <sst/core/params.h>

cpu::cpu( ComponentId_t id, Params& params ) :
  IntrospectedComponent( id ), outstanding(0), memOps(0), inst(0)
{
  printf("making cpu\n");

  std::string frequency = "2.2 GHz";
  if ( params.find( "clock" ) != params.end() ) {
    frequency = params["clock"];
  }

  if ( params.find( "threads" ) != params.end() ) {
    stringstream(params["threads"]) >> threads;
    memSet_t blank;
    for (int i = 0; i < threads; ++i) {
      thrOutstanding.push_back(blank);
      coreAddr.push_back(10000+i*100);
    }
  } else {
    printf(" no <threads> tag defined for cpu\n");
    exit(-1);
  }

  if ( params.find( "app" ) != params.end() ) {
    stringstream(params["app"]) >> app;
  } else {
    printf(" no <app> tag defined for cpu\n");
    exit(-1);
  }

  if ( params.find( "bwlimit" ) != params.end() ) {
    stringstream(params["bwlimit"]) >> bwlimit;
  } else {
    printf(" no <bwlimit> tag defined for cpu\n");
    exit(-1);
  }

  // connect chain
  toMem = new memChan_t( *this, params, "toMem" );

  registerClock( frequency, new Clock::Handler<cpu>(this, &cpu::clock) );

  printf("made cpu %p\n", toMem);
  //srandomdev();
}

int cpu::Finish() 
{
  printf("CPU completed %lld memOps\n", memOps);
  printf("CPU issued %lld inst\n", inst);
  return 0;
}

bool cpu::clock( Cycle_t current )
{
  memChan_t::event_t* event;

  // check for events from the memory chain
  while(toMem->recv( &event )) {
    //printf("CPU got event %lld\n", current);
    memOps++;
    outstanding--;
    const thrSet_t::iterator e = thrOutstanding.end();
    for (thrSet_t::iterator i = thrOutstanding.begin(); i != e; ++i) {
      i->erase(event->addr);
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
	if (thrOutstanding[c].size() < MAX_OUT_THR) {
	  memChan_t::event_t *event = getInst(1,app,c); // L1
	  inst++;
	  if (event) {
	    if ( ! toMem->send( event ) ) {
	      _abort(cpu::clock,"cpu send failed\n");
	    }
	    outstanding++;
	    Missued++;
	    if (event->reqType == memChan_t::event_t::READ) {
	      thrOutstanding[c].insert(event->addr);
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

