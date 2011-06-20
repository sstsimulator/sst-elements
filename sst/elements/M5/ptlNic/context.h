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
        ptl_size_t              count;
        ptl_size_t              size;
    };

    struct ME {
        ptl_me_t                me;
        void*                   user_ptr;
        ptl_size_t              offset;
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
    void addCT( int handle, ptl_size_t value );
    ptl_ct_event_t* findCTEvent( int handle );
    Addr findCTAddr( int handle );
    int freeCT( int handle );

    int allocEQ( Addr vaddr, int count );
    int freeEQ( int handle );
    struct EQ&  findEQ( int handle );
    Addr findEventAddr( int handle, int pos );

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

    void writeEvent( int eq_handle,
                    ptl_event_kind_t    type,
                    ptl_process_t&      initiator,
                    ptl_pt_index_t      pt_index,
                    ptl_uid_t           uid,
                    ptl_jid_t           jid,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          rlength,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              start,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_ni_fail_t       ni_fail_type );

    void writeCtEvent( int ct_handle, ptl_ct_event_t&  );

    void writeReplyEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );

    void writeAckEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );

    void writeSendEvent( int eq_handle,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );


    struct YYY {
        PtlHdr      hdr;
        void       *user_ptr;
        int         md_handle; 
        CallbackBase* callback;
    };

    //*************
    struct PutSendEntry : YYY {
        enum { WaitSend, WaitAck } state;
    };
    typedef Callback< Context, PutSendEntry >  PutCallback;
    bool putCallback( PutSendEntry* );

    //*************
    struct GetSendEntry : YYY {
        ptl_size_t  local_offset;
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
        EventEntry( int _handle ) : handle( _handle ) {}
        struct PtlEventInternal event;
        ptl_ct_event_t          ctEvent;
        CallbackBase*           callback;
        int handle;
    };
    typedef Callback<Context,EventEntry>  EventCallback;
    bool eventCallback( EventEntry* );
    void writeEvent( EventEntry* );
    void writeCtEvent( EventEntry* );

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

inline void Context::writeCtEvent( int ct_handle, ptl_ct_event_t& event )
{
    EventEntry* entry = new EventEntry( ct_handle );
    assert( entry );

    entry->callback = new EventCallback( this, &Context::eventCallback, entry );
    entry->ctEvent = event;

    writeCtEvent( entry );
}

inline void Context::writeEvent( int eq_handle,
                    ptl_event_kind_t    type,
                    ptl_process_t&      initiator,
                    ptl_pt_index_t      pt_index,
                    ptl_uid_t           uid,
                    ptl_jid_t           jid,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          rlength,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              start,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_ni_fail_t       ni_fail_type )
{
    EventEntry* entry = new EventEntry( eq_handle );
    assert( entry );

    entry->callback = new EventCallback( this, &Context::eventCallback, entry );

    entry->event.user.type          = type;
    entry->event.user.initiator     = initiator;
    entry->event.user.pt_index      = pt_index;
    entry->event.user.uid           = uid;
    entry->event.user.jid           = jid;
    entry->event.user.match_bits    = match_bits;
    entry->event.user.rlength       = rlength;
    entry->event.user.mlength       = mlength;
    entry->event.user.remote_offset = remote_offset;
    entry->event.user.start         = start;
    entry->event.user.user_ptr      = user_ptr;
    entry->event.user.hdr_data      = hdr_data;
    entry->event.user.ni_fail_type  = ni_fail_type;
//    entry->event.user.atomic_operation = atomic_operation;
//    entry->event.user.atomic_type      = atomic_type;

    writeEvent( entry );
}

inline void Context::writeReplyEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;
    
    writeEvent( eq_handle,
                PTL_EVENT_REPLY,
                initiator,
                0,
                0,
                0,
                0,
                0,
                mlength,
                remote_offset,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}

inline void Context::writeAckEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;

    writeEvent( eq_handle,
                PTL_EVENT_ACK,
                initiator,
                0,
                0,
                0,
                0,
                0,
                mlength,
                remote_offset,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}

inline void Context::writeSendEvent( int eq_handle,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;

    writeEvent( eq_handle,
                PTL_EVENT_SEND,
                initiator,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}


#endif
