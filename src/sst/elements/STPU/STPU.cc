// Copyright 2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018, NTESS
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
#include "STPU.h"

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include "../memHierarchy/memEvent.h"

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::STPUComponent;

STPU::STPU(ComponentId_t id, Params& params) :
    Component(id), state(IDLE), now(0)
{
    uint32_t outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("STPU:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // get parameters
    numNeurons = params.find<int>("neurons", 32);
    if (numNeurons <= 0) {
        out.fatal(CALL_INFO, -1,"number of neurons invalid\n");
    }    
    BWPpTic = params.find<int>("BWPperTic", 2);
    if (BWPpTic <= 0) {
        out.fatal(CALL_INFO, -1,"BWPperTic invalid\n");
    }    
    STSDispatch = params.find<int>("STSDispatch", 2);
    if (BWPpTic <= 0) {
        out.fatal(CALL_INFO, -1,"STSDispatch invalid\n");
    }  
    STSParallelism = params.find<int>("STSParallelism", 2);
    if (BWPpTic <= 0) {
        out.fatal(CALL_INFO, -1,"STSParallelism invalid\n");
    }    



    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // init memory
    memory = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");
    }
    memory->initialize("mem_link", new Interfaces::SimpleMem::Handler<STPU> (this, &STPU::handleEvent));

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<STPU>(this, &STPU::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);
}

STPU::STPU() : Component(-1)
{
	// for serialization only
}


void STPU::init() {
    using namespace Neuron_Loader_Types;
    
    // create STS units
    for(int i = 0; i < STSParallelism; ++i) {
        STSUnits.push_back(STS());
    }

    // initialize neurons
    neurons = new neuron[numNeurons];

    // <should read these in>
    // neurons
    for (int nrn_num=0;nrn_num<=8;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){1000,-2.0,0.0});
    for (int nrn_num=9;nrn_num<=11;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){ 750,-2.0,0.0});
    for (int nrn_num=12;nrn_num<=12;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){1000,-2.0,0.0});
    for (int nrn_num=13;nrn_num<=15;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){ 750,-2.0,0.0});
    for (int nrn_num=16;nrn_num<=23;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){ 500,-2.0,0.0});
    for (int nrn_num=24;nrn_num<=31;nrn_num++)
        neurons[nrn_num].configure((T_NctFl){1500,-2.0,0.0});

    // brain wave pulses
    int bwpl_len = 16;
    Ctrl_And_Stat_Types::T_BwpFl* bwpl = (Ctrl_And_Stat_Types::T_BwpFl*)calloc(bwpl_len,sizeof(Ctrl_And_Stat_Types::T_BwpFl));
    bwpl[0]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,0};
    bwpl[1]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,1};
    bwpl[2]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,2};
    bwpl[3]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,3};
    bwpl[4]  = (Ctrl_And_Stat_Types::T_BwpFl){1,4,0};
    bwpl[5]  = (Ctrl_And_Stat_Types::T_BwpFl){1,4,1};
    bwpl[6]  = (Ctrl_And_Stat_Types::T_BwpFl){1,4,2};
    bwpl[7]  = (Ctrl_And_Stat_Types::T_BwpFl){1,4,3};
    bwpl[8]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,4};
    bwpl[9]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,5};
    bwpl[10] = (Ctrl_And_Stat_Types::T_BwpFl){1,0,6};
    bwpl[11] = (Ctrl_And_Stat_Types::T_BwpFl){1,0,7};
    bwpl[12] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,4};
    bwpl[13] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,5};
    bwpl[14] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,6};
    bwpl[15] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,7};
    for (int i = 0; i < bwpl_len; ++i) {
        BWPs.insert(std::pair<uint,Ctrl_And_Stat_Types::T_BwpFl>(bwpl[i].TmpSft, bwpl[i]));
    }
}

void STPU::init(unsigned int phase)
{
    memory->init(phase);
}

void STPU::handleEvent(Interfaces::SimpleMem::Request * req)
{
    std::map<uint64_t, SimTime_t>::iterator i = requests.find(req->id);
    if (i == requests.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->id);
    } else {
        requests.erase(i);
        // handle event
    }
    delete req;
}

void STPU::deliver(float val, int targetN, int time) {
    if(targetN < numNeurons) {
        neurons[targetN].deliverSpike(val, time);
    } else {
        out.fatal(CALL_INFO, -1,"Invalid Neuron Address\n");
    }
}

// returns true if no more to deliver
bool STPU::deliverBWPs() {
    int tries = BWPpTic;

    while (tries > 0) {
        BWPBuf_t::iterator i = BWPs.find(now);
        if (i != BWPs.end()) {
            // deliver it
            const Ctrl_And_Stat_Types::T_BwpFl &pulse = i->second;
            deliver(pulse.InpValFl, pulse.InpNrn, pulse.TmpSft);
            BWPs.erase(i);
        } else {
            return true;
        }
        tries--;
    }

    if (BWPs.find(now) == BWPs.end()) {
        return true;
    } else {
        return false;
    }
}

// find a free STS unit to assign the spike to
void STPU::assignSTS() {
    int tries = STSDispatch;

    while(tries > 0 && !firedNeurons.empty()) {
        // try to find a free unit
        int freeSTS = STSParallelism;
        for(auto &e: STSUnits) {
            if (e.isFree()) {
                e.assign(firedNeurons.front());
                firedNeurons.pop_front();
            } else {
                freeSTS--;
            }
        }
        if (0 == freeSTS) {
            // nothing free
            return;
        }
        tries--;
    }
}

void STPU::processFire() {
    // has to: deliver incoming brain wave pulses, assign neuron
    // firings to lookup units (spike transfer structures), process
    // neuron firings into activations

    // brain wave pulses: Deliver all before moving on
    bool BWPDone = deliverBWPs();

    bool allSpikesDelivered = true;
    if (BWPDone) {
        // assign firings to STSs
        assignSTS();
        
        // process neuron firings into activations
        for(auto &e: STSUnits) {
            e.advance();
            bool unitDone = e.isFree();
            allSpikesDelivered &= unitDone;
        }
    }

    // do we move on?
    if (BWPDone && allSpikesDelivered & firedNeurons.empty()) {
        state = LIF;
    }
}

// run LIF on all neurons
void STPU::lifAll() {
    for (uint n = 0; n < numNeurons; ++n) {
        bool fired = neurons[n].lif(now);
        if (fired) {
            firedNeurons.push_back(n);
        }
    }
}

bool STPU::clockTic( Cycle_t )
{

    switch(state) {
    case IDLE:
        state = PROCESS_FIRE; // for now
        break;
    case PROCESS_FIRE:
        processFire();
        break;
    case LIF:
        now++;
        lifAll();
        state = PROCESS_FIRE;
        printf("%lu neurons fired\n", firedNeurons.size());        
        break;
    default:
        out.fatal(CALL_INFO, -1,"Invalid STPU state\n");
    }

    // return false so we keep going
    return false;
}


