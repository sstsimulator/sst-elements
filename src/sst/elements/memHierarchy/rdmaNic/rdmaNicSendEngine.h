
class SendStream {
  public:
    SendStream( RdmaNic& nic, SendEntry* entry );
    ~SendStream( );
    bool process();
  private:
    void readResp( int thread, StandardMem::Request* resp );

	bool isReadyQfull() {
		return m_readyPktQ.size() == m_maxQueueSize;
	}
	void processReadyQ() {
		if ( ! m_nic.m_networkQ->full( m_sendEntry->getVC() ) && ! m_readyPktQ.empty() ) {
			m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"give packet to network\n");
        	m_nic.m_networkQ->add( m_sendEntry->getVC(), m_sendEntry->getDestNode(), m_readyPktQ.front() );
			m_readyPktQ.pop();
		}
	}

	int calcSeqLen() {
		int totalLen = sizeof(*m_sendEntry->getStreamHdr()) + m_sendEntry->getLength();
		return totalLen / m_nic.getNetPktMtuLen() + ( 0 != totalLen % m_nic.getNetPktMtuLen() );
	}

	RdmaNicNetworkEvent*  queuePkt( RdmaNicNetworkEvent* pkt ) {
        pkt->setSrcPid( m_sendEntry->getThread() );
        pkt->setDestPid( m_sendEntry->getDestPid() );
        pkt->setSrcNode( m_nic.m_nicId );
        pkt->setStreamId( m_streamId );
        pkt->setStreamSeqNum( m_streamSeqNum++ );

		m_readyPktQ.push(pkt);
		return  new RdmaNicNetworkEvent();
	}

	int readId;
	int curReadId;
	int readReqNum; 
    StreamId m_streamId;
    int m_streamSeqNum;
	int m_maxQueueSize;
    std::queue< RdmaNicNetworkEvent* > m_readyPktQ;
    RdmaNicNetworkEvent* m_pkt;
    std::map<int,StandardMem::ReadResp*> m_respMap;
    int m_numReadsPending;
    MemRequest::Callback* m_callback;
    size_t m_offset;
    RdmaNic& m_nic;
    SendEntry* m_sendEntry;
};

class SendEngine {
  public:
    SendEngine( RdmaNic& nic, int numVC ) : nic(nic) {
        vcQueues.resize( numVC );
		vcStreams.resize( numVC, NULL );
    }

    void process() {

    	for ( int i = 0; i < vcQueues.size(); i++ ) {
        	if ( vcStreams[i] ) {
            	if ( vcStreams[i]->process() ) { 
                	nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG, "stream vc=%d done\n",i);
					delete vcStreams[i];
					if ( ! vcQueues[i].empty() ) {
                		nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG, "load new stream vc=%d\n",i);
						vcStreams[i] = new SendStream( nic, vcQueues[i].front() );
						vcQueues[i].pop();
					} else {
						vcStreams[i] = NULL;
					}
            	}   
        	}
    	} 
	}

    void add( int vc, SendEntry* entry ) { 
		if ( vcStreams[vc] ) {
            nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG, "queue the send\n");
			vcQueues[vc].push( entry ); 
		} else {
            nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG, "start the stream\n");
			vcStreams[vc] = new SendStream( nic, entry );
		}
	}
  private:
    RdmaNic& nic;
    std::vector< std::queue<SendEntry*> > vcQueues;
	std::vector< SendStream* > vcStreams;
};
