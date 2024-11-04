// Copyright 2018-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018-2024, NTESS
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
#include "gensa.h"

#include <fstream>

#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>

#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
//using namespace SST::MemHierarchy;
using namespace SST::gensaComponent;
using namespace std;


gensa::gensa (ComponentId_t id, Params & params)
:   Component (id)
{
    now            = 0;
    neuronIndex    = -1;
    synapseIndex   = -1;
    syncSent       = false;
    numFirings     = 0;
    numDeliveries  = 0;

    uint32_t outputLevel = params.find<uint32_t> ("verbose", 0);
    out.init ("gensa:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // get parameters
    modelPath       = params.find<string>("modelPath",       "model");
    steps           = params.find<int>   ("steps",           1000);
    Neuron::dt      = params.find<float> ("dt",              1);  // In seconds. Don't bother with UnitAlgebra because this is usually specified by wrapper script.
    maxRequestDepth = params.find<int>   ("maxRequestDepth", 2);

    //set our clock
    string clockFreq = params.find<string> ("clock", "1GHz");
    clockTC = registerClock (clockFreq, new Clock::Handler<gensa> (this, &gensa::clockTic));

    // tell the simulator not to end without us
    registerAsPrimaryComponent ();
    primaryComponentDoNotEndSim ();

    // init memory
    memory = loadUserSubComponent<Interfaces::StandardMem> (
        "memory",
        ComponentInfo::SHARE_NONE, clockTC,
        new Interfaces::StandardMem::Handler<gensa> (this, &gensa::handleMemory)
    );
    if (!memory)
    {
        params.insert ("port", "mem_link");
        memory = loadAnonymousSubComponent<Interfaces::StandardMem> (
            "memHierarchy.standardInterface", "memory", 0,
            ComponentInfo::SHARE_PORTS, params, clockTC,
            new Interfaces::StandardMem::Handler<gensa>(this, &gensa::handleMemory)
        );
    }
    if (!memory) out.fatal (CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent\n");

    link = loadUserSubComponent<Interfaces::SimpleNetwork> ("networkIF", ComponentInfo::SHARE_NONE, 1);
    if (!link) out.fatal (CALL_INFO, 1, "No networkIF subcomponent\n");
    link->setNotifyOnReceive (new Interfaces::SimpleNetwork::Handler<gensa> (this, &gensa::handleNetwork));
}

gensa::gensa ()
:   Component(-1)
{
    // for serialization only
}

gensa::~gensa ()
{
    for (auto n : neurons) delete n;
    while (! networkRequests.empty ())
    {
        delete networkRequests.front ();
        networkRequests.pop ();
    }
}

void
gensa::init (unsigned int phase)
{
    memory->init (phase);
    link  ->init (phase);

    // Everything below we only do once
    if (phase != 0) return;

    // Read data
    // format:
    // index,Vinit,Vthreshold,Vreset,leak,p -- neuron info; for input neurons, only specify index   TODO: add Vbias
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
            float Vinit = atof(piece);
            piece = strtok(0, ",");
            float Vthreshold = atof(piece);
            piece = strtok(0, ",");
            float Vreset = atof(piece);
            piece = strtok(0, ",");
            float leak = 1 - atof(piece);  // The parameter in the file is the portion of voltage to get rid of each cycle. It's simpler for us to compute with (1-decay).
            piece = strtok(0, ",");  // Actually, there should only be one piece left, with no more commas.
            float p = atof(piece);
            n = neurons[id] = new NeuronLIF(Vinit, Vthreshold, Vreset, leak, p);
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
                    char buffer[32];
                    if (p == "spike") snprintf(buffer, 32, "%i",    id);              // Default column name for spike is just neuron index,
                    else              snprintf(buffer, 32, "%i.%s", id, p.c_str ());  // while default name for anything else is index.probe
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

    int numNeurons = neurons.size ();
    printf("Constructed %d neurons with %d links\n", numNeurons, countLinks);
}

void
gensa::setup ()
{
	memory->setup ();
	link->setup ();
}

void
gensa::complete (unsigned int phase)
{
	memory->complete (phase);
	link->complete (phase);
}

void
gensa::finish ()
{
	memory->finish ();
	link  ->finish ();
    for (auto i : Neuron::outputs) delete i.second;  // flushes last row

	printf ("Completed %d neuron firings\n", numFirings);
    printf ("Completed %d spike deliveries\n", numDeliveries);
}

// We simulate a von Nuemann style neuromorphic processor, working through our list of nuerons serially.
// We should execute one FLOP and one tightly-coupled-memory access per CPU cycle.
// Currently, for simplicity, we assume the full LIF model can execute in one cycle.
// Also, neuron load/save is not charged memory access time.
// Retrieving synapse records and transmitting spike packets can run in parallel with executing the LIF model,
// but the LIF model needs to stall until all spikes are sent.
bool
gensa::clockTic (Cycle_t t)
{
    using namespace Interfaces;
    if (! networkRequests.empty ())
    {
        SimpleNetwork::Request * req = networkRequests.front ();
        if (link->send (req, 0)) networkRequests.pop ();
    }

    if (synapseIndex < 0)  // Ready for next neuron.
    {
        int count = neurons.size ();
        if (neuronIndex >= count)  // Waiting for sync
        {
            if (syncSent) return false;
            if (! memoryRequests.empty ()) return false;  // Must finish all spikes before going to next cycle.

            SyncEvent * event = new SyncEvent;
            event->phase = 0;

            SimpleNetwork::nid_t source = 0;
            SimpleNetwork::nid_t dest   = 0;  // TODO: should broadcast to whole network
            SimpleNetwork::Request * req = new SimpleNetwork::Request (dest, source, 1, false, false, event);
            networkRequests.push (req);
            syncSent = true;

            return false;
        }
        syncSent = false;  // Although this is a wasted operation most of the time, it's the simplest way to reset sync state.

        neuronIndex++;
        if (neuronIndex < count)
        {
            Neuron * n = neurons[neuronIndex];
            if (n->update (now))
            {
                numFirings++;
                if (n->synapseCount) synapseIndex = 0;  // Start iterating through synapses.
            }
        }
    }
    else   // Working through the synapse list for current neuron.
    {
        // Check if we're ready to send
        if (networkRequests.size () >= maxRequestDepth) return false;
        if (memoryRequests.size () >= maxRequestDepth) return false;

        Neuron * n = neurons[neuronIndex];
        uint64_t address = n->synapseBase + synapseIndex * sizeof (Synapse);
        StandardMem::Read * req = new StandardMem::Read (address, sizeof (Synapse));
        memory->send (req);  // Unlike network, it seems that memory has unlimited capacity for requests.
        memoryRequests.insert (address);  // But we still limit the number of outstanding requests.

        synapseIndex++;
        if (synapseIndex >= n->synapseCount) synapseIndex = -1;
    }

    return false;  // keep going
}

void
gensa::handleMemory (Interfaces::StandardMem::Request * req)
{
    SST::Interfaces::StandardMem::ReadResp * resp = dynamic_cast<SST::Interfaces::StandardMem::ReadResp *> (req);
    assert (resp);
    memoryRequests.erase (resp->pAddr);

    Synapse * s = (Synapse *) &resp->data[0];
    SpikeEvent * event = new SpikeEvent;
    event->neuron = s->target;
    event->weight = s->weight;
    event->delay  = s->delay;
    delete req;

    using namespace Interfaces;
    SimpleNetwork::nid_t source = 0;
    SimpleNetwork::nid_t dest   = 0;
    networkRequests.push (new SimpleNetwork::Request (dest, source, event->getSize (), false, false, event));
}

bool
gensa::handleNetwork (int vn)
{
    // Ignore vn. It should always be 0 because that's all we registered for.

    using namespace Interfaces;
    while (SimpleNetwork::Request * req = link->recv (0))
    {
        Event * event = req->inspectPayload ();
        if (SpikeEvent * spike = dynamic_cast<SpikeEvent *> (event))
        {
            if (spike->neuron >= neurons.size ()) out.fatal (CALL_INFO, -1, "Invalid Neuron Address\n");
            neurons[spike->neuron]->deliverSpike (spike->weight, spike->delay+now);
            numDeliveries++;
        }
        else if (SyncEvent * sync = dynamic_cast<SyncEvent *> (event))
        {
            now++;
            neuronIndex = -1;
            if (now >= steps) primaryComponentOKToEndSim ();
        }
        delete req;
    }
    return true;
}


// class SyncEvent -----------------------------------------------------------

uint32_t
SyncEvent::cls_id () const
{
    return 1235;
}

string
SyncEvent::serialization_name () const
{
    return "SyncEvent";
}

