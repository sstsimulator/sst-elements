#ifndef _ptlNic_h
#define _ptlNic_h

#include "ptlNicEvent.h"
#include "portals4_types.h"
#include "dmaEngine.h"
#include "context.h"
#include "callback.h"

#include "sst/elements/SS_router/SS_router/RtrIF.h"

struct PtlHdr;

class PtlNic : public RtrIF 
{
    class SendEntry {
      public:
        SendEntry( ptl_nid_t nid, PtlHdr* hdr, Addr vaddr, ptl_size_t length,
                                            CallbackBase* callback ) :
            m_destNid( nid ),
            m_hdr( hdr ),
            m_vaddr( vaddr ),
            m_length( length ),
            m_callback( callback ), 
            m_offset( 0 ),
            m_dmaed( 0 ),
            m_hdrDone( false )
        {} 

        bool hdrDone() { return m_hdrDone; } 
        void setHdrDone() { m_hdrDone = true; } 
        ptl_nid_t destNid() { return m_destNid; }
        ptl_size_t bytesLeft( ) { return m_length - m_offset; }
        bool dmaDone() { return (m_length == m_dmaed ); }
        Addr vaddr() { return m_vaddr + m_offset; }
        void incOffset( size_t nbytes ) { m_offset += nbytes; }
        void incDmaed( size_t nbytes ) { m_dmaed += nbytes; }
        CallbackBase* callback() { return m_callback; }
        PtlHdr* ptlHdr() { return m_hdr; }

      private:
        ptl_nid_t       m_destNid;
        PtlHdr*         m_hdr;
        Addr            m_vaddr;
        ptl_size_t      m_length; 
        CallbackBase*   m_callback;
        ptl_size_t      m_offset; 
        ptl_size_t      m_dmaed; 
        bool            m_hdrDone;
    };

    class VCInfo {

        struct XXX {
            ptl_nid_t   destNid;            
            size_t      offset;
            bool        head;
            bool        tail;
            std::vector<unsigned char>  buf; 
            Callback< VCInfo, XXX >*    callback;
        }; 

        typedef Callback<VCInfo, XXX> XXXCallback;

      public:
        VCInfo( PtlNic& );
        void addMsg( SendEntry* );
        void process();
        void setVC(int);

      private:    
        void processRtrQ();
        void processSendQ();
        bool dmaCallback( XXX* );

        std::deque< SendEntry* > m_sendQ;
        std::deque< XXX* >       m_rtrQ;
        PtlNic&                  m_nic;
        RtrEvent*                m_rtrEvent;
        int                      m_vc;
    };

  public: 
    static const int MsgVC = 0;

    class Cmd {
      public:
        Cmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
            m_event( e ),
            m_retval( -PTL_FAIL ),
            m_done( false ),
            m_nic( nic ),
            m_ctx( ctx )
        {
        }

        ~Cmd() {
            delete m_event;
        }

        static Cmd* create( PtlNic&, Context&, PtlNicEvent* );
        virtual bool work() = 0;
        int retval() { return m_retval; }
        std::string name() { return m_cmdNames[m_event->cmd]; }

      protected:
        enum State { Finished, NotFinished };
        PtlNicEvent*        m_event;
        int                 m_retval;
        bool                m_done;
        PtlNic&             m_nic;
        Context&            m_ctx;
        static const char*  m_cmdNames[];
    };

    struct RtrXfer {
        int             dest;
        void*           buf;
        size_t          nbytes;
    };

  public:
    PtlNic( SST::ComponentId_t id, Params_t& params );
    ptl_pid_t allocPid( ptl_pid_t );
    DmaEngine& dmaEngine() { return m_dmaEngine; }
    int nid() { return m_nid; }
    bool sendMsg( ptl_nid_t, PtlHdr*, Addr, ptl_size_t, CallbackBase* );

  private:
    int allocContext();
    int freeContext( int );
    Context* getContext( int ctx );
    Context* findContext( ptl_pid_t pid );
    void processVCs();
    void processFromRtr();
    int Setup();
    bool clock( SST::Cycle_t cycle );
    void rtrHandler( SST::Event* );
    void mmifHandler( SST::Event* );
    void processPtlCmdQ();
    void contextInit( PtlNicEvent* );
    void contextFini( PtlNicEvent* );
    void ptlCmd( PtlNicEvent* );

    typedef std::map<ptl_nid_t,RecvEntry*>  nidRecvEntryM_t;
         
    int                     m_nid;
    SST::Link*              m_mmifLink;
    std::deque<Cmd*>        m_ptlCmdQ;
    std::vector<Context*>   m_contextV;
    DmaEngine               m_dmaEngine;
    std::vector< VCInfo >   m_vcInfoV;
    nidRecvEntryM_t         m_nidRecvEntryM;
};

#endif
