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

#ifndef SST_ELEMENTS_CARCOSA_STUCKATFAULT_H
#define SST_ELEMENTS_CARCOSA_STUCKATFAULT_H

#include "../faultInjectorBase.h"

namespace SST::Carcosa {

class StuckAtFault : public SST::Carcosa::FaultInjectorBase
{
public:
    SST_ELI_REGISTER_PORTMODULE(
        StuckAtFault,
        "carcosa",
        "stuckAtFault",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Basic PortModule to enable a stuck bit hardware fault"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"installDirection", "Flag which direction the injector should read from on a port. Valid optins are \'Send\', \'Receive\', and \'Both\'. Default is \'Receive\'."},
        {"injectionProbability", "The probability with which an injection should occur. Valid inputs range from 0 to 1. Default = 0.5."},
        {"stuckAtAddrs", "Map of addresses and bits that are stuck, along with the values of those stuck bits."}
    )

    StuckAtFault(Params& params);

    StuckAtFault() = default;
    ~StuckAtFault() {}
private:

    // map of addr->{bit, value} for saving stuck bit values
    std::map<SST::MemHierarchy::Addr, std::vector<std::pair<int, bool>>> stuckAtMap;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(installDirection_);
        SST_SER(injectionProbability_);
        SST_SER(stuckAtMap);
    }
    ImplementSerializable(SST::Carcosa::StuckAtFault)

    /**
     * Read event payload and perform the following:
     *  - If stuckAtMap.at(addr) exists, compare all listed bits with payload value
     *  - If payload value does not match mapped value, add bit to flip mask
     *  - Once all stored bit values have been compared, use flip mask to modify address data
     */
    void fault(Event*& ev);
};

}

#endif // SST_ELEMENTS_CARCOSA_STUCKATFAULT_H