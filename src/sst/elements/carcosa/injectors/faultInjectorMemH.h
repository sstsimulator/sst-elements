// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_FAULTINJECTORMEMH_H
#define SST_ELEMENTS_CARCOSA_FAULTINJECTORMEMH_H

#include <sst_config.h>
#include "sst/core/event.h"
#include "sst/core/statapi/statbase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include "sst/elements/carcosa/components/pmDataRegistry.h"
#include <vector>

namespace SST::Carcosa {

/**
 * PortModule for MemHierarchy fault injection with PMDataRegistry support.
 * Inherits from FaultInjectorBase; adds PM registry integration so events
 * carrying PM data skip fault injection.
 */
class FaultInjectorMemH : public FaultInjectorBase
{
public:
    SST_ELI_REGISTER_PORTMODULE(
        FaultInjectorMemH,
        "carcosa",
        "faultInjectorMemH",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "MemHierarchy fault injection PortModule with PMDataRegistry support"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"pmRegistryIds", "Comma-separated registry ids to listen to (e.g. 'default' or 'hali_0,hali_1'). Default 'default'."},
        {"pmId", "This PM's id for manager tracking. Optional."},
        {"debugManagerLogic", "If true, print [ManagerLogic] and PM-read debug messages. Default 0."},
        {"injection_probability", "Probability for fault injection to trigger. Default 0.5."},
        {"faultType", "The type of fault to be injected. Options are stuckAt, randomFlip, randomDrop, corruptMemRegion, and custom."}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"skipped_pm_events", "Count of events whose fault injection was skipped due to PM data match.", "count", 1}
    )

    FaultInjectorMemH(Params& params);

    FaultInjectorMemH() = default;
    ~FaultInjectorMemH() {}

    /** Call during init to push RegisterPM to all registries. Invoked by link if supported. */
    void init(unsigned phase);

protected:
    double injection_probability_ = 0.5;

    bool doInjection() override;
    void executeFaults(Event*& ev) override;

private:
    std::vector<std::string> pmRegistryIds_;
    std::vector<PMDataRegistry*> registries_;
    std::string pmId_;
    bool registerPMSent_ = false;
    bool debugManagerLogic_ = false;
    Statistics::Statistic<uint64_t>* stat_skipped_ = nullptr;

    /** Resolve registry ids to pointers and push RegisterPM if not done yet. Call from init() or first event. */
    void ensureRegistriesResolved();

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        FaultInjectorBase::serialize_order(ser);
        SST_SER(injection_probability_);
        SST_SER(pmRegistryIds_);
        /* registries_ not serialized; re-resolved from pmRegistryIds_ via ensureRegistriesResolved() */
        SST_SER(pmId_);
        SST_SER(registerPMSent_);
        SST_SER(debugManagerLogic_);
        SST_SER(stat_skipped_);
    }
    ImplementVirtualSerializable(SST::Carcosa::FaultInjectorMemH)
};

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_FAULTINJECTORMEMH_H
