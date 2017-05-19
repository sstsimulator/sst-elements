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


#ifndef COMPONENTS_FIREFLY_NIC_H 
#define COMPONENTS_FIREFLY_NIC_H 

#include <sstream>
#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/link.h>

#include "sst/elements/thornhill/detailedCompute.h"
#include "ioVec.h"
#include "merlinEvent.h"

namespace SST {
namespace Firefly {

#define NIC_DBG_DMA_ARBITRATE 1<<1
#define NIC_DBG_DETAILED_MEM 1<<2
#define NIC_DBG_SEND_MACHINE 1<<3
#define NIC_DBG_RECV_MACHINE 1<<4

typedef std::function<void()> Callback;

class NicInitEvent : public Event {

  public:
    int node;
    int vNic;
    int num_vNics;

	NicInitEvent() :
		Event() {}

    NicInitEvent( int _node, int _vNic, int _num_vNics ) :
        Event(),
        node( _node ),
        vNic( _vNic ),
        num_vNics( _num_vNics )
    {
    }

public:	
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & node;
        ser & vNic;
        ser & num_vNics;
    }
    
    ImplementSerializable(SST::Firefly::NicInitEvent);     
};

class NicCmdEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, RegMemRgn } type;
    int  node;
	int dst_vNic;
    int tag;
    std::vector<IoVec> iovec;
    void* key;

    NicCmdEvent( Type _type, int _vNic, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key ) :
        Event(),
        type( _type ),
        node( _node ),
		dst_vNic( _vNic ),  
        tag( _tag ),
        iovec( _vec ),
        key( _key )
    {
    }

    NotSerializable(NicCmdEvent)
};

class NicRespEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, Put, Get, NeedRecv } type;
    int src_vNic;
    int node;
    int tag;
    int len;
    void* key;

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len, void* _key ) :
        Event(),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len ),
        key( _key )
    {
    }

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len ) :
        Event(),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len )
    {
    }

    NicRespEvent( Type _type, void* _key ) :
        Event(),
        type( _type ),
        key( _key )
    {
    }

    NotSerializable(NicRespEvent)
};

class Nic : public SST::Component  {

  public:
    typedef uint32_t NodeId;
    static const NodeId AnyId = -1;

  private:

    struct MsgHdr {
        enum Op { Msg, Rdma } op;
        size_t len;
        int    tag;
        unsigned short    dst_vNicId;
        unsigned short    src_vNicId;
    };

    class Entry;
    class SelfEvent : public SST::Event {
      public:

        SelfEvent( Entry* entry) : 
            entry(entry), callback(NULL) {}
        SelfEvent( Callback _callback ) :
            entry(NULL), callback( _callback) {}
        
        Entry*              entry;
        Callback            callback;
        
        NotSerializable(SelfEvent)
    };

    #include "nicVirtNic.h" 

    class MemRgnEntry {
      public:
        MemRgnEntry( int _vNicNum, std::vector<IoVec>& iovec ) : 
            m_vNicNum( _vNicNum ),
            m_iovec( iovec )
        { }

        std::vector<IoVec>& iovec() { return m_iovec; } 
        int vNicNum() { return m_vNicNum; }

      private:
        int          m_vNicNum;
        std::vector<IoVec>  m_iovec;
        
    };

    template < class TRetval = void  >
    class NotifyFunctorBase {
    public:
        virtual TRetval operator()() = 0;
        virtual ~NotifyFunctorBase() {}
    };

    template < class T1, class A1, class TRetval = void >
    class NotifyFunctor_1 : public NotifyFunctorBase< TRetval > {
      private:
        T1* m_obj;
        TRetval ( T1::*m_fptr )( A1 );
        A1 m_a1;

      public:
        NotifyFunctor_1( T1* obj, TRetval (T1::*fptr)(A1), A1 a1 ) :
            m_obj( obj ),
            m_fptr( fptr ), 
            m_a1( a1 )
        {}

        virtual TRetval operator()( ) {
            return (*m_obj.*m_fptr)( m_a1  );
        }
    };

    template < class T1, class A1, class A2, class TRetval = void >
    class NotifyFunctor_2 : public NotifyFunctorBase< TRetval > {
      private:
        T1* m_obj;
        TRetval ( T1::*m_fptr )( A1, A2 );
        A1 m_a1;
        A2 m_a2;

      public:
        NotifyFunctor_2( T1* obj, TRetval (T1::*fptr)(A1,A2), A1 a1, A2 a2 ) :
            m_obj( obj ),
            m_fptr( fptr ), 
            m_a1( a1 ),
            m_a2( a2 )
        {}

        virtual TRetval operator()( ) {
            return (*m_obj.*m_fptr)( m_a1, m_a2  );
        }
    };

    template < class T1, class A1, class A2, class A3, class A4, class A5, 
                    class A6, class TRetval = void >
    class NotifyFunctor_6 : public NotifyFunctorBase< TRetval > {
      private:
        T1* m_obj;
        TRetval ( T1::*m_fptr )( A1, A2, A3, A4, A5, A6 );
        A1 m_a1;
        A2 m_a2;
        A3 m_a3;
        A4 m_a4;
        A5 m_a5;
        A6 m_a6; 

      public:
        NotifyFunctor_6( T1* obj, TRetval (T1::*fptr)(A1, A2, A3, A4, A5, A6 ),
                 A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6 ) :
            m_obj( obj ),
            m_fptr( fptr ), 
            m_a1( a1 ),
            m_a2( a2 ),
            m_a3( a3 ),
            m_a4( a4 ),
            m_a5( a5 ),
            m_a6( a6 )
        {}

        virtual TRetval operator()( ) {
            return (*m_obj.*m_fptr)( m_a1, m_a2, m_a3, m_a4, m_a5, m_a6  );
        }
    };

    class Entry {
      public:
        Entry( int local_vNic, NicCmdEvent* cmd = NULL ) :
            currentVec(0),
            currentPos(0),
            currentLen(0),
            m_cmdEvent( cmd ),
            m_local_vNic( local_vNic ),
            m_ioVec( NULL ),
            m_notifyFunctor( NULL )
        {
        }
        virtual ~Entry() { 
            if ( m_cmdEvent ) delete m_cmdEvent; 
            if ( m_notifyFunctor ) delete m_notifyFunctor;
        }

        int local_vNic()   { return m_local_vNic; }

        std::vector<IoVec>& ioVec() { 
            assert(m_ioVec);
            return *m_ioVec; 
        }

        size_t totalBytes() {
            assert(m_ioVec);
            size_t bytes = 0;
            for ( unsigned int i = 0; i < m_ioVec->size(); i++ ) {
                bytes += m_ioVec->at(i).len; 
            } 
            return bytes;
        }

        size_t      currentVec;
        size_t      currentPos;  
        size_t      currentLen; 
        virtual void* key() { 
            assert( m_cmdEvent );
            return m_cmdEvent->key; 
        }

        virtual int node() {
            assert( m_cmdEvent );
            return m_cmdEvent->node;
        }
        void setNotifier( NotifyFunctorBase<>* notifier) {
            m_notifyFunctor = notifier;
        }
        void notify() {
            if ( m_notifyFunctor ) {
                (*m_notifyFunctor)(); 
            }
        }
        bool isDone() {
            return ( currentVec == ioVec().size() );
        }
        
      protected:
        NicCmdEvent*        m_cmdEvent;
        int                 m_local_vNic;
        std::vector<IoVec>* m_ioVec;

      private:
        NotifyFunctorBase<>* m_notifyFunctor;
    };

    class SendEntry: public Entry {
      public:

        SendEntry( int local_vNic, NicCmdEvent* cmd = NULL ) : 
            Entry( local_vNic, cmd )
        {
            m_ioVec = &m_cmdEvent->iovec;
        }

        virtual ~SendEntry() {
        }

        virtual MsgHdr::Op getOp() {
            return MsgHdr::Msg;
        }

        virtual int tag()       { return m_cmdEvent->tag; }
        virtual int dst_vNic( ) { return m_cmdEvent->dst_vNic; } 
        virtual int type()      { return m_cmdEvent->type; }
    };

    class RecvEntry: public Entry {

      public:
        RecvEntry( int local_vNic, NicCmdEvent* cmd = NULL ) : 
            Entry( local_vNic, cmd) 
        {
            if ( cmd ) {
                m_ioVec = &cmd->iovec;  
            }
        }
        virtual ~RecvEntry() {
        }

        void setMatchInfo( int src, MsgHdr& hdr ) { 
            m_matchSrc = src;
            m_matchHdr = hdr; 
        } 

        int match_src()    { return m_matchSrc; }
        virtual int match_tag()    { return m_matchHdr.tag; }  
        virtual size_t match_len() { return m_matchHdr.len; }  
        int src_vNic()     { return m_matchHdr.src_vNicId; }

      private:
        int                 m_matchSrc;
        MsgHdr              m_matchHdr;
    };

    class PutRecvEntry : public RecvEntry {
      public:
        PutRecvEntry( int local_vNic, std::vector<IoVec>* ioVec ) :
            RecvEntry( local_vNic ),
            m_recvVec(*ioVec)
        {
            m_ioVec = &m_recvVec;
        }

        virtual ~PutRecvEntry() {
        }

        int match_tag() { return -1; }
        size_t match_len() { return totalBytes(); }

      private:
//        void* m_key;
        std::vector<IoVec> m_recvVec;
    };

    struct RdmaMsgHdr {
        enum { Put, Get, GetResp } op;
        uint16_t    rgnNum;
        uint16_t    respKey;
        uint32_t    offset;
    };

    class GetOrgnEntry : public SendEntry {
      public:
        GetOrgnEntry( int local_vNic, NicCmdEvent* cmd, int respKey ) :
            SendEntry( local_vNic, cmd ),
            m_rdmaVec( 1 ) 
        { 
            m_hdr.respKey = respKey;
            m_hdr.rgnNum = cmd->tag; 
            m_hdr.offset = -1;
            m_hdr.op = RdmaMsgHdr::Get;
            m_rdmaVec[0].addr.simVAddr = 100;
            m_rdmaVec[0].addr.backing = &m_hdr;
            m_rdmaVec[0].len = sizeof( m_hdr );
            m_ioVec = &m_rdmaVec;
        }

        virtual ~GetOrgnEntry() {
        }

        virtual MsgHdr::Op getOp() {
            return MsgHdr::Rdma;
        }
        
      private:
        RdmaMsgHdr          m_hdr;
        std::vector<IoVec>  m_rdmaVec; 
    };

    class PutOrgnEntry : public SendEntry {
      public:
        PutOrgnEntry( int local_vNic, int dst_node,int dst_vNic,
                int respKey, MemRgnEntry* memRgn ) :
            SendEntry( local_vNic ),
            m_dst_node( dst_node ),
            m_dst_vNic( dst_vNic ), 
            m_memRgn( memRgn )
        {
            m_putVec.resize(1);
            m_hdr.respKey = respKey;
            m_hdr.op = RdmaMsgHdr::GetResp;
            m_putVec[0].addr.simVAddr = 101;
            m_putVec[0].addr.backing = &m_hdr;
            m_putVec[0].len = sizeof(m_hdr);
            for ( unsigned int i = 0; i < memRgn->iovec().size(); i++ ) {
                m_putVec.push_back( memRgn->iovec()[i] );
            }

            m_ioVec = &m_putVec;
        }
        ~PutOrgnEntry() {
            delete m_memRgn;
        }

        virtual MsgHdr::Op getOp() {
            return MsgHdr::Rdma;
        }
        virtual int tag() { return -1; }
        virtual int dst_vNic() { return m_dst_vNic; }
        virtual int node() { return m_dst_node; }
        virtual int type() { return NicCmdEvent::Put; }

      private:
        int                 m_dst_node;
        int                 m_dst_vNic;
        MemRgnEntry*        m_memRgn;
        RdmaMsgHdr          m_hdr;
        std::vector<IoVec>  m_putVec;
    };

	struct DmaVec {
		uint64_t addr;
		size_t   length;
	};

    #include "nicSendMachine.h"
    #include "nicRecvMachine.h"
    #include "nicArbitrateDMA.h"

public:

    Nic(ComponentId_t, Params& );
    ~Nic();

    void init( unsigned int phase );
    int getNodeId() { return m_myNodeId; }
    int getNum_vNics() { return m_num_vNics; }
    void printStatus(Output &out);

    void detailedMemOp( Thornhill::DetailedCompute* detailed,
            std::vector<DmaVec>& vec, std::string op, Callback callback ) {

        std::deque< std::pair< std::string, SST::Params> > gens;
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
						"%s %zu vectors\n", op.c_str(), vec.size());

		for ( unsigned i = 0; i < vec.size(); i++ ) {

			Params params;
			std::stringstream tmp;

			if ( 0 == vec[i].addr ) {
				m_dbg.fatal(CALL_INFO,-1,"Invalid addr %" PRIx64 "\n", vec[i].addr);	
			}
			if ( vec[i].addr < 0x100 ) {
				m_dbg.output(CALL_INFO,"Warn addr %" PRIx64 " ignored\n", vec[i].addr);	
				i++;
				continue;	
			}

			if ( 0 == vec[i].length ) {
				i++;
				m_dbg.verbose(CALL_INFO,-1,NIC_DBG_DETAILED_MEM,
						"skip 0 length vector addr=0x%" PRIx64 "\n",vec[i].addr);
				continue;	
			}

			int opWidth = 8;
			m_dbg.verbose(CALL_INFO,1,NIC_DBG_DETAILED_MEM,
				"addr=0x%" PRIx64 " length=%zu\n",
				vec[i].addr,vec[i].length);
			size_t count = vec[i].length / opWidth;
			count += vec[i].length % opWidth ? 1 : 0;
			tmp << count; 
			params.insert( "count", tmp.str() );		

			tmp.str( std::string() ); tmp.clear();
			tmp << opWidth;
			params.insert( "length", tmp.str() );		

			tmp.str( std::string() ); tmp.clear();
			tmp << vec[i].addr;
			params.insert( "startat", tmp.str() );		

			tmp.str( std::string() ); tmp.clear();
			tmp << 0x100000000;
			params.insert( "max_address", tmp.str() );		

			params.insert( "memOp", op );		
            #if 0 
			params.insert( "generatorParams.verbose", "1" );		
			params.insert( "verbose", "5" );		
            #endif
			
        	gens.push_back( std::make_pair( "miranda.SingleStreamGenerator", params ) );
		}

		if ( gens.empty() ) {
			schedCallback( callback, 0 );
		} else {
	    	std::function<int()> foo = [=](){
           		callback( );
				return 0;
			};
			detailed->start( gens, foo );	
		}
		
	}

    void dmaRead( std::vector<DmaVec>& vec, Callback callback ) {

		if ( m_useDetailedCompute && m_detailedCompute[0] ) {
			
			detailedMemOp( m_detailedCompute[0], vec, "Read", callback );

		} else {
			size_t len = 0;	
			for ( unsigned i = 0; i < vec.size(); i++ ) {
				len += vec[i].length;
			} 
        	m_arbitrateDMA->canIRead( callback, len );
		}
    }

    void dmaWrite( std::vector<DmaVec>& vec, Callback callback ) {

		if ( m_useDetailedCompute && m_detailedCompute[1] ) {
			
			detailedMemOp( m_detailedCompute[1], vec, "Write", callback );

		} else {
			size_t len = 0;	
			for ( unsigned i = 0; i < vec.size(); i++ ) {
				len += vec[i].length;
			} 
        	m_arbitrateDMA->canIWrite( callback, len );
		}
    }

    void schedCallback(  Callback callback, uint64_t delay = 0 ) {
        schedEvent( new SelfEvent( callback ), delay);
    }

    void setNotifyOnSend( int vc ) {
        assert( ! m_sendNotify[vc] ); 
        m_sendNotify[vc] = true;
        ++m_sendNotifyCnt;
        m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
    }

    std::vector<bool> m_sendNotify;
    int m_sendNotifyCnt;

  private:
    void handleSelfEvent( Event* );
    void handleVnicEvent( Event*, int );
    void dmaSend( NicCmdEvent*, int );
    void pioSend( NicCmdEvent*, int );
    void dmaRecv( NicCmdEvent*, int );
    void get( NicCmdEvent*, int );
    void put( NicCmdEvent*, int );
    void regMemRgn( NicCmdEvent*, int );
    void processNetworkEvent( FireflyNetworkEvent* );

    void schedEvent( SelfEvent* event, int delay = 0 ) {
        m_selfLink->send( delay, event );
    }

    void notifySendDmaDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendDmaDone(  key );
    }

    void notifySendPioDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendPioDone(  key );
    }

    void notifyRecvDmaDone(int vNic, int src_vNic, int src, int tag,
                                                    size_t len, void* key) {
        m_vNicV[vNic]->notifyRecvDmaDone( src_vNic, src, tag, len, key );
    }

    void notifyNeedRecv( int vNic, int src_vNic, int src, int tag,
                                                    size_t length ) {
    	m_dbg.verbose(CALL_INFO,2,1,"src_vNic=%d src=%d tag=%#x len=%lu\n",
                                            src_vNic,src,tag,length);

        m_vNicV[vNic]->notifyNeedRecv( src_vNic, src, tag, length );
    }

    void notifyPutDone( int vNic, void* key ) {
        m_dbg.verbose(CALL_INFO,2,1,"%p\n",key);
        assert(0);
    }

    void notifyGetDone( int vNic, void* key ) {
        m_dbg.verbose(CALL_INFO,2,1,"%p\n",key);
        m_vNicV[vNic]->notifyGetDone( key );
    }

    uint16_t genGetKey() {
        return ++m_getKey;
    }

    int NetToId( int x ) { return x; }
    int IdToNet( int x ) { return x; }

    std::vector<SendMachine> m_sendMachine;
    RecvMachine m_recvMachine;
    ArbitrateDMA* m_arbitrateDMA;

    std::vector< std::map< int, MemRgnEntry* > > m_memRgnM;
    std::map< int, PutRecvEntry* > m_getOrgnM;
 
    int                     m_myNodeId;
    int                     m_num_vNics;
    SST::Link*              m_selfLink;

    // the interface to to Merlin
    // Merlin::LinkControl*                m_linkControl;
    // Merlin::LinkControl::Handler<Nic> * m_recvNotifyFunctor;
    // Merlin::LinkControl::Handler<Nic> * m_sendNotifyFunctor;
    SST::Interfaces::SimpleNetwork*     m_linkControl;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_recvNotifyFunctor;
    SST::Interfaces::SimpleNetwork::Handler<Nic>* m_sendNotifyFunctor;
    bool sendNotify(int);
    bool recvNotify(int);

    Output                  m_dbg;
    std::vector<VirtNic*>   m_vNicV;
    std::vector<Thornhill::DetailedCompute*> m_detailedCompute;
	bool m_useDetailedCompute;

    uint16_t m_getKey;
  public:
	int m_tracedPkt;
	int m_tracedNode;
}; 

} // namesapce Firefly 
} // namespace SST

#endif
