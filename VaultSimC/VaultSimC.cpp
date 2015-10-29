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
#include <sys/mman.h>
#include <sst/core/serialization.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "VaultSimC.h"

static size_t MEMSIZE = size_t(4096)*size_t(1024*1024);

using namespace SST::MemHierarchy;

VaultSimC::VaultSimC(ComponentId_t id, Params& params) : IntrospectedComponent( id ), numOutstanding(0) 
{
    int debugLevel = params.find_integer("debug_level", 0);
    dbg.init("@R:VaultSim::@p():@l " + getName() + ": ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));  
    if(debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    std::string frequency = "1.0 GHz";
    frequency = params.find_string("clock", "1.0 Ghz");

    // number of bits to determine vault address
    int nv2 = params.find_integer("numVaults2", -1);
    if (-1 == nv2) {
        dbg.fatal(CALL_INFO, -1, "numVaults2 (number of bits to determine vault address) not set! \
                Should be log2(number of vaults per cube)\n");
    } else {
        numVaults2 = nv2;
    }

    memChan = configureLink("bus", "1 ns");

    int vid = params.find_integer("VaultID", -1);
    if (-1 == vid) {
        dbg.fatal(CALL_INFO, -1, "VaultID not set\n");
    } else {
        vaultID = vid;
    }

    bool statistics = params.find_integer("statistics", -1);
    if (-1 == vid) {
        dbg.fatal(CALL_INFO, -1, "statistics not set\n");
    }

    registerClock(frequency, new Clock::Handler<VaultSimC>(this, &VaultSimC::clock));

    // Phx Library configuratoin
    memorySystem = new Vault(vaultID, &dbg, statistics, frequency);
    if (!memorySystem) {
        dbg.fatal(CALL_INFO, -1, "MemorySystem() failed\n");
    }

    CallbackBase<void, unsigned, uint64_t, uint64_t> *readDataCB = 
        new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::readData);
    CallbackBase<void, unsigned, uint64_t, uint64_t> *writeDataCB = 
        new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::writeData);

    dbg.output(CALL_INFO, "VaultSimC %u: made vault %u\n", vaultID, vaultID);
    memorySystem->registerCallback(readDataCB, writeDataCB);

    // setup backing store
    size_t memSize = MEMSIZE;
    memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (NULL == memBuffer) {
        dbg.fatal(CALL_INFO, -1, "Unable to MMAP backing store for Memory\n");
    }

    memOutStat = registerStatistic<uint64_t>("Mem_Outstanding","1");
}

void VaultSimC::finish() 
{
    munmap(memBuffer, MEMSIZE);
    memorySystem->finish();
}

void VaultSimC::init(unsigned int phase) 
{
    SST::Event *ev = NULL;
    while ((ev = memChan->recvInitData()) != NULL) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if (NULL != me) {
            /* Push data to memory */
            if (me->isWriteback()) {
                uint32_t chunkSize = (1 << VAULT_SHIFT);
                if (me->getSize() > chunkSize) {
                    dbg.fatal(CALL_INFO, -1, "vault got too large init\n");
                }
                
                for (size_t i = 0; i < me->getSize(); i++) {
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

void VaultSimC::readData(unsigned id, uint64_t addr, uint64_t clockcycle) 
{
    t2MEMap_t::iterator mi = transactionToMemEventMap.find(addr);
    if (mi == transactionToMemEventMap.end()) {
        dbg.fatal(CALL_INFO, -1, "Vault %d can't find transaction %p\n", vaultID,(void*)addr);
    }

    MemEvent *parentEvent = mi->second;
    MemEvent *event = parentEvent->makeResponse();
    // printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
    // assert(bp.burstLength == parentEvent->getSize());

    // copy data from backing store to event
    // event->setSize(bp.burstLength);
    // for (size_t i = 0; i < event->getSize(); i++) {
    //     event->getPayload()[i] = memBuffer[getInternalAddress(bp.physicalAddress + i)];
    // }

    memChan->send(event);
    dbg.output(CALL_INFO, "VaultSimC %d: read req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);

    // delete old event
    delete parentEvent;

    transactionToMemEventMap.erase(mi);
}

void VaultSimC::writeData(unsigned id, uint64_t addr, uint64_t clockcycle)
{
    t2MEMap_t::iterator mi = transactionToMemEventMap.find(addr);
    if (mi == transactionToMemEventMap.end()) {
        dbg.fatal(CALL_INFO, -1, "Vault %d can't find transaction %p\n", vaultID,(void*)addr);
    }

    MemEvent *parentEvent = mi->second;
    MemEvent *event = parentEvent->makeResponse();
    // printf("Burst length is %d. is that 64?: %s %d\n",bp.burstLength, __FILE__, __LINE__);
    // assert(bp.burstLength == parentEvent->getSize());

    // write the data to the backing store
    // for ( size_t i = 0 ; i < bp.burstLength ; i++ ) {
    //     memBuffer[getInternalAddress(bp.physicalAddress + i)] = parentEvent->getPayload()[i];
    // }

    // send event
    memChan->send(event);
    dbg.output(CALL_INFO, "VaultSimC %d: write req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);

    // delete old event
    delete parentEvent;

    transactionToMemEventMap.erase(mi);
}

bool VaultSimC::clock(Cycle_t currentCycle) 
{
    memorySystem->update();

    SST::Event *ev = NULL;
    while ((ev = memChan->recv())) {
        // process incoming events
        MemEvent *event  = dynamic_cast<MemEvent*>(ev);
        if (NULL == event) {
            dbg.fatal(CALL_INFO, -1, "Vault %d got bad event\n", vaultID);
        }

        //TransactionType transType = convertType( event->getCmd() );
        //dbg.output(CALL_INFO, "transType=%d addr=%p\n", transType, (void*)event->getAddr());

        // save the memEvent eventID based on addr so we can respond to it correctly
        transactionToMemEventMap.insert(pair<uint64_t, MemHierarchy::MemEvent*>(event->getAddr(), event));

        bool isWrite = false;
        switch(event->getCmd()) {
        case SST::MemHierarchy::GetS:
            isWrite = false;
            break;
        case SST::MemHierarchy::GetSEx:
        case SST::MemHierarchy::GetX:
            isWrite = true;
            break;
        default: 
            // _abort(VaultSimC,"Tried to convert unknown memEvent request type (%d) to PHXSim transaction type \n", event->getCmd());
            break;
        }

        // add to the Q
        transaction_c transaction (isWrite, event->getAddr());
        if (event->getHMCInstType() == HMC_NONE) {
            transaction.resetAtomic();
            dbg.output(CALL_INFO, "VaultSimC %d got a req for %p in clock=%lu (%lu %d)\n", 
                    vaultID, (void*)event->getAddr(), currentCycle, event->getID().first, event->getID().second);
        } else {
            transaction.setAtomic();
            transaction.resetIsWrite(); //FIXME: all hmc ops come as read. true?
            transaction.setHmcOpType(event->getHMCInstType());
            dbg.output(CALL_INFO, "VaultSimC %d got an atomic req for %p of type %s in clock=%lu\n", 
                    vaultID, (void *)transaction.getAddr(), transaction.getHmcOpTypeStr(), currentCycle);
        }
        transQ.push_back(transaction);
    }

    bool ret = true; 
    while (!transQ.empty() && ret) {
        // send events off for processing
        transaction_c transaction = transQ.front();
        if ((ret = memorySystem->addTransaction(transaction))) {
            dbg.output(CALL_INFO, "VaultSimC %d AddTransaction %s succeeded %p in clock=%lu\n", 
                    vaultID, transaction.getIsWrite() ? "write" : "read", (void *)transaction.getAddr(), currentCycle);
            transQ.pop_front();
        } else {
            dbg.output(CALL_INFO, "VaultSimC %d AddTransaction %s  failed %p in clock=%lu\n", 
                    vaultID, transaction.getIsWrite() ? "write" : "read", (void *)transaction.getAddr(), currentCycle);
            ret = false;
        }
    }

    memOutStat->addData(transactionToMemEventMap.size());
    return false;
}


extern "C" {
    VaultSimC* VaultSimCAllocComponent(SST::ComponentId_t id,  SST::Params& params) 
    {
        return new VaultSimC(id, params);
    }
}

