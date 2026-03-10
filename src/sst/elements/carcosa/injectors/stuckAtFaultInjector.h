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

#ifndef SST_ELEMENTS_CARCOSA_STUCKATFAULTINJECTOR_H
#define SST_ELEMENTS_CARCOSA_STUCKATFAULTINJECTOR_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include <string>

namespace SST::Carcosa {

class StuckAtFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        StuckAtFaultInjector,
        "carcosa",
        "StuckAtFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a stuck bit within a given component"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"masks", "String array formatted as [\"addr0, byte0, zeroMask0, oneMask0\",...,\"addrN, byteN, zeroMaskN, oneMaskN\"]." \
        "Addresses are expected to be in hexadecimal, and masks are 8 bit strings."},
        {"endianness", "Byte ordering in memory. Given as a string containing \'little\' or \'big\'. Default: little"}
    )

    StuckAtFaultInjector(Params& params);

    StuckAtFaultInjector() = default;
    ~StuckAtFaultInjector() {}
protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
    }
    ImplementVirtualSerializable(SST::Carcosa::StuckAtFaultInjector)
}; // class StuckAtFaultInjector

} // namespace SST::Carcosa

#endif