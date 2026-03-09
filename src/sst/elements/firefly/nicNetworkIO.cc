#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

// ========================================================================
// NetworkIO Constructor - Initialize storage configuration from parameters
// ========================================================================
Nic::NetworkIO::NetworkIO(Nic& nic, Params& params, Output& output) : 
    m_nic(nic), m_dbg(output)
{
    m_prefix = "@t:" + std::to_string(nic.getNodeId()) + ":Nic::NetworkIO::@p():@l ";
    
       
    // Initialize per-core pending operation tracking (same size as vNIC count)
    m_pendingOps.resize(nic.getNum_vNics());
    
    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO, 
        "storage config:cores=%d\n", nic.getNum_vNics());
}

// ========================================================================
// Main Event Handler - Dispatch to READ or WRITE
// ========================================================================
void Nic::NetworkIO::handleEvent(NicNetworkIOCmdEvent* event, int id)
{
    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO, 
        "core=%d `%s`\n", id, event->getTypeStr().c_str());

    // Dispatch based on operation type
    switch (event->type) {
        case NicNetworkIOCmdEvent::NetworkIORead:
            handleNetworkIORead(static_cast<NicNetworkIOReadCmdEvent*>(event), id);
            break;
        case NicNetworkIOCmdEvent::NetworkIOWrite:
            handleNetworkIOWrite(static_cast<NicNetworkIOWriteCmdEvent*>(event), id);
            break;
        default:
            m_dbg.fatal(CALL_INFO, -1, "core=%d Unknown NetworkIOCmdEvent type %d\n", id, event->type);
    }
}

// ========================================================================
// NetworkIO READ Handler - Get data from remote storage node
// ========================================================================
void Nic::NetworkIO::handleNetworkIORead(NicNetworkIOReadCmdEvent* event, int id)
{
    int targetNid = event->getTargetNid();
    Hermes::Vaddr destAddr = event->getDest();
    size_t length = event->getLen();
    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO,
                       "READ core=%d targetNid=%d dest=%#lx len=%zu \n",
                       id, targetNid, destAddr, length);

    auto callback = event->getCallback();

    NetworkIOStorageReadEntry* entry = new NetworkIOStorageReadEntry(
        id,          // local vNic
        targetNid,   // destination storage node
        destAddr,    // local buffer for received data
        0,           // local offset (not used in new API)
        length       // transfer size
    );

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void()>([=]() {
        m_dbg.verbosePrefix(m_prefix.c_str(), CALL_INFO_LAMBDA, "handleNetworkIORead",
                           1, NIC_DBG_NETWORKIO, "core=%d ACK received\n", id);
        m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, callback, 0);
    }));
    entry->setRespKey(respKey);

    m_nic.qSendEntry(entry);

    delete event;
}

// ========================================================================
// NetworkIO WRITE Handler - Put data to remote storage node
// ========================================================================
void Nic::NetworkIO::handleNetworkIOWrite(NicNetworkIOWriteCmdEvent* event, int id)
{
    int targetNid = event->getTargetNid();
    Hermes::Vaddr srcAddr = event->getSrc();
    size_t length = event->getLen();
    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO,
                       "WRITE core=%d targetNid=%d src=%#lx len=%zu \n",
                       id, targetNid, srcAddr, length);

    auto callback = event->getCallback();

    NetworkIOStorageWriteEntry* entry = new NetworkIOStorageWriteEntry(
        id,          // local vNic
        targetNid,   // destination storage node
        0,           // remote offset (not used in new API)
        srcAddr,     // local source buffer
        length       // transfer size
    );

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void()>([=]() {
        m_dbg.verbosePrefix(m_prefix.c_str(), CALL_INFO_LAMBDA, "handleNetworkIOWrite",
                           1, NIC_DBG_NETWORKIO, "core=%d ACK received\n", id);
        m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, callback, 0);
    }));
    entry->setRespKey(respKey);

    m_nic.qSendEntry(entry);

    delete event;
}