// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"
#include <logicLayer.h>

#include <sst/core/interfaces/stringEvent.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

#define DBG(fmt, args...) m_dbg.write( "%s():%d: " fmt, __FUNCTION__, __LINE__, ##args)

logicLayer::logicLayer(ComponentId_t id, Params& params) : IntrospectedComponent( id ), memOps(0) 
{
    int debugLevel = params.find_integer("debug_level", 0);
    dbg.init("@R:LogicLayer::@p():@l " + getName() + ": ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    if(debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    std::string frequency = "2.2 GHz";
    frequency = params.find_string("clock", "2.2 Ghz");

    int ident = params.find_integer("llID", -1);
    if (-1 == ident) { dbg.fatal(CALL_INFO, -1, "no llID defined\n"); }
    llID = ident;
    dbg.debug(_INFO_, "Making LogicLayer with id=%d & clock=%s\n", llID, frequency.c_str());

    bwLimit = params.find_integer("bwlimit", -1);
    if (-1 == bwLimit) { dbg.fatal(CALL_INFO, -1, " no <bwlimit> tag defined for logiclayer\n"); }

    int mask = params.find_integer("LL_MASK", -1);
    if (-1 == mask) { dbg.fatal(CALL_INFO, -1, " no <LL_MASK> tag defined for logiclayer\n"); }
    LL_MASK = mask;

    bool terminal = params.find_integer("terminal", 0);
    int numVaults = params.find_integer("vaults", -1);
    if (-1 != numVaults) {
        // connect up our vaults
        for (int i = 0; i < numVaults; ++i) {
            char bus_name[50];
            snprintf(bus_name, 50, "bus_%d", i);
            memChan_t *chan = configureLink(bus_name, "1 ns");
            if (chan) {
                memChans.push_back(chan);
                dbg.debug(_INFO_, "\tConnected %s\n", bus_name);
            } else {
                dbg.fatal(CALL_INFO, -1, " could not find %s\n", bus_name);
            }
        }
        printf(" Connected %d Vaults\n", numVaults);
    } else {
        dbg.fatal(CALL_INFO, -1, " no <vaults> tag defined for LogicLayer\n");
    }

    // connect chain
    toCPU = configureLink( "toCPU");
    if (!terminal) {
        toMem = configureLink( "toMem");
    } else {
        toMem = NULL;
    }

    registerClock(frequency, new Clock::Handler<logicLayer>(this, &logicLayer::clock));
    dbg.debug(_INFO_, "Made LogicLayer %d toMem:%p toCPU:%p\n", llID, toMem, toCPU);

    // register bandwidth stats
    bwUsedToCpu[0] = registerStatistic<uint64_t>("BW_recv_from_CPU", "1");  
    bwUsedToCpu[1] = registerStatistic<uint64_t>("BW_send_to_CPU", "1");
    bwUsedToMem[0] = registerStatistic<uint64_t>("BW_recv_from_Mem", "1");
    bwUsedToMem[1] = registerStatistic<uint64_t>("BW_send_to_Mem", "1");
}

void logicLayer::finish() 
{
    dbg.debug(_INFO_, "Logic Layer %d completed %lld ops\n", llID, memOps);
}

void logicLayer::init(unsigned int phase) 
{
    // tell the bus (or whaterver) that we are here. only the first one
    // in the chain should report, so every one sends towards the cpu,
    // but only the first one will arrive.
    if (!phase) {
        toCPU->sendInitData(new SST::Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
    }

    // rec data events from the direction of the cpu
    SST::Event *ev = NULL;
    while ((ev = toCPU->recvInitData())) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if (NULL != me) {
            /* Push data to memory */
            if (me->isWriteback()) {
                uint32_t chunkSize = (1 << VAULT_SHIFT);
                if (me->getSize() > chunkSize) {
                    // may need to break request up in to 256 byte chunks (minimal
                    // vault width)
                    int numNewEv = (me->getSize() / chunkSize) + 1;
                    uint8_t *inData = &(me->getPayload()[0]);
                    SST::MemHierarchy::Addr addr = me->getAddr();
                    for (int i = 0; i < numNewEv; ++i) {
                        // make new event
                        MemEvent *newEv = new MemEvent(this, addr, me->getBaseAddr(), me->getCmd());

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
                            unsigned int vaultID = (newEv->getAddr() >> VAULT_SHIFT) % memChans.size();
                            memChans[vaultID]->sendInitData(newEv);      
                        } else {
                            // send down the chain
                            toMem->sendInitData(newEv);
                        }
                    }
                    delete ev;
                } else {
                    if (isOurs(me->getAddr())) {
                        // send to the vault
                        unsigned int vaultID = (me->getAddr() >> 8) % memChans.size();
                        memChans[vaultID]->sendInitData(me);      
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

bool logicLayer::clock(Cycle_t current) 
{
    SST::Event* ev = NULL;

    int tm[2] = {0,0}; //recv send
    int tc[2] = {0,0};

    // check for events from the CPU
    while ((tc[0] < bwLimit) && (ev = toCPU->recv())) {
        MemEvent *event  = dynamic_cast<MemEvent*>(ev);
        dbg.debug(_L4_, "ll%d got req for %p (%" PRIu64 " %d)\n", llID, (void*)event->getAddr(), event->getID().first, event->getID().second);
        if (NULL == event) { 
            dbg.fatal(CALL_INFO, -1, "logic layer got bad event\n"); 
        }

        tc[0]++;
        if (isOurs(event->getAddr())) {
            // it is ours!
            unsigned int vaultID = (event->getAddr() >> VAULT_SHIFT) % memChans.size();
            dbg.debug(_L4_, "ll%d sends %p to vault @ %" PRIu64 "\n", llID, event, current);
            memChans[vaultID]->send(event);      
        } else {
            // it is not ours
            if (NULL != toMem) {
                toMem->send( event );
                tm[1]++;
                dbg.debug(_L4_, "ll%d sends %p to next\n", llID, event);
            } else {
                printf("ll%d not sure what to do with %p...\n", llID, event);
            }
        }
    }

    // check for events from the memory chain
    if (NULL != toMem) {
        while ((tm[0] < bwLimit) && (ev = toMem->recv())) {
            MemEvent *event  = dynamic_cast<MemEvent*>(ev);
            if (NULL == event) { 
                dbg.fatal(CALL_INFO, -1, "logic layer got bad event\n"); 
            }

            tm[0]++;
            // pass along to the CPU
            dbg.debug(_L4_, "ll%d sends %p towards cpu (%" PRIu64 " %d)\n", llID, event, event->getID().first, event->getID().second);
            toCPU->send(event);
            tc[1]++;
        }
    }

    // check for incoming events from the vaults
    for (memChans_t::iterator i = memChans.begin(); i != memChans.end(); ++i) {
        memChan_t *m_memChan = *i;
        while ((ev = m_memChan->recv())) {
            MemEvent *event  = dynamic_cast<MemEvent*>(ev);
            if (event == NULL) {
                dbg.fatal(CALL_INFO, -1, "logic layer got bad event from vaults\n");
            }

            dbg.debug(_L4_, "ll%d got an event %p from vault @ %" PRIu64 ", sends " "towards cpu\n", llID, event, current);
            // send to CPU
            memOps++;
            toCPU->send(event);
            tc[1]++;
        }    
    }

    if (tm[0] > bwLimit || tm[1] > bwLimit || tc[0] > bwLimit || tc[1] > bwLimit) {
        //dbg.output(CALL_INFO, "ll%d Bandwdith: %d %d %d %d\n", llID, tm[0], tm[1], tc[0], tc[1]);
    }

    bwUsedToCpu[0]->addData(tc[0]);
    bwUsedToCpu[1]->addData(tc[1]);
    bwUsedToMem[0]->addData(tm[0]);
    bwUsedToMem[1]->addData(tm[1]);

    return false;
}

extern "C" {
    Component* create_logicLayer( SST::ComponentId_t id,  SST::Params& params ) 
    {
        return new logicLayer( id, params );
    }
}
