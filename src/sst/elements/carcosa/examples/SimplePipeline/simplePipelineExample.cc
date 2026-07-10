// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.

#include "sst_config.h"

#include "sst/elements/carcosa/examples/SimplePipeline/simplePipelineExample.h"

#include <sstream>

using namespace SST;
using namespace SST::Carcosa;
using namespace SST::Carcosa::Examples;

// ============================================================================
// Stage name table
// ============================================================================

const char* SST::Carcosa::Examples::simpleStageName(int id)
{
    switch (id) {
    case STAGE_FETCH:   return "FETCH";
    case STAGE_DECODE:  return "DECODE";
    case STAGE_EXECUTE: return "EXECUTE";
    case STAGE_COMMIT:  return "COMMIT";
    default:            return "UNKNOWN";
    }
}

// ============================================================================
// SimplePipelineProducer
// ============================================================================

SimplePipelineProducer::SimplePipelineProducer(ComponentId_t id, Params& params)
    : SST::Component(id)
{
    out_ = new Output("", 1, 0, Output::STDOUT);

    stateKey_    = params.find<std::string>("state_key", "");
    totalCycles_ = params.find<int>("total_cycles", 4);
    verbose_     = params.find<bool>("verbose", false);

    if (stateKey_.empty()) stateKey_ = getName();

    outLink_ = configureLink("out");
    sst_assert(outLink_, CALL_INFO, -1,
               "Error in %s: 'out' link configuration failed\n",
               getName().c_str());

    // Publish the initial snapshot. The gate may consult this before our first
    // tick fires; seed currentKernel=-1 so it doesn't accidentally match any
    // real stage id in the drop set.
    state_ = PipelineStateRegistry<PipelineStateBase>::getOrCreate(stateKey_);
    state_->currentKernel = -1;
    state_->pipelineCycle = 0;

    // Demonstrate the region-publish API with a single dummy region; not used
    // by the gate in this example but exercised so the pattern is visible.
    state_->stagedBase = 0x10000;
    state_->stagedSize = 0x1000;
    state_->commitStagedRegion(0);

    const std::string clock_freq = params.find<std::string>("clock", "1MHz");
    registerClock(clock_freq,
                  new Clock::Handler<SimplePipelineProducer,
                                     &SimplePipelineProducer::tick>(this));

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    if (verbose_) {
        out_->output("%s: producer ready, state_key='%s', total_cycles=%d, clock=%s\n",
                     getName().c_str(), stateKey_.c_str(), totalCycles_, clock_freq.c_str());
    }
}

SimplePipelineProducer::~SimplePipelineProducer()
{
    delete out_;
}

bool SimplePipelineProducer::tick(Cycle_t /*cycle*/)
{
    const int stage = tickCount_ % NUM_STAGES;
    const int cycle = tickCount_ / NUM_STAGES;

    // Publish BEFORE sending, so any PortModule on the receiving side observes
    // the snapshot that corresponds to the event it is about to see.
    state_->currentKernel = stage;
    state_->pipelineCycle = cycle;

    outLink_->send(new SimpleStageEvent(stage));

    if (verbose_) {
        out_->output("%s: tick=%d stage=%s cycle=%d -> sent event\n",
                     getName().c_str(), tickCount_, simpleStageName(stage), cycle);
    }

    ++tickCount_;

    // After COMMIT of the last configured pipeline cycle, stop the clock and
    // release the simulation end-gate.
    if (totalCycles_ > 0 && cycle + 1 >= totalCycles_ && stage == STAGE_COMMIT) {
        if (verbose_) {
            out_->output("%s: reached total_cycles=%d, ending simulation\n",
                         getName().c_str(), totalCycles_);
        }
        primaryComponentOKToEndSim();
        return true;
    }
    return false;
}

// ============================================================================
// SimplePipelineSink
// ============================================================================

SimplePipelineSink::SimplePipelineSink(ComponentId_t id, Params& params)
    : SST::Component(id)
{
    out_     = new Output("", 1, 0, Output::STDOUT);
    verbose_ = params.find<bool>("verbose", false);

    inLink_ = configureLink("in",
                            new Event::Handler<SimplePipelineSink,
                                               &SimplePipelineSink::handleEvent>(this));
    sst_assert(inLink_, CALL_INFO, -1,
               "Error in %s: 'in' link configuration failed\n",
               getName().c_str());
}

SimplePipelineSink::~SimplePipelineSink()
{
    delete out_;
}

void SimplePipelineSink::handleEvent(Event* ev)
{
    auto* sev = dynamic_cast<SimpleStageEvent*>(ev);
    if (!sev) {
        out_->fatal(CALL_INFO, -1,
                    "Error in %s: received unexpected event type\n",
                    getName().c_str());
    }
    if (sev->stageId >= 0 && sev->stageId < NUM_STAGES) {
        ++received_[sev->stageId];
    }
    if (verbose_) {
        out_->output("%s: received stage=%s\n",
                     getName().c_str(), simpleStageName(sev->stageId));
    }
    delete sev;
}

void SimplePipelineSink::finish()
{
    out_->output("[%s] received per-stage counts:", getName().c_str());
    for (int i = 0; i < NUM_STAGES; ++i) {
        out_->output(" %s=%" PRIu64, simpleStageName(i), received_[i]);
    }
    out_->output("\n");
}

// ============================================================================
// SimpleStageGate (PortModule)
// ============================================================================

std::set<int> SimpleStageGate::parseIntCsv(const std::string& s)
{
    std::set<int> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        size_t b = 0;
        while (b < tok.size() && std::isspace(static_cast<unsigned char>(tok[b]))) ++b;
        size_t e = tok.size();
        while (e > b && std::isspace(static_cast<unsigned char>(tok[e - 1]))) --e;
        if (e == b) continue;
        try { out.insert(std::stoi(tok.substr(b, e - b))); }
        catch (...) { /* ignore malformed token */ }
    }
    return out;
}

SimpleStageGate::SimpleStageGate(Params& params)
    : SST::PortModule()
{
    verbose_    = params.find<bool>("verbose", false);
    out_        = new Output("", verbose_ ? 1 : 0, 0, Output::STDOUT);

    stateKey_   = params.find<std::string>("state_key", "");
    dropStages_ = parseIntCsv(params.find<std::string>("drop_stages", ""));

    if (stateKey_.empty()) {
        out_->fatal(CALL_INFO, -1,
                    "SimpleStageGate: 'state_key' is required (must match the producer's state_key or getName()).\n");
    }

    if (verbose_) {
        std::string list;
        for (int s : dropStages_) {
            if (!list.empty()) list += ",";
            list += simpleStageName(s);
        }
        out_->output("SimpleStageGate: state_key='%s' drop_stages=[%s]\n",
                     stateKey_.c_str(), list.c_str());
    }
}

SimpleStageGate::~SimpleStageGate()
{
    if (out_) {
        out_->output("[SimpleStageGate %s] summary: evaluated=%" PRIu64 " dropped=%" PRIu64 "\n",
                     stateKey_.c_str(), evaluated_, dropped_);
        delete out_;
    }
}

const PipelineStateBase* SimpleStageGate::resolveState() const
{
    if (cached_) return cached_;
    cached_ = PipelineStateRegistry<PipelineStateBase>::get(stateKey_);
    return cached_;
}

void SimpleStageGate::eventSent(uintptr_t /*key*/, Event*& /*ev*/)
{
    // Send-side intercept is a no-op in this example; the gate installs only
    // on receives. Required override because PortModule declares it = 0.
}

void SimpleStageGate::interceptHandler(uintptr_t /*key*/, Event*& ev, bool& cancel)
{
    cancel = false;
    ++evaluated_;

    const PipelineStateBase* st = resolveState();
    if (!st) return;
    if (!dropStages_.count(st->currentKernel)) return;

    cancel = true;
    ++dropped_;

    if (verbose_) {
        out_->output("[SimpleStageGate %s] DROP stage=%s cycle=%d\n",
                     stateKey_.c_str(),
                     simpleStageName(st->currentKernel),
                     st->pipelineCycle);
    }

    // When cancel=true, the PortModule contract requires deleting the event
    // and setting it to nullptr (see portModule.h::interceptHandler docs).
    delete ev;
    ev = nullptr;
}
