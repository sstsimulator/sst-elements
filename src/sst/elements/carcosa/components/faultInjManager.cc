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

#include "sst_config.h"
#include "sst/elements/carcosa/components/faultInjManager.h"
#include "sst/elements/carcosa/components/pmDataRegistry.h"
#include <limits>
#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
using namespace SST::Carcosa;

FaultInjManager::FaultInjManager(ComponentId_t id, Params& params) : FaultInjManagerAPI(id, params)
{
    connectionId_ = params.find<std::string>("pmRegistryId", "default");
    debugManagerLogic_ = params.find<bool>("debugManagerLogic", false);
    PMRegistryResolver::registerRegistry(connectionId_, &registry_);
}

FaultInjManager::~FaultInjManager()
{
    PMRegistryResolver::unregisterRegistry(connectionId_);
}

void FaultInjManager::processMessagesFromPMs()
{
    std::vector<ManagerMessage> msgs = registry_.popAllMessagesFromPMs();
    for (const ManagerMessage& m : msgs) {
        if (m.type == ManagerMessage::Type::RegisterPM) {
            managedPMIds_.insert(m.pmId);
            if (debugManagerLogic_) {
                getSimulationOutput().output("[ManagerLogic] %s (registry=%s): registered PM pmId=%s (total managed=%zu)\n",
                    getParentComponentName().c_str(), connectionId_.c_str(), m.pmId.c_str(), managedPMIds_.size());
            }
        }
    }
}

// PM data is tracked in the registry by event ID; the original event is always returned unchanged.
SST::MemHierarchy::MemEvent* FaultInjManager::processHighLinkMessage(SST::MemHierarchy::MemEvent* event)
{
    if (!pendingHighLinkRequests.empty()) {
        std::string PM_Instr = pendingHighLinkRequests.front();
        pendingHighLinkRequests.erase(pendingHighLinkRequests.begin());
        auto eventId = event->getID();
        registry_.registerPMData(eventId, PM_Instr);
#ifdef __SST_DEBUG_OUTPUT__
        getSimulationOutput().output("FaultInjManager: Registered PM data for event ID <%llu,%d> cmd: %s\n",
                  eventId.first, eventId.second, PM_Instr.c_str());
#endif
    }
    return event;
}

SST::MemHierarchy::MemEvent* FaultInjManager::processLowLinkMessage(SST::MemHierarchy::MemEvent* event)
{
    if (!pendingLowLinkRequests.empty()) {
        std::string PM_Instr = pendingLowLinkRequests.front();
        pendingLowLinkRequests.erase(pendingLowLinkRequests.begin());
        registry_.registerPMData(event->getID(), PM_Instr);
    }
    return event;
}

void FaultInjManager::addHighLinkRequest(const std::string& request)
{
    pendingHighLinkRequests.push_back(request);
}

void FaultInjManager::addLowLinkRequest(const std::string& request)
{
    pendingLowLinkRequests.push_back(request);
}

void FaultInjManager::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SubComponent::serialize_order(ser);
    SST_SER(connectionId_);
    SST_SER(debugManagerLogic_);
    SST_SER(pendingHighLinkRequests);
    SST_SER(pendingLowLinkRequests);
    SST_SER(managedPMIds_);
    SST_SER(registry_);

    // Re-register with the resolver so FaultInjectorMemH lookups work after restore.
    if (ser.mode() == SST::Core::Serialization::serializer::UNPACK) {
        PMRegistryResolver::registerRegistry(connectionId_, &registry_);
    }
}
