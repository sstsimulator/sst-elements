// ========================================================================
// NetworkIO Storage SendEntry Classes
// ========================================================================
// Three types: READ, WRITE, and ACK

// Base class for all NetworkIO storage SendEntry types
class NetworkIOSendEntryBase : public SendEntryBase {
  public:
    NetworkIOSendEntryBase(int local_vNic, int streamNum) : 
        SendEntryBase(local_vNic, streamNum) {}
    ~NetworkIOSendEntryBase() {}

    MsgHdr::Op getOp() { return MsgHdr::NetworkIO; }  // NetworkIO storage message type
    void* hdr() { return nullptr; }
    size_t hdrSize() { return 0; }
    int dst_vNic() { return 0; }  // Storage nodes use vNic 0
    int vn() { return 0; }        // Virtual NetworkIO 0
};

// ========================================================================
// NetworkIO Storage READ Entry
// ========================================================================
class NetworkIOStorageReadEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOStorageReadEntry(int local_vNic, int dst_node, Hermes::Vaddr dest,
                           Hermes::Vaddr src, size_t len) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),  // Target storage node (calculated from offset)
        m_dest(dest),          // Local buffer to receive data
        m_src(src),            // Remote storage offset
        m_len(len),            // Transfer size
        m_respKey(0)           // Response key for ACK matching
    {}

    // Build READ request packet
    // Packet format: [NetworkIOMsgHdr::Read][src_offset][dest_addr][length][respKey]
    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        // READ request: NO MemOp - just sending request, no data to read from host
        
        // Append NetworkIOMsgHdr with READ operation type
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Read;
        event.bufAppend(&netHdr, sizeof(netHdr));
        
        // Append operation parameters
        event.bufAppend(&m_src, sizeof(m_src));          // Storage offset
        event.bufAppend(&m_dest, sizeof(m_dest));        // Local buffer
        event.bufAppend(&m_len, sizeof(m_len));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }  // Packet sent, awaiting ACK

    size_t totalBytes() override { return m_len; }
    int dest() override { return m_dst_node; }  // Target storage node
    
    // Set response key for blocking operations (ACK matching)
    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;                 // Target storage node ID
    Hermes::Vaddr m_dest;           // Local buffer address
    Hermes::Vaddr m_src;            // Remote storage offset
    size_t m_len;                   // Transfer length
    uint32_t m_respKey;             // Response key for ACK matching
};

// ========================================================================
// NetworkIO Storage WRITE Entry
// ========================================================================
class NetworkIOStorageWriteEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOStorageWriteEntry(int local_vNic, int dst_node, Hermes::Vaddr dest,
                            Hermes::Vaddr src, size_t len) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dst_node(dst_node),  // Target storage node
        m_dest(dest),          // Remote storage offset
        m_src(src),            // Local source buffer
        m_len(len),            // Transfer size
        m_respKey(0)           // Response key for ACK matching
    {}

    // Build WRITE request packet
    // Packet format: [NetworkIOMsgHdr::Write][dest_offset][src_addr][length][respKey]
    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        // WRITE request: NO MemOp - just sending request
        
        // Append NetworkIOMsgHdr with WRITE operation type
        Nic::NetworkIOMsgHdr netHdr;
        netHdr.op = Nic::NetworkIOMsgHdr::Write;
        event.bufAppend(&netHdr, sizeof(netHdr));
        
        // Append operation parameters
        event.bufAppend(&m_dest, sizeof(m_dest));        // Storage offset
        event.bufAppend(&m_src, sizeof(m_src));          // Local buffer
        event.bufAppend(&m_len, sizeof(m_len));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }  // Packet sent, awaiting ACK

    size_t totalBytes() override { return m_len; }
    int dest() override { return m_dst_node; }
    bool isWriteOp() override { return true; }  // This is a WRITE operation
    
    void setRespKey(uint32_t key) { m_respKey = key; }

  private:
    int m_dst_node;
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_len;
    uint32_t m_respKey;
};

// ========================================================================
// NetworkIO Storage ACK Entry
// ========================================================================
// Sends ACK packet back to requester after storage DMA completes
// Small packet containing only: [isAck=true][respKey]
// Requester matches respKey to invoke stored callback
class NetworkIOAckSendEntry : public NetworkIOSendEntryBase {
  public:
    NetworkIOAckSendEntry(int local_vNic, int dest_node, uint32_t respKey) :
        NetworkIOSendEntryBase(local_vNic, 0),
        m_dest_node(dest_node),  // Requester node to send ACK to
        m_respKey(respKey)       // Response key for callback matching
    {
        m_isAck = true;  // Flag to identify ACK packets
    }

    // Build ACK packet
    // Packet format: [isAck=true][respKey]
    void copyOut(Output& dbg, int numBytes, FireflyNetworkEvent& event, std::vector<MemOp>& vec) override
    {
        event.bufAppend(&m_isAck, sizeof(m_isAck));
        event.bufAppend(&m_respKey, sizeof(m_respKey));
    }

    bool isDone() override { return true; }     // Single packet
    size_t totalBytes() override { return 0; }  // No data payload
    int dest() override { return m_dest_node; }  // Back to requester

  private:
    int m_dest_node;   // Original requester node
    uint32_t m_respKey; // Response key to match with stored callback
    bool m_isAck;      // ACK flag
};
