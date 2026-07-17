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

// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

// ========================================================================
// Network Storage Stream Constructor
// ========================================================================
// PR 9 dispatch table:
//   Read/Write          -> processStorageOp (fileId-aware)
//   Open/Close          -> processOpenClose (storage-side FileSlot mgmt)
//   Ack/AckDeny         -> processAck       (extended w/ bytes_completed + err)
//   OpenAck/CloseAck    -> processAck       (handle-side completion routing)
Nic::RecvMachine::NetworkIOStream::NetworkIOStream( Output& output, Ctx* ctx,
            int srcNode, int srcPid, int destPid, FireflyNetworkEvent* ev ) :
    StreamBase( output, ctx, srcNode, srcPid, destPid ),
    m_offset(0),
    m_length(0)
{
    ev->bufPop(sizeof(MsgHdr));

    unsigned char* bufPtr = (unsigned char*)ev->bufPtr();

    Nic::NetworkIOMsgHdr netHdr;
    assert(ev->bufSize() >= sizeof(netHdr));
    memcpy(&netHdr, bufPtr, sizeof(netHdr));

    if (netHdr.op == Nic::NetworkIOMsgHdr::Ack ||
        netHdr.op == Nic::NetworkIOMsgHdr::AckDeny ||
        netHdr.op == Nic::NetworkIOMsgHdr::OpenAck ||
        netHdr.op == Nic::NetworkIOMsgHdr::CloseAck) {
        ev->bufPop(sizeof(netHdr));
        processAck(ev, (unsigned char*)ev->bufPtr(), netHdr.op);
        m_ctx->nic().schedCallback([ev]() { delete ev; }, 0);
        setDone();
        return;
    }

    if (netHdr.op == Nic::NetworkIOMsgHdr::Open ||
        netHdr.op == Nic::NetworkIOMsgHdr::Close) {
        ev->bufPop(sizeof(netHdr));
        processOpenClose(ev, (unsigned char*)ev->bufPtr(), netHdr.op);
        m_ctx->nic().schedCallback([ev]() { delete ev; }, 0);
        setDone();
        return;
    }

    processStorageOp(ev, bufPtr);
}

// ========================================================================
// Process ACK Packet (Ack / AckDeny / OpenAck / CloseAck)
// ========================================================================
// Wire layout (post-netHdr):
//   [respKey:4][bytes_completed:8][error_code:1]
// The legacy `std::function<void(int)>` callback receives the status int
// (cast of Status enum). PR 9 adds bytes_completed to the lower 32 bits of
// a packed 64-bit "encoded result" if needed by future paths — for v1 we
// route through a small ack adapter installed by HadesNetworkIO so the
// per-stripe completion accounting on the client can extract the
// per-ACK bytes_completed.
void Nic::RecvMachine::NetworkIOStream::processAck(FireflyNetworkEvent* ev,
        unsigned char* bufPtr, uint8_t op)
{
    uint32_t respKey = 0;
    memcpy(&respKey, bufPtr, sizeof(respKey));
    ev->bufPop(sizeof(respKey));

    uint64_t bytesCompleted = 0;
    memcpy(&bytesCompleted, bufPtr + sizeof(respKey), sizeof(bytesCompleted));
    ev->bufPop(sizeof(bytesCompleted));

    uint8_t errorCode = 0;
    memcpy(&errorCode,
           bufPtr + sizeof(respKey) + sizeof(bytesCompleted),
           sizeof(errorCode));
    ev->bufPop(sizeof(errorCode));

    int status = static_cast<int>(errorCode);
    if (status == 0 && op == Nic::NetworkIOMsgHdr::AckDeny) {
        status = static_cast<int>(Hermes::NetworkIO::Status::PermissionDenied);
    }

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                "ACK op=%u respKey=%u status=%d bytes=%" PRIu64 "\n",
                op, respKey, status, bytesCompleted);

    auto* respCallback = static_cast<std::function<void(int)>*>(
        m_ctx->nic().getRespKeyValue(respKey));
    if (respCallback) {
        (*respCallback)(status);
        delete respCallback;
    }
}

// ========================================================================
// Process Open/Close (Storage Node Side, PR 9)
// ========================================================================
// Open wire layout (post-netHdr):
//   [fileId:4][respKey:4][stripeUnit:8][capacity:8]
//   [stripeNidCount:4][stripeNids[stripeNidCount]:4*N]
//   [poolNameLen:4][poolName:poolNameLen]
// Close wire layout (post-netHdr):
//   [fileId:4][respKey:4]
// Each op queues an OpenAck/CloseAck back to the requester so the
// client-side HadesNetworkIO can fire its StatusCallback once it has
// collected ACKs from every stripe-set member.
void Nic::RecvMachine::NetworkIOStream::processOpenClose(
        FireflyNetworkEvent* ev, unsigned char* bufPtr, uint8_t op)
{
    uint32_t fileId = 0;
    memcpy(&fileId, bufPtr, sizeof(fileId));
    bufPtr += sizeof(fileId);
    ev->bufPop(sizeof(fileId));

    uint32_t respKey = 0;
    memcpy(&respKey, bufPtr, sizeof(respKey));
    bufPtr += sizeof(respKey);
    ev->bufPop(sizeof(respKey));

    Nic::NetworkIOMsgHdr::Op ackOp;
    uint8_t errCode = 0;
    uint64_t bytesCompleted = 0;

    HadesStorageController* sc = m_ctx->nic().getStorageControllerPtr();

    if (op == Nic::NetworkIOMsgHdr::Open) {
        uint64_t stripeUnit = 0, capacity = 0;
        memcpy(&stripeUnit, bufPtr, sizeof(stripeUnit));
        bufPtr += sizeof(stripeUnit);
        ev->bufPop(sizeof(stripeUnit));
        memcpy(&capacity, bufPtr, sizeof(capacity));
        bufPtr += sizeof(capacity);
        ev->bufPop(sizeof(capacity));

        uint32_t nidCount = 0;
        memcpy(&nidCount, bufPtr, sizeof(nidCount));
        bufPtr += sizeof(nidCount);
        ev->bufPop(sizeof(nidCount));

        std::vector<int> stripeNids;
        stripeNids.reserve(nidCount);
        for (uint32_t i = 0; i < nidCount; ++i) {
            int32_t n = 0;
            memcpy(&n, bufPtr, sizeof(n));
            bufPtr += sizeof(n);
            ev->bufPop(sizeof(n));
            stripeNids.push_back(n);
        }

        uint32_t nameLen = 0;
        memcpy(&nameLen, bufPtr, sizeof(nameLen));
        bufPtr += sizeof(nameLen);
        ev->bufPop(sizeof(nameLen));
        std::string poolName(reinterpret_cast<const char*>(bufPtr), nameLen);
        bufPtr += nameLen;
        ev->bufPop(nameLen);

        if (sc) {
            sc->addFileSlot(fileId, poolName, stripeNids, stripeUnit, capacity);
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "Open fileId=%u stripeNids=%zu stripeUnit=%" PRIu64
                        " capacity=%" PRIu64 " pool=%s\n",
                        fileId, stripeNids.size(), stripeUnit, capacity,
                        poolName.c_str());
        } else {
            errCode = static_cast<uint8_t>(Hermes::NetworkIO::Status::BadFileHandle);
        }
        ackOp = Nic::NetworkIOMsgHdr::OpenAck;
    } else {
        if (sc) {
            if (!sc->removeFileSlot(fileId) && sc->findFileSlot(fileId) == nullptr) {
                m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                            "Close fileId=%u (unknown; idempotent ok)\n", fileId);
            } else {
                m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                            "Close fileId=%u\n", fileId);
            }
        }
        ackOp = Nic::NetworkIOMsgHdr::CloseAck;
    }

    NetworkIOAckSendEntry* ack = new NetworkIOAckSendEntry(
        m_myPid, m_srcNode, respKey, ackOp, bytesCompleted, errCode);
    m_ctx->nic().qSendEntry(ack);
}

// ========================================================================
// Process Storage Operation (Storage Node Side)
// ========================================================================
// PR 9 wire layout (post-netHdr):
//   [fileId:4][offset:8][src/dst:8][length:size_t][respKey:4]
// fileId == 0 -> legacy mapper-only path; non-zero -> FileSlot lookup for
// (a) capacity short-read enforcement and (b) future per-file routing.
void Nic::RecvMachine::NetworkIOStream::processStorageOp(FireflyNetworkEvent* ev, unsigned char* bufPtr)
{
    Nic::NetworkIOMsgHdr netHdr;
    memcpy(&netHdr, bufPtr, sizeof(netHdr));
    bufPtr += sizeof(netHdr);
    ev->bufPop(sizeof(netHdr));

    // PR 7 receive-side permit enforcement. Wire format past netHdr is
    // [fileId:4][offset:8][src/dst:8][length:size_t][respKey:4]. Peek
    // respKey at fixed offset to bounce an AckDeny without touching body.
    if ( m_ctx->nic().networkIOPoolEnabled() &&
         !m_ctx->nic().permittedSender( m_srcNode ) ) {
        uint32_t respKey = 0;
        unsigned char* keyP = bufPtr + sizeof(uint32_t)
                                     + sizeof(uint64_t)
                                     + sizeof(Hermes::Vaddr)
                                     + sizeof(size_t);
        memcpy( &respKey, keyP, sizeof(respKey) );
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                    "permit-deny srcNode=%d respKey=%u\n",
                    m_srcNode, respKey);
        uint8_t denyErr = static_cast<uint8_t>(
            Hermes::NetworkIO::Status::PermissionDenied);
        NetworkIOAckSendEntry* denyEntry = new NetworkIOAckSendEntry(
            m_myPid, m_srcNode, respKey, Nic::NetworkIOMsgHdr::AckDeny,
            0, denyErr );
        m_ctx->nic().qSendEntry( denyEntry );
        m_ctx->nic().schedCallback([ev]() { delete ev; }, 0);
        setDone();
        return;
    }

    // PR 9: read fileId ahead of offset.
    uint32_t fileId = 0;
    memcpy(&fileId, bufPtr, sizeof(fileId));
    bufPtr += sizeof(fileId);
    ev->bufPop(sizeof(fileId));

    memcpy(&m_offset, bufPtr, sizeof(m_offset));
    bufPtr += sizeof(m_offset);
    memcpy(&m_src, bufPtr, sizeof(m_src));
    bufPtr += sizeof(m_src);
    memcpy(&m_length, bufPtr, sizeof(m_length));
    bufPtr += sizeof(m_length);
    ev->bufPop(sizeof(m_offset) + sizeof(m_src) + sizeof(m_length));

    uint32_t respKey;
    memcpy(&respKey, bufPtr, sizeof(respKey));
    ev->bufPop(sizeof(respKey));

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                "op=%u fileId=%u offset=%" PRIu64 " length=%zu respKey=%u\n",
                netHdr.op, fileId, m_offset, m_length, respKey);

    // PR 9: per-file slot capacity + BadFileHandle accounting.
    // fileId == 0 = legacy sentinel: skip slot lookup entirely.
    uint64_t effLength = m_length;
    uint8_t errCode = 0;
    if (fileId != 0) {
        HadesStorageController* sc = m_ctx->nic().getStorageControllerPtr();
        FileSlot* slot = sc ? sc->findFileSlot(fileId) : nullptr;
        if (slot == nullptr) {
            uint8_t bh = static_cast<uint8_t>(
                Hermes::NetworkIO::Status::BadFileHandle);
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "BadFileHandle fileId=%u\n", fileId);
            NetworkIOAckSendEntry* badEntry = new NetworkIOAckSendEntry(
                m_myPid, m_srcNode, respKey, Nic::NetworkIOMsgHdr::Ack,
                0, bh);
            m_ctx->nic().qSendEntry(badEntry);
            m_ctx->nic().schedCallback([ev]() { delete ev; }, 0);
            setDone();
            return;
        }
        if (slot->capacity > 0 && m_offset + m_length > slot->capacity) {
            if (m_offset >= slot->capacity) {
                effLength = 0;
            } else {
                effLength = slot->capacity - m_offset;
            }
            errCode = static_cast<uint8_t>(
                netHdr.op == Nic::NetworkIOMsgHdr::Read
                ? Hermes::NetworkIO::Status::ShortRead
                : Hermes::NetworkIO::Status::NoSpace);
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "Short fileId=%u eff=%" PRIu64 " (cap=%" PRIu64 ")\n",
                        fileId, effLength, slot->capacity);
        }
        if (netHdr.op == Nic::NetworkIOMsgHdr::Read) {
            slot->bytesOut += effLength;
        } else {
            slot->bytesIn += effLength;
        }
    }

    if (effLength == 0) {
        NetworkIOAckSendEntry* zeroEntry = new NetworkIOAckSendEntry(
            m_myPid, m_srcNode, respKey, Nic::NetworkIOMsgHdr::Ack,
            0, errCode);
        m_ctx->nic().qSendEntry(zeroEntry);
        m_ctx->nic().schedCallback([ev]() { delete ev; }, 0);
        setDone();
        return;
    }

    m_length = effLength;

    std::vector<MemOp>* vec = new std::vector<MemOp>;
    if (netHdr.op == Nic::NetworkIOMsgHdr::Read) {
        vec->push_back(MemOp(m_offset, m_length, MemOp::Op::BusDmaFromHost));
    } else {
        vec->push_back(MemOp(m_offset, m_length, MemOp::Op::BusDmaToHost));
    }

    int unit = m_ctx->nic().allocNicRecvUnit(m_myPid);
    uint64_t ackBytes = effLength;

    if (netHdr.op == Nic::NetworkIOMsgHdr::Read) {
        auto callback = [=]() {
            m_dbg.debug(CALL_INFO_LAMBDA,"NetworkIOStream",1,NIC_DBG_RECV_STREAM,
                       "DMA READ complete, ACK respKey=%u node=%d bytes=%" PRIu64 " err=%u\n",
                       respKey, m_srcNode, ackBytes, errCode);
            NetworkIOAckSendEntry* ackEntry = new NetworkIOAckSendEntry(
                m_myPid, m_srcNode, respKey, Nic::NetworkIOMsgHdr::Ack,
                ackBytes, errCode);
            m_ctx->nic().qSendEntry(ackEntry);
            delete ev;
            setDone();
        };
        if(m_ctx->nic().getSimpleSSDPtr()!=NULL){
            SimpleSSD* m_ssd = m_ctx->nic().getSimpleSSDPtr();
            assert(m_ssd);
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "SSD READ node=%d offset=%" PRIu64 " length=%zu\n",
                        m_ctx->nic().getNodeId(), m_offset, m_length);
            m_ssd->read(m_offset, m_length, callback);
        } else {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "DMA READ node=%d offset=%" PRIu64 " length=%zu\n",
                        m_ctx->nic().getNodeId(), m_offset, m_length);
            m_ctx->nic().dmaRead(unit, m_myPid, vec, callback);
        }
    } else {
        auto writeCallback = [=]() {
            m_dbg.debug(CALL_INFO_LAMBDA,"NetworkIOStream",1,NIC_DBG_RECV_STREAM,
                        "DMA WRITE complete, ACK respKey=%u node=%d bytes=%" PRIu64 " err=%u\n",
                        respKey, m_srcNode, ackBytes, errCode);
            NetworkIOAckSendEntry* ackEntry = new NetworkIOAckSendEntry(
                m_myPid, m_srcNode, respKey, Nic::NetworkIOMsgHdr::Ack,
                ackBytes, errCode);
            m_ctx->nic().qSendEntry(ackEntry);
            delete ev;
            setDone();
        };

        if(m_ctx->nic().getSimpleSSDPtr()!=NULL){
            SimpleSSD* m_ssd = m_ctx->nic().getSimpleSSDPtr();
            assert(m_ssd);
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "SSD WRITE node=%d offset=%" PRIu64 " length=%zu\n",
                        m_ctx->nic().getNodeId(), m_offset, m_length);
            m_ssd->write(m_offset, m_length, writeCallback);
        } else {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,
                        "DMA WRITE node=%d offset=%" PRIu64 " length=%zu\n",
                        m_ctx->nic().getNodeId(), m_offset, m_length);
            m_ctx->nic().dmaWrite(unit, m_myPid, vec, writeCallback);
        }
    }
}
