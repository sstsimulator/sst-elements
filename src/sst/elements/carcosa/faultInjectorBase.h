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
#include <sst_config.h>
#include <vector>
#include <string>
#include <random>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;

class FaultInjectorBase;

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
    /************** FaultBase **************/

    /** TODO:
     * Parameters for switching between interface-driven and normal instantiation
     * A way to read in data to choose which logic to use
     *  - Might be possible to parameterize the entirety of the logic, but would be easier if I can 
     *    build a "library" of functions that are loaded dynamically
    */

    class FaultBase {
    public:
        
        enum memEventType {
            DataRequest = 0,
            Response,
            Writeback,
            RoutedByAddr,
            Invalid
        };

        FaultBase(Params& params, FaultInjectorBase* injector);

        FaultBase() = default;
        ~FaultBase() {}

        virtual void faultLogic(Event*& ev) {}

        SST::Output*& getSimulationOutput();

        SST::Output*& getSimulationDebug();

        installDirection setInstallDirection(std::string param);

        SST::MemHierarchy::MemEvent* convertMemEvent(Event*& ev);

        dataVec& getMemEventPayload(Event*& ev);

        void setMemEventPayload(Event*& ev, dataVec newPayload);

        memEventType getMemEventCommandType(Event*& ev);

        bool doInjection();

    protected:

        FaultInjectorBase* injector_ = nullptr;

        bool valid_installation_[2] = {false, false};
        std::default_random_engine generator_;
        std::uniform_real_distribution<double> distribution_;

        void toggleReceiveValid() {
            valid_installation_[0] = true;
        }

        void toggleSendValid() {
            valid_installation_[1] = true;
        }

        void toggleSendReceiveValid() {
            valid_installation_[0] = true;
            valid_installation_[1] = true;
        }
        // TODO: Figure out how to properly set up serialization for this
        // void serialize_order(SST::Core::Serialization::serializer& ser) 
        // {
        //     SST::PortModule::serialize_order(ser);
        //     // serialize parameters like `SST_SER(<param>)
        // }
        // ImplementSerializable(SST::Carcosa::FaultBase)
    };

    SST_ELI_REGISTER_PORTMODULE(
        FaultInjectorBase,
        "carcosa",
        "faultInjectorBase",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Barebones PortModule used to connect fault injection logic to components"
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
    ~FaultInjectorBase() {
        delete fault;
    }

    void virtual eventSent(uintptr_t key, Event*& ev) override;
    void virtual interceptHandler(uintptr_t key, Event*& data, bool& cancel) override;

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

    installDirection getInstallDirection() {
        return installDirection_;
    }

protected:
    FaultBase* fault;

    bool* cancel_;

    installDirection installDirection_ = installDirection::Receive;
    double injectionProbability_ = 0.5;

    SST::Output* out_;
    SST::Output* dbg_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        // SST_SER(fault);
        SST_SER(installDirection_);
        SST_SER(injectionProbability_);
    }
    ImplementSerializable(SST::Carcosa::FaultInjectorBase)

    // Statistics
    // Statistic<uint64_t> stat_eventsArrived;
    // Statistic<uint64_t> stat_faultsTriggered;
};

} // namespace SST::FaultInjectorBase

#endif // SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H