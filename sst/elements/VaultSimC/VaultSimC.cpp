// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <VaultSimC.h>
#include <vaultGlobals.h>
#include <sys/mman.h>
#include <sstream> // for stringstream() so I don't have to use atoi()

#define DBG( fmt, args... )m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)
//typedef  VaultCompleteFn; 

static size_t MEMSIZE = size_t(4096)*size_t(1024*1024);

using namespace SST::Interfaces;

VaultSimC::VaultSimC( ComponentId_t id, Params_t& params ) :
	IntrospectedComponent( id ),
	m_printStats("no"),
	m_dbg( *new Log< VAULTSIMC_DBG >( "VaultSimC::", false ) ),
	m_log( *new Log< >( "INFO VaultSimC: ", false ) )
{
  std::string frequency = "2.2 GHz";
  if ( params.find( "info" ) != params.end() ) {
    if ( params[ "info" ].compare( "yes" ) == 0 ) {
      m_log.enable();
    }
  }
  
  if ( params.find( "debug" ) != params.end() ) {
    if ( params[ "debug" ].compare( "yes" ) == 0 ) {
      m_dbg.enable();
    }
  }
  
  if ( params.find( "clock" ) != params.end() ) {
    frequency = params["clock"];
  }
  
  // number of bits to determin vault address
  if ( params.find( "numVaults2" ) != params.end() ) {
    stringstream(params["numVaults2"]) >> numVaults2;
  } else {
    _abort(VaultSimC,"numVaults2 (number of bits to determine vault address) not set! Should be log2(number of vaults per cube)\n");
  }

  DBG("new id=%lu\n",id);
  
  m_memChan = configureLink( "bus", "1 ns" );
  
  Params_t::iterator it = params.begin();
  while( it != params.end() ) {
    DBG("key=%s value=%s\n", it->first.c_str(),it->second.c_str());
    
    if ( ! it->first.compare("VaultID") ) {
      stringstream(it->second) >> vaultID;
    }
    ++it;
  }
  
  TimeConverter* tc = registerClock( frequency, new Clock::Handler<VaultSimC>(this, &VaultSimC::clock) );
  m_log.write("period %ld\n",tc->getFactor());
  
  m_memorySystem = new Vault(vaultID);
  if ( ! m_memorySystem ) 
    {
      _abort(VaultSimC,"MemorySystem() failed\n");
    }
  
  PHXSim::VaultCompleteCB* readDataCB = new PHXSim::Callback< VaultSimC, void, BusPacket, unsigned > (this, &VaultSimC::readData);
  PHXSim::VaultCompleteCB* writeDataCB = new PHXSim::Callback< VaultSimC, void, BusPacket, unsigned > (this, &VaultSimC::writeData);
  
  printf("made vault %u\n", vaultID);
  
  m_memorySystem->RegisterCallback(readDataCB, writeDataCB);

  // setup backing store
  size_t memSize = MEMSIZE;
  memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, 
			     MAP_PRIVATE|MAP_ANON, -1, 0);
  if ( !memBuffer ) {
    _abort(MemController, "Unable to MMAP backing store for Memory\n");
  }
}

int VaultSimC::Finish() 
{
  munmap(memBuffer, MEMSIZE);

  return 0;
}

void VaultSimC::init(unsigned int phase)
{
  SST::Event *ev = NULL;
  while ( (ev = m_memChan->recvInitData()) != NULL ) {
    MemEvent *me = dynamic_cast<MemEvent*>(ev);
    if ( me ) {
      /* Push data to memory */
      if ( me->getCmd() == WriteReq ) {
	//printf("Vault received Init Command: of size 0x%x at addr 0x%lx\n", me->getSize(), me->getAddr() );
	uint32_t chunkSize = (1 << VAULT_SHIFT);
	if (me->getSize() > chunkSize)
	  _abort(VaultSimC::init, "vault got too large init\n");
	for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
	  memBuffer[getInternalAddress(me->getAddr() + i)] =
	    me->getPayload()[i];
	}
      } else {
	_abort(VaultSimC::init, "vault got bad init command\n");
      }
    } else {
      _abort(VaultSimC::init, "vault got bad init event\n");
    }
    delete ev;
  }
}

void VaultSimC::readData(BusPacket bp, unsigned clockcycle)
{
  printf(" readData() id=%d addr=%#lx clock=%lu\n",bp.transactionID,bp.physicalAddress,clockcycle);
  
#ifdef STUPID_DEBUG
  static unsigned long reads_returned=0; 
  reads_returned++;
  printf("read %lu: id=%d addr=%#lx clock=%lu\n",reads_returned,bp.transactionID,bp.physicalAddress,clockcycle);
#endif
  

  t2MEMap_t::iterator mi = transactionToMemEventMap.find(bp.transactionID);
  if (mi == transactionToMemEventMap.end()) {
    _abort(VaultSimC::readData, "can't find transaction\n");
  }
  MemEvent *parentEvent = mi->second;
  MemEvent *event = parentEvent->makeResponse(this);
  printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
  //assert(bp.burstLength == parentEvent->getSize());
  
  // copy data from backing store to event
  event->setSize(bp.burstLength);
  for ( size_t i = 0 ; i < event->getSize() ; i++ ) {
    event->getPayload()[i] = memBuffer[getInternalAddress(bp.physicalAddress + i)];
  }
  m_memChan->send(event);
  delete parentEvent;
  transactionToMemEventMap.erase(mi);
}

void VaultSimC::writeData(BusPacket bp, unsigned clockcycle)
{
  DBG("id=%d addr=%#lx clock=%lu\n",bp.transactionID, bp.physicalAddress, clockcycle);
#ifdef STUPID_DEBUG
  static unsigned long writes_returned=0; 
  writes_returned++; 
  printf("write %lu: id=%d addr=%#lx clock=%lu\n",writes_returned, bp.transactionID, bp.physicalAddress, clockcycle);
#endif
  
  // create response
  t2MEMap_t::iterator mi = transactionToMemEventMap.find(bp.transactionID);
  if (mi == transactionToMemEventMap.end()) {
    _abort(VaultSimC::writeData, "can't find transaction\n");
  }
  MemEvent *parentEvent = mi->second;
  MemEvent *event = parentEvent->makeResponse(this);
  printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
  //assert(bp.burstLength == parentEvent->getSize());

  // write the data to the backing store
  for ( size_t i = 0 ; i < bp.burstLength ; i++ ) {
    memBuffer[getInternalAddress(bp.physicalAddress + i)] = parentEvent->getPayload()[i];
  }

  // send event
  m_memChan->send(event);
  // delete old event
  delete parentEvent;
}

bool VaultSimC::clock( Cycle_t current )
{
  
#ifdef STUPID_DEBUG
  static unsigned long reads_sent=0;	
  static unsigned long writes_sent=0;	
#endif
  
  m_memorySystem->Update();
  
  SST::Event *e = 0;
  while ((e = m_memChan->recv())) {
    MemEvent *event  = dynamic_cast<MemEvent*>(e);
    if (event == NULL) {
      _abort(VaultSimC::clock, "vault got bad event\n");
    }
    
    printf(" Vault %d got a req for %p (%d %d)\n", vaultID, event->getAddr(), event->getID().first, event->getID().second);
    DBG("got an event %x\n", event);
    
    TransactionType transType = convertType( event->getCmd() );
    printf("transType=%d addr=%#lx\n", transType, event->getAddr() );
    static unsigned id=0; 
    unsigned thisTransactionID = id++;

    // save the memEvent eventID so we can respond to it correctly
    transactionToMemEventMap[thisTransactionID] = event;

    // add to the Q
    m_transQ.push_back( Transaction( transType, 64, event->getAddr(), thisTransactionID));
  }
  int ret = 1; 
  while ( ! m_transQ.empty() && ret ) {
    if (  ( ret = m_memorySystem->AddTransaction( m_transQ.front() ) ) ) {
      printf(" addTransaction succeeded %#x\n", m_transQ.front().address);
      
#ifdef STUPID_DEBUG
      if (m_transQ.front().transactionType == PHXSim::DATA_WRITE)
	writes_sent++;
      else
	reads_sent++; 
      printf("addTransaction succeeded %#x (rd:%lu, wr:%lu)\n", m_transQ.front().address, reads_sent, writes_sent);
#endif
      
      m_transQ.pop_front();
    } else {
      printf("addTransaction failed\n");
    }
  }
  return false;
}

extern "C" {
	VaultSimC* VaultSimCAllocComponent( SST::ComponentId_t id,  SST::Component::Params_t& params )
	{
		return new VaultSimC( id, params );
	}
}

