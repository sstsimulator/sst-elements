#ifndef _ptlAPI_h
#define _ptlAPI_h

#include <map>
#include <vector>

#include <ptlEvent.h>

namespace PtlNic {

class PtlAPI {

    struct EventQ {
        long count; 
        long size; 
        struct PtlEventInternal* eventV;     
    };

  public:
    PtlAPI( PtlIF& ptlIF ) :
        m_ptlIF( ptlIF ) 
    {
        PTL_DBG2("\n");
        assert( sizeof(ptl_size_t) == sizeof(long ) );
    }

    ~PtlAPI() {
        PTL_DBG2("\n");
    } 

    int ptlNIInit( 
              unsigned int      options,
              ptl_pid_t         pid,
              ptl_ni_limits_t   *desired,
              ptl_ni_limits_t   *actual,
              ptl_size_t        map_size,
              ptl_process_t     *desired_mapping,
              ptl_process_t     *actual_mapping
    )
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlNIInit, 7, options, pid, desired,
                        actual, map_size, desired_mapping, actual_mapping ); 
    }

    int ptlNIFini( )
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlNIFini, 0 );
    }

    int ptlPTAlloc( unsigned int    options,
                 ptl_handle_eq_t eq_handle,
                 ptl_pt_index_t  pt_index_req )
    {                                     
        PTL_DBG2("\n");
        return  m_ptlIF.push_cmd( PtlPTAlloc, 3, options, 
                        eq_handle, pt_index_req ); 
    }

    int ptlPTFree( ptl_pt_index_t pt_index ) 
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlPTFree, 1, pt_index ); 
    }

    int ptlMDBind( ptl_md_t* md )
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlMDBind, 1, md ); 
    }

    int ptlMDRelease( ptl_handle_md_t md_handle )
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlMDRelease, 1, md_handle ); 
    }

    int ptlMEAppend( ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void*               user_ptr ) 
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlMEAppend, 4, pt_index,me,ptl_list,user_ptr);
    }

    int ptlMEUnlink(ptl_handle_me_t me_handle)
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlMDRelease, 1, me_handle ); 
    }


    int ptlGetId( ptl_process_t*     id)
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlGetId, 1, id ); 
    }

    int ptlCTAlloc( )
    {

        ptl_ct_event_t* event = new ptl_ct_event_t;
        if ( !event ) return -PTL_NO_SPACE;
        event->success = event->failure = 0;
        PTL_DBG2("\n");

        int handle = m_ptlIF.push_cmd( PtlCTAlloc, 1, event ); 
        
        if ( handle <  0 ) {
            delete event;
            return handle;
        }
        m_ctM[handle] = event;
        
        return handle;
    }

    int ptlCTFree( ptl_handle_ct_t ct )
    {
        PTL_DBG2("\n");
        int retval = m_ptlIF.push_cmd( PtlCTFree, 1, ct ); 
        if ( retval == PTL_OK ) {
            delete m_ctM[ct];
        }
        return retval;
    }

    int ptlCTWait(ptl_handle_ct_t   ct_handle,
                    ptl_size_t        test,
                    ptl_ct_event_t *  event)
    {
        PTL_DBG2("\n");
        while ( 1 ) {
            *event = *m_ctM[ct_handle];
            if ( event->failure || event->success >= test ) {
                return PTL_OK;
            }
        }
    }

    int ptlEQAlloc( ptl_size_t size )
    {
        EventQ* eventQ = new EventQ;
        if ( !eventQ ) return -PTL_NO_SPACE;

        eventQ->size = size; 
        eventQ->count = 0; 
        eventQ->eventV = new PtlEventInternal[ size ];

        for ( int i = 0; i < (int) size; i++ ) {
            eventQ->eventV[i].count1 = eventQ->eventV[i].count2 = -1;
            // we don't write the user part of the event and it's never marked
            // dirty by the M5 Cache, when a write comes from the dma engine
            // it doesn't make it into the cache, is this correct behaviour?
            // touch the event makes things work
            memset( &eventQ->eventV[i].user,0,sizeof(eventQ->eventV[i].user));
        }

        PTL_DBG2("%p\n",&eventQ->eventV[0]);
        int handle =  m_ptlIF.push_cmd( PtlEQAlloc, 2, size,
                                        &eventQ->eventV[0] ); 
        if ( handle < 0 ) {
            delete eventQ;
            return handle;
        }
        PTL_DBG2("eq_handle=%d\n",handle);
        m_eqM[handle] = eventQ;

        return handle;
    }

    int ptlEQFree( ptl_handle_eq_t eq_handle )
    {
        int retval = m_ptlIF.push_cmd( PtlEQFree, 1, eq_handle ); 
        PTL_DBG2("\n");
        if ( retval == PTL_OK ) {
            delete m_eqM[eq_handle];
        }
        return retval;
    }

    int ptlEQWait(ptl_handle_ct_t eq_handle, ptl_event_t* event )
    {
        EventQ*    foo = m_eqM[ eq_handle ];
        ptl_size_t pos = foo->count % foo->size;

        PTL_DBG2( "eq_handle=%d count=%lu\n", eq_handle, foo->count );

        while ( foo->eventV[ pos ].count1 < foo->count );
        while ( foo->eventV[ pos ].count2 < foo->count );

        *event = foo->eventV[ pos ].user;
        
        if ( foo->eventV[ pos ].count2 != foo->count ) {
            return -PTL_EQ_DROPPED;
        }

        ++foo->count;

        return PTL_OK;
    }

    int ptlPut(ptl_handle_md_t  md_handle,
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
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlPut, 10, 
                    md_handle,
                    local_offset,
                    length,
                    ack_req,
                    target_id,
                    pt_index,
                    match_bits,
                    remote_offset,
                    user_ptr,
                    hdr_data ); 
    }

    int ptlGet(ptl_handle_md_t  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr)
    {
        PTL_DBG2("\n");
        return m_ptlIF.push_cmd( PtlGet, 8, 
                    md_handle,
                    local_offset,
                    length,
                    target_id,
                    pt_index,
                    match_bits,
                    remote_offset,
                    user_ptr);
    }

  private:
    PtlIF& m_ptlIF;
    std::map<int,ptl_ct_event_t*>  m_ctM;
    std::map<int,EventQ*>     m_eqM;
};

}

#endif
