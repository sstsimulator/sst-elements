// Copyright 2018-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018-2022, NTESS
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
#include "GNA.h"

#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include <fstream>

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::GNAComponent;

GNA::GNA(ComponentId_t id, Params& params)
:   Component(id),
    state(IDLE),
    now(0),
    numFirings(0),
    numDeliveries(0)
{
    uint32_t outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("GNA:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // get parameters
    modelPath = params.find<std::string>("modelPath", "model");
    cerr << "modelPath=" << modelPath << endl;
    InputsPerTic = params.find<int>("InputsPerTic", 2);
    if (InputsPerTic < 1) {
        out.fatal(CALL_INFO, -1,"InputsPerTic invalid\n");
    }
    STSDispatch = params.find<int>("STSDispatch", 2);
    if (STSDispatch < 1) {
        out.fatal(CALL_INFO, -1,"STSDispatch invalid\n");
    }
    STSParallelism = params.find<int>("STSParallelism", 2);
    if (STSParallelism < 1) {
        out.fatal(CALL_INFO, -1,"STSParallelism invalid\n");
    }
    maxOutMem = params.find<int>("MaxOutMem", STSParallelism);
    if (maxOutMem < 1) {
        out.fatal(CALL_INFO, -1,"MaxOutMem invalid\n");
    }

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<GNA>(this, &GNA::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // init memory
    memory = loadUserSubComponent<Interfaces::StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new Interfaces::StandardMem::Handler<GNA>(this, &GNA::handleEvent));
    if (!memory) {
        params.insert("port", "mem_link");
        memory = loadAnonymousSubComponent<Interfaces::StandardMem>("memHierarchy.standardInterface", "memory", 0,
                ComponentInfo::SHARE_PORTS, params, clockTC, new Interfaces::StandardMem::Handler<GNA>(this, &GNA::handleEvent));
    }
    if (!memory)
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent\n");
}

GNA::GNA()
:   Component(-1)
{
    // for serialization only
}


void GNA::init(unsigned int phase) {
    // init memory
    memory->init(phase);

    // Everything below we only do once
    if (phase != 0) return;

    // create STS units
    for(int i = 0; i < STSParallelism; ++i) {
        STSUnits.push_back(STS(this,i));
    }

    // Neurons
    // format: index,Vthreshold,Vreset,leak,p
    ifstream ifs (modelPath.c_str ());
    if (! ifs.good ()) cerr << "Failed to open file: " << modelPath << endl;
    string line;
    while (ifs.good ()) {
        getline (ifs, line);
        if (line.empty ()) break;  // blank line indicates transition from neurons to synapses
        char * piece = strtok (const_cast<char *> (line.c_str ()), ",");
        int id = atoi (piece);
        piece = strtok (0, ",");
        float Vthreshold = atof (piece);
        piece = strtok (0, ",");
        float Vreset = atof (piece);
        piece = strtok (0, ",");
        float leak = atof (piece);
        piece = strtok (0, ",");  // Actually, there should only be one piece left, with no ore commas.
        float p = atof (piece);

        while (neurons.size () <= id) neurons.push_back (Neuron ());
        neurons[id].configure (Vthreshold, Vreset, leak, p);
    }

    // Count synapses per neuron
    std::streampos pos = ifs.tellg ();
    int countLinks = 0;
    while (ifs.good ()) {
        getline (ifs, line);
        if (line.empty ()) break;
        char * piece = strtok (const_cast<char *> (line.c_str ()), ",");
        int from = atoi (piece);
        neurons[from].synapseCount++;
        countLinks++;
    }
    ifs.seekg (pos);

    // Synapses
    // format: from,to,weight,delay
    assert(sizeof(Synapse) == 8);
    uint64_t startAddr = 0x10000;
    while (ifs.good ()) {
        getline (ifs, line);
        if (line.empty ()) break;  // blank line indicates transition from synapses to inputs
        char * piece = strtok (const_cast<char *> (line.c_str ()), ",");
        int from = atoi (piece);
        piece = strtok (0, ",");
        int target = atoi (piece);
        piece = strtok (0, ",");
        float weight = atof (piece);
        piece = strtok (0, ",");
        int delay = atoi (piece);

        Neuron & neuron = neurons[from];
        if (neuron.synapseBase == 0)
        {
            neuron.synapseBase = startAddr;  // This implies that startAddr must begin higher than 0
            startAddr += sizeof(Synapse) * neuron.synapseCount;
            neuron.synapseCount = 0;
        }
        std::vector<uint8_t> data(sizeof(Synapse), 0);
        uint64_t reqAddr = neuron.synapseBase + sizeof(Synapse) * neuron.synapseCount++;
        using namespace Interfaces;
        StandardMem::Write * req = new StandardMem::Write(reqAddr, sizeof(Synapse), data);
        Synapse * synapse = (Synapse *) &req->data[0];
        synapse->weight = weight;
        synapse->delay  = delay;
        synapse->target = target;
        memory->sendUntimedData(req);
    }

    int numNeurons = neurons.size ();
    printf("Constructed %d neurons with %d links\n", numNeurons, countLinks);

    // Input pulses
    // format: timestep,target,weight
    while (ifs.good ()) {
        getline (ifs, line);
        if (line.empty ()) break;
        char * piece = strtok (const_cast<char *> (line.c_str ()), ",");
        int timestep = atoi (piece);
        piece = strtok (0, ",");
        int target = atoi (piece);
        piece = strtok (0, ",");
        float weight = atof (piece);

        Synapse input;
        input.weight = weight;
        input.target = target;
        input.delay  = timestep;
        inputBuffer.insert(make_pair(timestep, input));
    }
}

void GNA::finish() {
	printf("Completed %d neuron firings\n", numFirings);
    printf("Completed %d spike deliveries\n", numDeliveries);
}

// handle incoming memory
void GNA::handleEvent(Interfaces::StandardMem::Request * req)
{
    std::map<uint64_t, STS*>::iterator i = requests.find(req->getID());
    if (i == requests.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->getID());
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
    if(targetN < neurons.size ()) {
        neurons[targetN].deliverSpike(val, time);
        //printf("deliver %f to %d @ %d\n", val, targetN, time);
    } else {
        out.fatal(CALL_INFO, -1,"Invalid Neuron Address\n");
    }
}

Neuron & GNA::getNeuron(int n) {
    return neurons[n];
}

void GNA::readMem(Interfaces::StandardMem::Request *req, STS *requestor) {
    outgoingReqs.push(req);  // queue the request to send later
    requests.insert(std::make_pair(req->getID(), requestor));  // record who it came from
}

// returns true if no more to deliver
bool GNA::deliverInputs() {
    int tries = InputsPerTic;

    while (tries > 0) {
        inputBuffer_t::iterator i = inputBuffer.find(now);
        if (i != inputBuffer.end()) {
            // deliver it
            const Synapse & pulse = i->second;
            printf("Input weight %.1f to neuron %d\n", pulse.weight, pulse.target);
            deliver(pulse.weight, pulse.target, pulse.delay);
            inputBuffer.erase(i);
        } else {
            return true;
        }
        tries--;
    }

    if (inputBuffer.find(now) == inputBuffer.end()) {
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
    bool inputsDone = deliverInputs();

    bool allSpikesDelivered = true;
    if (inputsDone) {
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
    if (inputsDone && allSpikesDelivered & firedNeurons.empty()) {
        state = LIF;
    }
}

// run LIF on all neurons
void GNA::lifAll() {
    for (uint n = 0; n < neurons.size (); ++n) {
        bool fired = neurons[n].lif(now);
        if (fired) {
            //printf(" %d fired\n", n);
            firedNeurons.push_back(n);
        }
    }
}

bool GNA::clockTic( Cycle_t ) {
    // send some outgoing mem reqs
    int maxOut = maxOutMem;
    //if((!outgoingReqs.empty()) && (now & 0x3f) == 0) {
    //    printf(" outRqst Q %d\n", outgoingReqs.size());
    //}
    while(!outgoingReqs.empty() && maxOut > 0) {
        memory->send(outgoingReqs.front());
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

