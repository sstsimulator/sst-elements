// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class SendEntry {
  public:
    SendEntry( int thread, NicCmd* cmd = NULL ) : cmd(cmd), thread(thread), vc(0) {}
	~SendEntry() {
		if ( cmd ) {
			delete cmd;
		}
	}
	StreamHdr* getStreamHdr() { return &m_streamHdr; }	
	virtual int getAddr() = 0;
	virtual int getLength() = 0;
	virtual int getCqId() = 0;
	virtual int getContext() = 0;
	virtual int getDestNode() = 0;
	virtual int getDestPid() = 0;
	int getThread() { return thread; }
	int getVC() { return vc; }
  protected:
	StreamHdr m_streamHdr;
    NicCmd* cmd;
    int thread;
	int vc;
};

class MsgSendEntry : public SendEntry {
  public:
    MsgSendEntry( NicCmd* cmd, int thread ) : SendEntry(thread,cmd) {
    	m_streamHdr.type = StreamHdr::Msg;
    	m_streamHdr.data.msgKey = cmd->data.send.rqKey;
    	m_streamHdr.payloadLength = cmd->data.send.len;
	}
	int getCqId() { return cmd->data.send.cqId; }
	int getAddr() { return cmd->data.send.addr; }
	int getLength() { return cmd->data.send.len; }
	int getContext() { return cmd->data.send.context; }
	int getDestNode() { return cmd->data.send.node; }
	int getDestPid() { return cmd->data.send.pe; }
  private:	
}; 

class WriteSendEntry : public SendEntry {
  public:
    WriteSendEntry( NicCmd* cmd, int thread ): SendEntry(thread,cmd) {
    	m_streamHdr.type = StreamHdr::Write;
    	m_streamHdr.data.rdma.memRgnKey = cmd->data.write.key;
    	m_streamHdr.data.rdma.offset = cmd->data.write.offset;
    	m_streamHdr.payloadLength = cmd->data.write.len;
	}
	int getLength() { return cmd->data.write.len; }
	int getAddr() { return cmd->data.write.srcAddr; }
	int getCqId() { return cmd->data.write.cqId; }
	int getContext() { return cmd->data.write.context; }
	int getDestNode() { return cmd->data.write.node; }
	int getDestPid() { return cmd->data.write.pe; }
}; 
class ReadSendEntry : public SendEntry {
  public:
    ReadSendEntry( NicCmd* cmd, int thread, int readRespKey ): SendEntry(thread,cmd) {
    	m_streamHdr.type = StreamHdr::ReadReq;
    	m_streamHdr.payloadLength = 0;
    	m_streamHdr.data.rdma.memRgnKey = cmd->data.read.key;
    	m_streamHdr.data.rdma.offset = cmd->data.read.offset;
		m_streamHdr.data.rdma.readRespKey = readRespKey;
		m_streamHdr.data.rdma.readLength = cmd->data.read.len;
	}
	// we are only sending a read header so lengh and address are 0
	// the read does send a completeion until the read completes so these are
	// passed to the ReadRespRecvEntry
	int getLength() { return 0; }
	int getAddr() { return 0; }
	int getCqId() { return -1; }
	int getContext() { return 0; }
	int getDestNode() { return cmd->data.read.node; }
	int getDestPid() { return cmd->data.read.pe; }
  private:
}; 

class ReadRespSendEntry : public SendEntry {
  public:
    ReadRespSendEntry( int thread, int destNode, int destPid, Addr_t srcAddr, int length, int readRespKey ): 
		SendEntry(thread), m_destNode(destNode), m_destPid(destPid), m_srcAddr(srcAddr), m_length(length) 
	{
    	m_streamHdr.type = StreamHdr::ReadResp;
    	m_streamHdr.payloadLength = m_length;
		m_streamHdr.data.rdma.readRespKey = readRespKey;
	}

	int getLength() { return m_length; }
	int getAddr() { return m_srcAddr; }
	int getCqId() { return -1; }
	int getContext() { return 0; }
	int getDestNode() { return m_destNode; }
	int getDestPid() { return m_destPid; }
  private:
	int m_srcAddr;
	int m_length;
	int m_destNode;
	int m_destPid;
}; 




