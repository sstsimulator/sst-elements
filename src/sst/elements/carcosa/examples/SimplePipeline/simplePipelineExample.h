// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory of the distribution.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.
//
// ============================================================================
// SimplePipelineExample
// ----------------------------------------------------------------------------
// A minimal four-stage pipeline (FETCH -> DECODE -> EXECUTE -> COMMIT) that
// demonstrates how a user-level simulator component can publish its FSM state
// through PipelineStateRegistry<PipelineStateBase> so that PortModule fault
// injectors attached to its downstream links can make state-aware decisions.
//
// Wiring:
//
//   +-----------------------+          +-----------------------+
//   | SimplePipelineProducer| -- out ->| SimplePipelineSink    |
//   |   (4-stage FSM)       |          |  (counts per stage)   |
//   +-----------------------+          +-----------------------+
//             |                                 ^
//             | publishes PipelineStateBase     | PortModule on "in"
//             v                                 | reads snapshot and
//   PipelineStateRegistry<PipelineStateBase>    | cancels events whose
//             ^                                 | currentKernel is in
//             |    subscribed by key            | drop_stages
//             +-----------------------> SimpleStageGate (PortModule)
//
// The registry lookup rendezvous key is a plain string; the producer publishes
// under `state_key` (defaults to getName()) and the gate subscribes with the
// same key.
// ============================================================================

#ifndef SST_ELEMENTS_CARCOSA_EXAMPLES_SIMPLE_PIPELINE_H
#define SST_ELEMENTS_CARCOSA_EXAMPLES_SIMPLE_PIPELINE_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/portModule.h>
#include <sst/core/serialization/serializer.h>

#include "sst/elements/carcosa/components/pipelineStateRegistry.h"

#include <cstdint>
#include <set>
#include <string>

namespace SST {
namespace Carcosa {
namespace Examples {

/** The four pipeline stages the producer cycles through. */
enum SimpleStage {
    STAGE_FETCH   = 0,
    STAGE_DECODE  = 1,
    STAGE_EXECUTE = 2,
    STAGE_COMMIT  = 3,
    NUM_STAGES    = 4
};

const char* simpleStageName(int id);

/** Event carrying the producer's stage id at send time. */
class SimpleStageEvent : public SST::Event {
public:
    SimpleStageEvent() : SST::Event() {}
    explicit SimpleStageEvent(int stage) : SST::Event(), stageId(stage) {}

    int stageId = -1;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(stageId);
    }
    ImplementSerializable(SST::Carcosa::Examples::SimpleStageEvent)
};

/**
 * Clocked 4-stage FSM publisher; emits SimpleStageEvent on `out` each tick.
 */
class SimplePipelineProducer : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        SimplePipelineProducer,
        "carcosa",
        "SimplePipelineProducer",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Four-stage FSM driver that publishes stage state via PipelineStateRegistry",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",   "PipelineStateRegistry key to publish under (empty = getName()).", ""},
        {"total_cycles","Number of full FETCH..COMMIT iterations before signalling sim-end. 0 = run forever.", "4"},
        {"clock",       "Clock frequency; one stage advance per tick.",                                         "1MHz"},
        {"verbose",     "Log each transition to stdout.",                                                        "false"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"out", "Outgoing stage events", { "carcosa.Examples.SimpleStageEvent" } }
    )

    SimplePipelineProducer(SST::ComponentId_t id, SST::Params& params);
    ~SimplePipelineProducer() override;

private:
    bool tick(SST::Cycle_t cycle);

    SST::Output*       out_        = nullptr;
    SST::Link*         outLink_    = nullptr;
    PipelineStateBase* state_      = nullptr;

    std::string state_key_;
    int  totalCycles_   = 0;
    int  tickCount_     = 0;
    bool verbose_       = false;
};

/** Counts events per stage id and prints the histogram in finish(). */
class SimplePipelineSink : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        SimplePipelineSink,
        "carcosa",
        "SimplePipelineSink",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Counts stage events per stage id (sink for SimplePipelineProducer)",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose", "Log each received event to stdout.", "false"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"in", "Incoming stage events", { "carcosa.Examples.SimpleStageEvent" } }
    )

    SimplePipelineSink(SST::ComponentId_t id, SST::Params& params);
    ~SimplePipelineSink() override;

    void finish() override;

private:
    void handleEvent(SST::Event* ev);

    SST::Output* out_    = nullptr;
    SST::Link*   inLink_ = nullptr;
    bool         verbose_ = false;
    uint64_t     received_[NUM_STAGES] = {};
};

/**
 * Drops events when producer's currentKernel is in drop_stages (lazy snapshot).
 */
class SimpleStageGate : public SST::PortModule {
public:
    SST_ELI_REGISTER_PORTMODULE(
        SimpleStageGate,
        "carcosa",
        "SimpleStageGate",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Drops events when the subscribed producer's stage id matches a configured set"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",   "PipelineStateRegistry key published by the producer.",                       ""},
        {"drop_stages", "Comma-separated stage ids (0..3) on which to drop. Empty = never drop.",     ""},
        {"verbose",     "Log each drop to stdout.",                                                    "false"}
    )

    SimpleStageGate(SST::Params& params);
    SimpleStageGate() = default;
    ~SimpleStageGate() override;

    bool installOnReceive() override { return true; }
    bool installOnSend()    override { return false; }

    void eventSent(uintptr_t key, SST::Event*& ev) override;
    void interceptHandler(uintptr_t key, SST::Event*& ev, bool& cancel) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        SST::PortModule::serialize_order(ser);
    }
    ImplementVirtualSerializable(SST::Carcosa::Examples::SimpleStageGate)

private:
    const PipelineStateBase* resolveState() const;
    static std::set<int>     parseIntCsv(const std::string& s);

    SST::Output*  out_ = nullptr;
    std::string   state_key_;
    std::set<int> dropStages_;
    bool          verbose_ = false;

    uint64_t evaluated_ = 0;
    uint64_t dropped_   = 0;

    mutable const PipelineStateBase* cached_ = nullptr;
};

} // namespace Examples
} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_EXAMPLES_SIMPLE_PIPELINE_H */
