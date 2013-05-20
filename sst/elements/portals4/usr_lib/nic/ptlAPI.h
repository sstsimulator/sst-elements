// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ptlAPI_h
#define _ptlAPI_h

#include <map>
#include <vector>
#include <deque>

#include <stddef.h>
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
        m_id.phys.nid = ptlIF.nid();

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
              ptl_ni_limits_t   *actual
    )
    {
        PTL_DBG2("requested pid %d\n",pid);
        int handle = 1; 

        assert( pid == PTL_PID_ANY );
        // this has to match what's in cnos_get_nidpid_map
#if USE_PALACIOS_DEV
        m_id.phys.pid = 100; 
#else
        m_id.phys.pid = getpid(); 
#endif
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot(PtlNIInitCmd );
        cmd.niInit.pid     = m_id.phys.pid; 
        cmd.niInit.options = options;
        m_ptlIF.commitCmd();

        initLimits( m_ptlIF.limits() );
        if ( actual ) {
            *actual = m_ptlIF.limits();
        }
        return handle;
    }

    int ptlNIFini( )
    {
        PTL_DBG2("\n");
        m_ptlIF.getCmdSlot( PtlNIFiniCmd );
        m_ptlIF.commitCmd();
        return PTL_OK;
    }

    int ptlPTAlloc( unsigned int    options,
                 ptl_handle_eq_t eq_handle,
                 ptl_pt_index_t  pt_index_req )
    {                                     
        PTL_DBG2("\n");
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlPTAllocCmd );
        cmd.ptAlloc.options   = options;
        cmd.ptAlloc.eq_handle = eq_handle;
        cmd.ptAlloc.pt_index  = pt_index_req;
        m_ptlIF.commitCmd();
        return pt_index_req;
    }

    int ptlPTFree( ptl_pt_index_t pt_index ) 
    {
        PTL_DBG2("\n");
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlPTFreeCmd );
        cmd.ptAlloc.pt_index = pt_index;
        m_ptlIF.commitCmd();
        return PTL_OK;
    }

    int ptlMDBind( ptl_md_t* md )
    {
        if( m_freeMDHandles.empty() ) return -PTL_FAIL; 
        int handle = m_freeMDHandles.front();
        m_freeMDHandles.pop_front();

        PTL_DBG2("handle=%d\n",handle);
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlMDBindCmd );
        cmd.mdBind.handle = handle;
        cmd.mdBind.md = *md;
        m_ptlIF.commitCmd();
        return handle;
    }

    int ptlMDRelease( ptl_handle_md_t md_handle )
    {
        PTL_DBG2("\n");

        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlMDReleaseCmd );
        cmd.mdRelease.handle = md_handle;
        m_ptlIF.commitCmd();

        m_freeMDHandles.push_back( md_handle );
        return PTL_OK;
    }

    int ptlMEAppend( ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void*               user_ptr ) 
    {
        PTL_DBG2("\n");
        if( m_freeMEHandles.empty() ) return -PTL_FAIL; 
        int handle = m_freeMEHandles.front();
        m_freeMEHandles.pop_front();
        PTL_DBG2("%#lx\n",0xdeadbeefL);

        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlMEAppendCmd );
        cmd.meAppend.handle   = handle;
        cmd.meAppend.pt_index = pt_index;
        cmd.meAppend.me       = *me;
        PTL_DBG2("%#lx\n",(unsigned long) me->match_bits);
        cmd.meAppend.list     = ptl_list;
        cmd.meAppend.user_ptr = user_ptr;
        m_ptlIF.commitCmd();
        return handle;
    }

    int ptlMEUnlink(ptl_handle_me_t me_handle)
    {
        PTL_DBG2("\n");
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlMEUnlinkCmd );
        cmd.meUnlink.handle = me_handle;
        m_ptlIF.commitCmd();
        m_freeMEHandles.push_back(me_handle);
        return PTL_OK;
    }


    int ptlGetId( ptl_process_t*     id)
    {
        PTL_DBG2("\n");
        *id = m_id;
        return PTL_OK;
    }

    int ptlCTAlloc( )
    {
        PTL_DBG2("\n");

        if( m_freeCTHandles.empty() ) return -PTL_FAIL; 
        int handle = m_freeCTHandles.front();
        m_freeCTHandles.pop_front();

        ptl_ct_event_t* event = new ptl_ct_event_t;
        if ( !event ) return -PTL_NO_SPACE;
        event->success = event->failure = 0;
        m_ctM[handle] = event;

        cmdUnion_t& cmd  = m_ptlIF.getCmdSlot( PtlCTAllocCmd );

        cmd.ctAlloc.handle = handle;
        cmd.ctAlloc.addr   = event;
        
        m_ptlIF.commitCmd();
        
        return handle;
    }

    int ptlCTFree( ptl_handle_ct_t ct )
    {
        PTL_DBG2("\n");
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlCTFreeCmd ); 

        cmd.ctFree.handle = ct;

        m_ptlIF.commitCmd();

        delete m_ctM[ct];
        m_freeCTHandles.push_back( ct );

        return PTL_OK;
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
        if( m_freeEQHandles.empty() ) return -PTL_FAIL; 
        int handle = m_freeEQHandles.front();
        m_freeEQHandles.pop_front();

        EventQ* eventQ = new EventQ;
        if ( !eventQ ) return -PTL_NO_SPACE;

        eventQ->size = size; 
        eventQ->count = 0; 
        eventQ->eventV = new PtlEventInternal[ size ];

        for ( int i = 0; i < (int) size; i++ ) {
            eventQ->eventV[i].count1 = eventQ->eventV[i].count2 = -1;
        }

        PTL_DBG2("%p\n",&eventQ->eventV[0]);

        cmdUnion_t& cmd  = m_ptlIF.getCmdSlot( PtlEQAllocCmd );

        cmd.eqAlloc.handle = handle;
        cmd.eqAlloc.size   = size;
        cmd.eqAlloc.addr   = &eventQ->eventV[0];
        
        m_ptlIF.commitCmd();

        m_eqM[handle] = eventQ;

        return handle;
    }

    int ptlEQFree( ptl_handle_eq_t eq_handle )
    {
        PTL_DBG2("eq_handle=%d\n",eq_handle );
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlEQFreeCmd ); 

        cmd.eqFree.handle = eq_handle;

        m_ptlIF.commitCmd();

        delete m_eqM[eq_handle];
        m_freeEQHandles.push_back( eq_handle );

        return PTL_OK;
    }

    int ptlEQWait(ptl_handle_ct_t eq_handle, ptl_event_t* event )
    {
        int retval;
        while ( ( retval = ptlEQGet( eq_handle, event ) ) == -PTL_EQ_EMPTY );

        return retval;
    }

    void checkMeUnlinked() {
        int handle; 
        while ( ( handle = m_ptlIF.popMeUnlinked() ) != -1 ) {
            PTL_DBG2( "handle=%d\n",handle );
            m_freeMEHandles.push_back(handle);
        } 
    }

    int ptlEQGet(ptl_handle_ct_t eq_handle, ptl_event_t* event )
    {
        EventQ*    foo = m_eqM[ eq_handle ];
        ptl_size_t pos = foo->count % foo->size;

        checkMeUnlinked();

#if 0
        PTL_DBG2( "nid=%d eq_handle=%d want count=%li cur=%li %p\n", 
                    m_id.phys.nid,
                    eq_handle, foo->count, foo->eventV[ pos ].count1, 
                                &foo->eventV[pos] );
#endif

        if ( foo->eventV[ pos ].count1 < foo->count ) return -PTL_EQ_EMPTY;
        while ( foo->eventV[ pos ].count2 < foo->count );

        *event = foo->eventV[ pos ].user;
        PTL_DBG2("nid=%d type=%d\n",m_id.phys.nid,event->type);
        
        if ( foo->eventV[ pos ].count2 != foo->count ) {
            return -PTL_EQ_DROPPED;
        }

        ++foo->count;
        return PTL_OK;
    }
    int ptlEQPoll(ptl_handle_eq_t* eq_handles, unsigned int size, 
                ptl_time_t timeout, ptl_event_t* event, unsigned int* which )
    {
        int retval;
        // timeout not supported
        if ( timeout != 0 ) return -PTL_ARG_INVALID;

        for ( unsigned int i = 0; i < size; i++ ) {
            retval = ptlEQGet( eq_handles[i], event );

            *which = i;
        
            if ( retval != -PTL_EQ_EMPTY)  {
                break;
            }
        }
        return retval;
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

        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlPutCmd ); 

        cmd.ptlPut.md_handle     = md_handle;
        cmd.ptlPut.local_offset  = local_offset;
        cmd.ptlPut.length        = length;
        cmd.ptlPut.ack_req       = ack_req;
        cmd.ptlPut.target_id     = target_id;
        cmd.ptlPut.pt_index      = pt_index;
        cmd.ptlPut.match_bits    = match_bits;
        cmd.ptlPut.remote_offset = remote_offset;
        cmd.ptlPut.user_ptr      = user_ptr;
        cmd.ptlPut.hdr_data     = hdr_data;

        m_ptlIF.commitCmd();
		return PTL_OK;
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
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlGetCmd ); 

        cmd.ptlGet.md_handle     = md_handle;
        cmd.ptlGet.local_offset  = local_offset;
        cmd.ptlGet.length        = length;
        cmd.ptlGet.target_id     = target_id;
        cmd.ptlGet.pt_index      = pt_index;
        cmd.ptlGet.match_bits    = match_bits;
        cmd.ptlGet.remote_offset = remote_offset;
        cmd.ptlGet.user_ptr      = user_ptr;

        m_ptlIF.commitCmd();
		return PTL_OK;
    }

    int ptlTrigGet(ptl_handle_md_t  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr,
            ptl_handle_ct_t trig_ct_handle,
            ptl_size_t      threshold )
    {
        PTL_DBG2("\n");
        cmdUnion_t& cmd = m_ptlIF.getCmdSlot( PtlTrigGetCmd ); 

        cmd.ptlTrigGet.md_handle     = md_handle;
        cmd.ptlTrigGet.local_offset  = local_offset;
        cmd.ptlTrigGet.length        = length;
        cmd.ptlTrigGet.target_id     = target_id;
        cmd.ptlTrigGet.pt_index      = pt_index;
        cmd.ptlTrigGet.match_bits    = match_bits;
        cmd.ptlTrigGet.remote_offset = remote_offset;
        cmd.ptlTrigGet.user_ptr      = user_ptr;
        cmd.ptlTrigGet.trig_ct_handle= trig_ct_handle;
        cmd.ptlTrigGet.threshold     = threshold;

        m_ptlIF.commitCmd();
		return PTL_OK;
    }

  private:

    void initLimits( ptl_ni_limits_t& limits )
    {
        for ( int i = 0; i < limits.max_cts; i++ ) {
            m_freeCTHandles.push_back(i);
        } 

        for ( int i = 0; i < limits.max_mds; i++ ) {
            m_freeMDHandles.push_back(i);
        } 

        for ( int i = 0; i < limits.max_list_size; i++ ) {
            m_freeMEHandles.push_back(i);
        } 

        for ( int i = 0; i < limits.max_eqs; i++ ) {
            m_freeEQHandles.push_back(i);
        } 
    }

    PtlIF&                          m_ptlIF;
    std::map<int,ptl_ct_event_t*>   m_ctM;
    std::map<int,EventQ*>           m_eqM;
    ptl_process_t                   m_id;
    std::deque<int>                 m_freeCTHandles;
    std::deque<int>                 m_freeMDHandles;
    std::deque<int>                 m_freeMEHandles;
    std::deque<int>                 m_freeEQHandles;
};

}

#endif
