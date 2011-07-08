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
        m_jid( cmd.jid ) 
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
    m_ptV[cmd.pt_index].meL[cmd.list].push_back( cmd.handle );
    m_meV[cmd.handle].user_ptr = cmd.user_ptr; 
    m_meV[cmd.handle].me = cmd.me;
    m_meV[cmd.handle].offset = 0;
}

void Context::MEUnlink( cmdPtlMEUnlink_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n",cmd.handle);
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
    PRINT_AT(Context,"handle=%d addr=%#lx\n", cmd.handle );
    m_mdV[cmd.handle].md = cmd.md;
}

void Context::MDRelease( cmdPtlMDRelease_t& cmd )
{
    PRINT_AT(Context,"handle=%d\n", cmd.handle );
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

void Context::writeCtEvent( EventEntry* entry )
{
    PRINT_AT(Context,"ct_handle=%#x\n", entry->handle );

    m_nic->dmaEngine().write( findCTAddr( entry->handle ),
                                (uint8_t*) &entry->ctEvent,
                                sizeof( entry->ctEvent ),
                                entry->callback );
}

void Context::writeEvent( EventEntry* entry ) 
{
    struct EQ& eq = findEQ( entry->handle ); 

    PRINT_AT(Context,"eq_handle=%d eq.count=%d vaddr=%#lx\n", 
            entry->handle, eq.count, 
            findEventAddr( entry->handle, eq.count % eq.size ) );

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
    PRINT_AT(Context,"%p %lu \n",this,m_ptV.size());
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
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",hdr->pt_index, hdr->match_bits );

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

    std::list< int >::iterator iter = 
                m_ptV[hdr->pt_index].meL[PTL_PRIORITY_LIST].begin();
    std::list< int >::iterator end = 
                m_ptV[hdr->pt_index].meL[PTL_PRIORITY_LIST].end();

    for ( ; iter != end ; ++iter ) {
        ptl_me_t* me = findME( *iter );
        assert( me );
        ptl_match_bits_t dont_ignore_bits = ~(me->ignore_bits);
        PRINT_AT(Context, "me->match_bits %#lx dont_ignore %#lx\n", 
                        me->match_bits, dont_ignore_bits  );
        /* check the match_bits */
        if ( ( (hdr->match_bits ^ me->match_bits) & dont_ignore_bits ) != 0 ) {
             continue;
        }    
        /* check for forbidden truncation */
        if (((me->options & PTL_ME_NO_TRUNCATE) != 0) &&
                 ((hdr->offset + hdr->length) > me->length)) {
            continue; 
        }

        if (( me->match_id.phys.nid != PTL_NID_ANY ) &&
            ( me->match_id.phys.nid != 0 ) ) {
            continue;
        } 
        if (( me->match_id.phys.pid != PTL_PID_ANY ) &&
            ( me->match_id.phys.pid != hdr->src_pid ) ) {
            continue;
        } 
        break;
    }
    if ( iter == end ) {
        printf("No Match\n");
    } else {
        return processMatch( nid, hdr, *iter );
    }
    return NULL;
}

RecvEntry* Context::processMatch( ptl_nid_t nid, PtlHdr* hdr, int me_handle )
{
    if ( hdr->op == ::Get ) {
        processGet( nid, hdr, me_handle );
    } else {
        processPut( nid, hdr, me_handle );
    }
}

RecvEntry* Context::processGet( ptl_nid_t nid, PtlHdr* hdr, int me_handle )
{
    PRINT_AT(Context,"\n");

    GetRecvEntry* entry = new GetRecvEntry; 
    entry->hdr = *hdr;
    entry->nid = nid;
    entry->me_handle = me_handle;
    entry->callback = 
                new GetRecvCallback( this, &Context::getRecvCallback,entry );
    entry->hdr.op = Reply;
    
    m_nic->sendMsg( nid, &entry->hdr, 
                    (Addr) m_meV[me_handle].me.start + entry->hdr.offset, 
                    entry->hdr.length, entry->callback );  
    return NULL;
}

bool Context::getRecvCallback( GetRecvEntry* entry )
{
    PRINT_AT(Context,"\n");
    recvFini( entry->nid, &entry->hdr, entry->me_handle );
    delete entry;
    return true;
}

RecvEntry* Context::processPut( ptl_nid_t nid, PtlHdr* hdr, int me_handle )
{
    PRINT_AT(Context,"\n");
    if ( hdr->length == 0 ) {
        recvFini( nid, hdr, me_handle );
        if ( ! (findME(me_handle)->options & PTL_ME_ACK_DISABLE ) ) {
            // need to add ACK for zero length message
            assert( hdr->ack_req != PTL_ACK_REQ );
        }
        return NULL;
    } else {
        PutRecvEntry* entry = new PutRecvEntry; 
        entry->nid = nid;
        entry->hdr = *hdr;
        entry->me_handle = me_handle;
        entry->state = PutRecvEntry::WaitRecvComp;
        entry->callback = 
                new PutRecvCallback( this, &Context::putRecvCallback,entry );
        ME& me = m_meV[me_handle];

        ptl_size_t length = hdr->length;
        ptl_size_t offset;
        Addr start = (Addr) me.me.start;

        if ( me.me.options & PTL_ME_MANAGE_LOCAL ) {
            offset = me.offset;
            me.offset += length;
        } else {
            offset = hdr->offset;
        }
        entry->hdr.offset = offset;
        entry->hdr.length = length;

        start += offset;

        return new RecvEntry( m_nic->dmaEngine(), 
                                    start, length, entry->callback );
    }
}

bool Context::putRecvCallback( PutRecvEntry* entry )
{
    PRINT_AT(Context,"\n");

    if ( entry->state == PutRecvEntry::WaitRecvComp ) {
        recvFini( entry->nid, &entry->hdr, entry->me_handle );

        if ( entry->hdr.ack_req == PTL_ACK_REQ && 
                ! (findME(entry->me_handle)->options & PTL_ME_ACK_DISABLE ) ) {
            entry->hdr.op = Ack;
            entry->hdr.dest_pid = entry->hdr.src_pid;
            entry->hdr.src_pid = m_pid;
            PRINT_AT(Context,"send ack\n");
            PRINT_AT(Context,"mlength=%lu remote_offset=%lu\n",
                                    entry->hdr.length,entry->hdr.offset);
            m_nic->sendMsg( entry->nid, &entry->hdr, 0, 0, entry->callback );  
            entry->state = PutRecvEntry::WaitAckSent;
            return false;
        }
    }

    delete entry;
    return true;
}
void Context::recvFini( ptl_nid_t nid, PtlHdr* hdr, int me_handle )
{
    PRINT_AT( Context, "pt_index=%d\n", hdr->pt_index );
    int eq_handle = m_ptV[ hdr->pt_index ].eq_handle;

    ME& me = m_meV[ me_handle ];
    ptl_size_t mlength = hdr->length;
    void* start = (unsigned char*)me.me.start + hdr->offset;
    ptl_size_t offset = hdr->offset;

    int ct_handle = findME(me_handle)->ct_handle;
    if ( ct_handle != -1 ) {
        addCT( ct_handle, 1 );
        writeCtEvent( ct_handle, *findCTEvent( ct_handle) );
    }

    if ( eq_handle != -1 ) {
        ptl_process_t initiator;
        initiator.phys.nid = nid; 
        initiator.phys.pid = hdr->src_pid;

        writeEvent( eq_handle,
            hdr->op == Reply ? PTL_EVENT_GET : PTL_EVENT_PUT,
            initiator,
            hdr->pt_index,
            hdr->uid,
            hdr->jid,
            hdr->match_bits,
            hdr->length,
            mlength,
            offset,
            start,
            me.user_ptr,
            hdr->hdr_data,
            PTL_NI_OK
        );
    }
}
