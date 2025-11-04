// TODO: copyright goes here

#ifndef COMPONENTS_MERLIN_ENDPOINTNIC_H
#define COMPONENTS_MERLIN_ENDPOINTNIC_H

#include <sst/core/subcomponent.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include "sst/core/interfaces/simpleNetwork.h"
#include "NICPlugin.h"
#include <vector>

namespace SST {
namespace Merlin {

class endpointNIC : public SST::Interfaces::SimpleNetwork
{
protected:
    int vns;
    SST::Interfaces::SimpleNetwork* link_control;
    Output out;
    nid_t EP_id;

    // Pipeline of NIC plugins
    std::vector<NICPlugin*> plugin_pipeline;

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        endpointNIC,
        "merlin",
        "endpointNIC",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Endpoint NIC with pluggable functionality pipeline",
        SST::Interfaces::SimpleNetwork
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"plugins", "Comma-separated list of plugin names to load in order", ""}
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" },
        {"sourceRoutingPlugin", "NIC plugin slot for enabling source routing", "SST::Merlin::NICPlugin"}
        // If new plugin slots are added, document them here
    )

    endpointNIC(ComponentId_t cid, Params& params, int vns);
    ~endpointNIC();

    // SimpleNetwork interface methods - forward to link_control
    void init(unsigned int phase);
    void setup();
    void complete(unsigned int phase);
    void finish();

    bool send(Request* req, int vn);
    bool spaceToSend(int vn, int num_bits);
    Request* recv(int vn);
    bool requestToReceive(int vn);

    void sendUntimedData(Request* req);
    Request* recvUntimedData();

    void setNotifyOnReceive(HandlerBase* functor);
    void setNotifyOnSend(HandlerBase* functor);

    bool isNetworkInitialized() const;
    nid_t getEndpointID() const;
    const UnitAlgebra& getLinkBW() const;

protected:
    // Virtual methods for child classes to for NIC-specific functionality
    Request* processOutgoingRequest(Request* req, int vn);
    Request* processIncomingRequest(Request* req, int vn);

    // Load plugins from parameters
    void loadPlugins(Params& params);

    // Process request through plugin pipeline
    Request* processThroughPipeline(Request* req, int vn, bool outgoing);
};

} // namespace Merlin
} // namespace SST

#endif