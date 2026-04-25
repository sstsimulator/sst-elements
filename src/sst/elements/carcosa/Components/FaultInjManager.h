// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_FAULTINJMANAGER_H
#define CARCOSA_FAULTINJMANAGER_H

#include "sst/elements/carcosa/Components/FaultInjManagerAPI.h"
#include "sst/elements/carcosa/Components/PMDataRegistry.h"
#include <vector>
#include <string>
#include <set>

namespace SST {
namespace Carcosa {

class FaultInjManager : public FaultInjManagerAPI
{
public:

    SST_ELI_REGISTER_SUBCOMPONENT(
            FaultInjManager,
            "Carcosa",
            "FaultInjManager",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Manages fault injection for Carcosa components",
            SST::Carcosa::FaultInjManagerAPI
            )

    SST_ELI_DOCUMENT_PARAMS()

    FaultInjManager(ComponentId_t id, Params& params);
    ~FaultInjManager();

    SST::MemHierarchy::MemEvent* processHighLinkMessage(SST::MemHierarchy::MemEvent* event) override;
    SST::MemHierarchy::MemEvent* processLowLinkMessage(SST::MemHierarchy::MemEvent* event) override;

    void addHighLinkRequest(const std::string& request) override;
    void addLowLinkRequest(const std::string& request) override;

    void processMessagesFromPMs() override;

    FaultInjManager() : FaultInjManagerAPI() {};
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Carcosa::FaultInjManager);

private:
    std::string connectionId_;
    bool debugManagerLogic_ = false;
    PMDataRegistry registry_;
    std::vector<std::string> pendingHighLinkRequests;
    std::vector<std::string> pendingLowLinkRequests;
    std::set<std::string> managedPMIds_;
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_FAULTINJMANAGER_H */
