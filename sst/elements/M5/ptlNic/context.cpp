#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "context.h"
#include "ptlNic.h"
#include "callback.h"
#include "ptlHdr.h"
#include "recvEntry.h"

Context::Context( PtlNic* nic ) :
        m_nic( nic ) 
{
    TRACE_ADD( Context );
    PRINT_AT(Context,"\n");
    m_limits.max_pt_index = 63;
    m_limits.max_cts = 63;
    m_limits.max_mds = 63;
    m_limits.max_entries = 63;
    m_limits.max_eqs = 63;
    m_limits.max_list_size = 63;
    m_ptV.resize( m_limits.max_pt_index );
    m_ctV.resize( m_limits.max_cts );
    m_mdV.resize( m_limits.max_mds );
    m_meV.resize( m_limits.max_entries );
    m_eqV.resize( m_limits.max_eqs );
    for ( int i = 0; i < m_ptV.size(); i++ ) {
        m_ptV[i].used = false;
    }
    for ( int i = 0; i < m_ctV.size(); i++ ) {
        m_ctV[i].avail = true;
    }
    for ( int i = 0; i < m_eqV.size(); i++ ) {
        m_eqV[i].avail = true;
    }
    for ( int i = 0; i < m_meV.size(); i++ ) {
        m_meV[i].avail = true;
    }
    for ( int i = 0; i < m_mdV.size(); i++ ) {
        m_mdV[i].avail = true;
    }
}

Context::~Context() {
    PRINT_AT(Context,"\n");
}

void Context::initPid( ptl_pid_t pid ) {
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

int Context::allocMD() {
    for ( int i = 0; i < m_mdV.size(); i++ ) {
        if ( m_mdV[i].avail ) {
            m_mdV[i].avail = false;
            PRINT_AT(Context,"%d\n",i);
            return i;
        }       
    }
    return -PTL_FAIL;
}

int Context::freeMD( int handle ) {
    PRINT_AT(Context,"%d\n",handle);
    m_mdV[handle].avail = true;
    return PTL_OK;
}

ptl_md_t* Context::findMD( int handle ) {
    return &m_mdV[handle].md;
}

int Context::appendPT( ptl_pt_index_t pt_index, ptl_list_t list, int me_handle )
{
    PRINT_AT(Context,"pt_index=%d list=%d handle=%d\n",pt_index, list, me_handle );
    if ( m_ptV[pt_index].meL[list].size() >= m_limits.max_list_size ) {
        return -PTL_LIST_TOO_LONG;
    }
    m_ptV[pt_index].meL[list].push_back( me_handle );
    return PTL_OK;
}

int Context::allocME( ptl_pt_index_t pt_index, ptl_list_t list, void* userPtr )
{
    if ( ! isvalidPT( pt_index ) ) return -PTL_ARG_INVALID; 

    for ( int i = 0; i < m_meV.size(); i++ ) {
        if ( m_meV[i].avail ) {
            int retval; 
            if ( ( retval = appendPT( pt_index, list, i ) ) < 0 )  {
                return retval;
            } 
            m_meV[i].avail = false;
            m_meV[i].userPtr = userPtr; 
            return i;
        }       
    }
    return -PTL_FAIL;
}

ptl_me_t* Context::findME( int handle ) {
    return &m_meV[handle].me;
}

int Context::freeME( int handle ) {
    m_meV[handle].avail = true;
    return PTL_OK;
}


int Context::allocCT( Addr eventAddr ) {
    for ( int i = 0; i <= m_ctV.size(); i++ ) {
        if ( m_ctV[i].avail ) {
            m_ctV[i].avail = false;
            m_ctV[i].vaddr = eventAddr;
            m_ctV[i].event.success = 0;
            m_ctV[i].event.failure = 0;
            return i;
        }
    }
}

int Context::addCT( int handle, ptl_size_t value ) {
    m_ctV[handle].event.success += value;
}

ptl_ct_event_t* Context::findCTEvent( int handle ){
    return &m_ctV[handle].event;
}

Addr Context::findCTAddr( int handle ) {
    return m_ctV[handle].vaddr;
}

int Context::freeCT( int handle ) {
    m_ctV[handle].avail = true;
    return PTL_OK;
}

int Context::allocEQ( Addr vaddr, int count ) {
    for ( int i = 0; i <= m_eqV.size(); i++ ) {
        if ( m_eqV[i].avail  == true) {
            m_eqV[i].avail = false;
            m_eqV[i].vaddr = vaddr;
            m_eqV[i].count = 0;
            m_eqV[i].size = count;
            return i;
        }
    }
    return -PTL_FAIL;
}

int Context::freeEQ( int handle ) {
    m_eqV[handle].avail = true;
    return PTL_OK;
}

struct Context::EQ& Context::findEQ( int handle )
{
    return m_eqV[handle];
}

PtlEventInternal* Context::findEvent( int handle )
{
    return &m_eqV[handle].event;
}

Addr Context::findEventAddr( int handle ) 
{
    struct EQ& eq = m_eqV[handle]; 
    return eq.vaddr + ( eq.count * sizeof( eq.event ) ); 
}

int Context::allocPT( unsigned int options, int eq_handle, 
                    ptl_pt_index_t req_pt ) 
{
    if ( req_pt == PTL_PT_ANY ) {
        req_pt = -PTL_PT_FULL;
        for ( int i = 0; i < m_limits.max_pt_index; i++ ) {
            if ( ! m_ptV[i].used ) {
                req_pt = i;
                break;
            }
        } 
    } else if ( req_pt <= m_limits.max_pt_index ) {
        if ( m_ptV[req_pt].used ) {
            req_pt -PTL_PT_IN_USE;
        }
    } else {
        req_pt -PTL_ARG_INVALID;
    }

    int tmp = req_pt;
    if ( tmp >= 0 ) {
        PRINT_AT(Context,"pt_index=%d eq_handle=%#x\n",req_pt,eq_handle);
        m_ptV[req_pt].used = true; 
        m_ptV[req_pt].eq_handle = eq_handle;
        m_ptV[req_pt].options = options;
    }
    return req_pt;
}

int Context::freePT( int pt_index ) {
    int retval; 
    if ( pt_index > m_limits.max_pt_index ) {
        retval -PTL_ARG_INVALID;
    } else if ( ! m_ptV[pt_index].used )  {
        retval -PTL_ARG_INVALID;
    } else if ( ! m_ptV[pt_index].meL[PTL_PRIORITY_LIST].empty() ) {
        retval -PTL_PT_IN_USE;
    } else if ( ! m_ptV[pt_index].meL[PTL_OVERFLOW].empty() ) {
        retval -PTL_PT_IN_USE;
    } else {
        m_ptV[pt_index].used = false;
        retval = PTL_OK;
    }
    return retval;
}

ptl_ni_limits_t* Context::limits() {
    return &m_limits;
}

ptl_process_t* Context::id() {
    return &m_id;
}

int Context::put( int md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_ack_req_t    ack_req,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr,
           ptl_hdr_data_t   hdr_data)
{
    PRINT_AT(Context,"md_handle=%d length=%lu local_offset=%#lx remote_offset=%#lx\n",
                            md_handle, length, local_offset, remote_offset );
    if ( m_logicalIF ) {
        PRINT_AT(Context,"target rank=%d\n",target_id.rank);
    } else {
        PRINT_AT(Context,"target nid=%d pid=%d\n",
                    target_id.phys.nid,target_id.phys.pid);
    }
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",pt_index,match_bits); 

    if ( pt_index > m_limits.max_pt_index ) {
        return -PTL_ARG_INVALID;
    }

    PutEntry* entry = new PutEntry;
    assert(entry);

    entry->user_ptr = user_ptr;
    entry->state = PutEntry::WaitPut;
    entry->callback = 
         new PutCallback(this, &Context::putCallback, entry );

    entry->md_handle = md_handle;
    entry->hdr.length = length;
    entry->hdr.ack_req = ack_req;
    entry->hdr.pt_index = pt_index;
    entry->hdr.dest_pid = target_id.phys.pid;
    entry->hdr.src_pid  = m_pid;
    entry->hdr.offset = remote_offset;
    entry->hdr.match_bits = match_bits;
    entry->hdr.hdr_data = hdr_data;

    m_nic->sendMsg( target_id.phys.nid, &entry->hdr, 
                    (Addr) m_mdV[md_handle].md.start + local_offset, 
                    length, entry->callback );  
    return PTL_OK;
}

bool Context::putCallback( PutEntry* entry )
{
    switch ( entry->state ) {
      case  PutEntry::WaitPut:
        PRINT_AT(Context,"put Done\n");
        if ( m_mdV[entry->md_handle].md.ct_handle != -1  ) {
            PRINT_AT(Context,"write CT\n");
            addCT( m_mdV[entry->md_handle].md.ct_handle, 1 );
            writeCtEvent( m_mdV[entry->md_handle].md.ct_handle, 
                                                    entry->callback );
            entry->state = PutEntry::WaitCtEvent;
            break;
        }

      case PutEntry::WaitCtEvent:
            PRINT_AT(Context,"CT Done\n");
            entry->state = PutEntry::Done;
    }

    if ( entry->state == PutEntry::Done ) {
        PRINT_AT(Context,"complete\n");
        delete entry;
        return true;
    } else {
        return false;
    }
}

void Context::writeCtEvent( int ct_handle, PutCallback* callback ) 
{
        ptl_ct_event_t* event = findCTEvent( ct_handle );
        m_nic->dmaEngine().write(findCTAddr( ct_handle ),
                                (uint8_t*) event,
                                sizeof( *event ),
                                callback );
}

bool Context::eventCallback( EventEntry* entry )
{
    PRINT_AT(Context,"delete EventEntry %p\n",entry);
    delete entry;
    return true;
}

void Context::writeEvent( int eq_handle, EventCallback* callback ) 
{
    PRINT_AT(Context,"eq_handle=%#x\n",eq_handle);
    PtlEventInternal* event = findEvent( eq_handle );
    m_nic->dmaEngine().write( findEventAddr( eq_handle ),
                                (uint8_t*) event,
                                sizeof( *event ),
                                callback );
}

RecvEntry* Context::processHdrPkt( void* pkt )
{
    CtrlFlit* cFlit = (CtrlFlit*) pkt;
    RecvEntry* entry = processHdrPkt( cFlit->s.nid, (PtlHdr*) (cFlit + 1) );
}

RecvEntry* Context::processHdrPkt( ptl_nid_t nid, PtlHdr* hdr )
{
    PRINT_AT(Context,"srcNid=%d srcPid=%d targetPid=%d\n",
                            nid, hdr->src_pid, hdr->dest_pid );
    PRINT_AT(Context,"length=%lu offset=%#lx\n",hdr->length, hdr->offset);
    PRINT_AT(Context,"pt_index=%d match_bits=%#lx\n",hdr->pt_index, hdr->match_bits );

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
        if ( ( (hdr->match_bits ^ me->match_bits) & dont_ignore_bits ) != 0 ) {
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
    PRINT_AT(Context,"\n");
    if ( hdr->length == 0 ) {
        recvFini( nid, hdr, me_handle );
        return NULL;
    } else {
        RecvCBEntry* entry = new RecvCBEntry; 
        entry->nid = nid;
        entry->hdr = *hdr;
        entry->me_handle = me_handle;
        entry->callback = 
                new RecvCallback( this, &Context::recvCallback,entry );
        ptl_me_t* me = &m_meV[me_handle].me;
 
        return new RecvEntry( m_nic->dmaEngine(), 
                                    (Addr) me->start + hdr->offset, 
                                    me->length, entry->callback );
    }
}

bool Context::recvCallback( RecvCBEntry* entry )
{
    PRINT_AT(Context,"\n");
    recvFini( entry->nid, &entry->hdr, entry->me_handle );
    delete entry;
    return true;
}
void Context::recvFini( ptl_nid_t nid, PtlHdr* hdr, int me_handle )
{
    PRINT_AT(Context,"pt_index=%d\n",hdr->pt_index);
    int eq_handle = m_ptV[hdr->pt_index].eq_handle;
    if ( eq_handle != -1 ) {
        EventEntry* entry = new EventEntry;
        assert(entry);
        PRINT_AT(Context,"eq_handle=%#x\n",eq_handle);

        entry->callback = 
            new EventCallback(this, &Context::eventCallback, entry );

        struct EQ& eq = findEQ( eq_handle ); 
        ptl_size_t count = (eq.count + 1) % eq.size;

        PRINT_AT(Context,"eq.count=%d\n",count);
        eq.event.count1 = count; 
        eq.event.count2 = count; 
        
        writeEvent( eq_handle, entry->callback );

        eq.count = count;
    }
}
