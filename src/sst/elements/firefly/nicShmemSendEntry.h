// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


class ShmemSendEntryBase: public SendEntryBase {
  public:
    ShmemSendEntryBase( int local_vNic, int streamNum ) : SendEntryBase( local_vNic, streamNum ) { }
    ~ShmemSendEntryBase() { }
    
    MsgHdr::Op getOp() { return MsgHdr::Shmem; }
    void* hdr() { return &m_hdr; }
    size_t hdrSize() { return sizeof(m_hdr); }
  protected:
    Nic::ShmemMsgHdr m_hdr;
};

class ShmemCmdSendEntry: public ShmemSendEntryBase {
  public:
    ShmemCmdSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event) : 
        ShmemSendEntryBase( local_vNic, streamNum ), m_event( event ) { }
    int dst_vNic() { return m_event->getVnic(); }
    int dest() { return m_event->getNode(); }
  protected:
    NicShmemSendCmdEvent* m_event;
};

class ShmemAckSendEntry: public ShmemSendEntryBase {
  public:
    ShmemAckSendEntry( int local_vNic, int streamNum, int dest_node, int dest_vNic  ) :
        ShmemSendEntryBase( local_vNic, streamNum ), m_dest_node(dest_node), m_dest_vNic(dest_vNic)
    { 
        m_hdr.op = ShmemMsgHdr::Ack; 
        m_isAck = true;
    }
    int dst_vNic() { return m_dest_vNic; }
    int dest() { return m_dest_node; }
    size_t totalBytes() { return 0; } 
    bool isDone() { return true; }
    virtual void copyOut( Output&, int numBytes, 
            FireflyNetworkEvent&, std::vector<MemOp>& ) {};
  private:
	int m_dest_vNic;
	int m_dest_node;
};


class ShmemRespSendEntry: public ShmemCmdSendEntry {
  public:
    ShmemRespSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event ) : 
        ShmemCmdSendEntry( local_vNic, streamNum, event )
    {
        m_hdr.vaddr = m_event->getFarAddr();
        m_hdr.length = m_event->getLength(); 
    }
    bool shouldDelete() { return false; }

    void setRespKey( RespKey_t key ) {
        m_hdr.respKey = key;
    }

    size_t totalBytes() { return 0; } 
    bool isDone() { return true; }
    virtual void copyOut( Output&, int numBytes, 
            FireflyNetworkEvent&, std::vector<MemOp>& ) {};
    NicShmemSendCmdEvent* getCmd() { return m_event; }
};

class ShmemGetvSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    ShmemGetvSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, streamNum, event ), m_callback(callback)
    { 
        m_hdr.op = ShmemMsgHdr::Get; 
    }
    void callback( Hermes::Value& value ) { m_callback(value); }
  private:
    Callback  m_callback;
};


class ShmemFaddSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;

    ShmemFaddSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, streamNum, event ), m_callback(callback)
    { 
        m_shmemMove = new ShmemSendMoveValue( event->getValue() );
        m_hdr.op = ShmemMsgHdr::Fadd; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemFaddSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<MemOp>&  vec) { 
        m_shmemMove->copyOut( dbg, numBytes, ev, vec ); 
    }
  private:
    Callback  m_callback;
    ShmemSendMove* m_shmemMove;
};

class ShmemSwapSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;
    ShmemSwapSendEntry( int local_vNic, int streamNum, NicShmemSwapCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, streamNum, event ), m_callback(callback)
    {
        m_shmemMove = new ShmemSendMoveValue( event->getValue() );
        m_hdr.op = ShmemMsgHdr::Swap; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemSwapSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<MemOp>&  vec) { 
        m_shmemMove->copyOut( dbg, numBytes, ev, vec ); 
    }
  private:
    Callback        m_callback;
    ShmemSendMove*  m_shmemMove;
};

class ShmemCswapSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void(Hermes::Value&)> Callback;
    ShmemCswapSendEntry( int local_vNic, int streamNum, NicShmemCswapCmdEvent* event, Callback callback  ) :
        ShmemRespSendEntry( local_vNic, streamNum, event ), m_callback(callback)
    {
        m_shmemMove = new ShmemSendMove2Value( event->getValue(), event->getCond() );
        m_hdr.op = ShmemMsgHdr::Cswap; 
        m_hdr.dataType = m_event->getDataType();
    }
    ~ShmemCswapSendEntry() { delete m_shmemMove; }

    void callback( Hermes::Value& value ) { m_callback(value); }

    void copyOut( Output& dbg, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<MemOp>&  vec) { 
        m_shmemMove->copyOut( dbg, numBytes, ev, vec ); 
    }
  private:
    Callback        m_callback;
    ShmemSendMove*  m_shmemMove;
};

class ShmemGetbSendEntry: public ShmemRespSendEntry {
  public:
    typedef std::function<void()> Callback;

    ShmemGetbSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event, Callback callback ) : 
        ShmemRespSendEntry( local_vNic, streamNum, event ), m_callback(callback) 
    { 
        m_hdr.op = ShmemMsgHdr::Get; 
    }
    void callback() { m_callback(); }
  private:
    Callback  m_callback;
};

class ShmemPutSendEntry: public ShmemCmdSendEntry  {
  public:
    typedef std::function<void()> Callback;
    ShmemPutSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event,
                                                Callback callback ) : 
        ShmemCmdSendEntry( local_vNic, streamNum, event ),
        m_callback(callback)
    {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.op2 = (unsigned char) m_event->getOp();
        m_hdr.dataType = (unsigned char) m_event->getDataType();
        m_hdr.vaddr = m_event->getFarAddr();
        m_hdr.length = m_event->getLength(); 
        m_hdr.respKey = 0;
    }

    ~ShmemPutSendEntry() {
        m_callback( );
        delete m_event;
        delete m_shmemMove;
    }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<MemOp>& vec ) {
        m_shmemMove->copyOut( dbg, numBytes, ev, vec ); 
    };

  protected:
    Callback m_callback;
    ShmemSendMove* m_shmemMove;
};

class ShmemPutbSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutbSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event, void* backing,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, streamNum, event, callback )
    {
        m_shmemMove = new ShmemSendMoveMem( backing, event->getLength(), event->getMyAddr() );
    }
    ~ShmemPutbSendEntry() {
    }
};

class ShmemPutvSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutvSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, streamNum, event, callback )
    {
        m_shmemMove = new ShmemSendMoveValue( event->getValue() );
    }
};


class ShmemAddSendEntry: public ShmemPutvSendEntry {
  public:
    ShmemAddSendEntry( int local_vNic, int streamNum, NicShmemSendCmdEvent* event, Callback callback ) :

        ShmemPutvSendEntry( local_vNic, streamNum, event, callback )
    { 
        m_hdr.op = ShmemMsgHdr::Add; 
        m_hdr.dataType = event->getDataType();
    }
};

class ShmemPut2SendEntry: public ShmemSendEntryBase  {
  public:
    ShmemPut2SendEntry( int local_vNic, int streamNum, int destNode, int dest_vNic,
            void* ptr, size_t length, uint64_t key, Hermes::Vaddr addr ) :
        ShmemSendEntryBase( local_vNic, streamNum ),
        m_node( destNode ),
        m_vnic(dest_vNic),
        m_value(NULL)
    {
        init( length, key );
        m_shmemMove = new ShmemSendMoveMem( ptr, length, addr );
    }
    ShmemPut2SendEntry( int local_vNic, int streamNum, int destNode, int dest_vNic,
            Hermes::Value* value, uint64_t key ) :
        ShmemSendEntryBase( local_vNic, streamNum ),
        m_node( destNode ),
        m_vnic(dest_vNic),
        m_value(value) 
    {
        init( value->getLength(), key );
        m_shmemMove = new ShmemSendMoveValue( *value );
    }

    void init( uint64_t length, uint64_t key ) {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.respKey = key;
        m_hdr.length = length; 
    }

    ~ShmemPut2SendEntry() {
        delete m_shmemMove;
        if ( m_value) { delete m_value; }
    }
    int dst_vNic() { return m_vnic; }
    int dest() { return m_node; }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<MemOp>& vec ) {
        m_shmemMove->copyOut( dbg, numBytes, ev, vec ); };

  private:
    ShmemSendMove* m_shmemMove;
    int m_vnic;
    int m_node;
    Hermes::Value* m_value;
};
