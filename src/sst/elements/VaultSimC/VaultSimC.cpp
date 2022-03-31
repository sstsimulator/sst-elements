// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <VaultSimC.h>

#include <sys/mman.h>

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/elements/VaultSimC/memReqEvent.h>

#include <vaultGlobals.h>

//typedef  VaultCompleteFn;

static size_t MEMSIZE = size_t(4096)*size_t(1024*1024);

using namespace SST::VaultSim;
using namespace SST::MemHierarchy;

VaultSimC::VaultSimC( ComponentId_t id, Params& params ) :
    Component( id ), numOutstanding(0) {
    dbg.init("@R:Vault::@p():@l " + getName() + ": ", 0, 0,
             (Output::output_location_t)params.find<uint32_t>("debug", 0));

    std::string frequency = "1.0 GHz";
    frequency = params.find<std::string>("clock", "1.0 Ghz");

    // number of bits to determin vault address
    int nv2 = params.find( "numVaults2", -1 );
    if ( -1 == nv2) {
        dbg.fatal(CALL_INFO, -1,"numVaults2 (number of bits to determine vault "
               "address) not set! Should be log2(number of vaults per cube)\n");
    } else {
        numVaults2 = nv2;
    }

    //DBG("new id=%lu\n",id);

    m_memChan = configureLink( "bus", "1 ns" );

    int vid = params.find("VaultID", -1);
    if ( -1 == vid) {
        dbg.fatal(CALL_INFO, -1,"not VaultID Set\n");
    } else {
        vaultID = vid;
    }

    // Configuration if we're not using Phx Library

    registerClock( frequency,
                   new Clock::Handler<VaultSimC>(this, &VaultSimC::clock) );

    std::string delay = "40ns";
    delay = params.find<std::string>("delay", "40ns");
    delayLine = configureSelfLink( "delayLine", delay);

    // setup backing store
    size_t memSize = MEMSIZE;
    /*memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE,
                               MAP_PRIVATE|MAP_ANON, -1, 0);
    if ( !memBuffer ) {
        dbg.fatal(CALL_INFO, -1, "Unable to MMAP backing store for Memory\n");
    }
*/
    memOutStat = registerStatistic<uint64_t>("Mem_Outstanding","1");
}

    int VaultSimC::Finish()
{
    //munmap(memBuffer, MEMSIZE);

    return 0;
}

void VaultSimC::init(unsigned int phase)
{
    SST::Event *ev = NULL;
    while ( (ev = m_memChan->recvInitData()) != NULL ) {
        assert(0);
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            /* Push data to memory */
            if ( me->isWriteback() ) {
                //printf("Vault received Init Command: of size 0x%x at addr 0x%lx\n", me->getSize(), me->getAddr() );
                uint32_t chunkSize = (1 << VAULT_SHIFT);
                if (me->getSize() > chunkSize)
                    dbg.fatal(CALL_INFO, -1, "vault got too large init\n");
                /*for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
                    memBuffer[getInternalAddress(me->getAddr() + i)] =
                        me->getPayload()[i];
                }*/
            } else {
                dbg.fatal(CALL_INFO, -1, "vault got bad init command\n");
            }
        } else {
            dbg.fatal(CALL_INFO, -1, "vault got bad init event\n");
        }
        delete ev;
    }
}

bool VaultSimC::clock( Cycle_t current ) {
    SST::Event *e = 0;
    while (NULL != (e = m_memChan->recv())) {
        // process incoming events
        MemReqEvent *event  = dynamic_cast<MemReqEvent*>(e);
        if (NULL == event) {
            dbg.fatal(CALL_INFO, -1, "vault got bad event\n");
        }
        // handle event...
        delayLine->send(1, event);
        numOutstanding++;
    }

    e = 0;
    while (NULL != (e = delayLine->recv())) {
        // process returned events
        MemReqEvent *event  = dynamic_cast<MemReqEvent*>(e);
        if (NULL == event) {
            dbg.fatal(CALL_INFO, -1, "vault got bad event from delay line\n");
        } else {
            MemRespEvent *respEvent = new MemRespEvent(
				event->getReqId(), event->getAddr(), event->getFlags() );

            m_memChan->send(respEvent);
            numOutstanding--;
            delete event;
        }
    }

    memOutStat->addData(numOutstanding);
    return false;
}


