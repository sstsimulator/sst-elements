    class SendMachine {
        enum State { Idle, Sending, WaitDelay, WaitTX, WaitDMA } m_state;
      public:
        SendMachine( Nic& nic, Output& output ) : m_state( Idle ),
            m_nic(nic), m_dbg(output), m_currentSend(NULL), m_txDelay(50) { }
        ~SendMachine();

        void init( int txDelay, int packetSizeInBytes, int packetSizeInBits ) {
            m_txDelay = txDelay;
            m_packetSizeInBytes = packetSizeInBytes;
            m_packetSizeInBits = packetSizeInBits;
        }

        void run( SendEntry* entry = NULL);

      private:
        SendEntry* processSend( SendEntry* );
        bool copyOut( Output& dbg, MerlinFireflyEvent& event,
                                            Nic::Entry& entry );

        Nic&        m_nic;
        Output&     m_dbg;

        std::deque<SendEntry*>  m_sendQ;
        SendEntry*              m_currentSend;
        int                     m_txDelay;
        unsigned int            m_packetSizeInBytes;
        int                     m_packetSizeInBits;
    };
