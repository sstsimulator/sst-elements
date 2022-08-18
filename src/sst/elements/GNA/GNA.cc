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

#include <fstream>

#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>

#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::GNAComponent;
using namespace std;


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
    modelPath      = params.find<string>("modelPath",      "model");
    steps          = params.find<int>   ("steps",          1000);
    Neuron::dt     = params.find<float> ("dt",             1);
    InputsPerTic   = params.find<int>   ("InputsPerTic",   2);
    STSDispatch    = params.find<int>   ("STSDispatch",    2);
    STSParallelism = params.find<int>   ("STSParallelism", 2);
    maxOutMem      = params.find<int>   ("MaxOutMem",      STSParallelism);
    if (InputsPerTic   < 1) out.fatal(CALL_INFO, -1, "InputsPerTic invalid\n");
    if (STSDispatch    < 1) out.fatal(CALL_INFO, -1, "STSDispatch invalid\n");
    if (STSParallelism < 1) out.fatal(CALL_INFO, -1, "STSParallelism invalid\n");
    if (maxOutMem      < 1) out.fatal(CALL_INFO, -1, "MaxOutMem invalid\n");

    //set our clock
    string clockFreq = params.find<string>("clock", "1GHz");
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
    if (!memory) out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent\n");
}

GNA::GNA()
:   Component(-1)
{
    // for serialization only
}

GNA::~GNA()
{
    for (auto n : neurons) delete n;
}

void GNA::init(unsigned int phase)
{
    // init memory
    memory->init(phase);

    // Everything below we only do once
    if (phase != 0) return;

    // create STS units
    for (int i = 0; i < STSParallelism; ++i) {
        STSUnits.push_back(STS(this,i));
    }

    // Read data
    // format:
    // index,Vthreshold,Vreset,leak,p -- neuron info; for input neurons, only specify index
    //  to,weight,delay -- synapse info
    //  s<timing list> -- optional spike list for input neurons
    //  o -- optional output configuration
    //   p<probe type> -- (V, spike), default is spike
    //   f<file name> -- default is "out"
    //   c<column name> -- default is neuron index
    //   m<mode flags> -- default is blank

    ifstream ifs (modelPath.c_str());
    if (! ifs.good()) cerr << "Failed to open file: " << modelPath << endl;
    int countLinks = 0;
    string line;
    while (ifs.good()) {
        getline(ifs, line);
        if (line.empty()) break;

        char * piece = strtok(const_cast<char *>(line.c_str()), ",");
        int id = atoi(piece);
        while (neurons.size () <= id) neurons.push_back(nullptr);

        Neuron * n;
        piece = strtok(0, ",");
        if (piece) {
            float Vthreshold = atof(piece);
            piece = strtok(0, ",");
            float Vreset = atof(piece);
            piece = strtok(0, ",");
            float leak = atof(piece);
            piece = strtok(0, ",");  // Actually, there should only be one piece left, with no more commas.
            float p = atof(piece);
            n = neurons[id] = new NeuronLIF(Vthreshold, Vreset, leak, p);
        } else {
            n = neurons[id] = new NeuronInput();
        }

        // Scan indented lines
        while (ifs.good()) {
            streampos pos = ifs.tellg();
            getline(ifs, line);
            if (line.empty()  ||  line[0] != ' ') {
                ifs.seekg(pos);
                break;
            }

            char c = line[1];
            if (c == 'r') {  // spike raster
                int count = line.size();
                for (int i = 2; i < count; i++) {
                    if (line[i] == '1') ((NeuronInput *) n)->spikes.push_back(i - 2);
                }
            } else if (c == 't') {  // spike time list
                char * piece = strtok(const_cast<char *>(line.c_str() + 2), ",");
                while (piece) {
                    ((NeuronInput *) n)->spikes.push_back(atoi(piece));
                    piece = strtok(0, ",");
                }
            } else if (c == 'o') {  // output
                string p = "spike";
                string f = "out";
                string col;
                string m;

                // Scan lines indented under 'o'
                while (ifs.good()) {
                    pos = ifs.tellg();
                    getline(ifs, line);
                    if (line.empty()  ||  line[0] != ' '  ||  line[1] != ' ') {
                        ifs.seekg(pos);
                        break;
                    }
                    int last = line.size() - 1;
                    if (line[last] == '\r') line.resize(last);  // Get rid of extraneous CR

                    c = line[2];
                    if      (c == 'p') {p   = line.substr(3);}
                    else if (c == 'f') {f   = line.substr(3);}
                    else if (c == 'c') {col = line.substr(3);}
                    else if (c == 'm') {m   = line.substr(3);}
                }

                if (col.empty()) {
                    char buffer[16];
                    sprintf(buffer, "%i", id);
                    col = buffer;
                }

                map<string,OutputHolder *>::iterator it = Neuron::outputs.find(f);
                if (it == Neuron::outputs.end()) {
                    OutputHolder * h = new OutputHolder(f);
                    it = Neuron::outputs.insert(make_pair(f, h)).first;
                }
                OutputHolder * h = it->second;

                Trace * t = new Trace;
                t->next = n->traces;
                n->traces = t;
                t->holder = h;
                t->column = col;
                if (! m.empty()) t->mode = strdup(m.c_str());
                if (p == "V") t->probe = 1;
                else          t->probe = 0;
            } else {  // synapse
                // In this pass, just determine memory requirements for each neuron.
                n->synapseCount++;
                countLinks++;
            }
        }
    }

    // Synapses
    // This requires a second pass, now that we know the memory size for each neuron.
    ifs.close ();
    ifs.open (modelPath.c_str());
    assert(sizeof(Synapse) == 8);
    uint64_t startAddr = 0x10000;
    while (ifs.good()) {
        getline(ifs, line);
        if (line.empty()) break;

        char * piece = strtok (const_cast<char *>(line.c_str()), ",");
        int id = atoi(piece);
        Neuron * n = neurons[id];

        // Scan indented lines
        while (ifs.good()) {
            streampos pos = ifs.tellg();
            getline(ifs, line);
            if (line.empty()  ||  line[0] != ' ') {
                ifs.seekg(pos);
                break;
            }

            char c = line[1];
            if (c == 'r'  ||  c == 't'  ||  c == 'o'  ||  c == ' ') continue;

            char * piece = strtok(const_cast<char *>(line.c_str()), ",");
            int target = atoi(piece);
            piece = strtok(0, ",");
            float weight = atof(piece);
            piece = strtok(0, ",");
            int delay = atoi(piece);

            if (n->synapseBase == 0)
            {
                n->synapseBase = startAddr;  // This implies that startAddr must begin higher than 0
                startAddr += sizeof(Synapse) * n->synapseCount;
                n->synapseCount = 0;
            }
            vector<uint8_t> data(sizeof(Synapse), 0);
            uint64_t reqAddr = n->synapseBase + sizeof(Synapse) * n->synapseCount++;
            using namespace Interfaces;
            StandardMem::Write * req = new StandardMem::Write(reqAddr, sizeof(Synapse), data);
            Synapse * synapse = (Synapse *) &req->data[0];
            synapse->target = target;
            synapse->weight = weight;
            synapse->delay  = delay;
            memory->sendUntimedData(req);
        }
    }

    int numNeurons = neurons.size();
    printf("Constructed %d neurons with %d links\n", numNeurons, countLinks);
}

void GNA::finish()
{
	printf("Completed %d neuron firings\n", numFirings);
    printf("Completed %d spike deliveries\n", numDeliveries);
}

// handle incoming memory
void GNA::handleEvent(Interfaces::StandardMem::Request * req)
{
    map<uint64_t, STS*>::iterator i = requests.find(req->getID());
    if (i == requests.end()) out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->getID());

    // handle event
    STS * requestor = i->second;
    requestor->returnRequest(req);
    // clean up
    requests.erase(i);
}

void GNA::deliver(float val, int targetN, int time)
{
    // AFR: should really throttle this in some way
    if (targetN >= neurons.size()) out.fatal(CALL_INFO, -1, "Invalid Neuron Address\n");
    neurons[targetN]->deliverSpike(val, time);
    numDeliveries++;
}

void GNA::readMem(Interfaces::StandardMem::Request *req, STS *requestor)
{
    outgoingReqs.push(req);  // queue the request to send later
    requests.insert(make_pair(req->getID(), requestor));  // record who it came from
}

void GNA::processFire()
{
    // assign neuron firings to lookup units (spike transfer structures)
    int remainDispatches = STSDispatch;
    for (auto & e : STSUnits) {
        if (firedNeurons.empty()) break;
        if (e.isFree()) {
            e.assign(firedNeurons.front());
            firedNeurons.pop_front();
            remainDispatches--;
        }
        if (remainDispatches == 0) break;
    }

    // process neuron firings into activations
    bool allSpikesDelivered = true;
    for (auto & e : STSUnits) {
        e.advance(now);
        allSpikesDelivered &= e.isFree();
    }

    // do we move on?
    if (allSpikesDelivered & firedNeurons.empty()) state = LIF;
}

bool GNA::clockTic(Cycle_t)
{
    // send some outgoing mem reqs
    for (int i = 0; i < maxOutMem  &&  ! outgoingReqs.empty(); i++) {
        memory->send(outgoingReqs.front());
        outgoingReqs.pop();
    }

    switch(state) {
    case IDLE:
        state = PROCESS_FIRE; // for now
        break;
    case PROCESS_FIRE:
        processFire();
        break;
    case LIF:
        for (auto n : neurons) if (n->update(now)) firedNeurons.push_back(n);
        now++;
        if (now >= steps) primaryComponentOKToEndSim();
        state = PROCESS_FIRE;
        numFirings += firedNeurons.size();
        break;
    default:
        out.fatal(CALL_INFO, -1,"Invalid GNA state\n");
    }

    return false;  // keep going
}
