#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <portals4.h>
#include <m5rt.h>
#include <assert.h>

static void nid0( ptl_handle_ni_t, ptl_process_t*, int );
static void nid1( ptl_handle_ni_t, ptl_process_t*, int );

#define SYS_virt2phys 442

static inline unsigned long virt2phys( void* addr ) {
    return (unsigned long) syscall( SYS_virt2phys, addr );
}    

static void printEvent( ptl_event_t *event );

#define TEST_1 (1 << 0)
#define TEST_2 (2 << 1)

//#define ACK_REQ    PTL_NO_ACK_REQ
#define ACK_REQ    PTL_ACK_REQ
#define TEST_NUM TEST_2
#define OFFSET_DIV 1

//#define MSG_SIZE 0x2000 
#define MSG_SIZE  100
#define PTL_INDEX 11
#define HDR_DATA 0xdeadbeef01234567
ptl_match_bits_t match_bits = 0xdeadbeef;

static int rank;


extern char **environ;
int main( int argc, char* argv[] )
{
    printf("hello mike\n");

    ptl_handle_ni_t ni_handle;
    ptl_ni_limits_t  ni_ask_limits;
    ptl_ni_limits_t  ni_got_limits;

    ptl_process_t id;
    int retval;

    if ( ( retval = PtlInit() ) != PTL_OK ){
        printf("PtlInit() failed %d\n", retval ); abort();
    }
    
    ni_ask_limits.max_entries = 10;
    ni_ask_limits.max_mds = 7;
    ni_ask_limits.features = 0xdeadbeef;
    if ( ( retval = PtlNIInit( "PtlNic",
                PTL_NI_MATCHING | PTL_NI_PHYSICAL, 
                PTL_PID_ANY,  
                &ni_ask_limits,
                &ni_got_limits,
                0,
                NULL,
                NULL, 
                &ni_handle ) ) != PTL_OK ) {
        printf("PtlNIInit() failed %d\n", retval ); abort();
    }
    printf("max_pt_index=%d\n",ni_got_limits.max_pt_index);

    if ( ( retval = PtlGetId( ni_handle, &id ) ) != PTL_OK ) {
        printf("PtlNIInit() failed %d\n",retval); abort();
    }

    printf("id.phys.nid=%#x id.phys.pid=%d\n",id.phys.nid,id.phys.pid);

    rank = id.phys.nid;
    if ( id.phys.nid == 0 ) {
        nid0(ni_handle,&id,TEST_NUM);
    } else {
        nid1(ni_handle,&id,TEST_NUM);
    }

    if ( ( retval = PtlNIFini( ni_handle ) ) != PTL_OK ) { 
        printf("PtlNIFini() failed %d\n",retval); abort();
    }
    PtlFini();

    printf("goodbye\n");
    m5_barrier();
    return 0;
}


static void nid0( ptl_handle_ni_t ni_handle, ptl_process_t* id, int testNum )
{
    int retval;
    ptl_handle_md_t md_handle;
    ptl_md_t md;
    ptl_process_t   target_id = *id;
    printf("%s():%d ni_handle=%#lx\n",__func__,__LINE__,
                                        (unsigned long) ni_handle);

    md.start = malloc(MSG_SIZE);
    md.length = MSG_SIZE;
    md.options = 0;
    md.eq_handle = PTL_EQ_NONE;
    md.ct_handle = PTL_CT_NONE;

    if ( testNum & TEST_1 ) {
        md.options |= PTL_MD_EVENT_CT_SEND;
        md.options |= PTL_MD_EVENT_CT_ACK;
    }
    if ( ! (testNum & TEST_2 ) ) {
        md.options |= PTL_MD_EVENT_SUCCESS_DISABLE;
    }

    int i;
    for ( i = 0; i < md.length; i++ ) {
        ((unsigned char*)md.start)[i] = i;
    }

    if ( testNum & TEST_1 ) {
        if ( PtlCTAlloc( ni_handle, &md.ct_handle ) != PTL_OK ) {
            printf("PtlCTAlloc() failed\n"); abort();
        } 
    }

    if ( testNum & TEST_2 ) {
        if ( ( retval = PtlEQAlloc( ni_handle, 3, &md.eq_handle ) ) != PTL_OK )
        {
            printf("PtlEQAlloc() failed %d\n",retval); abort();
        }
    }

    if ( PtlMDBind( ni_handle, &md, &md_handle ) != PTL_OK ) {
        printf("PtlMDBind() failed\n"); abort();
    } 
    target_id.phys.nid = 1;

    printf("%d:%d: calling barrier\n",id->phys.nid,id->phys.pid);
    m5_barrier(); 
    printf("%d:%d: barrier returning\n",id->phys.nid,id->phys.pid);

    ptl_ack_req_t ack_req = ACK_REQ;
    ptl_hdr_data_t hdr_data = HDR_DATA;

    ptl_size_t offset = md.length - ( md.length / OFFSET_DIV);
    ptl_size_t length = md.length - offset;

    if ( PtlPut( md_handle, offset, length , ack_req, target_id, PTL_INDEX,
                        match_bits, offset, &md, hdr_data ) ) { 
        printf("PtlPtl() failed\n"); abort();
    } 

    if ( testNum & TEST_1 ) { 
        ptl_ct_event_t ct_event;
        ptl_size_t wantCnt = 1; 
        if ( ack_req == PTL_ACK_REQ ) {
            ++wantCnt; 
        }

        if ( PtlCTWait( md.ct_handle, wantCnt, &ct_event) != PTL_OK ) {
            printf("PtlCTWait() failed\n"); abort();
        } 
        printf("PtlPut ct_event sucess %lu failure %lu\n",
                        ct_event.success, ct_event.failure);
    }

    if ( testNum & TEST_2 ) {
        ptl_event_t event;
        if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
            printf("PtlEQWait() failed %d\n",retval); abort();
        }
        //printEvent( &event );
        assert( event.type == PTL_EVENT_SEND );
        assert( event.user_ptr == &md ); 
        assert( event.ni_fail_type == PTL_NI_OK );
        printf("got PTL_EVENT_SEND\n");

        if ( ack_req == PTL_ACK_REQ ) {
            if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
                printf("PtlEQWait() failed %d\n",retval); abort();
            }
            //printEvent( &event );
            assert( event.type == PTL_EVENT_ACK );
            assert( event.mlength == length );
            assert( event.user_ptr == &md ); 
            assert( event.ni_fail_type == PTL_NI_OK );
            printf("got PTL_EVENT_ACK, remote_offset=%lu\n",
                                                event.remote_offset);
        }
    }

    if ( PtlMDRelease( md_handle ) != PTL_OK ) {
        printf("PtlMDRelease() failed\n"); abort();
    } 

    if ( testNum & TEST_1 ) { 
        if ( PtlCTFree( md.ct_handle ) != PTL_OK ) {
            printf("PtlCTFree() failed\n"); abort();
        } 
    }

    if ( testNum & TEST_2 ) {
        if ( (retval = PtlEQFree( md.eq_handle ) ) != PTL_OK ) {
            printf("PtlEQFree() failed %d\n",retval); abort();
        }
    }

    printf("%s():%d\n",__func__,__LINE__);
}

static void nid1( ptl_handle_ni_t ni_handle, ptl_process_t* id, int testNum )
{
    int retval;
    ptl_handle_me_t  me_handle;
    ptl_pt_index_t   pt_index;
    ptl_handle_eq_t  eq_handle = PTL_EQ_NONE;
    ptl_me_t me;
    printf("%s():%d\n",__func__,__LINE__);

    me.ct_handle = PTL_CT_NONE;

    if ( testNum & TEST_1 ) {
        if ( PtlCTAlloc( ni_handle, &me.ct_handle ) != PTL_OK ) {
            printf("PtlCTAlloc() failed\n"); abort();
        } 
    }

    if ( testNum & TEST_2 ) {
        if ( ( retval = PtlEQAlloc( ni_handle, 2, &eq_handle ) ) != PTL_OK ) {
            printf("PtlEQAlloc() failed %d\n",retval); abort();
        }
    }

    if ( (retval = PtlPTAlloc( ni_handle, 
                    0, 
                    eq_handle,
                    PTL_INDEX,
                    &pt_index ) ) != PTL_OK ) {
        printf("PtlPTAlloc() failed %d\n",retval); abort();
    }

    assert( PTL_INDEX == pt_index );

    me.start = malloc(MSG_SIZE);
    me.length = MSG_SIZE;
    me.min_free = 0;
    me.options = PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL;
    me.match_id.phys.nid = 0;
    me.match_id.phys.pid = 1;
    me.match_bits = match_bits;
    me.ignore_bits = 0;

    if ( ( retval = PtlMEAppend( ni_handle,  
                        pt_index,
                        &me,
                        PTL_PRIORITY_LIST,
                        &me, // user_ptr
                        &me_handle) ) != PTL_OK ) {
        printf("PtlMEAppend() failed %d\n",retval); abort();
    }

    printf("%d:%d: calling barrier\n",id->phys.nid,id->phys.pid);
    m5_barrier(); 
    printf("%d:%d: barrier returning\n",id->phys.nid,id->phys.pid);

    if ( testNum & TEST_1 ) {
        ptl_ct_event_t ct_event;
        ptl_size_t wantCnt = 1; 

        if ( PtlCTWait( me.ct_handle, wantCnt, &ct_event) != PTL_OK ) {
            printf("PtlCTWait() failed\n"); abort();
        } 
        printf("PtlPut ct_event sucess %lu failure %lu\n",
                        ct_event.success, ct_event.failure);
    }

    if ( testNum & TEST_2 ) {
        ptl_event_t event;
        if ( ( retval = PtlEQWait( eq_handle, &event ) ) != PTL_OK ) {
            printf("PtlEQWait() failed %d\n",retval); abort();
        }
        //printEvent( &event );
        assert( event.type == PTL_EVENT_PUT );
        assert( event.initiator.phys.nid == 0 );
        assert( event.initiator.phys.pid == 1 );
        assert( event.pt_index == pt_index );
        assert( event.start == me.start + event.remote_offset );
        assert( event.user_ptr == &me );
        assert( event.hdr_data == HDR_DATA );
        assert( event.ni_fail_type == PTL_NI_OK );
        assert( event.user_ptr == &me );

        printf("got PTL_EVENT_PUT, mlength=%lu remote_offset=%lu\n",
                            event.mlength, event.remote_offset );
        int i;
        for ( i = 0; i < event.mlength; i++ ) {
            unsigned int tmp = (i + event.remote_offset ) % 256;
            unsigned char c = ( (unsigned char*) event.start )[ i ]; 
            if ( c !=  tmp ) {
                printf("%d != %d\n", tmp, c );
            }
        }
    }

    if ( ( retval = PtlMEUnlink( me_handle ) ) != PTL_OK ) {
        printf("PtlMEFree() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTFree( ni_handle, 
                    pt_index ) ) != PTL_OK ) {
        printf("PtlPTFree() failed %d\n",retval); abort();
    }

    if ( testNum & TEST_2 ) {
        if ( (retval = PtlEQFree( eq_handle ) ) != PTL_OK ) {
            printf("PtlEQFree() failed %d\n",retval); abort();
        }
    }
}    

static void printEvent( ptl_event_t *event )
{
    printf("type=%d\n",event->type);
    printf("initiator.phys=%d:%d\n",event->initiator.phys.nid,
                                event->initiator.phys.pid);
    printf("pt_index=%d\n",event->pt_index);
    printf("uid=%d\n",event->uid);
    printf("jid=%d\n",event->jid);
    printf("match_bits=%#lx\n",event->match_bits);
    printf("rlength=%lu\n",event->rlength);
    printf("mlength=%lu\n",event->mlength);
    printf("remote_offset=%lu\n",event->remote_offset);
    printf("start=%p\n",event->start);
    printf("user_ptr=%p\n",event->user_ptr);
    printf("hdr_data=%#lx\n",event->hdr_data); 
    printf("ni_fail_type=%d\n",event->ni_fail_type);
    printf("atomic_operation=%d\n",event->atomic_operation);
    printf("atomic_type=%d\n",event->atomic_type);
}
