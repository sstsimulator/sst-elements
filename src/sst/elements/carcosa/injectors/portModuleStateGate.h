// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_PORTMODULESTATEGATE_H
#define SST_ELEMENTS_CARCOSA_PORTMODULESTATEGATE_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include "sst/elements/carcosa/components/pipelineStateRegistry.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace SST::Carcosa {

/**
 * State-gated drop/flip PortModule; predicates AND (empty list matches all).
 */
class PortModuleStateGate : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        PortModuleStateGate,
        "carcosa",
        "PortModuleStateGate",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "State-gated drop/flip fault injector. Consults PipelineStateRegistry "
        "before firing the wrapped fault."
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",             "Required. Key into PipelineStateRegistry<PipelineStateBase>."},
        {"fault_mode",            "One of 'drop', 'flip', or 'drop_flip'. Default 'drop'."},
        {"drop_probability",      "Probability a matching event is dropped (used by 'drop'/'drop_flip'). Default 1.0."},
        {"flip_probability",      "Probability a matching event has a bit flipped (used by 'flip'/'drop_flip'). Default 1.0."},
        {"kernels",               "Optional CSV of kernel ids that enable the gate when currentKernel is in the set."},
        {"pipeline_cycle_start",  "Optional inclusive lower bound for pipelineCycle."},
        {"pipeline_cycle_end",    "Optional inclusive upper bound for pipelineCycle."},
        {"region_ids",            "Optional CSV of MemoryRegion ids; gate enables if the event's address range overlaps a valid region whose id is in the set."},
        {"region_names",          "Optional CSV of MemoryRegion::name strings; gate enables if the event's address range overlaps a valid region with a matching name."}
    )

    PortModuleStateGate(Params& params);
    PortModuleStateGate() = default;
    ~PortModuleStateGate() override = default;

protected:
    /** Event address range; valid=false for non-MemEvents. */
    struct EventAddress {
        bool     valid = false;
        uint64_t addr  = 0;
        uint64_t size  = 0;
    };

    /** Predicate over PipelineStateBase + event address. */
    using Predicate = std::function<bool(const PipelineStateBase&, const EventAddress&)>;

    enum class Mode { Drop, Flip, DropFlip };

    // Configuration
    std::string            stateKey_;
    Mode                   mode_ = Mode::Drop;
    double                 dropProb_ = 1.0;
    double                 flipProb_ = 1.0;

    // Predicate configuration, kept in serializable form so checkpoint
    // restore can rebuild predicates_ (std::function is not serializable).
    std::string            kernelsCsv_;
    bool                   hasCycleRange_ = false;
    int                    cycleStart_ = 0;
    int                    cycleEnd_   = INT32_MAX;
    std::string            regionIdsCsv_;
    std::string            regionNamesCsv_;

    // Composed predicates (AND semantics; empty list => always-match).
    std::vector<Predicate> predicates_;

    // drop_flip-style dual-trigger state, set in doInjection(), read in executeFaults().
    std::array<bool, 2>    triggered_ = {{false, false}};

    /**
     * Parse params then rebuildPredicates() — that rebuild also runs on restore.
     */
    virtual void buildPredicates(Params& params);

    /** Rebuild predicates_ from serializable config (also after deserialize). */
    virtual void rebuildPredicates();

    /** Default: AND of predicates_. Override for arbitrary match logic. */
    virtual bool matchesState(const PipelineStateBase& state,
                              const EventAddress& ea) const;

    bool doInjection(Event* ev) override;
    void executeFaults(Event*& ev) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultInjectorBase::serialize_order(ser);
        SST_SER(stateKey_);
        SST_SER(mode_);
        SST_SER(dropProb_);
        SST_SER(flipProb_);
        SST_SER(triggered_);
        SST_SER(kernelsCsv_);
        SST_SER(hasCycleRange_);
        SST_SER(cycleStart_);
        SST_SER(cycleEnd_);
        SST_SER(regionIdsCsv_);
        SST_SER(regionNamesCsv_);
        // predicates_ is a vector<std::function> and cannot be serialized;
        // checkpoint restore uses the serialization ctor (NOT the params
        // ctor), so rebuild the lambdas from the config members here.
        if (ser.mode() == SST::Core::Serialization::serializer::UNPACK)
            rebuildPredicates();
    }
    ImplementVirtualSerializable(SST::Carcosa::PortModuleStateGate)

private:
    static Mode parseMode(const std::string& s);
};

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_PORTMODULESTATEGATE_H
