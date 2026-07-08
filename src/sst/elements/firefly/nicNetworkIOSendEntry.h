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

// ========================================================================
// NetworkIO Storage SendEntry Classes
// ========================================================================
// Wire-format extension for PR 9: every Read/Write request now carries a
// 4-byte fileId immediately after the 1-byte NetworkIOMsgHdr. fileId == 0
// is the legacy sentinel (no per-file routing; storage side falls through
// to mapper). Non-zero fileId routes to a FileSlot lookup. Open and Close
// entries are new (variable-length stripeNids capped at MAX_STRIPE_NIDS).

#ifndef FIREFLY_NETWORKIO_MAX_STRIPE_NIDS
#define FIREFLY_NETWORKIO_MAX_STRIPE_NIDS 64
#endif

class NetworkIOSendEntryBase : public SendEntryBase {
  public:
    NetworkIOSendEntryBase(int local_vNic, int streamNum) :
        SendEntryBase(local_vNic, streamNum) {}
    ~NetworkIOSendEntryBase() {}

    MsgHdr::Op getOp() { return MsgHdr::NetworkIO; }
    void* hdr() { return nullptr; }
    size_t hdrSize() { return 0; }
    int dst_vNic() { return 0; }
    int vn() { return 0; }
};

// ========================================================================
// NetworkIO Storage READ Entry
// ========================================================================
// PR 9 wire layout:
//   [NetworkIOMsgHdr:1][fileId:4][offset:8][dest:8][length:size_t][respKey:4]
class NetworkIOStorageReadEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOStorageReadEntry(int local_vNic, int dst_node, Hermes::Vaddr dest,
                           Hermes::Vaddr src, size_t len,
                           uint32_t fileId = 0) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),
        m_dest(dest),
        m_src(src),
        m_len(len),
        m_respKey(0),
        m_fileId(fileId)
    {}

    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Read;
        event.bufAppend(&netHdr, sizeof(netHdr));

        event.bufAppend(&m_fileId, sizeof(m_fileId));        // PR 9 fileId
        event.bufAppend(&m_src, sizeof(m_src));              // Storage offset
        event.bufAppend(&m_dest, sizeof(m_dest));            // Local buffer
        event.bufAppend(&m_len, sizeof(m_len));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }

    size_t totalBytes() override {
        return sizeof(Nic::NetworkIOMsgHdr) + sizeof(m_fileId)
             + sizeof(m_src) + sizeof(m_dest)
             + sizeof(m_len) + sizeof(m_respKey);
    }
    int dest() override { return m_dst_node; }

    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_len;
    uint32_t m_respKey;
    uint32_t m_fileId;
};

// ========================================================================
// NetworkIO Storage WRITE Entry
// ========================================================================
// PR 9 wire layout:
//   [NetworkIOMsgHdr:1][fileId:4][offset:8][src:8][length:size_t][respKey:4]
class NetworkIOStorageWriteEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOStorageWriteEntry(int local_vNic, int dst_node, Hermes::Vaddr dest,
                            Hermes::Vaddr src, size_t len,
                            uint32_t fileId = 0) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),
        m_dest(dest),
        m_src(src),
        m_len(len),
        m_respKey(0),
        m_fileId(fileId)
    {}

    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Write;
        event.bufAppend(&netHdr, sizeof(netHdr));

        event.bufAppend(&m_fileId, sizeof(m_fileId));        // PR 9 fileId
        event.bufAppend(&m_dest, sizeof(m_dest));            // Storage offset
        event.bufAppend(&m_src, sizeof(m_src));              // Local buffer
        event.bufAppend(&m_len, sizeof(m_len));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }

    size_t totalBytes() override {
        return sizeof(Nic::NetworkIOMsgHdr) + sizeof(m_fileId)
             + sizeof(m_dest) + sizeof(m_src)
             + sizeof(m_len) + sizeof(m_respKey);
    }
    int dest() override { return m_dst_node; }
    bool isWriteOp() override { return true; }

    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_len;
    uint32_t m_respKey;
    uint32_t m_fileId;
};

// ========================================================================
// NetworkIO Storage ACK Entry
// ========================================================================
// PR 9 wire layout for ACK/AckDeny:
//   [NetworkIOMsgHdr:1][respKey:4][bytes_completed:8][error_code:1]
// AckDeny still rides the wire with the same byte budget so the deny path
// keeps its constant-cost guarantees. Ack carries bytes_completed (for
// short-read/short-write surfacing) and a 1-byte error_code (Status enum
// cast). On the receive side the requester reconstructs the IOStatus.
class NetworkIOAckSendEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOAckSendEntry(int local_vNic, int dest_node, uint32_t respKey,
                          Nic::NetworkIOMsgHdr::Op op = Nic::NetworkIOMsgHdr::Ack,
                          uint64_t bytesCompleted = 0,
                          uint8_t errorCode = 0) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dest_node(dest_node),
        m_respKey(respKey),
        m_op(op),
        m_bytesCompleted(bytesCompleted),
        m_errorCode(errorCode)
    {
        m_isAck = true;
    }

    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = m_op;
        event.bufAppend(&netHdr, sizeof(netHdr));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
        event.bufAppend(&m_bytesCompleted, sizeof(m_bytesCompleted));
        event.bufAppend(&m_errorCode, sizeof(m_errorCode));
    }

    bool isDone() override { return true; }
    size_t totalBytes() override {
        return sizeof(Nic::NetworkIOMsgHdr) + sizeof(m_respKey)
             + sizeof(m_bytesCompleted) + sizeof(m_errorCode);
    }
    int dest() override { return m_dest_node; }

  private:
    int m_dest_node;
    uint32_t m_respKey;
    Nic::NetworkIOMsgHdr::Op m_op;
    uint64_t m_bytesCompleted;
    uint8_t  m_errorCode;
};

// ========================================================================
// NetworkIO OPEN Entry (PR 9)
// ========================================================================
// Wire layout:
//   [NetworkIOMsgHdr::Open:1][fileId:4][respKey:4][stripeUnit:8][capacity:8]
//   [stripeNidCount:4][stripeNids[stripeNidCount]:4*N]
//   [poolNameLen:4][poolName:poolNameLen]
// Variable-length stripeNids capped at MAX_STRIPE_NIDS (R-7).
class NetworkIOOpenSendEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOOpenSendEntry(int local_vNic, int dst_node,
                           uint32_t fileId,
                           uint64_t stripeUnit,
                           uint64_t capacity,
                           const std::vector<int>& stripeNids,
                           const std::string& poolName) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),
        m_fileId(fileId),
        m_respKey(0),
        m_stripeUnit(stripeUnit),
        m_capacity(capacity),
        m_stripeNids(stripeNids),
        m_poolName(poolName)
    {
        assert(m_stripeNids.size() <= FIREFLY_NETWORKIO_MAX_STRIPE_NIDS
               && "NetworkIOOpenSendEntry: stripeNids exceeds MAX_STRIPE_NIDS");
    }

    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Open;
        event.bufAppend(&netHdr, sizeof(netHdr));

        event.bufAppend(&m_fileId, sizeof(m_fileId));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
        event.bufAppend(&m_stripeUnit, sizeof(m_stripeUnit));
        event.bufAppend(&m_capacity, sizeof(m_capacity));

        uint32_t nidCount = static_cast<uint32_t>(m_stripeNids.size());
        event.bufAppend(&nidCount, sizeof(nidCount));
        for (uint32_t i = 0; i < nidCount; ++i) {
            int32_t n = static_cast<int32_t>(m_stripeNids[i]);
            event.bufAppend(&n, sizeof(n));
        }
        uint32_t nameLen = static_cast<uint32_t>(m_poolName.size());
        event.bufAppend(&nameLen, sizeof(nameLen));
        if (nameLen > 0) {
            event.bufAppend(const_cast<char*>(m_poolName.data()), nameLen);
        }
    }

    bool isDone() override { return true; }
    size_t totalBytes() override {
        return sizeof(Nic::NetworkIOMsgHdr) + sizeof(m_fileId)
             + sizeof(m_respKey) + sizeof(m_stripeUnit)
             + sizeof(m_capacity)
             + sizeof(uint32_t) + m_stripeNids.size() * sizeof(int32_t)
             + sizeof(uint32_t) + m_poolName.size();
    }
    int dest() override { return m_dst_node; }

    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;
    uint32_t m_fileId;
    uint32_t m_respKey;
    uint64_t m_stripeUnit;
    uint64_t m_capacity;
    std::vector<int> m_stripeNids;
    std::string m_poolName;
};

// ========================================================================
// NetworkIO CLOSE Entry (PR 9)
// ========================================================================
// Wire layout:
//   [NetworkIOMsgHdr::Close:1][fileId:4][respKey:4]
class NetworkIOCloseSendEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOCloseSendEntry(int local_vNic, int dst_node, uint32_t fileId) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),
        m_fileId(fileId),
        m_respKey(0)
    {}

    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Close;
        event.bufAppend(&netHdr, sizeof(netHdr));

        event.bufAppend(&m_fileId, sizeof(m_fileId));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }
    size_t totalBytes() override {
        return sizeof(Nic::NetworkIOMsgHdr) + sizeof(m_fileId)
             + sizeof(m_respKey);
    }
    int dest() override { return m_dst_node; }

    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;
    uint32_t m_fileId;
    uint32_t m_respKey;
};
