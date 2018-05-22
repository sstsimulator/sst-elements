// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
#include <logicLayer.h>

#include <sst/core/interfaces/stringEvent.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/VaultSimC/memReqEvent.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::VaultSim;

#define DBG( fmt, args... )m_dbg.write( "%s():%d: " fmt, __FUNCTION__, __LINE__, ##args)
//typedef  VaultCompleteFn; 

logicLayer::logicLayer( ComponentId_t id, Params& params ) :
  Component( id ), memOps(0)
{
  dbg.init("@R:LogicLayer::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find("debug", 0));
  dbg.output(CALL_INFO, "making logicLayer\n");

  std::string frequency = "2.2 GHz";
  frequency = params.find<std::string>("clock", "2.2 Ghz");

  int ident = params.find("llID", -1);
  if (-1 == ident) {
    dbg.fatal(CALL_INFO, -1, "no llID defined\n");
  }
  llID = ident;

  bwlimit = params.find( "bwlimit", -1 );
  if (-1 == bwlimit ) {
    dbg.fatal(CALL_INFO, -1, 
	   " no <bwlimit> tag defined for logiclayer\n");
  }

  int mask = params.find( "LL_MASK", -1 );
  if ( -1 == mask ) {
    dbg.fatal(CALL_INFO, -1, 
	   " no <LL_MASK> tag defined for logiclayer\n");
  }
  LL_MASK = mask;

  bool terminal = params.find("terminal", 0);

  int numVaults = params.find("vaults", -1);
  if ( -1 != numVaults) {
    // connect up our vaults
    for (int i = 0; i < numVaults; ++i) {
      char bus_name[50];
      snprintf(bus_name, 50, "bus_%d", i);
      memChan_t *chan = configureLink( bus_name, "1 ns" );
      if (chan) {
	m_memChans.push_back(chan);
	dbg.output(" connected %s\n", bus_name);
      } else {
	dbg.fatal(CALL_INFO, -1, 
	       " could not find %s\n", bus_name);
      }
    }
    printf(" Connected %d Vaults\n", numVaults);
  } else {
    dbg.fatal(CALL_INFO, -1, 
	   " no <vaults> tag defined for LogicLayer\n");
  }

  // connect chain
  toCPU = configureLink( "toCPU");
  if (!terminal) {
    toMem = configureLink( "toMem");
  } else {
    toMem = 0;
  }

  registerClock( frequency, new Clock::Handler<logicLayer>(this, &logicLayer::clock) );

  dbg.output(CALL_INFO, "made logicLayer %d %p %p\n", llID, toMem, toCPU);

  // register bandwidth stats
  bwUsedToCpu[0] = registerStatistic<uint64_t>("BW_recv_from_CPU", "1");  
  bwUsedToCpu[1] = registerStatistic<uint64_t>("BW_send_to_CPU", "1");
  bwUsedToMem[0] = registerStatistic<uint64_t>("BW_recv_from_Mem", "1");
  bwUsedToMem[1] = registerStatistic<uint64_t>("BW_send_to_Mem", "1");
}

int logicLayer::Finish() 
{
  printf("Logic Layer %d completed %lld ops\n", llID, memOps);
  return 0;
}

void logicLayer::init(unsigned int phase) {
    // tell the bus (or whaterver) that we are here. only the first one
    // in the chain should report, so every one sends towards the cpu,
    // but only the first one will arrive.
    if ( !phase ) {
        toCPU->sendInitData(new SST::Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
    }
    
    // rec data events from the direction of the cpu
    SST::Event *ev = NULL;
    while ( (ev = toCPU->recvInitData()) != NULL ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            /* Push data to memory */
	  if ( me->isWriteback() || me->getCmd() == Command::GetX) {
	         //printf("Memory received Init Command: of size 0x%x at addr 0x%lx\n", me->getSize(), me->getAddr() );
                uint32_t chunkSize = (1 << VAULT_SHIFT);
                if (me->getSize() > chunkSize) {
                    // may need to break request up in to 256 byte chunks (minimal
                    // vault width)
                    int numNewEv = (me->getSize() / chunkSize) + 1;
                    uint8_t *inData = &(me->getPayload()[0]);
                    SST::MemHierarchy::Addr addr = me->getAddr();
                    for (int i = 0; i < numNewEv; ++i) {
                        // make new event
                        MemEvent *newEv = new MemEvent(this, addr, 
                                                       me->getBaseAddr(), 
                                                       me->getCmd());
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
                        unsigned int vaultID = (me->getAddr() >> VAULT_SHIFT) % m_memChans.size();
                        // printf("Propagating initial write %p to vault %d\n", me, vaultID);
                        m_memChans[vaultID]->sendInitData(me);      
                    } else {

                        // send down the chain
                        toMem->sendInitData(ev);
                    }
                }
            } else {
	        printf("Memory received unexpected Init Command: %s\n", CommandString[(int)me->getCmd()] );
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
  while((tc[0] < bwlimit) && (e = toCPU->recv())) {
    MemReqEvent *event  = dynamic_cast<MemReqEvent*>(e);
//    dbg.output(CALL_INFO, "LL%d got req for %p (%lld %d)\n", llID, 
    dbg.output(CALL_INFO, "LL%d got req for %p (%" PRIu64 " %d)\n", llID, 
	       (void*)event->getAddr(), event->getID().first, event->getID().second);
    if (event == NULL) {
      dbg.fatal(CALL_INFO, -1, "logic layer got bad event\n");
    }

    tc[0]++;
    if (isOurs(event->getAddr())) {
      // it is ours!
      unsigned int vaultID = (event->getAddr() >> VAULT_SHIFT) % m_memChans.size();
//      dbg.output(CALL_INFO, "ll%d sends %p to vault @ %lld\n", llID, event, 
      dbg.output(CALL_INFO, "ll%d sends %p to vault @ %" PRIu64 "\n", llID, event, 
		 current);
      m_memChans[vaultID]->send(event);      
    } else {
      // it is not ours
      if (toMem) {
	toMem->send( event );
	tm[1]++;
	dbg.output(CALL_INFO, "ll%d sends %p to next\n", llID, event);
      } else {
	//printf("ll%d not sure what to do with %p...\n", llID, event);
      }
    }
  }

  // check for events from the memory chain
  if (toMem) {
    while((tm[0] < bwlimit) && (e = toMem->recv())) {
      MemRespEvent *event  = dynamic_cast<MemRespEvent*>(e);
      if (event == NULL) {
	dbg.fatal(CALL_INFO, -1, "logic layer got bad event\n");
      }

      tm[0]++;
      // pass along to the CPU
//      dbg.output(CALL_INFO, "ll%d sends %p towards cpu (%lld %d)\n", 
      dbg.output(CALL_INFO, "ll%d sends %p towards cpu (%" PRIu64 " %d)\n", 
		 llID, event, event->getID().first, event->getID().second);
      toCPU->send( event );
      tc[1]++;
    }
  }
	
  // check for incoming events from the vaults
  for (memChans_t::iterator i = m_memChans.begin(); 
       i != m_memChans.end(); ++i) {
    memChan_t *m_memChan = *i;
    while ((e = m_memChan->recv())) {
      MemRespEvent *event  = dynamic_cast<MemRespEvent*>(e);
      if (event == NULL) {
        dbg.fatal(CALL_INFO, -1, "logic layer got bad event from vaults\n");
      }
//      dbg.output(CALL_INFO, "ll%d got an event %p from vault @ %lld, sends "
      dbg.output(CALL_INFO, "ll%d got an event %p from vault @ %" PRIu64 ", sends "
		 "towards cpu\n", llID, event, current);
      
      // send to CPU
      memOps++;
      toCPU->send( event );
      tc[1]++;
    }    
  }

  if (tm[0] > bwlimit || 
      tm[1] > bwlimit || 
      tc[0] > bwlimit || 
      tc[1] > bwlimit) {
    dbg.output(CALL_INFO, "ll%d Bandwdith: %d %d %d %d\n", 
	       llID, tm[0], tm[1], tc[0], tc[1]);
  }
  bwUsedToCpu[0]->addData(tc[0]);
  bwUsedToCpu[1]->addData(tc[1]);
  bwUsedToMem[0]->addData(tm[0]);
  bwUsedToMem[1]->addData(tm[1]);
  

  return false;
}
