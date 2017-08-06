// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
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
    ~DmaRecvEntry() { }

    void notify( int src_vNic, int src_node, int tag, size_t length ) {
        m_callback( src_vNic, src_node, tag, length, m_cmd->key );
    }

    std::vector<IoVec>& ioVec() { return m_cmd->iovec; }
    int node()  { return m_cmd->node; }

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

    ShmemRecvEntry( Hermes::MemAddr addr, size_t length, uint64_t key ) :
        RecvEntryBase(),
        m_addr( addr ),
        m_length( length ),
        m_key( key )
    { }

    ShmemRecvEntry( size_t length, uint64_t key ) :
        RecvEntryBase(),
        m_addr( &m_value ),
        m_length( length ),
        m_key( key ),
        m_value( 0 )
    { }
    virtual ~ShmemRecvEntry() {}

    std::vector<IoVec>& ioVec() { assert(0); }

    virtual bool copyIn( Output&, FireflyNetworkEvent&, std::vector<DmaVec>& );

    virtual void notify( int src_vNic, int src_node, int tag, size_t length ) {
        if ( m_key ) {
            NicShmemCmdEvent* cmd = (NicShmemCmdEvent*) m_key; 
            cmd->callback( m_value );
            delete cmd;
        }
    }

  private:
    Hermes::MemAddr m_addr;
    size_t          m_length;
    uint64_t        m_key;
    uint64_t        m_value;
};
