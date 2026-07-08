// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
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
        case NicNetworkIOCmdEvent::NetworkIOOpen:
            handleNetworkIOOpen(static_cast<NicNetworkIOOpenCmdEvent*>(event), id);
            break;
        case NicNetworkIOCmdEvent::NetworkIOClose:
            handleNetworkIOClose(static_cast<NicNetworkIOCloseCmdEvent*>(event), id);
            break;
        default:
            m_dbg.fatal(CALL_INFO, -1, "core=%d Unknown NetworkIOCmdEvent type %d\n", id, event->type);
    }
}

void Nic::NetworkIO::handleNetworkIOOpen(NicNetworkIOOpenCmdEvent* event, int id)
{
    int targetNid    = event->getTargetNid();
    uint32_t fileId  = event->getFileId();
    uint32_t cb_id   = event->getCbId();

    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO,
        "OPEN core=%d targetNid=%d fileId=%u\n", id, targetNid, fileId);

    NetworkIOOpenSendEntry* entry = new NetworkIOOpenSendEntry(
        id, targetNid,
        fileId,
        event->getStripeUnit(),
        event->getCapacity(),
        event->getStripeNids(),
        event->getPoolName());

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void(int)>(
        [this, id, cb_id](int status) {
            m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, cb_id, status);
        }));
    entry->setRespKey(respKey);
    m_nic.qSendEntry(entry);

    delete event;
}

void Nic::NetworkIO::handleNetworkIOClose(NicNetworkIOCloseCmdEvent* event, int id)
{
    int targetNid    = event->getTargetNid();
    uint32_t fileId  = event->getFileId();
    uint32_t cb_id   = event->getCbId();

    m_dbg.verbosePrefix(prefix().c_str(), CALL_INFO, 1, NIC_DBG_NETWORKIO,
        "CLOSE core=%d targetNid=%d fileId=%u\n", id, targetNid, fileId);

    NetworkIOCloseSendEntry* entry = new NetworkIOCloseSendEntry(
        id, targetNid, fileId);

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void(int)>(
        [this, id, cb_id](int status) {
            m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, cb_id, status);
        }));
    entry->setRespKey(respKey);
    m_nic.qSendEntry(entry);

    delete event;
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
                       "READ core=%d targetNid=%d dest=%#" PRIx64 " len=%zu \n",
                       id, targetNid, destAddr, length);

    uint32_t cb_id = event->getCbId();

    m_nic.m_statNetworkIoReadTargetNid->addData(static_cast<uint64_t>(targetNid));
    SimTime_t startNs = m_nic.getCurrentSimTimeNano();

    NetworkIOStorageReadEntry* entry = new NetworkIOStorageReadEntry(
        id,                     // local vNic
        targetNid,              // destination storage node
        destAddr,               // local buffer for received data
        0,                      // local offset (not used in new API)
        length,                 // transfer size
        event->getFileId()      // PR 9 fileId (0 = legacy)
    );

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void(int)>([this, id, cb_id, startNs](int status) {
        SimTime_t endNs = m_nic.getCurrentSimTimeNano();
        m_nic.m_statNetworkIoReadLatency->addData(endNs - startNs);
        m_dbg.verbosePrefix(m_prefix.c_str(), CALL_INFO_LAMBDA, "handleNetworkIORead",
                           1, NIC_DBG_NETWORKIO, "core=%d cb_id=%u ACK received status=%d\n", id, cb_id, status);
        m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, cb_id, status);
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
                       "WRITE core=%d targetNid=%d src=%#" PRIx64 " len=%zu \n",
                       id, targetNid, srcAddr, length);

    uint32_t cb_id = event->getCbId();

    m_nic.m_statNetworkIoWriteTargetNid->addData(static_cast<uint64_t>(targetNid));
    SimTime_t startNs = m_nic.getCurrentSimTimeNano();

    NetworkIOStorageWriteEntry* entry = new NetworkIOStorageWriteEntry(
        id,                     // local vNic
        targetNid,              // destination storage node
        0,                      // remote offset (not used in new API)
        srcAddr,                // local source buffer
        length,                 // transfer size
        event->getFileId()      // PR 9 fileId (0 = legacy)
    );

    Nic::RespKey_t respKey = m_nic.genRespKey(new std::function<void(int)>([this, id, cb_id, startNs](int status) {
        SimTime_t endNs = m_nic.getCurrentSimTimeNano();
        m_nic.m_statNetworkIoWriteLatency->addData(endNs - startNs);
        m_dbg.verbosePrefix(m_prefix.c_str(), CALL_INFO_LAMBDA, "handleNetworkIOWrite",
                           1, NIC_DBG_NETWORKIO, "core=%d cb_id=%u ACK received status=%d\n", id, cb_id, status);
        m_nic.getVirtNic(id)->notifyNetworkIO(m_nic.m_nic2host_lat_ns, cb_id, status);
    }));
    entry->setRespKey(respKey);

    m_nic.qSendEntry(entry);

    delete event;
}