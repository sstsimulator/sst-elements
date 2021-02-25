// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


class RecvEntryBase : public EntryBase {
  public:

    RecvEntryBase( ) :
        EntryBase()
    { }
    virtual ~RecvEntryBase() {}

    virtual void notify( int src_vNic, int src_node, int tag, size_t length ) {assert(0);}
};

class DmaRecvEntry : public RecvEntryBase {

  public:
    typedef std::function<void(int, int, int, size_t, void*)> Callback;

    DmaRecvEntry( NicCmdEvent* cmd, Callback callback) :
        RecvEntryBase(), m_cmd( cmd ), m_callback( callback )
    { }
    ~DmaRecvEntry() {
        delete m_cmd;
    }

    void notify( int src_vNic, int src_node, int tag, size_t length ) {
        m_callback( src_vNic, src_node, tag, length, m_cmd->key );
    }

    std::vector<IoVec>& ioVec() { return m_cmd->iovec; }
    int node()  { return m_cmd->node; }
    int tag() { return m_cmd->tag; }

  private:
    NicCmdEvent* m_cmd;
    Callback m_callback;
};

class PutRecvEntry : public RecvEntryBase {
  public:
    PutRecvEntry( std::vector<IoVec>* ioVec ) :
        RecvEntryBase(), m_ioVec( *ioVec )
    { }

    std::vector<IoVec>& ioVec() { return m_ioVec; }

  private:
    std::vector<IoVec> m_ioVec;
};

class ShmemRecvEntry : public RecvEntryBase {
  public:

    ShmemRecvEntry( Shmem* shmem, int core, Hermes::MemAddr addr, size_t length ) :
        RecvEntryBase()
    {
        m_shmemMove = new ShmemRecvMoveMem(  addr.getBacking(), length, shmem, core, addr.getSimVAddr() );
    }

    ShmemRecvEntry( Shmem* shmem, int core, Hermes::MemAddr addr, size_t length,
                        Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType ) :
        RecvEntryBase()
    {
        m_shmemMove = new ShmemRecvMoveMemOp( addr.getBacking(), length, shmem, core, addr.getSimVAddr(), op, dataType );
    }
    ~ShmemRecvEntry() {
        delete m_shmemMove;
    }

    void notify( int src_vNic, int src_node, int tag, size_t length ) {}

    size_t totalBytes( ) { return m_shmemMove->totalBytes(); }
    std::vector<IoVec>& ioVec() { assert(0); }

    bool copyIn( Output& dbg, FireflyNetworkEvent& ev, std::vector<MemOp>& vec ) {
        return m_shmemMove->copyIn( dbg, ev, vec );
    }

  private:
    ShmemRecvMove*      m_shmemMove;
};

class ShmemGetRespRecvEntry : public RecvEntryBase {
  public:
    ShmemGetRespRecvEntry( ShmemRespSendEntry* entry ) :
        RecvEntryBase(), m_entry(entry)
    { }
    ~ShmemGetRespRecvEntry() {
        delete m_shmemMove;
        delete m_entry->getCmd();
        delete m_entry;
    }

    size_t totalBytes( ) { return m_shmemMove->totalBytes(); }
    std::vector<IoVec>& ioVec() { assert(0); }

    bool copyIn( Output& dbg, FireflyNetworkEvent& ev, std::vector<MemOp>& vec ) {
        return m_shmemMove->copyIn( dbg, ev, vec );
    }

  protected:
    ShmemRecvMove*          m_shmemMove;
    ShmemRespSendEntry*     m_entry;
};

class ShmemGetvRespRecvEntry : public ShmemGetRespRecvEntry {
  public:

    ShmemGetvRespRecvEntry( Shmem* shmem, size_t length, ShmemGetvSendEntry* entry ) :
        ShmemGetRespRecvEntry( entry ),
        m_value( static_cast<NicShmemGetvCmdEvent*>(entry->getCmd())->getDataType() )
    {
        assert( length == m_value.getLength() );
        m_shmemMove = new ShmemRecvMoveValue( m_value );
    }

    size_t totalBytes( ) { return m_shmemMove->totalBytes(); }
    virtual void notify( int src_vNic, int src_node, int tag, size_t length ) {
        static_cast<ShmemGetvSendEntry*>(m_entry)->callback( m_value );
    }

  private:
    Hermes::Value       m_value;
};

class ShmemGetbRespRecvEntry : public ShmemGetRespRecvEntry {
  public:

    ShmemGetbRespRecvEntry( Shmem* shmem, int core, size_t length, ShmemGetbSendEntry* entry, void* backing ) :
        ShmemGetRespRecvEntry( entry )
    {

        NicShmemGetCmdEvent* cmd =    static_cast<NicShmemGetCmdEvent*>(entry->getCmd());

        assert( length ==  cmd->getLength() );
        m_shmemMove = new ShmemRecvMoveMem( backing, length, shmem, core, cmd->getMyAddr() );
    }

    size_t totalBytes( ) { return m_shmemMove->totalBytes(); }
    virtual void notify( int src_vNic, int src_node, int tag, size_t length ) {
        static_cast<ShmemGetbSendEntry*>(m_entry)->callback( );
    }
};
