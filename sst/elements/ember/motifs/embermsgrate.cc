// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "embermsgrate.h"
#include "embergettimeev.h"
#include "emberbarrierev.h"
#include "emberisendev.h"
#include "emberirecvev.h"
#include "emberwaitev.h"
#include "emberwaitallev.h"

using namespace SST::Ember;

EmberMsgRateGenerator::EmberMsgRateGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params),
    m_startTime( 0 ),
    m_stopTime( 0 ),
    m_totalTime( 0 ),
    m_output( new Output )
{

	msgSize = (uint32_t) params.find_integer("msgSize", 0);
	numMsgs = (uint32_t) params.find_integer("numMsgs", 128);
	iterations = (uint32_t) params.find_integer("iterations", 1);
}

void EmberMsgRateGenerator::configureEnvironment(const SST::Output* output, uint32_t pRank, uint32_t worldSize) {
	rank = pRank;
    size = worldSize;

    reqs.resize( numMsgs );
    int jobId = 0;
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:%s::MsgRate::@p():@l ",jobId,pRank,output->getPrefix().c_str());
    m_output->init( buffer, 1, 0, output->getOutputLocation() ); 
    
    if ( 0 == rank ) {
        output->output("iterations=%d numMsgs=%d msgSize=%d\n",
            iterations,numMsgs,msgSize);
    }
}

void EmberMsgRateGenerator::finish(const SST::Output* output) {
    output->verbose(CALL_INFO, 2, 0, "Generator finishing\n");
}


void EmberMsgRateGenerator::generate(const SST::Output* output, const uint32_t phase,
	std::queue<EmberEvent*>* evQ) {

    m_totalTime += m_stopTime - m_startTime;
    if ( phase < iterations ) {
        if ( 0 == rank ) {
            evQ->push( new EmberBarrierEvent((Communicator)0) );
            evQ->push( new EmberGetTimeEvent( &m_startTime ) );
            for ( unsigned int i = 0; i < numMsgs; i++ ) {
		        evQ->push( new EmberISendEvent((uint32_t) 1, msgSize, 0,
                                    (Communicator) 0, &reqs[i]) );
            }

            evQ->push( new EmberWaitallEvent( numMsgs, &reqs[0], false ) ); 
            evQ->push( new EmberGetTimeEvent( &m_stopTime ) );
        } else {

            for ( unsigned int i = 0; i < numMsgs; i++ ) {
		        evQ->push( new EmberIRecvEvent((uint32_t) 0, msgSize, 0,
                                    (Communicator) 0, &reqs[i]) );
            }

            evQ->push( new EmberBarrierEvent((Communicator)0) );
            evQ->push( new EmberGetTimeEvent( &m_startTime ) );
            evQ->push( new EmberWaitallEvent( numMsgs, &reqs[0], false ) ); 
            evQ->push( new EmberGetTimeEvent( &m_stopTime ) );
        }
    } else {
        int totalMsgs = numMsgs * iterations;
        double tmp = (double) m_totalTime / 1000000000.0;
        output->output("%s: totalTime %.6f sec, %.3f msg/sec, %.3f MB/s\n",
                        0 == rank ? "Send" : "Recv",
                        tmp,
                        totalMsgs / tmp,
                        (totalMsgs*msgSize/1000000)/tmp );
        output->output("schedule FinalizeEvent\n");
        evQ->push(new EmberFinalizeEvent() );
    }
}
