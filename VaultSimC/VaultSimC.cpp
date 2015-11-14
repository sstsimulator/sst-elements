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

#include <sys/mman.h>

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "VaultSimC.h"

static size_t MEMSIZE = size_t(4096)*size_t(1024*1024);

using namespace SST::MemHierarchy;

VaultSimC::VaultSimC(ComponentId_t id, Params& params) : IntrospectedComponent( id ), numOutstanding(0) 
{
    out.init("", 0, 0, Output::STDOUT);

    int debugLevel = params.find_integer("debug_level", 0);
    if (debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg.init("@R:VaultSim::@p():@l " + getName() + ": ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));

    statsFormat = params.find_integer("statistics_format", 0);

    std::string frequency = "1.0 GHz";
    frequency = params.find_string("clock", "1.0 Ghz");

    // number of bits to determine vault address
    int nv2 = params.find_integer("numVaults2", -1);
    if (-1 == nv2) 
        dbg.fatal(CALL_INFO, -1, "numVaults2 not set! should be log2(number of vaults per cube)\n");
    numVaults2 = nv2;

    memChan = configureLink("bus");     //link delay is configurable by python scripts

    int vid = params.find_integer("vault.id", -1);
    if (-1 == vid) 
        dbg.fatal(CALL_INFO, -1, "vault.id not set\n");
    vaultID = vid;

    registerClock(frequency, new Clock::Handler<VaultSimC>(this, &VaultSimC::clock));

    Params vaultParams = params.find_prefix_params("vault.");
    memorySystem = dynamic_cast<Vault*>(loadSubComponent("VaultSimC.Vault", this, vaultParams));
    if (!memorySystem) 
        dbg.fatal(CALL_INFO, -1, "Unable to load Vault as sub-component\n");

    CallbackBase<void, unsigned, uint64_t, uint64_t> *readDataCB = 
        new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::readData);
    CallbackBase<void, unsigned, uint64_t, uint64_t> *writeDataCB = 
        new Callback<VaultSimC, void, unsigned, uint64_t, uint64_t>(this, &VaultSimC::writeData);

    memorySystem->registerCallback(readDataCB, writeDataCB);
    dbg.output(CALL_INFO, "VaultSimC %u: made vault %u\n", vaultID, vaultID);
}

void VaultSimC::finish() 
{
    dbg.debug(_INFO_, "VaultSim %d finished\n", vaultID);
    memorySystem->finish();
}

void VaultSimC::readData(unsigned id, uint64_t addr, uint64_t clockcycle) 
{
    t2MEMap_t::iterator mi = transactionToMemEventMap.find(addr);
    if (mi == transactionToMemEventMap.end()) {
        dbg.fatal(CALL_INFO, -1, "Vault %d can't find transaction %p\n", vaultID,(void*)addr);
    }

    MemEvent *parentEvent = mi->second;
    MemEvent *event = parentEvent->makeResponse();

    memChan->send(event);
    dbg.debug(_L5_, "VaultSimC %d: read req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);

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

    // send event
    memChan->send(event);
    dbg.debug(_L5_, "VaultSimC %d: write req %p answered in clock=%lu\n", vaultID, (void*)addr, clockcycle);

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

        #ifdef USE_VAULTSIM_HMC
        HMC_Type HMCTypeEvent = event->getHMCInstType();
        if (HMCTypeEvent == HMC_NONE || HMCTypeEvent == HMC_CANDIDATE) {
            transaction.resetAtomic();
            dbg.debug(_L6_, "VaultSimC %d got a req for %p in clock=%lu (%lu %d)\n", 
                    vaultID, (void*)event->getAddr(), currentCycle, event->getID().first, event->getID().second);
        } 
        else {
            transaction.setAtomic();
            transaction.resetIsWrite(); //FIXME: all hmc ops come as read. true?
            transaction.setHmcOpType(event->getHMCInstType());
            dbg.debug(_L6_, "VaultSimC %d got an atomic req for %p of type %s in clock=%lu\n", 
                    vaultID, (void *)transaction.getAddr(), transaction.getHmcOpTypeStr(), currentCycle);
        }
        #else
        transaction.resetAtomic();
        dbg.debug(_L6_, "VaultSimC %d got a req for %p in clock=%lu (%lu %d)\n", 
                vaultID, (void*)event->getAddr(), currentCycle, event->getID().first, event->getID().second);
        #endif
        
        transQ.push_back(transaction);
    }

    bool ret = true; 
    while (!transQ.empty() && ret) {
        // send events off for processing
        transaction_c transaction = transQ.front();
        if ((ret = memorySystem->addTransaction(transaction))) {
            dbg.debug(_L6_, "VaultSimC %d AddTransaction %s succeeded %p in clock=%lu\n", 
                    vaultID, transaction.getIsWrite() ? "write" : "read", (void *)transaction.getAddr(), currentCycle);
            transQ.pop_front();
        } else {
            dbg.debug(_L6_, "VaultSimC %d AddTransaction %s  failed %p in clock=%lu\n", 
                    vaultID, transaction.getIsWrite() ? "write" : "read", (void *)transaction.getAddr(), currentCycle);
            ret = false;
        }
    }

    return false;
}


extern "C" {
    VaultSimC* VaultSimCAllocComponent(SST::ComponentId_t id,  SST::Params& params) 
    {
        return new VaultSimC(id, params);
    }
}

