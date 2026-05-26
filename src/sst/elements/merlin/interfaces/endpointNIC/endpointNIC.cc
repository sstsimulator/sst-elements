// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "endpointNIC.h"

namespace SST {
namespace Merlin {

EndpointNIC::EndpointNIC(ComponentId_t cid, Params& params, int vns) :
    SST::Interfaces::SimpleNetwork(cid),
    link_control(nullptr),
    vns(vns)
{

    out.init(getName() + ": ", 0, 0, Output::STDOUT);
    // See if we need to set up a nid map
    bool found = false;
    EP_id = params.find<int>("EP_id",-1,found);
    if ( !found ) {
        out.fatal(CALL_INFO, -1, "EndpointNIC requires 'EP_id' parameter to be set\n");
    }

    loadPlugins(params);

    link_control = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
    ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);

    if (!link_control) {
        out.fatal(CALL_INFO, -1, "Failed to load LinkControl subcomponent\n");
    }
}

// Pure virtual destructor must have implementation even though class is abstract
EndpointNIC::~EndpointNIC()
{
}

void EndpointNIC::loadPlugins(Params& params)
{
    std::vector<std::string> plugin_names;
    params.find_array<std::string>("plugin_names",plugin_names);

    // Load indexed plugin slots
    int slot_num = 0;

    while (slot_num < static_cast<int>(plugin_names.size())) {
        // Load user subcomponent
        NICPlugin* plugin = loadUserSubComponent<NICPlugin>(
            plugin_names[slot_num], ComponentInfo::SHARE_NONE);

        if (plugin) {
            plugin_pipeline.push_back(plugin);
            out.verbose(CALL_INFO, 1, 0, "Loaded plugin %d: %s\n",
                       slot_num, plugin->getPluginName().c_str());
        } else {
            out.fatal(CALL_INFO, -1, "Failed to load plugin %d: %s\n",
                     slot_num, plugin_names[slot_num].c_str());
        }
        slot_num++;
    }

    if (plugin_pipeline.empty()) {
        out.verbose(CALL_INFO, 1, 0, "No plugins loaded - operating as pass-through NIC\n");
    }
}

void EndpointNIC::init(unsigned int phase)
{
    for (auto* plugin : plugin_pipeline) {
        plugin->plugin_init(phase);
    }
    link_control->init(phase);
}

void EndpointNIC::setup()
{
    for (auto* plugin : plugin_pipeline) {
        plugin->plugin_setup();
    }
    link_control->setup();
}

void EndpointNIC::complete(unsigned int phase)
{
    for (auto* plugin : plugin_pipeline) {
        plugin->plugin_complete();
    }
    link_control->complete(phase);
}

void EndpointNIC::finish()
{
    for (auto* plugin : plugin_pipeline) {
        plugin->plugin_finish();
    }
    link_control->finish();
}

bool EndpointNIC::send(Request* req, int vn)
{
    // Process through outgoing pipeline
    Request* processed_req = processThroughPipeline(req, vn, true);
    if (!processed_req) {
        return false;
    }

    return link_control->send(processed_req, vn);
}

bool EndpointNIC::spaceToSend(int vn, int num_bits)
{
    return link_control->spaceToSend(vn, num_bits);
}

SST::Interfaces::SimpleNetwork::Request* EndpointNIC::recv(int vn)
{
    Request* req = link_control->recv(vn);
    if (!req) {
        return nullptr;
    }

    // Process through incoming pipeline
    return processThroughPipeline(req, vn, false);
}

bool EndpointNIC::requestToReceive(int vn)
{
    return link_control->requestToReceive(vn);
}

void EndpointNIC::sendUntimedData(Request* req)
{
    // For untimed data, we typically don't process it through NIC logic
    link_control->sendUntimedData(req);
}

SST::Interfaces::SimpleNetwork::Request* EndpointNIC::recvUntimedData()
{
    return link_control->recvUntimedData();
}

void EndpointNIC::setNotifyOnReceive(HandlerBase* functor)
{
    link_control->setNotifyOnReceive(functor);
}

void EndpointNIC::setNotifyOnSend(HandlerBase* functor)
{
    link_control->setNotifyOnSend(functor);
}

bool EndpointNIC::isNetworkInitialized() const
{
    return link_control->isNetworkInitialized();
}

SST::Interfaces::SimpleNetwork::nid_t EndpointNIC::getEndpointID() const
{
    return link_control->getEndpointID();
}

const UnitAlgebra& EndpointNIC::getLinkBW() const
{
    return link_control->getLinkBW();
}

// Base implementations for child classes to override
SST::Interfaces::SimpleNetwork::Request* EndpointNIC::processOutgoingRequest(Request* req, int vn)
{
    // Base implementation just passes through
    return req;
}

SST::Interfaces::SimpleNetwork::Request* EndpointNIC::processIncomingRequest(Request* req, int vn)
{
    // Base implementation just passes through
    return req;
}

SST::Interfaces::SimpleNetwork::Request* EndpointNIC::processThroughPipeline(
    Request* req, int vn, bool outgoing)
{
    if (!req) return nullptr;

    Request* current_req = req;

    if (outgoing) {
        // Process through pipeline in forward order
        for (auto* plugin : plugin_pipeline) {
            current_req = plugin->processOutgoing(current_req, vn);
            if (!current_req) {
                // Plugin dropped the packet
                return nullptr;
            }
        }
    } else {
        // Process through pipeline in reverse order
        for (auto it = plugin_pipeline.rbegin(); it != plugin_pipeline.rend(); ++it) {
            current_req = (*it)->processIncoming(current_req, vn);
            if (!current_req) {
                // Plugin dropped the packet
                return nullptr;
            }
        }
    }

    return current_req;
}

} // namespace Merlin
} // namespace SST
