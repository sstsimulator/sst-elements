// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.   

class SendMachine {
        enum State { Idle, Sending, WaitDelay, WaitTX, WaitRead } m_state;
      public:
        SendMachine( Nic& nic, Output& output ) : m_state( Idle ),
//            m_nic(nic), m_dbg(output), m_currentSend(NULL), m_txDelay(50), m_packetId(0) { }
            m_nic(nic), m_dbg(output), m_currentSend(NULL), m_txDelay(50) { }
        ~SendMachine();

        void init( int txDelay, int packetSizeInBytes, int packetSizeInBits ) {
            m_txDelay = txDelay;
            m_packetSizeInBytes = packetSizeInBytes;
            m_packetSizeInBits = packetSizeInBits;
        }

        void run( SendEntry* entry = NULL);

      private:
        State processSend( SendEntry* );
        bool copyOut( Output& dbg, FireflyNetworkEvent& event,
                                            Nic::Entry& entry );

        Nic&        m_nic;
        Output&     m_dbg;

        std::deque<SendEntry*>  m_sendQ;
        SendEntry*              m_currentSend;
        int                     m_txDelay;
        unsigned int            m_packetSizeInBytes;
        int                     m_packetSizeInBits;
//        int                     m_packetId;
};
