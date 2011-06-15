#ifndef _context_h
#define _context_h

#include <deque>
#include <vector>
#include <list>
#include "ptlEvent.h"
#include "ptlHdr.h"
#include "callback.h"
#include "ptlNicTypes.h"

class PtlNic;
class RecvEntry;
class Context {

    struct PT {
        bool used;
        int eq_handle;
        unsigned int options;
        std::list<int> meL[2];
    };

    struct CT {
        bool                    avail;
        Addr                    vaddr;
        ptl_ct_event_t          event;
    };

    struct EQ {
        bool                    avail;
        Addr                    vaddr;
        struct PtlEventInternal event;
        ptl_size_t              count;
        ptl_size_t              size;
    };
    struct ME {
        ptl_me_t                me;
        void*                   user_ptr;
        bool                    avail;
    };

    struct MD {
        ptl_md_t                md;
        bool                    avail;
    };

  public:
    Context( PtlNic* nic, ptl_uid_t, ptl_jid_t );
    ~Context();
    void initPid( ptl_pid_t pid );
    ptl_pid_t pid() { return m_pid; }
    void initOptions( int options );
    void initId();
    ptl_ni_limits_t* limits();
    ptl_process_t* id();

    RecvEntry* processHdrPkt( void* pkt );

    int allocMD();
    int freeMD( int handle );
    ptl_md_t* findMD( int handle );

    int allocME( ptl_pt_index_t portal, ptl_list_t list, void* user_ptr );
    ptl_me_t* findME( int handle );
    int freeME( int handle );

    int allocCT( Addr eventAddr );
    int addCT( int handle, ptl_size_t value );
    ptl_ct_event_t* findCTEvent( int handle );
    Addr findCTAddr( int handle );
    int freeCT( int handle );

    int allocEQ( Addr vaddr, int count );
    int freeEQ( int handle );
    struct EQ&  findEQ( int handle );
    PtlEventInternal* findEvent( int handle );
    Addr findEventAddr( int handle );

    int allocPT( unsigned int options, int eq_handle, 
                    ptl_pt_index_t req_pt ); 
    int freePT( int pt_index );

    bool isvalidPT( int pt_index ) {
        return m_ptV[pt_index].used; 
    }

    int appendPT( ptl_pt_index_t, ptl_list_t list, int handle );

    int put( int  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_ack_req_t    ack_req,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr,
           ptl_hdr_data_t   hdr_data);

    int get( int  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr );

  private:

    RecvEntry* processHdrPkt( ptl_nid_t nid, PtlHdr* hdr );
    RecvEntry* processMatch( ptl_nid_t, PtlHdr*, int me_handle );
    RecvEntry* processPut( ptl_nid_t, PtlHdr*, int me_handle );
    RecvEntry* processGet( ptl_nid_t, PtlHdr*, int me_handle );
    void processAck( PtlHdr * );
    RecvEntry* processReply( PtlHdr * );
    void recvFini( ptl_nid_t nid, PtlHdr* hdr, int me_handle );

    struct YYY {
        PtlHdr      hdr;
        void       *user_ptr;
        int         md_handle; 
        CallbackBase* callback;
    };

    //*************
    struct PutSendEntry : YYY {
        enum { WaitPut, WaitAck, WaitCtEvent, WaitEvent, Done } state;
    };
    typedef Callback< Context, PutSendEntry >  PutCallback;
    bool putCallback( PutSendEntry* );

    //*************
    struct GetSendEntry : YYY {
        ptl_size_t  local_offset;
        enum { WaitPut, WaitReply,  WaitCtEvent, WaitEvent, Done } state;
    };
    typedef Callback< Context, GetSendEntry >  GetCallback;
    bool getCallback( GetSendEntry* );

    struct XXX {
        PtlHdr        hdr;
        ptl_nid_t     nid;
        int           me_handle;
        CallbackBase* callback;
    };

    struct PutRecvEntry : XXX {
        enum { WaitRecvComp, WaitAckSent } state;
    };

    typedef Callback<Context,PutRecvEntry>  PutRecvCallback;
    bool putRecvCallback( PutRecvEntry* );

    //*************
    struct GetRecvEntry : XXX {
    };
    typedef Callback< Context, GetRecvEntry >  GetRecvCallback;
    bool getRecvCallback( GetRecvEntry* );

    //*************
    struct EventEntry {
        Callback<Context,EventEntry>* callback;
    };
    typedef Callback<Context,EventEntry>  EventCallback;
    void writeEvent( int, EventCallback* );
    bool eventCallback( EventEntry* );

    void writeCtEvent( int, CallbackBase* );

    bool                    m_logicalIF;
    bool                    m_matching; 

    ptl_process_t           m_id;
    ptl_pid_t               m_pid;
    ptl_uid_t               m_uid;
    ptl_jid_t               m_jid;

    std::vector<PT>         m_ptV;
    std::vector<CT>         m_ctV;
    std::vector<EQ>         m_eqV;
    std::vector<MD>         m_mdV;
    std::vector<ME>         m_meV;
    ptl_ni_limits_t         m_limits;
    PtlNic*                 m_nic;
    std::map< int, PutSendEntry* >   m_putM;
    std::map< int, GetSendEntry* >   m_getM;
 };

#endif
