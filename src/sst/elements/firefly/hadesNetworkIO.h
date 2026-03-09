#pragma once

#include <sst/core/output.h>
#include <sst/elements/hermes/networkIOapi.h>
#include <vector>

namespace SST {
namespace Firefly {

class VirtNic;

class HadesNetworkIO : public Hermes::NetworkIO::Interface {

  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        HadesNetworkIO,
        "firefly",
        "hadesNetworkIO",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Network Storage I/O API",
        SST::Hermes::Interface
    )
    SST_ELI_DOCUMENT_PARAMS(
        {"storageNodesList", "List of storage nodes Ids used in simulation", ""},
        {"storageNodeCapacity", "The capacity of a storage node", "1 GiB"},
        {"verboseLevel","Sets the level of debug verbosity","0"},
        {"verboseMask","Sets the debug mask","-1"},
    )

    HadesNetworkIO(ComponentId_t id, Params& params);

    ~HadesNetworkIO() {}

    void setOS( Hermes::OS* os ) override;

    void setup() override;

    std::string getName() override { return "networkIO"; }

    std::string getType() override { return "networkIO"; }

    void networkIORead(Hermes::Vaddr dest, uint64_t offset, uint64_t length,
                      Callback callback) override;
    
    void networkIOWrite(uint64_t offset, Hermes::Vaddr src, uint64_t length,
                       Callback callback) override;

  private:
    Hades* m_osPtr;
    VirtNic* m_nicPtr;
    SST::Output m_dbg;
    std::vector<int> m_storageNodesList;
    int64_t m_numSsdNodes;
    int64_t m_ssd_start_node;
    int64_t m_storageNodeCapacity;

    int64_t calcTargetNid(int64_t offset);
};

} // namespace Firefly
} // namespace SST

