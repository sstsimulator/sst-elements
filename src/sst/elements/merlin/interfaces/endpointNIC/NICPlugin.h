#ifndef COMPONENTS_MERLIN_NICPLUGIN_H
#define COMPONENTS_MERLIN_NICPLUGIN_H

#include <sst/core/subcomponent.h>
#include "sst/core/interfaces/simpleNetwork.h"

namespace SST {
namespace Merlin {

// Abstract base class for NIC plugins that can be chained in a pipeline
class NICPlugin : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Merlin::NICPlugin)

    NICPlugin(ComponentId_t cid, Params& params) : SubComponent(cid) {}
    virtual ~NICPlugin() {}

    // Process outgoing request (endpoint -> network)
    // Returns nullptr if packet should be dropped
    virtual SST::Interfaces::SimpleNetwork::Request* processOutgoing(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) = 0;

    // Process incoming request (network -> endpoint)
    // Returns nullptr if packet should be dropped
    virtual SST::Interfaces::SimpleNetwork::Request* processIncoming(
        SST::Interfaces::SimpleNetwork::Request* req, int vn) = 0;

    // Get plugin name for debugging
    virtual std::string getPluginName() const = 0;

    // Optional: called during init/setup/finish phases
    virtual void plugin_init(unsigned int phase) {}
    virtual void plugin_setup() {}
    virtual void plugin_finish() {}
    virtual void plugin_complete() {}
};

} // namespace Merlin
} // namespace SST

#endif
