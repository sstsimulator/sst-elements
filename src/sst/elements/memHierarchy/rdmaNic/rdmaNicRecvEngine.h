
class RecvEntry {
  public:
    RecvEntry( int thread, NicCmd* cmd = NULL ) : m_cmd(cmd), m_thread(thread), m_cqId(-1) {}
	~RecvEntry() { if ( m_cmd) delete m_cmd; }
	virtual size_t getPayloadLength() = 0;
	virtual Addr_t getAddr() = 0;
	virtual Addr_t getContext() = 0; 
	int getThread() { return m_thread; }
	
	// only used by MsgRecvEntry
	int getCqId() { return m_cqId; }
	void setCqId( int id ) { m_cqId = id; }
  protected:
	NicCmd* m_cmd;
	int m_thread;
	int m_cqId;
};

class MsgRecvEntry : public RecvEntry {
  public:
	MsgRecvEntry( NicCmd* cmd, int thread ) : RecvEntry( thread, cmd ) {} 
	size_t getPayloadLength() { return m_cmd->data.recv.len; }		
	Addr_t getAddr() { return m_cmd->data.recv.addr; }
	Addr_t getContext() { return m_cmd->data.recv.context; }
};

class MemRgnEntry : public RecvEntry  {
  public:
	MemRgnEntry( NicCmd* cmd, int thread ) :RecvEntry( thread, cmd ) { }
	int getKey() { return m_cmd->data.memRgnReg.key; }
	size_t getPayloadLength() { return m_cmd->data.memRgnReg.len; }
	Addr_t getAddr() { return m_cmd->data.memRgnReg.addr; }
	Addr_t getContext() { return 0; }
};

class ReadRespRecvEntry : public RecvEntry  {
  public:
	ReadRespRecvEntry( int thread, Addr_t destAddr, uint32_t len, CompQueueId cqId, Context context ) :
		RecvEntry(thread), m_destAddr(destAddr), m_length(len), m_context(context) { m_cqId = cqId; }
	size_t getPayloadLength() { return m_length; }
	Addr_t getAddr() {  return m_destAddr; } 
	Addr_t getContext() { return m_context; }; 
  private:
	uint32_t m_length;
	Addr_t m_destAddr;
	Context m_context;
};

class RecvQueue {
  public:
    RecvQueue( CompQueueId cqId ) : cqId(cqId) {}
    void push( MsgRecvEntry* entry ) { entry->setCqId(cqId); recvQ.push(entry); }
    std::queue<MsgRecvEntry*>& getQ() { return recvQ; }
  private:
    std::queue<MsgRecvEntry*> recvQ;
    CompQueueId cqId;
};

class RecvStream {
  public:
	// we don't need destAddr and lenght, we can get them from entry
	RecvStream( RdmaNic& nic, Addr_t destAddr, size_t length, RecvEntry* entry = NULL ) : 
			nic(nic), destAddr(destAddr), length(length), offset(0), recvEntry(entry), bytesWritten(0), callback(NULL) {
		if ( entry ) {
			callback = new MemRequest::Callback;
			*callback = std::bind( &RdmaNic::RecvStream::writeResp, this, recvEntry->getThread(), std::placeholders::_1 );
		}
	}
	~RecvStream( ); 
	void addPkt(RdmaNicNetworkEvent* pkt) { pktQ.push(pkt); } 
	bool process();

  private:
	void writeResp( int thread, StandardMem::Request* resp );

	size_t calcLen( Addr_t addr, size_t len, int maxLen ) {
		int tmp = maxLen - ( addr & ( maxLen - 1 ) );	
		nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"addr=%#x len=%zu maxLen=%d tmp=%d\n",addr,len,maxLen,tmp);
		return tmp > len ? len : tmp;
	}

    size_t bytesWritten;
    MemRequest::Callback* callback;

	RecvEntry* recvEntry;
	Addr_t destAddr;
	size_t length;	
   	std::queue<StandardMem::ReadResp*> respQ;
	size_t offset;
    std::queue<RdmaNicNetworkEvent*> pktQ;
	RdmaNic& nic;
};

class RecvEngine {
  public:
    RecvEngine( RdmaNic& nic, int numVC, int maxSize ) : nic(nic), maxSize(maxSize), m_nextRqId(0), m_nextReadRespKey(0) {
        queues.resize(numVC);   
    }
    void process();
	int removeMemRgn( int key ) {
		if ( m_memRegionMap.find( key ) == m_memRegionMap.end() ) {
			assert(0);
		}
		m_memRegionMap.erase(key);
		return 0;
	}
	int addMemRgn( MemRgnEntry* entry ) {
		// need to check this slot is empty
		if ( m_memRegionMap.find( entry->getKey() ) == m_memRegionMap.end() ) {
			m_memRegionMap[ entry->getKey() ] = entry;
			return 0;
		}else{
			return -1;
		}
	}
    void postRecv( int rqId, MsgRecvEntry* entry ) { 
        m_recvQueueMap[rqId]->push( entry );
    }   
    int createRQ( int cqId, int rqKey ) { 
        int rqId = m_nextRqId++;
        m_recvQueueMap[ rqId ] = new RecvQueue( cqId );
        m_recvQueueKeyMap[ rqKey ] = rqId;
        return rqId;
    }
	int addReadResp( int thread, Addr_t destAddr, uint32_t len, CompQueueId cqId, Context context ) {
		int key = m_nextReadRespKey++;
		m_readRespMap[key] = new ReadRespRecvEntry( thread, destAddr, len, cqId, context ); 
	}
  private:
    void processStreamHdr( RdmaNicNetworkEvent* );
	void processMsgHdr( RdmaNicNetworkEvent* ); 
	void processWriteHdr( RdmaNicNetworkEvent* ); 
	void processReadReqHdr( RdmaNicNetworkEvent* ); 
	void processReadRespHdr( RdmaNicNetworkEvent* ); 
	void processPayloadPkt( RdmaNicNetworkEvent* ); 

    void processQueuedPkts( std::queue< RdmaNicNetworkEvent* >& );
    void add( int vc, RdmaNicNetworkEvent* ev ) { queues[vc].push( ev ); }
	
    bool busy( int vc ) { return queues[vc].size() == maxSize; }
    RdmaNic& nic;
    std::vector< std::queue< RdmaNicNetworkEvent* > > queues;
    int maxSize;
    std::map< int, RecvQueue*> m_recvQueueMap;
    std::map< int, int > m_recvQueueKeyMap;
	std::map< int, MemRgnEntry* > m_memRegionMap; 

	std::map< int, ReadRespRecvEntry* > m_readRespMap;
	typedef uint64_t NodeStreamId;
	NodeStreamId calcNodeStreamId( int srcNode, StreamId id ) { return ((uint64_t)srcNode << 32) | id; }
	std::map<NodeStreamId,RecvStream*> m_recvStreamMap;

    int m_nextRqId;
	int m_nextReadRespKey;
};

