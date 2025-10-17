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

#ifndef SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H
#define SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H

#include "sst/core/portModule.h"
#include "sst/core/event.h"
#include "sst/core/output.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/carcosa/faultlogic/faultBase.h"
#include <sst_config.h>
#include "sst/core/rng/mersenne.h"
#include <vector>
#include <string>
#include <random>
#include <array>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;
class FaultBase;

#define SEND_RECEIVE_VALID  {{true, true}}
#define RECEIVE_VALID       {{true, false}}
#define SEND_VALID          {{false, true}}

/********** FaultInjectorBase **********/

enum installDirection {
    Send = 0,
    Receive,
    Invalid
};

/**
 * Base class containing required functions and basic data for
 * creating fault injection on component ports
 */
class FaultInjectorBase : public SST::PortModule
{
public:

    SST_ELI_REGISTER_PORTMODULE(
        FaultInjectorBase,
        "carcosa",
        "FaultInjectorBase",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Base PortModule class used to connect fault injection logic to components"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"installDirection", "Flag which direction the injector should read from on a port. Valid optins are \'Send\', \'Receive\', and \'Both\'. Default is \'Receive\'."},
        {"injectionProbability", "The probability with which an injection should occur. Valid inputs range from 0 to 1. Default = 0.5."},
        {"debug", "Integer determining if debug should be active. 0 disables, 1 sends output to STDOUT, 2 to STDERR. Default = 0"},
        {"debug_level", "Integer determining verbosity of debug output. 1 enables basic text output, 2 enables signficant activity output."}
    )

    // SST_ELI_DOCUMENT_STATISTICS(
    //     // Trigger Statistics
    //     {"EventsArrived",           "Number of events that passed through the fault injector", "count", 1},
    //     {"FaultsTriggered",         "Number of events that triggered a fault", "count", 1}
    // )

    FaultInjectorBase(Params& params);

    FaultInjectorBase() = default;
    ~FaultInjectorBase();

    void virtual eventSent(uintptr_t key, Event*& ev) override;
    void virtual interceptHandler(uintptr_t key, Event*& ev, bool& cancel) override;

    bool installOnReceive() override
    {
        switch (installDirection_) {
            case Send:
                return false;
            case Receive:
            default:
                return true;
        }
    }
    bool installOnSend() override
    {
        switch (installDirection_) {
            case Send:
                return true;
            case Receive:
            default:
                return false;
        }
    }

    double getInjectionProb() {
        return injectionProbability_;
    }

    void cancelDelivery() {
        *cancel_ = true;
    }

    installDirection setInstallDirection(std::string param);

    installDirection getInstallDirection() {
        return installDirection_;
    }

    enum memEventType {
        DataRequest = 0,
        Response,
        Writeback,
        RoutedByAddr,
        Invalid
    };

    SST::Output* getOutput() {
        return out_;
    }

    SST::Output* getDebug() {
        return dbg_;
    }

protected:
    SST::Output* out_;
    SST::Output* dbg_;
    std::vector<FaultBase*> fault;
    bool* cancel_;
    installDirection installDirection_ = installDirection::Receive;
    double injectionProbability_ = 0.5;
    SST::RNG::MersenneRNG base_rng_;
private:
    std::array<bool,2> valid_installation_ = {{false, false}};
    bool valid_installs_set = false;
protected: 

    bool doInjection();
    virtual void executeFaults(Event*& ev);

    /**
     * This function MUST be called by the derived class constructor
     * @arg params pass the same params object to this function
     * @arg valid_install_ pass either SEND_VALID, RECEIVE_VALID, 
     *      or SEND_RECEIVE_VALID
     */
    void setValidInstallation(Params& params, std::array<bool,2> valid_install);

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(out_);
        SST_SER(dbg_);
        SST_SER(fault);
        SST_SER(cancel_);
        SST_SER(installDirection_);
        SST_SER(injectionProbability_);
        SST_SER(base_rng_);
        SST_SER(valid_installation_);
        SST_SER(valid_installs_set);
    }
    ImplementVirtualSerializable(SST::Carcosa::FaultInjectorBase)

    // Statistics
    // Statistic<uint64_t> stat_eventsArrived;
    // Statistic<uint64_t> stat_faultsTriggered;
};

} // namespace SST::FaultInjectorBase

#endif // SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H