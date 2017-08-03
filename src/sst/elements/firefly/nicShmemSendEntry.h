// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
    ShmemSendEntryBase( int local_vNic ) : SendEntryBase( local_vNic ) {}
    MsgHdr::Op getOp() { return MsgHdr::Shmem; }
    void* hdr() { return &m_hdr; }
    size_t hdrSize() { return sizeof(m_hdr); }
  protected:
    Nic::ShmemMsgHdr m_hdr;
};

class ShmemCmdSendEntry: public ShmemSendEntryBase {
  public:
    ShmemCmdSendEntry( int local_vNic, NicShmemCmdEvent* event) : 
        ShmemSendEntryBase( local_vNic ), m_event( event ) {}
    int dst_vNic() { return m_event->vnic; }
    int dest() { return m_event->node; }
  protected:
    NicShmemCmdEvent* m_event;
};

class ShmemGetSendEntry: public ShmemCmdSendEntry {
  public:
    ShmemGetSendEntry( int local_vNic, NicShmemCmdEvent* event ) : 
        ShmemCmdSendEntry( local_vNic, event )
    {
        assert( sizeof( m_hdr.key) == sizeof(m_event )); 
        m_hdr.op = ShmemMsgHdr::Get; 
        m_hdr.simVAddrSrc = m_event->src.getSimVAddr();
        m_hdr.simVAddrDest = m_event->dest.getSimVAddr();
        m_hdr.key = (size_t)m_event;
        m_hdr.length = m_event->len; 
    }

    size_t totalBytes() { return 0; } 
    bool isDone() { return true; }
    void copyOut( Output&, int vc, int numBytes, 
            FireflyNetworkEvent&, std::vector<DmaVec>& ) {};
};

class ShmemMove {
  public:
    ShmemMove( void* ptr, size_t length ) : 
        m_ptr((uint8_t*)ptr), m_length( length ), m_offset(0) {}
    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>& vec );
    bool isDone() { return m_offset == m_length; }
  private:
    uint8_t*  m_ptr;
    size_t m_length;
    size_t m_offset;
};

class ShmemPutSendEntry: public ShmemCmdSendEntry  {
  public:
    typedef std::function<void()> Callback;
    ShmemPutSendEntry( int local_vNic, NicShmemCmdEvent* event,
                                                Callback callback ) : 
        ShmemCmdSendEntry( local_vNic, event ),
        m_callback(callback)
    {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.simVAddrSrc = 0;
        m_hdr.simVAddrDest = m_event->dest.getSimVAddr();
        m_hdr.key = 0;
        m_hdr.length = m_event->len; 
    }

    ~ShmemPutSendEntry() {
        m_callback( );
        delete m_event;
        delete m_shmemMove;
    }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>& vec ) {
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    };

  protected:
    Callback m_callback;
    ShmemMove* m_shmemMove;
};

class ShmemPutPSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutPSendEntry( int local_vNic, NicShmemCmdEvent* event,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, event, callback )
    {
        m_shmemMove = new ShmemMove( event->src.getBacking(), event->len );
    }
};

class ShmemPutVSendEntry: public ShmemPutSendEntry  {
  public:
    ShmemPutVSendEntry( int local_vNic, NicShmemCmdEvent* event,
                                                Callback callback ) : 
        ShmemPutSendEntry( local_vNic, event, callback )
    {
        m_shmemMove = new ShmemMove( &event->value, event->len );
    }
};


class ShmemPut2SendEntry: public ShmemSendEntryBase  {
  public:
    ShmemPut2SendEntry( int local_vNic, int destNode, int dest_vNic,
            uint64_t simVAddrDest, void* ptr, size_t length, uint64_t key ) :
        ShmemSendEntryBase( local_vNic ),
        m_node( destNode ),
        m_vnic(dest_vNic)
    {
        m_hdr.op = ShmemMsgHdr::Put; 
        m_hdr.simVAddrSrc = 0;
        m_hdr.simVAddrDest = simVAddrDest;
        m_hdr.key = key;
        m_hdr.length = length; 
        m_shmemMove = new ShmemMove( ptr, length );
    }
    ~ShmemPut2SendEntry() {
        delete m_shmemMove;
    }
    int dst_vNic() { return m_vnic; }
    int dest() { return m_node; }

    size_t totalBytes() { return m_hdr.length; } 
    bool isDone() { return m_shmemMove->isDone(); }
    void copyOut( Output& dbg, int vc, int numBytes, 
            FireflyNetworkEvent& ev, std::vector<DmaVec>& vec ) {
        m_shmemMove->copyOut( dbg, vc, numBytes, ev, vec ); 
    };

  private:
    ShmemMove* m_shmemMove;
    int m_vnic;
    int m_node;
};
