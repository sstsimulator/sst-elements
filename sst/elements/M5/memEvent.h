
#ifndef _memEvent_h
#define _memEvent_h

class MemEvent : public SST::Event {

  public:
    enum Type {
        Timing,
        Functional
    };

    class Request {
        friend class MemEvent;
      public:
        Request( ::Request &req ) :
            m_size( req.hasSize() ? req.getSize() : -1 ),
            m_paddr( req.hasPaddr() ? req.getPaddr() : -1 ),
            m_vaddr( req.hasPC() ? req.getVaddr() : -1 ),
            m_pc( req.hasPC() ? req.getPC() : -1 ),
            m_cid( req.hasContextId() ? req.contextId() : -1 ),
            m_tid( req.hasContextId() ? req.threadId() : -1 ),
            m_extraData( req.extraDataValid() ? req.getExtraData() : -1 ),
            m_flags( req.getFlags() ),
            m_time( req.time() )
        {
            //printf("m_flags %#x\n",m_flags);
        }

        ::Request* M5_Request() {
            ::Request* req = new ::Request( m_asid,
                                        m_vaddr,
                                        m_size,
                                        m_flags,
                                        m_pc,
                                        m_cid,
                                        m_tid );
            req->setPaddr( m_paddr );
            req->time( m_time );
            return req;
        }
      private:
        Request() {
        }

        int         m_size;
        Addr        m_paddr;
        Addr        m_vaddr;
        Addr        m_pc;
        int         m_cid;
        int         m_tid;
        int         m_asid;
        uint64_t    m_extraData;
        uint32_t    m_flags;
        uint64_t    m_time;

        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_NVP(m_size);
            ar & BOOST_SERIALIZATION_NVP(m_paddr);
            ar & BOOST_SERIALIZATION_NVP(m_vaddr);
            ar & BOOST_SERIALIZATION_NVP(m_pc);
            ar & BOOST_SERIALIZATION_NVP(m_cid);
            ar & BOOST_SERIALIZATION_NVP(m_tid);
            ar & BOOST_SERIALIZATION_NVP(m_asid);
            ar & BOOST_SERIALIZATION_NVP(m_extraData);
            ar & BOOST_SERIALIZATION_NVP(m_flags);
            ar & BOOST_SERIALIZATION_NVP(m_time);
        }
    };
    MemEvent( PacketPtr pkt, Type type = Timing ) :
        m_type( type ),
        //m_flags( pkt->flags ),
        m_cmd( pkt->cmd.toInt() ),
        m_req( *pkt->req ),
        m_addr( pkt->getAddr() ),
        m_size( pkt->getSize() ),
        m_src( -1 ),
        m_dst( pkt->getDest() ),
#if 0
        m_time( pkt->time ),
        m_finishTime( pkt->finishTime ),
        m_firstWordTime( pkt->firstWordTime ),
#endif
        m_senderState( (uint64_t) pkt->senderState )
    {
        setPriority( 50 );
        //printf("MemEvent::MemEvent()\n");
        //printPacket( "MemEvent::MemEvent()", pkt);
        if ( pkt->hasData() ) {
            m_data.resize( pkt->getSize() );
            pkt->writeData( &m_data[0] );
        }
        if ( pkt->isRequest( ) ) {
            m_src = pkt->getSrc();
        }

        if ( type == Timing)  {
            delete pkt;
        }
    }

    Type        type() { return m_type; }

    PacketPtr M5_Packet() {
        //printf("MemEvent::Packet()\n");

        ::Request* req = m_req.M5_Request();

        PacketPtr pkt = new ::Packet( req, MemCmd(m_cmd), m_dst, m_size );

        pkt->setSrc( m_src );
#if 0
        pkt->time = m_time;
        pkt->finishTime = m_finishTime;
        pkt->firstWordTime = m_firstWordTime;
#endif
        pkt->time = 0;
        pkt->finishTime = 0;
        pkt->firstWordTime = 0;
        pkt->senderState = (Packet::SenderState*) m_senderState;
        pkt->dataDynamicArray<uint8_t>( new uint8_t[ m_size ] );

        if ( pkt->hasData() ) {
            pkt->setData( &m_data[0] );
        }

        //printPacket( "MemEvent::MemEvent()", pkt);
        return pkt;
    }

  private:
    Type                    m_type;
    //int32_t                 m_flags;
    int                     m_cmd;
    Request                 m_req;
    std::vector<uint8_t>    m_data;
    Addr                    m_addr;
    unsigned                m_size;
    int                     m_src;
    int                     m_dst;
#if 0
    Tick                    m_time;
    Tick                    m_finishTime;
    Tick                    m_firstWordTime;
#endif
    uint64_t                m_senderState;

  private:

    MemEvent() {
    }

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(m_type);
        //ar & BOOST_SERIALIZATION_NVP(m_flags);
        ar & BOOST_SERIALIZATION_NVP(m_cmd);
        ar & BOOST_SERIALIZATION_NVP(m_req);
        ar & BOOST_SERIALIZATION_NVP(m_data);
        ar & BOOST_SERIALIZATION_NVP(m_addr);
        ar & BOOST_SERIALIZATION_NVP(m_size);
        ar & BOOST_SERIALIZATION_NVP(m_src);
        ar & BOOST_SERIALIZATION_NVP(m_dst);
#if 0
        ar & BOOST_SERIALIZATION_NVP(m_time);
        ar & BOOST_SERIALIZATION_NVP(m_finishTime);
        ar & BOOST_SERIALIZATION_NVP(m_firstWordTime);
#endif
        ar & BOOST_SERIALIZATION_NVP(m_senderState);
    }
};

#endif
