// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015 Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <VaultSimC.h>

#include <sys/mman.h>

#include "sst/core/serialization.h"
#include <sst/core/link.h>
#include <sst/core/params.h>

#include <vaultGlobals.h>

//typedef  VaultCompleteFn; 

static size_t MEMSIZE = size_t(4096)*size_t(1024*1024);

using namespace SST::MemHierarchy;

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
/**

*/

VaultSimC::VaultSimC( ComponentId_t id, Params& params ) :
  IntrospectedComponent( id ), numOutstanding(0) {
    dbg.init("@R:VaultSim::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));  

    std::string frequency = "1.0 GHz";
    frequency = params.find_string("clock", "1.0 Ghz");

    // number of bits to determine vault address
    int nv2 = params.find_integer("numVaults2", -1);
    if (-1 == nv2) {
      dbg.fatal(CALL_INFO, -1, "numVaults2 (number of bits to determine vault address) not set! Should be log2(number of vaults per cube)\n");
    } else {
      numVaults2 = nv2;
    }

    m_memChan = configureLink( "bus", "1 ns" );

    int vid = params.find_integer("VaultID", -1);
    if (-1 == vid)
      dbg.fatal(CALL_INFO, -1,"not VaultID Set\n");
    else 
      vaultID = vid;

    bool statistics = params.find_integer("statistics", -1);
    if (-1 == vid)
      dbg.fatal(CALL_INFO, -1,"not statistics Set\n");
    

    registerClock(frequency, new Clock::Handler<VaultSimC>(this, &VaultSimC::clock_phx));

    // Phx Library configuratoin
    m_memorySystem = new Vault(vaultID, &dbg, statistics, frequency);
    if (!m_memorySystem) {
      dbg.fatal(CALL_INFO, -1,"MemorySystem() failed\n");
    }

    CallbackBase<void, unsigned, uint64_t, uint64_t> *readDataCB = new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::readData);
    CallbackBase<void, unsigned, uint64_t, uint64_t> *writeDataCB = new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::writeData);

    dbg.output(CALL_INFO, "VaultSimC %u: made vault %u\n", vaultID, vaultID);
    m_memorySystem->RegisterCallback(readDataCB, writeDataCB);

    // setup backing store
    size_t memSize = MEMSIZE;
    memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if ( !memBuffer ) {
      dbg.fatal(CALL_INFO, -1, "Unable to MMAP backing store for Memory\n");
    }

    memOutStat = registerStatistic<uint64_t>("Mem_Outstanding","1");
}

void VaultSimC::finish() {
  munmap(memBuffer, MEMSIZE);
  m_memorySystem->finish();
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
/**

*/

void VaultSimC::init(unsigned int phase) {
  SST::Event *ev = NULL;
  while ( (ev = m_memChan->recvInitData()) != NULL ) {
    MemEvent *me = dynamic_cast<MemEvent*>(ev);
    if ( me ) {
      /* Push data to memory */
      if ( me->isWriteback() ) {
        uint32_t chunkSize = (1 << VAULT_SHIFT);
        if (me->getSize() > chunkSize)
          dbg.fatal(CALL_INFO, -1, "vault got too large init\n");
        for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
          memBuffer[getInternalAddress(me->getAddr() + i)] = me->getPayload()[i];
        }
      } else {
        dbg.fatal(CALL_INFO, -1, "vault got bad init command\n");
      }
    } else {
      dbg.fatal(CALL_INFO, -1, "vault got bad init event\n");
    }
    delete ev;
  }
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
/**

*/

void VaultSimC::readData(unsigned id, uint64_t addr, uint64_t clockcycle)
{
  t2MEMap_t::iterator mi = transactionToMemEventMap.find(addr);

  if (mi == transactionToMemEventMap.end()) {
    dbg.fatal(CALL_INFO, -1, "Vault %d can't find transaction %p\n", vaultID,(void*)addr);
  }
  MemEvent *parentEvent = mi->second;
  MemEvent *event = parentEvent->makeResponse();
  //printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
  //assert(bp.burstLength == parentEvent->getSize());

  // copy data from backing store to event
  //event->setSize(bp.burstLength);
  for ( size_t i = 0 ; i < event->getSize() ; i++ ) {
    //event->getPayload()[i] = memBuffer[getInternalAddress(bp.physicalAddress + i)];
  }
  m_memChan->send(event);
  dbg.output(CALL_INFO, "VaultSimC %d: read req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);
  // delete old event
  delete parentEvent;
  transactionToMemEventMap.erase(mi);
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
/**

*/

void VaultSimC::writeData(unsigned id, uint64_t addr, uint64_t clockcycle)
{
  t2MEMap_t::iterator mi = transactionToMemEventMap.find(addr);

  if (mi == transactionToMemEventMap.end()) {
    dbg.fatal(CALL_INFO, -1, "Vault %d can't find transaction %p\n", vaultID,(void*)addr);
  }
  MemEvent *parentEvent = mi->second;
  MemEvent *event = parentEvent->makeResponse();
  //printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
  //assert(bp.burstLength == parentEvent->getSize());

  // write the data to the backing store
  //for ( size_t i = 0 ; i < bp.burstLength ; i++ ) {
    //memBuffer[getInternalAddress(bp.physicalAddress + i)] = parentEvent->getPayload()[i];
  //}

  // send event
  m_memChan->send(event);
  dbg.output(CALL_INFO, "VaultSimC %d: write req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);
  // delete old event
  delete parentEvent;
  transactionToMemEventMap.erase(mi);
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
/**

*/

bool VaultSimC::clock_phx(Cycle_t currentCycle) {

  m_memorySystem->Update();

  SST::Event *e = 0;
  while ((e = m_memChan->recv())) {
    // process incoming events
    MemEvent *event  = dynamic_cast<MemEvent*>(e);
    if (event == NULL) {
      dbg.fatal(CALL_INFO, -1, "Vault %d got bad event\n", vaultID);
    }

    //TransactionType transType = convertType( event->getCmd() );
    //dbg.output(CALL_INFO, "transType=%d addr=%p\n", transType, (void*)event->getAddr());

    // save the memEvent eventID based on addr so we can respond to it correctly
    transactionToMemEventMap.insert( pair<uint64_t, MemHierarchy::MemEvent*>(event->getAddr(), event) );

    bool is_write = false;
    switch(event->getCmd()) {
      case SST::MemHierarchy::GetS:
        is_write = false;
        break;
      case SST::MemHierarchy::GetSEx:
      case SST::MemHierarchy::GetX:
        is_write = true;
        break;
      default: 
        //_abort(VaultSimC,"Tried to convert unknown memEvent request type (%d) to PHXSim transaction type \n", event->getCmd());
        break;
    }

    // add to the Q
    transaction_c transaction (is_write, event->getAddr());
    if (event->getHMCInstType() == HMC_NONE) {
      transaction.reset_atomic();
      dbg.output(CALL_INFO, "VaultSimC %d got a req for %p in clock=%lu (%lu %d)\n", vaultID, (void*)event->getAddr(), currentCycle, event->getID().first, event->getID().second);
    }
    else {
      transaction.set_atomic();
      transaction.reset_is_write(); //FIXME: all hmc ops come as read. true?
      transaction.set_HMCOpType(event->getHMCInstType());
      dbg.output(CALL_INFO, "VaultSimC %d got an atomic req for %p of type %s in clock=%lu\n", vaultID, (void *)transaction.get_addr(), transaction.get_HMCOpType_str(), currentCycle);
    }
    m_transQ.push_back(transaction);
  }

  int ret = 1; 
  while (!m_transQ.empty() && ret) {
    // send events off for processing
    transaction_c transaction = m_transQ.front();
    if ((ret = m_memorySystem->AddTransaction(transaction))) {
      dbg.output(CALL_INFO, "VaultSimC %d AddTransaction %s succeeded %p in clock=%lu\n", vaultID, transaction.is_write() ? "write" : "read", (void *)transaction.get_addr(), currentCycle);
      m_transQ.pop_front();
    } else {
      dbg.output(CALL_INFO, "VaultSimC %d AddTransaction %s  failed %p in clock=%lu\n", vaultID, transaction.is_write() ? "write" : "read", (void *)transaction.get_addr(), currentCycle);
      ret = 0;
    }
  }

  memOutStat->addData(transactionToMemEventMap.size());
  return false;
}


extern "C" {
  VaultSimC* VaultSimCAllocComponent(SST::ComponentId_t id,  SST::Params& params) {
    return new VaultSimC(id, params);
  }
}

