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

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::GNAComponent;

GNA::GNA(ComponentId_t id, Params& params) :
    Component(id), state(IDLE), rng(1,13), now(0), numFirings(0),
    numDeliveries(0)
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
    if (STSDispatch <= 0) {
        out.fatal(CALL_INFO, -1,"STSDispatch invalid\n");
    }  
    STSParallelism = params.find<int>("STSParallelism", 2);
    if (STSParallelism <= 0) {
        out.fatal(CALL_INFO, -1,"STSParallelism invalid\n");
    }    
    maxOutMem = params.find<int>("MaxOutMem", STSParallelism);
    if (maxOutMem <= 0) {
        out.fatal(CALL_INFO, -1,"MaxOutMem invalid\n");
    }    

    // graph params
    graphType = params.find<int>("graphType", 0);
    if (graphType < 0 || graphType > 1) {
        out.fatal(CALL_INFO, -1,"graphType invalid\n");
    }
    connPer = params.find<float>("connPer", 0.01);
    if (connPer <= 0) {
        out.fatal(CALL_INFO, -1,"connPer invalid\n");
    }
    connPer_high = params.find<float>("connPer_high", 0.1);
    if (connPer_high <= 0) {
        out.fatal(CALL_INFO, -1,"connPer_high invalid\n");
    }
    highConn = params.find<float>("highConn", 0.1);
    if (highConn < 0) {
        out.fatal(CALL_INFO, -1,"highConn invalid\n");
    }
    double randFire_f = params.find<float>("randFire", 0.0);
    if (randFire_f < 0) {    
        out.fatal(CALL_INFO, -1,"randFire invalid\n");
    } else {
        randFire = uint16_t(randFire_f * 256);
    }

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<GNA>(this, &GNA::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // init memory
    memory = loadUserSubComponent<Interfaces::SimpleMem>("memory", clockTC, new Interfaces::SimpleMem::Handler<GNA>(this, &GNA::handleEvent));
    if (!memory) {
        params.insert("port", "mem_link");
        memory = loadAnonymousSubComponent<Interfaces::SimpleMem>("memHierarchy.memInterface", "memory", 0, 
                ComponentInfo::SHARE_PORTS, params, clockTC, new Interfaces::SimpleMem::Handler<GNA>(this, &GNA::handleEvent));
    }
    if (!memory) 
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");
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

    // <should read these in>
    if (graphType == 0) {
        wavyGraph();
    } else {
        randomConnectivity();
    }

}

// makes a graph with random connectivity
void GNA::randomConnectivity() {
    using namespace Neuron_Loader_Types;
    using namespace White_Matter_Types;

    // connectivity chance (normal)  connPer
    // connectivity chance (high)    connPer_high
    // % high connectivity           highConn
    // random fire chance            randFire
    
    // neurons
    float avgConnectionsIn = (numNeurons * connPer * (1.0 - highConn)) +
        (numNeurons * connPer_high * highConn);
    printf("avg Conn %f\n", avgConnectionsIn);
    for (int nrn_num=0;nrn_num<numNeurons;nrn_num++) {
        float trig = avgConnectionsIn;
        T_NctFl config = {trig,0.0,float(trig/1000.),randFire};
        neurons[nrn_num].configure(config, &rng);
    }

    // white matter list
    uint64_t startAddr = 0x10000;
    int countLinks = 0;
    for (int n = 0; n < numNeurons; ++n) {
        using namespace Interfaces;
        vector<int> connections;
        double conChance;
        if (rng.nextUniform() < highConn) {
            conChance = connPer_high; // high connectivity node
        } else {
            conChance = connPer; // normal connectivity node
        }

        // find who we connec to
        for (int nn = 0; nn < numNeurons; ++nn) {
            if (rng.nextUniform() < conChance) {
                connections.push_back(nn);
            }
        }

        // fill out data structure
        int numCon = connections.size();
        countLinks += numCon;
        neurons[n].setWML(startAddr,numCon);
        uint64_t reqAddr = startAddr; //+nn*sizeof(T_Wme);
        SimpleMem::Request *req = 
            new SimpleMem::Request(SimpleMem::Request::Write, reqAddr,
                                   sizeof(T_Wme)*numCon);
        req->data.resize(sizeof(T_Wme)*numCon);
        uint offset=0;
        for (int nn=0; nn<numCon; ++nn) {
            uint16_t targ = connections[nn];

            uint32_t str = 1;
            uint32_t tmpOff = 2 + (rng.generateNextUInt32() % 12);
            req->data[offset+0] = (str>>8) & 0xff; // Synaptic Str upper
            req->data[offset+1] = (str) & 0xff; // Synaptic Str lower
            req->data[offset+2] = (tmpOff>>8) & 0xff; // temp offset upper
            req->data[offset+3] = (tmpOff) & 0xff; // temp offset lower
            req->data[offset+4] = (targ>>8) & 0xff; // address upper
            req->data[offset+5] = (targ) & 0xff; // address lower
            req->data[offset+6] = 0; // valid
            req->data[offset+7] = 0; // valid
            //printf("Writing n%d to targ%d at %p\n", n, targ, (void*)reqAddr);
            offset += sizeof(T_Wme);
        }
        memory->sendInitData(req);
        assert(sizeof(T_Wme) == 8);
        startAddr += numCon * sizeof(T_Wme);
    }

    printf("Constructed %d neurons with %d links\n", numNeurons, countLinks);
}

//creates a 'feed forward' sort of graph structure
void GNA::wavyGraph() {
    using namespace Neuron_Loader_Types;
    using namespace White_Matter_Types;
    

    // neurons
    for (int nrn_num=0;nrn_num<numNeurons;nrn_num++) {
        uint16_t trig = rng.generateNextUInt32() % 100 + 350;
        neurons[nrn_num]
            .configure((T_NctFl){float(trig),0.0,float(trig/10.),randFire}, &rng);
    }

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
    int bwpl_len = 2;
    Ctrl_And_Stat_Types::T_BwpFl* bwpl = (Ctrl_And_Stat_Types::T_BwpFl*)calloc(bwpl_len,sizeof(Ctrl_And_Stat_Types::T_BwpFl));
    for (int i = 0; i < bwpl_len; ++i) {
        int targ = rng.generateNextUInt32() % numNeurons;
        bwpl[i]  = (Ctrl_And_Stat_Types::T_BwpFl){2001,targ,i*61};
    }
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
            //printf("BWP st%.1f to %d\n", pulse.InpValFl, pulse.InpNrn);
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
        if ((now & 0x7) == 0)
            printf("%lu neurons fired @ %d\n", firedNeurons.size(), now);
        if (firedNeurons.size() == 0 && now > 100) {
            primaryComponentOKToEndSim();
        }
        if (now > 80) primaryComponentOKToEndSim();
        break;
    default:
        out.fatal(CALL_INFO, -1,"Invalid GNA state\n");
    }

    // return false so we keep going
    return false;
}


