class NetworkIO {
    public:
        NetworkIO(Nic& nic, Params& params, Output& output);
        ~NetworkIO() {}

        // Main event handler - dispatches to READ, WRITE, or QUIET handlers
        void handleEvent(NicNetworkIOCmdEvent* event, int id);

    private:
        // Network storage operation handlers
        void handleNetworkIORead(NicNetworkIOReadCmdEvent* event, int id);
        void handleNetworkIOWrite(NicNetworkIOWriteCmdEvent* event, int id);

        struct PendingOps {
                PendingOps() : readCount(0), writeCount(0) {}
                int readCount;                      // Number of pending non-blocking reads
                int writeCount;                     // Number of pending non-blocking writes
        };

        void incPendingReads(int id) {
                ++m_pendingOps[id].readCount;
        }

        void decPendingReads(int id) {
                PendingOps& ops = m_pendingOps[id];
                assert(ops.readCount > 0);
                --ops.readCount;
        }
    
    void incPendingWrites(int id) {
        ++m_pendingOps[id].writeCount;
    }
    
    void decPendingWrites(int id) {
        PendingOps& ops = m_pendingOps[id];
        assert(ops.writeCount > 0);
        --ops.writeCount;
    }
    
    // Core references
    Nic& m_nic;
    Output& m_dbg;
    std::string m_prefix;
    
    // Per-core tracking of pending non-blocking operations
    std::vector<PendingOps> m_pendingOps;
    
    std::string prefix() { return m_prefix; }
};
