// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_FAULTINJMANAGER_API_H
#define CARCOSA_FAULTINJMANAGER_API_H

#include <sst/core/subcomponent.h>
#include <sst/elements/memHierarchy/memLinkBase.h>
#include <sst/elements/memHierarchy/memEvent.h>

namespace SST {
namespace Carcosa {

class FaultInjManagerAPI : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Carcosa::FaultInjManagerAPI)

    FaultInjManagerAPI(ComponentId_t id, Params& params) : SubComponent(id) { }
    virtual ~FaultInjManagerAPI() { }

    virtual SST::MemHierarchy::MemEvent* processHighLinkMessage(SST::MemHierarchy::MemEvent* event) = 0;
    virtual SST::MemHierarchy::MemEvent* processLowLinkMessage(SST::MemHierarchy::MemEvent* event) = 0;

    virtual void addHighLinkRequest(const std::string& request) = 0;
    virtual void addLowLinkRequest(const std::string& request) = 0;

    /** Process all messages from PortModules (e.g. RegisterPM). Call from Hali::setup(). */
    virtual void processMessagesFromPMs() = 0;

    FaultInjManagerAPI() {};
    ImplementVirtualSerializable(SST::Carcosa::FaultInjManagerAPI);
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_FAULTINJMANAGER_API_H */
