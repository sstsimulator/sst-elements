// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

// NetworkStream: Handles incoming network storage operations
class NetworkIOStream : public StreamBase {
  public:
    NetworkIOStream( Output&, Ctx*, int srcNode, int srcPid, int destPid, FireflyNetworkEvent* );
    ~NetworkIOStream() {
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"NetworkIOStream destroyed\n");
    }
    
    void setDone() {
        m_ctx->deleteStream( this );
    }

  private:
    void processAck(FireflyNetworkEvent* ev, unsigned char* bufPtr);
    void processStorageOp(FireflyNetworkEvent* ev, unsigned char* bufPtr);
    
    uint64_t m_offset;    // Storage offset
    Hermes::Vaddr m_src;  // Source buffer (from sender)
    size_t m_length;      // Operation length
};
