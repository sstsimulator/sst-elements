#include "sst_config.h"

#include "hades.h"
#include "hadesNetworkIO.h"

using namespace SST::Firefly;

HadesNetworkIO::HadesNetworkIO(ComponentId_t id, Params& params) :
    Hermes::NetworkIO::Interface(id),
    m_nicPtr(NULL)
{
    m_dbg.init("@t:HadesNetworkIO::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",-1),
        Output::STDOUT );
    
    auto parse = [](const std::string& s) 
    {
        std::vector<int> v;
        std::istringstream ss(s);
        for(int i; ss >> i; ss.ignore()) v.push_back(i);
        return v;
    };
    m_numSsdNodes = params.find<int64_t>("numStorageNodes", 0);
    m_ssd_start_node = params.find<int64_t>("ssd_start_node", 0);
    
    for (int64_t i = 0; i < m_numSsdNodes; i++) {
        m_storageNodesList.push_back(m_ssd_start_node + i);
    }
    
    m_storageNodeCapacity = params.find<UnitAlgebra>("storageNodeCapacity", "1GiB").getRoundedValue();
}

void HadesNetworkIO::setOS( Hermes::OS* os )
{
    m_osPtr = dynamic_cast<Hades*>(os);
    assert(m_osPtr);
    m_nicPtr = m_osPtr->getNic();
}

void HadesNetworkIO::setup()
{
}


void HadesNetworkIO::networkIORead(Hermes::Vaddr dest, uint64_t offset, uint64_t length, Callback callback)
{
    m_dbg.verbose(CALL_INFO, 1, 0, "network_read: dest=%lx offset=%lu length=%lu \n", 
                  dest, offset, length);
    int targetNid = calcTargetNid(offset);
    m_nicPtr->networkIORead(targetNid, dest, length, callback);
}

void HadesNetworkIO::networkIOWrite(uint64_t offset, Hermes::Vaddr src, uint64_t length, Callback callback)
{
    m_dbg.verbose(CALL_INFO, 1, 0, "network_write: offset=%lu src=%lx length=%lu \n", 
                  offset, src, length);
    int targetNid = calcTargetNid(offset);
    m_nicPtr->networkIOWrite(targetNid, src, length, callback);

}

int64_t HadesNetworkIO::calcTargetNid(int64_t offset)
{
    assert(m_storageNodesList.size() > 0 && "No storage nodes defined in storageNodesList");
    
    int nodeIndex = (offset/m_storageNodeCapacity)%m_storageNodesList.size();
    return m_storageNodesList.at(nodeIndex);
}