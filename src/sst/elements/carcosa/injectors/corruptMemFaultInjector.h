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

#ifndef SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULTINJECTOR_H
#define SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULTINJECTOR_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"

namespace SST::Carcosa {

class CorruptMemFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        CorruptMemFaultInjector,
        "carcosa",
        "CorruptMemFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a whole memory region being corrupted"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"regions", "Formatted as an array of strings: [\"start_addr0, end_addr0\", \"start_addr1, end_addr1\",...,\"start_addrN, end_addrN\"]. Addresses expected in hexadecimal."}
    )

    CorruptMemFaultInjector(Params& params);

    CorruptMemFaultInjector() = default;
    ~CorruptMemFaultInjector() {}
protected:
    void executeFaults(Event*& ev) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
    }
    ImplementVirtualSerializable(SST::Carcosa::CorruptMemFaultInjector)
}; // class CorruptMemFaultInjector

} // namespace SST::Carcosa

#endif