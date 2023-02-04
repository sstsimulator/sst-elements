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

class NicCmdEntry {
  public:
    NicCmdEntry( RdmaNic& nic, int thread, NicCmd* tmp ) : 
        m_nic(nic), m_thread(thread), m_cmd( new NicCmd ), m_respAddr(tmp->respAddr) 
    {
        bzero( &m_resp.data, sizeof( m_resp.data ) );
        m_resp.retval = 0; 
		*m_cmd = *tmp;
    } 
    virtual ~NicCmdEntry() {
        if ( m_cmd ) {
            delete m_cmd;
        }
    }
    virtual bool process( ) {
        m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"%s respAddr=%#" PRIx32 " retval=%x\n", name().c_str(), m_respAddr, m_resp.retval );
        m_nic.sendRespToHost( m_respAddr, m_resp, m_thread );
        return true;
    }
    virtual bool isRecv() { return false; }
    virtual std::string name() = 0;
    int getThread() { return m_thread; }
  protected:

    Addr_t m_respAddr;
    NicResp m_resp;
    NicCmd* m_cmd;
    int m_thread;
    RdmaNic& m_nic;
 };

class RdmaCreateCQ_Cmd : public NicCmdEntry {
  public:
    RdmaCreateCQ_Cmd( RdmaNic& nic, int thread, NicCmd* cmd ) : NicCmdEntry(nic,thread,cmd)
	{
    	int cqId = m_nic.m_nextCqId++;
    	m_nic.m_compQueueMap[ cqId ] = new CompletionQueue( m_cmd );    
    	m_resp.retval = cqId;
    	m_resp.data.createCQ.tailIndexAddr = m_nic.calcCompQueueTailAddress( m_thread, cqId ); 
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cqId=%d headPtr=%" PRIx32 " datPtr=%" PRIx32 " num=%d\n",
                    cqId, m_cmd->data.createCQ.headPtr, m_cmd->data.createCQ.dataPtr, m_cmd->data.createCQ.num);
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cqId=%d headPtr=%" PRIx32 " datPtr=%" PRIx32 " num=%d tailIndexAddr=%" PRIx32 "\n",
                    cqId, m_cmd->data.createCQ.headPtr, m_cmd->data.createCQ.dataPtr, m_cmd->data.createCQ.num, m_resp.data.createCQ.tailIndexAddr);
    
    	// passed the cmd to the CompletionQueue 
    	m_cmd = NULL;
	}
    virtual std::string name() { return "CreateCQ"; }
}; 

class RdmaDestroyCQ_Cmd : public NicCmdEntry {
  public:
    RdmaDestroyCQ_Cmd( RdmaNic& nic, int thread, NicCmd* cmd ) : NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cqId=%#x\n", m_cmd->data.destroyCQ.cqId );
    
        m_resp.retval = -1;

        auto iter = m_nic.m_compQueueMap.find( m_cmd->data.destroyCQ.cqId );
        if ( iter != m_nic.m_compQueueMap.end() ) {
    	    delete m_nic.m_compQueueMap[ m_cmd->data.destroyCQ.cqId ];
            m_resp.retval = 0;
        }
	}
 
    virtual std::string name() { return "DestroyRQ"; }
}; 


class RdmaCreateRQ_Cmd : public NicCmdEntry {
  public:
    RdmaCreateRQ_Cmd( RdmaNic& nic, int thread, NicCmd* cmd ) : NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"rqKey=%x cqId=%x\n",
            m_cmd->data.createRQ.rqKey, m_cmd->data.createRQ.cqId );

        m_resp.retval = -1;
        
        auto iter = m_nic.m_compQueueMap.find( m_cmd->data.createRQ.cqId );
        if ( iter != m_nic.m_compQueueMap.end() ) {
    	    m_resp.retval = m_nic.m_recvEngine->createRQ(m_cmd->data.createRQ.cqId, m_cmd->data.createRQ.rqKey );
        }
	}
 
    virtual std::string name() { return "CreateRQ"; }
}; 

class RdmaDestroyRQ_Cmd : public NicCmdEntry {
  public:
    RdmaDestroyRQ_Cmd( RdmaNic& nic, int thread, NicCmd* cmd ) : NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"rqId=%#x\n", m_cmd->data.destroyRQ.rqId );
    	m_resp.retval = m_nic.m_recvEngine->destroyRQ(m_cmd->data.destroyRQ.rqId );
	}
 
    virtual std::string name() { return "DestroyRQ"; }
}; 

class RdmaFiniCmd : public NicCmdEntry {
  public:
    RdmaFiniCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"\n");
	}
    virtual std::string name() { return "Fini"; }
}; 

class RdmaBarrierCmd : public NicCmdEntry {
  public:
    RdmaBarrierCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"\n");
	}
    virtual bool process() {
		if (  m_nic.m_barrier->process() ) {
			m_resp.retval = 0;
			return NicCmdEntry::process();	
		}
		return false;
    }
    virtual std::string name() { return "Barrier"; }
}; 

class RdmaSendCmd : public NicCmdEntry {
  public:
	RdmaSendCmd( RdmaNic& nic, int thread, NicCmd* x ) : NicCmdEntry(nic,thread,x)
	{
		m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"pe=%d node=%d addr=%" PRIx32 " len=%d ctx=%#x\n",
            m_cmd->data.send.pe, m_cmd->data.send.node, m_cmd->data.send.addr, m_cmd->data.send.len, m_cmd->data.send.context );

    	m_nic.m_sendEngine->add( 0, new MsgSendEntry( m_cmd, m_thread ) );
    	// passed the cmd to the SendEntry
        m_cmd = NULL;
    }
    virtual std::string name() { return "Send"; }
}; 

class RdmaRecvCmd : public NicCmdEntry {
  public:
	RdmaRecvCmd( RdmaNic& nic, int thread, NicCmd* x ) : NicCmdEntry(nic,thread,x)
	{
		m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"addr=%" PRIx32 " len=%d rqId=%x ctx=%#x\n",
            m_cmd->data.recv.addr, m_cmd->data.recv.len, m_cmd->data.recv.rqId, m_cmd->data.recv.context );

    	m_nic.m_recvEngine->postRecv( m_cmd->data.recv.rqId, new MsgRecvEntry( m_cmd, m_thread ) );
    	// passed the cmd to the RecvEntry
		m_cmd = NULL;
    }
    virtual std::string name() { return "Recv"; }
    void writeResp( int thread, uint64_t cnt, StandardMem::Request* ev ) {}
    bool isRecv() { return true; }
}; 

class RdmaMemRgnRegCmd : public NicCmdEntry {
  public:
    RdmaMemRgnRegCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"key=%x addr=%" PRIx32 " length=%d\n", 
					m_cmd->data.memRgnReg.key, m_cmd->data.memRgnReg.addr,m_cmd->data.memRgnReg.len);
		m_resp.retval = m_nic.m_recvEngine->addMemRgn( new MemRgnEntry( m_cmd, m_thread ) );
    	// passed the cmd to the CompletionQueue 
		m_cmd = NULL;
	}
    virtual std::string name() { return "MemRgn"; }
}; 

class RdmaMemRgnUnregCmd : public NicCmdEntry {
  public:
    RdmaMemRgnUnregCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
		m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"key=%x\n", m_cmd->data.memRgnUnreg.key);
		m_resp.retval = m_nic.m_recvEngine->removeMemRgn( m_cmd->data.memRgnUnreg.key );
	}
    virtual std::string name() { return "MemRgn"; }
};

class RdmaMemWriteCmd : public NicCmdEntry {
  public:
    RdmaMemWriteCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"\n");
    	m_nic.m_sendEngine->add( 0, new WriteSendEntry( m_cmd, m_thread ) );
    	// passed the cmd to the CompletionQueue 
    	m_cmd = NULL;
	}
    virtual std::string name() { return "MemoryWrite"; }
}; 

class RdmaMemReadCmd : public NicCmdEntry {
  public:
    RdmaMemReadCmd( RdmaNic& nic, int thread, NicCmd* cmd ): NicCmdEntry(nic,thread,cmd)
	{
    	int readRespKey = m_nic.m_recvEngine->addReadResp( 
				thread, m_cmd->data.read.destAddr, m_cmd->data.read.len, m_cmd->data.read.cqId, m_cmd->data.read.context  );
    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"respRespKey=%d destBuffer=%#x\n",readRespKey, m_cmd->data.read.destAddr);
    	m_nic.m_sendEngine->add( 0, new ReadSendEntry( m_cmd, m_thread, readRespKey ) );
    	// passed the cmd to the CompletionQueue 
    	m_cmd = NULL;
	}
    virtual std::string name() { return "MemoryRead"; }
}; 
