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

class SendEntryBase {
  public:
    SendEntryBase( int local_vNic, int streamNum ) :
        m_local_vNic( local_vNic ), m_streamNum(streamNum), m_isCtrl(false), m_isAck(false)
    { }
    virtual ~SendEntryBase() { }

    virtual int local_vNic()   { return m_local_vNic; }
    virtual int streamNum()  { return m_streamNum; }

    virtual MsgHdr::Op getOp() = 0;
    virtual size_t totalBytes() = 0;
    virtual bool isDone() = 0;

    virtual int dst_vNic() = 0;
    virtual int dest() = 0;
    virtual void* hdr() = 0;
    virtual size_t hdrSize() = 0;
    virtual void copyOut( Output& dbg, int numBytes,
            FireflyNetworkEvent& event, std::vector<MemOp>& vec ) = 0; 
    virtual bool shouldDelete() { return true; }
    bool isCtrl() { return m_isCtrl; }
    bool isAck() { return m_isAck; }
    int txDelay() { return m_txDelay; }
    void setTxDelay(int delay) { m_txDelay = delay; } 
    uint64_t m_start;

  protected:
    bool m_isCtrl;
    bool m_isAck;

  private:
    int m_txDelay;
    int m_local_vNic;
    int m_streamNum;
};

class CmdSendEntry: public SendEntryBase, public EntryBase {
  public:

    CmdSendEntry( int local_vNic, int streamNum, NicCmdEvent* cmd, std::function<void(void*)> callback ) :
        SendEntryBase( local_vNic, streamNum ),
        m_cmd(cmd),
        m_callback(callback)
    {
        m_hdr.len = totalBytes();
        m_hdr.tag = m_cmd->tag;
    }

    ~CmdSendEntry() {
        m_callback (m_cmd->key );
        delete m_cmd;
    }

    std::vector<IoVec>& ioVec() { return m_cmd->iovec; }
    size_t totalBytes() { return EntryBase::totalBytes(); }
    bool isDone()       { return EntryBase::isDone(); }
    void copyOut( Output& dbg, int numBytes, 
                FireflyNetworkEvent& event, std::vector<MemOp>& vec ) {
        EntryBase::copyOut(dbg,numBytes, event, vec );
    }

    MsgHdr::Op getOp()  { return MsgHdr::Msg; }
    int dst_vNic( )     { return m_cmd->dst_vNic; }
    int dest()          { return m_cmd->node; }
    void* hdr()         { return &m_hdr; }
    size_t hdrSize()    { return sizeof(m_hdr); }

  private:
    MatchMsgHdr         m_hdr;
    NicCmdEvent* m_cmd;
    std::function<void(void*)> m_callback;
};

class MsgSendEntry: public SendEntryBase {
  public:

    MsgSendEntry( int local_vNic, int streamNum, int dst_node,int dst_vNic ) :
        SendEntryBase( local_vNic, streamNum ),
        m_dst_node( dst_node ),
        m_dst_vNic( dst_vNic )
    { }

    virtual ~MsgSendEntry() {}

    virtual int dest()     { return m_dst_node; }
    virtual int dst_vNic() { return m_dst_vNic; }

  private:
    int                 m_dst_node;
    int                 m_dst_vNic;
};

class GetOrgnEntry : public MsgSendEntry {
  public:
    GetOrgnEntry( int local_vNic, int streamNum, int dst_node, int dst_vNic, int rgnNum, int respKey ) :
            MsgSendEntry( local_vNic, streamNum, dst_node, dst_vNic )
    {
        m_hdr.respKey = respKey;
        m_hdr.rgnNum = rgnNum;
        m_hdr.offset = -1;
        m_hdr.op = RdmaMsgHdr::Get;
        m_isCtrl = true;
    }

    ~GetOrgnEntry() { }

    bool isDone()      { return true; }
    void copyOut( Output& dbg, int numBytes,
                FireflyNetworkEvent& event, std::vector<MemOp>& vec ) 
	{
		vec.push_back( MemOp( 0, numBytes, MemOp::Op::LocalLoad ) );
	}; 

    size_t totalBytes(){ return sizeof( m_hdr ); }
    MsgHdr::Op getOp() { return MsgHdr::Rdma; }
    void* hdr()        { return &m_hdr; }
    size_t hdrSize()   { return sizeof(m_hdr); }

  private:
    RdmaMsgHdr          m_hdr;
    std::vector<IoVec> m_ioVec;
};

class PutOrgnEntry : public MsgSendEntry, public EntryBase {
  public:
    PutOrgnEntry( int local_vNic, int streamNum, int dst_node,int dst_vNic,
            int respKey, MemRgnEntry* memRgn ) :
        MsgSendEntry( local_vNic, streamNum, dst_node, dst_vNic ),
        m_memRgn( memRgn )
    {
        m_hdr.respKey = respKey;
        m_hdr.op = RdmaMsgHdr::GetResp;
    }

    ~PutOrgnEntry()             { delete m_memRgn; }

    std::vector<IoVec>& ioVec() { return m_memRgn->iovec(); }

    void copyOut( Output& dbg, int numBytes,
                FireflyNetworkEvent& event, std::vector<MemOp>& vec ) {
        EntryBase::copyOut(dbg,numBytes, event, vec );
    }

    size_t totalBytes() { return EntryBase::totalBytes(); }
    bool isDone()       { return EntryBase::isDone(); }
    MsgHdr::Op getOp()  { return MsgHdr::Rdma; }
    void* hdr()         { return &m_hdr; }
    size_t hdrSize()    { return sizeof(m_hdr); }

  private:
    MemRgnEntry*        m_memRgn;
    RdmaMsgHdr          m_hdr;
};
