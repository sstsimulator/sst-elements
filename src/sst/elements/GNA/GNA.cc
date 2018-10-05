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
#include "GNA.h"

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::GNAComponent;

GNA::GNA(ComponentId_t id, Params& params) :
    Component(id), state(IDLE), now(0), numFirings(0), numDeliveries(0)
{
    uint32_t outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("GNA:@p:@l: ", outputLevel, 0, Output::STDOUT);

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
    maxOutMem = params.find<int>("MaxOutMem", STSParallelism);
    if (BWPpTic <= 0) {
        out.fatal(CALL_INFO, -1,"MaxOutMem invalid\n");
    }    



    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // init memory
    memory = dynamic_cast<Interfaces::SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");
    }
    memory->initialize("mem_link", new Interfaces::SimpleMem::Handler<GNA> (this, &GNA::handleEvent));

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<GNA>(this, &GNA::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);
}

GNA::GNA() : Component(-1)
{
	// for serialization only
}


void GNA::init(unsigned int phase) {
    using namespace Neuron_Loader_Types;
    using namespace White_Matter_Types;
    
    // init memory
    memory->init(phase);
    
    // Everything below we only do once
    if (phase != 0) {
        return;
    }

    // create STS units
    for(int i = 0; i < STSParallelism; ++i) {
        STSUnits.push_back(STS(this,i));
    }

    // initialize neurons
    neurons = new neuron[numNeurons];

    SST::RNG::MarsagliaRNG rng(1,13);

    // <should read these in>
    // neurons
#if 0 
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
#else
    for (int nrn_num=0;nrn_num<numNeurons;nrn_num++) {
        uint16_t trig = rng.generateNextUInt32() % 100 + 350;
        neurons[nrn_num].configure((T_NctFl){float(trig),0.0,float(trig/10.)});
    }
#endif

    // <Should read these in>
    // White matter list
    uint64_t startAddr = 0x10000;
    int countLinks = 0;
    for (int n = 0; n < numNeurons; ++n) {
        using namespace Interfaces;
        // most neurons connect to 1-4, 1% connect to 15
        uint16_t roll = rng.generateNextUInt32() % 100;
        uint numCon = 1;
        bool local = 1;
        if (roll == 0) {
            numCon = 15;
            if (rng.generateNextUInt32() % 100) local = 0;
        } else {
            numCon = 1 + (rng.generateNextUInt32() % 4);
            local = 1;
        }

        countLinks += numCon;
        neurons[n].setWML(startAddr,numCon);
        for (int nn=0; nn<numCon; ++nn) {

            uint16_t targ;
            if (local) {
                int diff = (rng.generateNextUInt32() % 10);
                targ = n + diff;
            } else {
                int diff = (rng.generateNextUInt32() % 50);
                targ = n + diff;
            }
            if (targ == n) {
                targ = n+1;
            }
            //targ %= numNeurons;
            if (targ >= numNeurons)
                targ = 0;

            uint64_t reqAddr = startAddr+nn*sizeof(T_Wme);
            SimpleMem::Request *req = 
                new SimpleMem::Request(SimpleMem::Request::Write, reqAddr,
                                       sizeof(T_Wme));
            req->data.resize(sizeof(T_Wme));
            uint32_t str = 300+(rng.generateNextUInt32() % 700);
            if (targ == 0) str = 1;
            uint32_t tmpOff = 2 + (rng.generateNextUInt32() % 12);
            if (!local) {
                tmpOff /= 2;
            }
            req->data[0] = (str>>8) & 0xff; // Synaptic Str upper
            req->data[1] = (str) & 0xff; // Synaptic Str lower
            req->data[2] = (tmpOff>>8) & 0xff; // temp offset upper
            req->data[3] = (tmpOff) & 0xff; // temp offset lower
            req->data[4] = (targ>>8) & 0xff; // address upper
            req->data[5] = (targ) & 0xff; // address lower
            req->data[6] = 0; // valid
            req->data[7] = 0; // valid
            //printf("Writing n%d to targ%d at %p\n", n, targ, (void*)reqAddr);
            memory->sendInitData(req);
        }
        assert(sizeof(T_Wme) == 8);
        startAddr += numCon * sizeof(T_Wme);
    }

    printf("Constructed %d neurons with %d links\n", numNeurons, countLinks);

    // brain wave pulses
#if 0
    int bwpl_len = 16;
    Ctrl_And_Stat_Types::T_BwpFl* bwpl = (Ctrl_And_Stat_Types::T_BwpFl*)calloc(bwpl_len,sizeof(Ctrl_And_Stat_Types::T_BwpFl));
    bwpl[0]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,0,0};
    bwpl[1]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,0,1};
    bwpl[2]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,0,2};
    bwpl[3]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,0,3};
    bwpl[4]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,4,0};
    bwpl[5]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,4,1};
    bwpl[6]  = (Ctrl_And_Stat_Types::T_BwpFl){1001,4,2};
    bwpl[7]  = (Ctrl_And_Stat_Types::T_BwpFl){1,4,3};
    bwpl[8]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,4};
    bwpl[9]  = (Ctrl_And_Stat_Types::T_BwpFl){1,0,5};
    bwpl[10] = (Ctrl_And_Stat_Types::T_BwpFl){1,0,6};
    bwpl[11] = (Ctrl_And_Stat_Types::T_BwpFl){1,0,7};
    bwpl[12] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,4};
    bwpl[13] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,5};
    bwpl[14] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,6};
    bwpl[15] = (Ctrl_And_Stat_Types::T_BwpFl){1,4,7};
#else
    int bwpl_len = 2;
    Ctrl_And_Stat_Types::T_BwpFl* bwpl = (Ctrl_And_Stat_Types::T_BwpFl*)calloc(bwpl_len,sizeof(Ctrl_And_Stat_Types::T_BwpFl));
    for (int i = 0; i < bwpl_len; ++i) {
        int targ = rng.generateNextUInt32() % numNeurons;
        bwpl[i]  = (Ctrl_And_Stat_Types::T_BwpFl){2001,targ,i*61};
    }
#endif
    for (int i = 0; i < bwpl_len; ++i) {
        BWPs.insert(std::pair<uint,Ctrl_And_Stat_Types::T_BwpFl>(bwpl[i].TmpSft, bwpl[i]));
    }
}

// handle incoming memory
void GNA::handleEvent(Interfaces::SimpleMem::Request * req)
{
    std::map<uint64_t, STS*>::iterator i = requests.find(req->id);
    if (i == requests.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->id);
    } else {
        // handle event
        STS* requestor = i->second;
        requestor->returnRequest(req);
        // clean up
        requests.erase(i);
    }
}

void GNA::deliver(float val, int targetN, int time) {
    // AFR: should really throttle this in some way
    numDeliveries++;
    if(targetN < numNeurons) {
        neurons[targetN].deliverSpike(val, time);
        //printf("deliver %f to %d @ %d\n", val, targetN, time);
    } else {
        out.fatal(CALL_INFO, -1,"Invalid Neuron Address\n");
    }
}

// returns true if no more to deliver
bool GNA::deliverBWPs() {
    int tries = BWPpTic;

    while (tries > 0) {
        BWPBuf_t::iterator i = BWPs.find(now);
        if (i != BWPs.end()) {
            // deliver it
            const Ctrl_And_Stat_Types::T_BwpFl &pulse = i->second;
            printf("BWP st%.1f to %d\n", pulse.InpValFl, pulse.InpNrn);
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
void GNA::assignSTS() {
    int remainDispatches = STSDispatch;

    // try to find a free unit
    for(auto &e: STSUnits) {
        if (firedNeurons.empty()) return;
        if (e.isFree()) {
            e.assign(firedNeurons.front());
            firedNeurons.pop_front();
            remainDispatches--;
        } 
        if (remainDispatches == 0) return;
    }
}

void GNA::processFire() {
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
            e.advance(now);
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
void GNA::lifAll() {
    for (uint n = 0; n < numNeurons; ++n) {
        bool fired = neurons[n].lif(now);
        if (fired) {
            //printf(" %d fired\n", n);
            firedNeurons.push_back(n);
        }
    }
}

bool GNA::clockTic( Cycle_t )
{
    // send some outgoing mem reqs
    int maxOut = maxOutMem;
    //if((!outgoingReqs.empty()) && (now & 0x3f) == 0) {
    //    printf(" outRqst Q %d\n", outgoingReqs.size());
    //}
    while(!outgoingReqs.empty() && maxOut > 0) {
        memory->sendRequest(outgoingReqs.front());
        outgoingReqs.pop();
        maxOut--;
    }


    switch(state) {
    case IDLE:
        state = PROCESS_FIRE; // for now
        break;
    case PROCESS_FIRE:
        processFire();
        break;
    case LIF:
        lifAll();
        now++;
        state = PROCESS_FIRE;
        numFirings += firedNeurons.size();
        if ((now & 0x3f) == 0)
            printf("%lu neurons fired @ %d\n", firedNeurons.size(), now);
        if (firedNeurons.size() == 0 && now > 100) {
            primaryComponentOKToEndSim();
        }
        break;
    default:
        out.fatal(CALL_INFO, -1,"Invalid GNA state\n");
    }

    // return false so we keep going
    return false;
}


