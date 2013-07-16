// Copyright 2009-2011 Sandia Corporation. Under the terms
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
#include <stdio.h>
#include "sst/core/serialization/element.h"
#include <logicLayer.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/memEvent.h>
#include <sstream> // for stringstream() so I don't have to use atoi()

using namespace SST::Interfaces;

#define DBG( fmt, args... )m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)
//typedef  VaultCompleteFn; 

logicLayer::logicLayer( ComponentId_t id, Params_t& params ) :
  IntrospectedComponent( id ), memOps(0)
{
  printf("making logicLayer\n");

  std::string frequency = "2.2 GHz";
  if ( params.find( "clock" ) != params.end() ) {
    frequency = params["clock"];
  }

  if ( params.find( "llID" ) != params.end() ) {
    stringstream(params["llID"]) >> llID;
  }

  if ( params.find( "bwlimit" ) != params.end() ) {
    stringstream(params["bwlimit"]) >> bwlimit;
  } else {
    printf(" no <bwlimit> tag defined for logiclayer\n");
    exit(-1);
  }

  if ( params.find( "LL_MASK" ) != params.end() ) {
    stringstream(params["LL_MASK"]) >> LL_MASK;
  } else {
    printf(" no <LL_MASK> tag defined for logiclayer\n");
    exit(-1);
  }

  bool terminal = 0;
  if ( params.find( "terminal" ) != params.end() ) {
    stringstream(params["terminal"]) >> terminal;
  }

  if ( params.find( "vaults" ) != params.end() ) {
    int numVaults;
    stringstream(params["vaults"]) >> numVaults;
    // connect up our vaults
    for (int i = 0; i < numVaults; ++i) {
      char bus_name[50];
      snprintf(bus_name, 50, "bus_%d", i);
      memChan_t *chan = configureLink( bus_name, "1 ns" );
      if (chan) {
	m_memChans.push_back(chan);
	printf(" connected %s\n", bus_name);
      } else {
	printf(" could not find %s\n", bus_name);
	exit(-1);
      }
    }
    printf(" Connected %d Vaults\n", numVaults);
  } else {
    printf(" no <vaults> tag defined for LogicLayer\n");
    exit(-1);
  }

  // connect chain
  toCPU = configureLink( "toCPU", "50 ps");
  if (!terminal) {
    toMem = configureLink( "toMem", "50 ps");
  } else {
    toMem = 0;
  }

  registerClock( frequency, new Clock::Handler<logicLayer>(this, &logicLayer::clock) );

  printf("made logicLayer %d %p %p\n", llID, toMem, toCPU);
}

int logicLayer::Finish() 
{
  printf("ll%d completed %lld ops\n", llID, memOps);
  return 0;
}

void logicLayer::init(unsigned int phase) {
  // tell the bus (or whaterver) that we are here. only the first one
  // in the chain should report, so every one sends towards the cpu,
  // but only the first one will arrive.
  if ( !phase ) {
    toCPU->sendInitData(new SST::Interfaces::StringEvent("SST::Interfaces::MemEvent"));
  }

  // rec data events from the direction of the cpu
  SST::Event *ev = NULL;
  while ( (ev = toCPU->recvInitData()) != NULL ) {
    MemEvent *me = dynamic_cast<MemEvent*>(ev);
    if ( me ) {
      /* Push data to memory */
      if ( me->getCmd() == WriteReq ) {
	//printf("Memory received Init Command: of size 0x%x at addr 0x%lx\n", me->getSize(), me->getAddr() );
	uint32_t chunkSize = (1 << VAULT_SHIFT);
	if (me->getSize() > chunkSize) {
	  // may need to break request up in to 256 byte chunks (minimal
	  // vault width)
	  int numNewEv = (me->getSize() / chunkSize) + 1;
	  uint8_t *inData = &(me->getPayload()[0]);
	  SST::Interfaces::Addr addr = me->getAddr();
	  for (int i = 0; i < numNewEv; ++i) {
	    // make new event
	    MemEvent *newEv = new MemEvent(this, addr, me->getCmd());
	    // set size and payload
	    if (i != (numNewEv - 1)) {
	      newEv->setSize(chunkSize);
	      newEv->setPayload(chunkSize, inData);
	      inData += chunkSize;
	      addr += chunkSize;
	    } else {
	      uint32_t remain = me->getSize() - (chunkSize * (numNewEv - 1));
	      newEv->setSize(remain);
	      newEv->setPayload(remain, inData);
	    }
	    // sent to where it needs to go
	    if (isOurs(newEv->getAddr())) {
	      // send to the vault
	      unsigned int vaultID = 
		(newEv->getAddr() >> VAULT_SHIFT) % m_memChans.size();
	      m_memChans[vaultID]->sendInitData(newEv);      
	    } else {
	      // send down the chain
	      toMem->sendInitData(newEv);
	    }
	  }
	  delete ev;
	} else {
	  if (isOurs(me->getAddr())) {
	    // send to the vault
	    unsigned int vaultID = (me->getAddr() >> 8) % m_memChans.size();
	    m_memChans[vaultID]->sendInitData(me);      
	  } else {
	    // send down the chain
	    toMem->sendInitData(ev);
	  }
	}
      } else {
	printf("Memory received unexpected Init Command: %d\n", me->getCmd() );
      }
    }
  }

}


bool logicLayer::clock( Cycle_t current )
{
  SST::Event* e = 0;

  int tm[2] = {0,0}; //recv send
  int tc[2] = {0,0};

  // check for events from the CPU
  while((e = toCPU->recv()) && tc[0] < bwlimit) {
    MemEvent *event  = dynamic_cast<MemEvent*>(e);
    printf("LL%d got req for %p (%d %d)\n", llID, event->getAddr(), event->getID().first, event->getID().second);
    if (event == NULL) {
      _abort(logicLayer::clock, "logic layer got bad event\n");
    }

    tc[0]++;
    if (isOurs(event->getAddr())) {
      // it is ours!
      unsigned int vaultID = (event->getAddr() >> VAULT_SHIFT) % m_memChans.size();
      printf("ll%d sends %p to vault @ %lld\n", llID, event, current);
      m_memChans[vaultID]->send(event);      
    } else {
      // it is not ours
      if (toMem) {
	toMem->send( event );
	tm[1]++;
	printf("ll%d sends %p to next\n", llID, event);
      } else {
	//printf("ll%d not sure what to do with %p...\n", llID, event);
      }
    }
  }

  // check for events from the memory chain
  if (toMem) {
    while((e = toMem->recv()) && tm[0] < bwlimit) {
      MemEvent *event  = dynamic_cast<MemEvent*>(e);
      if (event == NULL) {
	_abort(logicLayer::clock, "logic layer got bad event\n");
      }

      tm[0]++;
      // pass along to the CPU
      printf("ll%d sends %p towards cpu (%d %d)\n", llID, event, event->getID().first, event->getID().second);
      toCPU->send( event );
      tc[1]++;
    }
  }
	
  // check for incoming events from the vaults
  for (memChans_t::iterator i = m_memChans.begin(); 
       i != m_memChans.end(); ++i) {
    memChan_t *m_memChan = *i;
    while ((e = m_memChan->recv())) {
      MemEvent *event  = dynamic_cast<MemEvent*>(e);
      if (event == NULL) {
        _abort(logicLayer::clock, "logic layer got bad event from vaults\n");
      }
      printf("ll%d got an event %p from vault @ %lld, sends towards cpu\n", llID, event, current);
      
      // send to CPU
      memOps++;
      toCPU->send( event );
      tc[1]++;
    }    
  }

  //printf("ll%d: %d %d %d %d\n", llID, tm[0], tm[1], tc[0], tc[1]);
  if (tm[0] > bwlimit || 
      tm[1] > bwlimit || 
      tc[0] > bwlimit || 
      tc[1] > bwlimit) {
    printf("ll%d: %d %d %d %d\n", llID, tm[0], tm[1], tc[0], tc[1]);
  }

  return false;
}

extern "C" {
	Component* create_logicLayer( SST::ComponentId_t id,  SST::Component::Params_t& params )
	{
		return new logicLayer( id, params );
	}
}

