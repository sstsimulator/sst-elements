#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "context.h"
#include "ptlNic.h"
#include "callback.h"
#include "ptlHdr.h"
#include "recvEntry.h"

Context::Context( PtlNic* nic, cmdContextInit_t& cmd ) :
        m_nic( nic ),
        m_uid( cmd.uid ),
        m_jid( cmd.jid ), 
        m_meUnlinkedHostPtr( (int*) cmd.meUnlinkedPtr ),
        m_meUnlinkedPos( 0 )
{
    TRACE_ADD( Context );
    PRINT_AT(Context,"sizeof(PtlHdr) %d\n",sizeof(PtlHdr));

    m_limits.max_pt_index = MAX_PT_INDEX;
    m_limits.max_cts = MAX_CTS;
    m_limits.max_mds = MAX_MDS;
    m_limits.max_entries = MAX_ENTRIES;
    m_limits.max_eqs = MAX_EQS;
    m_limits.max_list_size = MAX_LIST_SIZE;

    m_ptV.resize( m_limits.max_pt_index );
    m_ctV.resize( m_limits.max_cts );
    m_mdV.resize( m_limits.max_mds );
    m_meV.resize( m_limits.max_entries );
    m_eqV.resize( m_limits.max_eqs );

    for ( int i = 0; i < m_ptV.size(); i++ ) {
        m_ptV[i].used = false;
    }

    for ( int i = 0; i < m_meV.size(); i++ ) {
        m_meV[i].used = false;
    }

    for ( int i = 0; i < m_mdV.size(); i++ ) {
        m_mdV[i].used = false;
    }

    m_nic->dmaEngine().write( cmd.limitsPtr, (uint8_t*) &m_limits, 
                        sizeof( m_limits ), NULL );

    // write this last, the host is waiting for value to change
    m_nic->dmaEngine().write( cmd.nidPtr, (uint8_t*) &m_nic->nid(), 
                        sizeof( m_nic->nid() ), NULL );
}

Context::~Context() {
    PRINT_AT(Context,"\n");
}

void Context::initPid( ptl_pid_t pid ) {
    PRINT_AT(Context,"pid=%d\n",pid);
    m_pid = pid;
    initId( );
} 
    
void Context::initOptions( int options ) {
    m_logicalIF = options & PTL_NI_LOGICAL ? true : false;
    m_matching = options & PTL_NI_MATCHING ? true : false;
}

void Context::initId() {
    if ( m_logicalIF == PTL_NI_LOGICAL ) {
        // walk the logical mapping table to find our rank
        //m_id.rank = X;
    } else {
        m_id.phys.nid = m_nic->nid();
        m_id.phys.pid = m_pid;
    }  
}

void Context::NIInit( cmdPtlNIInit_t& cmd )
{
    initPid( cmd.pid );
    initOptions( cmd.options );
}

void Context::NIFini( cmdPtlNIFini_t& cmd )
{
}

void Context::MEAppend( cmdPtlMEAppend_t& cmd )
{
    PRINT_AT(Context,"pt_index=%d handle=%d list=%d\n", 
                    cmd.pt_index, cmd.handle, cmd.list);
    PRINT_AT(Context,"match_bits=%#lx options=%#x\n", 
                                cmd.me.match_bits, cmd.me.options );

    assert( ! m_meV[cmd.handle].used );

    if ( cmd.list == PTL_PRIORITY_LIST ) { 

        XXX* entry = searchOverflow( cmd.me );

        if ( entry ) {

            // if callback is not null then the data movement is not complete
            assert( ! entry->callback );
            PtlHdr& hdr = entry->origHdr;
            PRINT_AT(Context,"Found Match in overflow me_handle=%d\n",
                                        entry->me_handle );    

            ptl_process_t initiator;
            initiator.phys.nid = entry->srcNid; 
            initiator.phys.pid = hdr.src_pid;

            writeEvent( 
                m_ptV[hdr.pt_index].eq_handle,
                PTL_EVENT_PUT_OVERFLOW,
                initiator,
                hdr.pt_index,
                hdr.uid,
                hdr.jid,
                hdr.match_bits,
                hdr.length,
                entry->mlength,
                hdr.offset,
                entry->start,
                cmd.user_ptr,
                hdr.hdr_data,
                PTL_NI_OK
            );
            delete entry;
        }
    }

    m_ptV[cmd.pt_index].meL[cmd.list].push_back( cmd.handle );
    m_meV[cmd.handle].user_ptr = cmd.user_ptr; 
    m_meV[cmd.handle].me = cmd.me;
    m_meV[cmd.handle].offset = 0;
    m_meV[cmd.handle].used = true;
}

void Context::MEUnlink( cmdPtlMEUnlink_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n",cmd.handle);
    assert( m_meV[cmd.handle].used );
    m_meV[cmd.handle].used = false;
}

ptl_me_t* Context::findME( int handle ) {
    return &m_meV[handle].me;
}

void Context::CTAlloc( cmdPtlCTAlloc_t& cmd )
{
    PRINT_AT(Context,"handle=%d addr=%#lx\n", cmd.handle, cmd.addr );
    m_ctV[cmd.handle].vaddr = (Addr)cmd.addr;
    m_ctV[cmd.handle].event.success = 0;
    m_ctV[cmd.handle].event.failure = 0;
}

void Context::CTFree( cmdPtlCTFree_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n", cmd.handle );
}

void Context::EQAlloc( cmdPtlEQAlloc_t& cmd )
{
    PRINT_AT(Context,"handle=%d addr=%#lx\n", cmd.handle, cmd.addr );
    m_eqV[cmd.handle].vaddr = (Addr)cmd.addr;
    m_eqV[cmd.handle].size = cmd.size;
    m_eqV[cmd.handle].count = 0;
}

void Context::EQFree( cmdPtlEQFree_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n", cmd.handle );
}

void Context::MDBind( cmdPtlMDBind_t& cmd )
{
    PRINT_AT(Context,"handle=%d addr=%#lx size=%lu\n", 
                    cmd.handle, cmd.md.start, cmd.md.length );
    assert( ! m_mdV[cmd.handle].used );
    m_mdV[cmd.handle].md = cmd.md;
    m_mdV[cmd.handle].used = true;
}

void Context::MDRelease( cmdPtlMDRelease_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n", cmd.handle );
    assert( m_mdV[cmd.handle].used );
    m_mdV[cmd.handle].used = false;
}


void Context::addCT( int handle, ptl_size_t value ) {
    m_ctV[handle].event.success += value;
}

ptl_ct_event_t* Context::findCTEvent( int handle ){
    return &m_ctV[handle].event;
}

Addr Context::findCTAddr( int handle ) {
    return m_ctV[handle].vaddr;
}

struct Context::EQ& Context::findEQ( int handle )
{
    return m_eqV[handle];
}

Addr Context::findEventAddr( int handle, int pos ) 
{
    struct EQ& eq = m_eqV[handle]; 
    return eq.vaddr + ( pos * sizeof( PtlEventInternal ) ); 
}

void Context::PTAlloc( cmdPtlPTAlloc_t& cmd )
{
    PRINT_AT(Context,"pt_index=%d eq_handle=%#x\n",cmd.pt_index,
                                            cmd.eq_handle);

    m_ptV[cmd.pt_index].used = true; 
    m_ptV[cmd.pt_index].eq_handle = cmd.eq_handle;
    m_ptV[cmd.pt_index].options = cmd.options;
}

void Context::PTFree( cmdPtlPTFree_t& cmd )
{
    PRINT_AT(Context,"pt_index=%d\n", cmd.pt_index );

    m_ptV[cmd.pt_index].used = false; 
}

void Context::Put( cmdPtlPut_t& cmd ) 
{
    PRINT_AT(Context,"me=%d\n",m_nic->nid());
    PRINT_AT(Context,"md_handle=%d length=%lu local_offset=%lu "
            "remote_offset=%lu\n", 
            cmd.md_handle, cmd.length, cmd.local_offset, cmd.remote_offset );

    if ( m_logicalIF ) {
        PRINT_AT(Context,"target rank=%d\n",cmd.target_id.rank);
    } else {
        PRINT_AT(Context,"target nid=%d pid=%d\n",
                    cmd.target_id.phys.nid,cmd.target_id.phys.pid);
    }
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",
                                    cmd.pt_index,cmd.match_bits); 

    PutSendEntry* entry = new PutSendEntry;
    assert(entry);

    entry->user_ptr = cmd.user_ptr;
    entry->state = PutSendEntry::WaitSend;
    entry->callback = new PutCallback(this, &Context::putCallback, entry );

    entry->md_handle = cmd.md_handle;
    entry->hdr.length = cmd.length;
    entry->hdr.ack_req = cmd.ack_req;
    entry->hdr.pt_index = cmd.pt_index;
    entry->hdr.dest_pid = cmd.target_id.phys.pid;
    entry->hdr.src_pid  = m_pid;
    entry->hdr.offset = cmd.remote_offset;
    entry->hdr.match_bits = cmd.match_bits;
    entry->hdr.hdr_data = cmd.hdr_data;
    entry->hdr.uid = m_uid;
    entry->hdr.jid = m_jid;
    
    entry->hdr.op = ::Put;

    // need some data structure that contains a map of used keys and deque of 
    // free keys
    int key = 5;
    entry->hdr.key = key; 
    m_putM[ key ] = entry;

    m_nic->sendMsg( cmd.target_id.phys.nid, &entry->hdr, 
                    (Addr) m_mdV[cmd.md_handle].md.start + cmd.local_offset, 
                    cmd.length, entry->callback );  
}

void Context::Get( cmdPtlGet_t& cmd ) 
{
    PRINT_AT(Context,"md_handle=%d length=%lu local_offset=%#lx "
    "remote_offset=%#lx\n", cmd.md_handle, cmd.length, cmd.local_offset, cmd.remote_offset );

    if ( m_logicalIF ) {
        PRINT_AT(Context,"target rank=%d\n",cmd.target_id.rank);
    } else {
        PRINT_AT(Context,"target nid=%d pid=%d\n",
                    cmd.target_id.phys.nid,cmd.target_id.phys.pid);
    }
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",cmd.pt_index,cmd.match_bits); 

    GetSendEntry* entry = new GetSendEntry;
    assert(entry);

    entry->user_ptr = cmd.user_ptr;
    entry->callback = new GetCallback(this, &Context::getCallback, entry );

    entry->local_offset = cmd.local_offset;
    entry->md_handle = cmd.md_handle;
    entry->hdr.length = cmd.length;
    entry->hdr.pt_index = cmd.pt_index;
    entry->hdr.dest_pid = cmd.target_id.phys.pid;
    entry->hdr.src_pid  = m_pid;
    entry->hdr.offset = cmd.remote_offset;
    entry->hdr.match_bits = cmd.match_bits;
    entry->hdr.op = ::Get;

    int key = 5;
    entry->hdr.key = key; 
    m_getM[ key ] = entry;

    m_nic->sendMsg( cmd.target_id.phys.nid, &entry->hdr, (Addr) 0, 0,  NULL );
}

void Context::TrigGet( cmdPtlTrigGet_t& cmd ) 
{
    PRINT_AT(Context,"md_handle=%d length=%lu local_offset=%#lx "
    "remote_offset=%#lx\n", cmd.md_handle, cmd.length, cmd.local_offset, cmd.remote_offset );

    if ( m_logicalIF ) {
        PRINT_AT(Context,"target rank=%d\n",cmd.target_id.rank);
    } else {
        PRINT_AT(Context,"target nid=%d pid=%d\n",
                    cmd.target_id.phys.nid,cmd.target_id.phys.pid);
    }
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",cmd.pt_index,cmd.match_bits); 
    PRINT_AT(Context,"ct_handle=%d threshold=%lu\n",
                            cmd.trig_ct_handle,cmd.threshold);

    GetSendEntry* entry = new GetSendEntry;
    assert(entry);

    entry->user_ptr = cmd.user_ptr;
    entry->callback = new GetCallback(this, &Context::getCallback, entry );

    entry->local_offset = cmd.local_offset;
    entry->md_handle = cmd.md_handle;
    entry->hdr.length = cmd.length;
    entry->hdr.pt_index = cmd.pt_index;
    entry->hdr.dest_pid = cmd.target_id.phys.pid;
    entry->hdr.src_pid  = m_pid;
    entry->hdr.offset = cmd.remote_offset;
    entry->hdr.match_bits = cmd.match_bits;
    entry->hdr.op = ::Get;
    entry->destNid = cmd.target_id.phys.nid;

    int key = 5;
    entry->hdr.key = key; 
    m_getM[ key ] = entry;
    TriggeredOP* op = new TriggeredOP;
    op->u.get = entry; 
    op->count = cmd.threshold;

    m_ctV[cmd.trig_ct_handle].triggered.push_back( op );
}


bool Context::getCallback( GetSendEntry* entry )
{
    PRINT_AT(Context,"\n");
    if ( m_mdV[entry->md_handle].md.ct_handle != -1  ) {
        addCT( m_mdV[entry->md_handle].md.ct_handle, 1 );
        writeCtEvent( m_mdV[entry->md_handle].md.ct_handle, 
                    *findCTEvent( m_mdV[entry->md_handle].md.ct_handle) );
    }

    if ( m_mdV[entry->md_handle].md.eq_handle != -1  ) {
        writeReplyEvent( m_mdV[entry->md_handle].md.eq_handle,
                        entry->hdr.length,
                        entry->hdr.offset,
                        entry->user_ptr,
                        PTL_NI_OK
        );
    }

    delete entry;
    return true;
}

bool Context::putCallback( PutSendEntry* entry )
{
    PRINT_AT(Context,"state %d\n",entry->state);
    if ( entry->state == PutSendEntry::WaitSend ) {

        PRINT_AT(Context,"Send complete\n");
        if ( m_mdV[entry->md_handle].md.eq_handle != -1  ) {
            writeSendEvent( m_mdV[entry->md_handle].md.eq_handle,
                        entry->user_ptr,
                        PTL_NI_OK
            );
        }

        if ( m_mdV[entry->md_handle].md.ct_handle != -1  ) {
            addCT( m_mdV[entry->md_handle].md.ct_handle, 1 );
            writeCtEvent( m_mdV[entry->md_handle].md.ct_handle, 
                    *findCTEvent( m_mdV[entry->md_handle].md.ct_handle) );
        }

        if ( entry->hdr.ack_req == PTL_ACK_REQ )  {
            PRINT_AT(Context,"need ack\n");
            entry->state = PutSendEntry::WaitAck;
            return false;
        }
    } else {

        PRINT_AT(Context,"Got Ack\n");
        if ( m_mdV[entry->md_handle].md.eq_handle != -1  ) {
            writeAckEvent( m_mdV[entry->md_handle].md.eq_handle,
                        entry->hdr.length,
                        entry->hdr.offset,
                        entry->user_ptr,
                        PTL_NI_OK
            );
        }

        if ( m_mdV[entry->md_handle].md.ct_handle != -1  ) {
            addCT( m_mdV[entry->md_handle].md.ct_handle, 1 );
            writeCtEvent( m_mdV[entry->md_handle].md.ct_handle, 
                    *findCTEvent( m_mdV[entry->md_handle].md.ct_handle) );
        }
    }

    PRINT_AT(Context,"complete\n");
    delete entry;
    return true;
}


bool Context::eventCallback( EventEntry* entry )
{
    PRINT_AT(Context,"delete EventEntry %p\n",entry);
    delete entry;
    return true;
}

void Context::doTriggered( int ct_handle )
{
    
    CT& ct = m_ctV[ct_handle];

    std::list<TriggeredOP*>::iterator iter = ct.triggered.begin(); 

    while ( iter != ct.triggered.end() )  {
        PRINT_AT(Context,"count=%d %d\n", ct.event.success, (*iter)->count );  
        if ( ct.event.success == (*iter)->count ) {
            m_nic->sendMsg( (*iter)->u.get->destNid, 
                    &(*iter)->u.get->hdr, (Addr) 0, 0,  NULL );
        }

        ++iter;
    }

}

void Context::writeCtEvent( EventEntry* entry )
{
    PRINT_AT(Context,"ct_handle=%#x\n", entry->handle );

    doTriggered( entry->handle );

    m_nic->dmaEngine().write( findCTAddr( entry->handle ),
                                (uint8_t*) &entry->ctEvent,
                                sizeof( entry->ctEvent ),
                                entry->callback );
}

void Context::writeEvent( EventEntry* entry ) 
{
    struct EQ& eq = findEQ( entry->handle ); 

    PRINT_AT(Context,"eq_handle=%d eq.count=%d vaddr=%#lx type=%d\n",
            entry->handle, eq.count, 
            findEventAddr( entry->handle, eq.count % eq.size ),
            entry->event.user.type );

    entry->event.count1 = entry->event.count2 = eq.count; 

    m_nic->dmaEngine().write( 
                            findEventAddr( entry->handle, eq.count % eq.size ),
                            (uint8_t*) &entry->event,
                            sizeof( entry->event ),
                            entry->callback );

    ++eq.count;
}

RecvEntry* Context::processHdrPkt( void* pkt )
{
    PRINT_AT(Context,"me=%d\n",m_nic->nid());
    CtrlFlit* cFlit = (CtrlFlit*) pkt;
    RecvEntry* entry = processHdrPkt( cFlit->s.nid, (PtlHdr*) (cFlit + 1) );
}

void Context::processAck( PtlHdr* hdr )
{
    PRINT_AT(Context,"key=%d\n",hdr->key);
    
    m_putM[hdr->key]->hdr = *hdr;

    PRINT_AT(Context,"length=%lu offset=%lu\n",hdr->length, hdr->offset );
    if ( (*m_putM[hdr->key]->callback)() ) {
        delete m_putM[hdr->key]->callback;
    }
    m_putM.erase( hdr->key );
}

RecvEntry* Context::processReply( PtlHdr* hdr )
{
    PRINT_AT(Context,"key=%d\n",hdr->key);
    GetSendEntry* entry = m_getM[ hdr->key ];
    m_getM.erase( hdr->key );

    // note we are writing over the PtlGet hdr
    entry->hdr = *hdr;

    ptl_md_t* md = &m_mdV[ entry->md_handle ].md;

    // what if the requested length does not match the returned length ?
    return new RecvEntry( m_nic->dmaEngine(), 
                                    (Addr) md->start + entry->local_offset, 
                                    hdr->length, entry->callback );
}

RecvEntry* Context::processHdrPkt( ptl_nid_t nid, PtlHdr* hdr )
{
    PRINT_AT(Context,"srcNid=%d srcPid=%d targetPid=%d\n",
                            nid, hdr->src_pid, hdr->dest_pid );
    PRINT_AT(Context,"length=%lu offset=%lu\n",hdr->length, hdr->offset);
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",
                                    hdr->pt_index, hdr->match_bits );

    if ( hdr->op == Ack ) {
        processAck( hdr );
        return NULL;
    }

    if ( hdr->op == Reply ) {
        return processReply( hdr );
    }

    if ( ! m_ptV[hdr->pt_index].used ) {
        printf("Drop\n");
        return NULL;
    }

    int me_handle = search( nid, *hdr, PTL_PRIORITY_LIST );
    if ( me_handle != -1 ) {
        return processMatch( nid, hdr, me_handle, PTL_PRIORITY_LIST );
    }

    PRINT_AT(Context,"No match in PRIORITY_LIST\n");

    me_handle = search( nid, *hdr, PTL_OVERFLOW );
    if ( me_handle != -1 ) {
        return processMatch( nid, hdr, me_handle, PTL_OVERFLOW );
    }
    return NULL;
}


Context::XXX* Context::searchOverflow( ptl_me_t& me )
{
    overflowHdrList_t::iterator iter = m_overflowHdrList.begin();
    while ( iter != m_overflowHdrList.end() ) {
        PtlHdr& hdr = (*iter)->origHdr;

        PRINT_AT(Context,"srcNid=%d srcPid=%d targetPid=%d\n",
                            (*iter)->srcNid, hdr.src_pid, hdr.dest_pid );
        PRINT_AT(Context,"length=%lu offset=%lu\n",hdr.length, hdr.offset);
        PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",hdr.pt_index, hdr.match_bits );

        if ( checkME( (*iter)->srcNid, hdr, me ) ) {
            XXX* tmp = *iter;
            m_overflowHdrList.erase(iter);
            return tmp; 
        }
        ++iter;
    }
    return NULL;
}

int Context::search( ptl_nid_t nid, PtlHdr& hdr, ptl_list_t list )
{
    int me_handle = -1;
    std::list< int >::iterator iter = 
                m_ptV[hdr.pt_index].meL[list].begin();
    std::list< int >::iterator end = 
                m_ptV[hdr.pt_index].meL[list].end();

    for ( ; iter != end ; ++iter ) {
        ptl_me_t* me = findME( *iter );
        assert( me );
        if ( ! checkME( nid, hdr, *me ) ) {
            continue;
        }
        me_handle = *iter;
        break;
    }
    PRINT_AT(Context,"me_handle=%d\n",me_handle);
    return me_handle;
}


bool Context::checkME( ptl_nid_t src_nid, PtlHdr& hdr, ptl_me_t& me )
{

    ptl_match_bits_t dont_ignore_bits = ~(me.ignore_bits);
    PRINT_AT(Context, "nid=%d pid=%d\n", me.match_id.phys.nid,
                                            me.match_id.phys.pid );
    PRINT_AT(Context, "me->match_bits %#lx dont_ignore %#lx\n", 
                        me.match_bits, dont_ignore_bits  );
    /* check the match_bits */
    if ( ( (hdr.match_bits ^ me.match_bits) & dont_ignore_bits ) != 0 ) {
        return false;
    }    

    PRINT_AT(Context," matched bits \n");

    /* check for forbidden truncation */
    if (((me.options & PTL_ME_NO_TRUNCATE) != 0) &&
             ((hdr.offset + hdr.length) > me.length)) {
        return false;
    }

    PRINT_AT(Context," matched options \n");

    if (( me.match_id.phys.nid != PTL_NID_ANY ) &&
            ( me.match_id.phys.nid != src_nid ) ) {
        return false;
    } 
    PRINT_AT(Context," matched nid \n");

    if (( me.match_id.phys.pid != PTL_PID_ANY ) &&
            ( me.match_id.phys.pid != hdr.src_pid ) ) {
        return false;
    } 
    PRINT_AT(Context," matched pid \n");

    return true;
}

RecvEntry* Context::processMatch( ptl_nid_t nid, PtlHdr* hdr, int me_handle,
            ptl_list_t list )
{
    PRINT_AT(Context,"found match me=%d list=%d\n", me_handle, list );
    if ( hdr->op == ::Get ) {
        processGet( nid, hdr, me_handle, list );
    } else {
        processPut( nid, hdr, me_handle, list );
    }
}

RecvEntry* Context::processGet( ptl_nid_t nid, PtlHdr* hdr, int me_handle,
                            ptl_list_t list )
{
    PRINT_AT(Context,"\n");

    GetRecvEntry* entry = new GetRecvEntry; 
    entry->op          = ::Get;
    entry->list        = list;
    entry->srcNid      = nid;
    entry->me_handle   = me_handle;
    entry->origHdr     = *hdr;
    entry->replyHdr    = *hdr;
    entry->replyHdr.op = Reply;
    
    entry->callback = 
                new GetRecvCallback( this, &Context::getRecvCallback,entry );

    // this needs to be fixed
    entry->mlength = hdr->length;
    entry->start   = (void*) ((Addr) m_meV[me_handle].me.start + hdr->offset);

    m_nic->sendMsg( nid, &entry->replyHdr, (Addr) entry->start,
                    entry->mlength, entry->callback );  
    return NULL;
}

bool Context::getRecvCallback( GetRecvEntry* entry )
{
    PRINT_AT(Context,"\n");
    recvFini( entry );
    delete entry;
    return true;
}

RecvEntry* Context::processPut( ptl_nid_t nid, PtlHdr* hdr, int me_handle,
                                    ptl_list_t list )
{
    PRINT_AT(Context,"\n");
    
    PutRecvEntry* entry = new PutRecvEntry;
    entry->op        = ::Put;
    entry->list      = list;
    entry->srcNid    = nid;
    entry->me_handle = me_handle;
    entry->origHdr   = *hdr;
    entry->unlink    = false;
    entry->ackHdr    = *hdr;
    entry->ackHdr.op = Ack;

    entry->state = PutRecvEntry::WaitRecvComp;

    entry->callback = 
                new PutRecvCallback( this, &Context::putRecvCallback,entry );

    if ( list == PTL_OVERFLOW ) {
        m_overflowHdrList.push_back( entry );
    }

    ME& me = m_meV[me_handle];

    ptl_size_t offset;
    entry->mlength = hdr->length;

    if ( me.me.options & PTL_ME_MANAGE_LOCAL ) {
        offset = me.offset;
        me.offset += entry->mlength;
    } else {
        offset = hdr->offset;
    }

    if ( ! ( me.me.options & PTL_ME_NO_TRUNCATE ) ) {
        if ( entry->mlength > me.me.length - offset ) {
            entry->mlength = me.me.length - offset;  
        }
    }

    if( me.me.options & PTL_ME_USE_ONCE || 
                        ( me.me.length - me.offset ) < me.me.min_free ) {
        PRINT_AT(Context,"unlink me_handle %d\n",me_handle);
        assert( m_meV[me_handle].used );
        m_ptV[hdr->pt_index].meL[list].remove(me_handle);
        m_meV[me_handle].used = false;

        m_meUnlinked[m_meUnlinkedPos] = me_handle;

        m_nic->dmaEngine().write( (Addr) (m_meUnlinkedHostPtr + m_meUnlinkedPos), 
                        (uint8_t*) &m_meUnlinked[m_meUnlinkedPos], 
                        sizeof( int ), NULL );

        m_meUnlinkedPos = (m_meUnlinkedPos + 1) % ME_UNLINKED_SIZE;

        // this is messy, we need to remove from our table and the host 
        // needs to free the handle
        entry->unlink = true;

        if ( ! ( me.me.options & PTL_ME_EVENT_UNLINK_DISABLE ) ) {
            // not supported yet
            // write PTL_EVENT_AUTO_UNLINK
            assert(0);
        }
    }

    entry->start   = (void*) ((Addr) m_meV[me_handle].me.start + offset);

    entry->ackHdr.offset = offset;
    entry->ackHdr.length = entry->mlength;

    PRINT_AT(Context,"hdr->length=%lu me.length=%lu mlength=%lu\n",
                                  hdr->length, me.me.length,entry->mlength);
    if ( entry->mlength ) {

        return new RecvEntry( m_nic->dmaEngine(), (Addr) entry->start, 
                                entry->mlength, entry->callback );
    } else {
        (*entry->callback)();
    }
    return NULL;
}

bool Context::putRecvCallback( PutRecvEntry* entry )
{
    PRINT_AT(Context,"\n");

    if ( entry->state == PutRecvEntry::WaitRecvComp ) {

        recvFini( entry );

        if ( entry->origHdr.ack_req == PTL_ACK_REQ && 
                ! (findME(entry->me_handle)->options & PTL_ME_ACK_DISABLE ) ) {
            entry->ackHdr.op = Ack;
            entry->ackHdr.dest_pid = entry->origHdr.src_pid;
            entry->ackHdr.src_pid = m_pid;
            PRINT_AT(Context,"send ack\n");
            PRINT_AT(Context,"mlength=%lu remote_offset=%lu\n",
                                entry->ackHdr.length, entry->ackHdr.offset);
            m_nic->sendMsg( entry->srcNid, &entry->ackHdr, 
                                                0, 0, entry->callback );  
            entry->state = PutRecvEntry::WaitAckSent;
            return false;
        }
    }

    if ( entry->list == PTL_PRIORITY_LIST ) {
        delete entry;
    } else {
        PRINT_AT(Context,"finished PTL_OVERFLOW data movement\n");
        entry->callback = NULL;
    } 

    return true;
}
void Context::recvFini( XXX* entry )
{
    PtlHdr& hdr   = entry->origHdr;
    ME& me        = m_meV[ entry->me_handle ];
    int eq_handle = m_ptV[ hdr.pt_index ].eq_handle;
    int ct_handle = findME( entry->me_handle )->ct_handle;

    PRINT_AT( Context, "pt_index=%d\n", hdr.pt_index );

    if ( ct_handle != -1 ) {
        addCT( ct_handle, 1 );
        writeCtEvent( ct_handle, *findCTEvent( ct_handle) );
    }

    if ( eq_handle != -1  && 
                ! (me.me.options & PTL_ME_EVENT_COMM_DISABLE ) ) {
        ptl_process_t initiator;
        initiator.phys.nid = entry->srcNid; 
        initiator.phys.pid = hdr.src_pid;

        writeEvent( eq_handle,
            entry->op == ::Get ? PTL_EVENT_GET : PTL_EVENT_PUT,
            initiator,
            hdr.pt_index,
            hdr.uid,
            hdr.jid,
            hdr.match_bits,
            hdr.length,
            entry->mlength,
            hdr.offset,
            entry->start,
            me.user_ptr,
            hdr.hdr_data,
            PTL_NI_OK
        );
    }
#if 0
    if ( eq_handle != -1  &&  entry->unlink &&
                ! (me.me.options & PTL_ME_EVENT_UNLINK_DISABLE ) ) {
        // not supported yet
        // write PTL_EVENT_AUTO_FREE
        assert(false);
    }
#endif
}
