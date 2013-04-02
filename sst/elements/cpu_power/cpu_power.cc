// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "cpu_power.h"
#include "simpleEvent.h"

using namespace SST;
using namespace SST::CPU_power;

Cpu_power::Cpu_power(ComponentId_t id, Params_t& params) :
  IntrospectedComponent(id),
  params(params) {

  // get parameters
  if ( params.find("workPerCycle") == params.end() ) {
    _abort(event_test,"couldn't find work per cycle\n");
  }
  workPerCycle = strtol( params[ "workPerCycle" ].c_str(), NULL, 0 );

  if ( params.find("commFreq") == params.end() ) {
    _abort(event_test,"couldn't find communication frequency\n");
  }
  commFreq = strtol( params[ "commFreq" ].c_str(), NULL, 0 );

  if ( params.find("commSize") == params.end() ) {
    _abort(event_test,"couldn't find communication size\n");
  }
  commSize = strtol( params[ "commSize" ].c_str(), NULL, 0 );
          
 
  // init randomness
  srand(1);
  neighbor = rand() % 4;

  // tell the simulator not to end without us
  registerExit();

  // configure out links
  N = configureLink( "Nlink", 
		     new Event::Handler<Cpu_power>(this,
							 &Cpu_power::
							 handleEvent) );
  S = configureLink( "Slink", 
		     new Event::Handler<Cpu_power>(this,
							 &Cpu_power::
							 handleEvent) );
  E = configureLink( "Elink", 
		     new Event::Handler<Cpu_power>(this,
							 &Cpu_power::
							 handleEvent) );
  W = configureLink( "Wlink", 
		     new Event::Handler<Cpu_power>(this,
							 &Cpu_power::
							 handleEvent) );
  assert(N);
  assert(S);
  assert(E);
  assert(W);

  //set our clock
  registerClock( "1GHz", 
		 new Clock::Handler<Cpu_power>(this, 
						     &Cpu_power::clockTic ) );

}

Cpu_power::Cpu_power() :
    IntrospectedComponent(-1)
{
    // for serialization only
}

// incoming events are scanned and deleted
void Cpu_power::handleEvent(Event *ev) {
  //printf("recv\n");
  simpleEvent *event = dynamic_cast<simpleEvent*>(ev);
  if (event) {
    // scan through each element in the payload and do something to it
    volatile int sum = 0;
    for (simpleEvent::dataVec::iterator i = event->payload.begin();
	 i != event->payload.end(); ++i) {
      sum += *i;
    }
    delete event;
  } else {
    printf("Error! Bad Event Type!\n");
  }
}

// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool Cpu_power::clockTic( Cycle_t ) {
  // do work
  // loop becomes:
  /*  00001ab5        movl    0xe0(%ebp),%eax
      00001ab8        incl    %eax
      00001ab9        movl    %eax,0xe0(%ebp)
      00001abc        incl    %edx
      00001abd        cmpl    %ecx,%edx
      00001abf        jne     0x00001ab5

      6 instructions. 
  */

  volatile int v = 0;
  for (int i = 0; i < workPerCycle; ++i) {
    v++;
  }

  // communicate?
  if ((rand() % commFreq) == 0) {
    // yes, communicate
    // create event
    simpleEvent *e = new simpleEvent();
    // fill payload with commSize bytes
    for (int i = 0; i < (commSize); ++i) {
      e->payload.push_back(1);
    }
    // find target
    neighbor = (++neighbor) % 4;
    // send
    switch (neighbor) {
    case 0:
      N->Send(e);
      break;
    case 1:
      S->Send(e);
      break;
    case 2:
      E->Send(e);
      break;
    case 3:
      W->Send(e);
      break;
    default:
      printf("bad neighbor\n");
    }
    //printf("sent\n");
    //first reset all counts to zero
	power->resetCounts(&mycounts);
    //updating usage counts for power
    	  //icache
	  mycounts.il1_read[0] = 2;
  	  mycounts.il1_readmiss[0] = 0;	  
	  //dcache
	  mycounts.dl1_read[0] = 2;
	  mycounts.dl1_readmiss[0] = 0;
	  mycounts.dl1_write[0] = 2;
	  mycounts.dl1_writemiss[0] = 0;
	  //L2
	  mycounts.L2_read[0] = 1;
	  mycounts.L2_readmiss[0] = 1;
	  mycounts.L2_write[0] = 1;
	  mycounts.L2_writemiss[0] = 1;
	  //RF
	  mycounts.int_regfile_reads[0] = 3;
    	  mycounts.int_regfile_writes[0] = 3; 
	  mycounts.float_regfile_reads[0] = 2;
	  mycounts.float_regfile_writes[0] = 2;
	  //ALU
	  mycounts.alu_access[0] = 5;
    	  mycounts.fpu_access[0] = 5;
	  //LSQ
	  mycounts.LSQ_read[0] = 4;
	  mycounts.LSQ_write[0] = 4;
	  //IB
	  mycounts.IB_read[0] = 3;
	  mycounts.IB_write[0] = 3;
	  //INST_DECODER
	  mycounts.ID_inst_read[0] = 3;
	  mycounts.ID_operand_read[0] = 3;
	  mycounts.ID_misc_read[0] = 2;
	  //router
	  mycounts.router_access=4;

    //power/temperature calculation
    pdata= power->getPower(this, CACHE_IL1, mycounts);
	pdata= power->getPower(this, CACHE_DL1, mycounts);
	pdata= power->getPower(this, CACHE_L2, mycounts);
	pdata= power->getPower(this, RF, mycounts);
	pdata= power->getPower(this, EXEU_ALU, mycounts);
	pdata= power->getPower(this, LSQ, mycounts);
	pdata= power->getPower(this, IB, mycounts);
	pdata= power->getPower(this, INST_DECODER, mycounts);
	pdata = power->getPower(this, ROUTER, mycounts);
	pdata = power->getPower(this, CLOCK, mycounts);

	power->compute_temperature(getId());
	regPowerStats(pdata);

	//reset all counts to zero
	power->resetCounts(&mycounts);
    
  }

  // return false so we keep going
  return false;
}


// Element Libarary / Serialization stuff
    
BOOST_CLASS_EXPORT(simpleEvent)
BOOST_CLASS_EXPORT(Cpu_power)

static Component*
create_cpu_power(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new Cpu_power( id, params );
}

static const ElementInfoComponent components[] = {
    { "cpu_power",
      "Simple Demo Component for power and thermal simulations",
      NULL,
      create_cpu_power
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo cpu_power_eli = {
        "Cpu_power",
        "Demo Power Component",
        components,
    };
}
