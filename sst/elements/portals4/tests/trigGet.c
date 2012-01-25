#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <portals4.h>
#include <m5rt.h>
#include <assert.h>

static void nid0( ptl_handle_ni_t, ptl_process_t* );
static void nid1( ptl_handle_ni_t, ptl_process_t* );
static void printEvent( ptl_event_t *event );

#define MSG_LENGTH  (sizeof(int) * 200 )
#define SHORT_MSG_LENGTH 100
#define PTL_INDEX  10
#define OFFSET_DIV 1 
ptl_match_bits_t MATCH_BITS = 0xdeadbeef;
#define HDR_DATA 0xdeadbeef01234567

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

    if ( id.phys.nid == 0 ) {
        nid0(ni_handle,&id);
    } else if ( id.phys.nid == 1 ) {
        nid1(ni_handle,&id);
    }else {
        cnos_barrier();
    }

    if ( ( retval = PtlNIFini( ni_handle ) ) != PTL_OK ) { 
        printf("PtlNIFini() failed %d\n",retval); abort();
    }
    PtlFini();

    printf("goodbye\n");
    cnos_barrier();
    return 0;
}

static void nid0( ptl_handle_ni_t ni_handle, ptl_process_t* id )
{
    int retval;
    ptl_pt_index_t  pt_index;
    ptl_handle_me_t me_handle;
    ptl_handle_eq_t eq_handle;
    ptl_handle_md_t md_handle;
    ptl_md_t        md;
    ptl_me_t        me;
    ptl_process_t   target_id = *id;
    ptl_event_t     event;
    int*           buf = (int*) malloc(MSG_LENGTH);

    int i;
    for ( i = 0; i < (signed) (MSG_LENGTH/sizeof(int)); i++ ) {
        buf[i] = i;
    }

    target_id.phys.nid = (id->phys.nid + 1 ) % 2;

    printf("%s():%d %d -> %d\n",__func__,__LINE__, 
                           id->phys.nid, target_id.phys.nid );

    if ( ( retval = PtlEQAlloc( ni_handle, 10, &eq_handle ) ) != PTL_OK ) {
        printf("PtlEQAlloc() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTAlloc( ni_handle, 
                    0, 
                    eq_handle,
                    PTL_INDEX,
                    &pt_index ) ) != PTL_OK ) {
        printf("PtlPTAlloc() failed %d\n",retval); abort();
    }
    assert( pt_index == PTL_INDEX );

    me.start = buf;
    me.length = MSG_LENGTH;
    me.ct_handle = PTL_CT_NONE;
    me.min_free = 0;
    me.options = PTL_ME_OP_GET | PTL_ME_MANAGE_LOCAL;
    me.match_id = target_id;
    me.match_bits = MATCH_BITS;
    me.ignore_bits = 0;

    if ( ( retval = PtlMEAppend( ni_handle,  
                        pt_index,
                        &me,
                        PTL_PRIORITY_LIST,
                        &me,
                        &me_handle) ) != PTL_OK ) {
        printf("PtlMEAppend() failed %d\n",retval); abort();
    }

    md.start = buf;
    md.length = SHORT_MSG_LENGTH;
    md.options = 0;
    md.ct_handle = PTL_CT_NONE;
    md.eq_handle = eq_handle;

    if ( PtlMDBind( ni_handle, &md, &md_handle ) != PTL_OK ) {
        printf("PtlMDBind() failed\n"); abort();
    } 
    
    cnos_barrier();

    if ( PtlPut( md_handle, 
                0, // local offset
                SHORT_MSG_LENGTH, // length
                PTL_NO_ACK_REQ, 
                target_id, 
                PTL_INDEX,
                MATCH_BITS, 
                0, // remote offset
                &md, //user ptr
                HDR_DATA ) ) { 
        printf("PtlGet() failed\n"); abort();
    } 

    printf("wait for PTL_EVENT_SEND\n");
    if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
        printf("PtlEQWait() failed %d\n",retval); abort();
    }

    //printEvent( &event );
    assert( event.type == PTL_EVENT_SEND );
    assert( event.mlength == 0 ); 
    assert( event.user_ptr == &md ); 
    assert( event.ni_fail_type == PTL_NI_OK );
    printf("got PTL_EVENT_SEND, remote_offset=%lu\n", event.remote_offset);

    
    printf("wait for PTL_EVENT_GET\n");
    if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
        printf("PtlEQWait() failed %d\n",retval); abort();
    }

    printEvent( &event );
    assert( event.type == PTL_EVENT_GET );
    assert( event.mlength == MSG_LENGTH - SHORT_MSG_LENGTH ); 
    assert( event.user_ptr == &me ); 
    assert( event.ni_fail_type == PTL_NI_OK );
    printf("got PTL_EVENT_GET, remote_offset=%lu\n", event.remote_offset);

    if ( PtlMDRelease( md_handle ) != PTL_OK ) {
        printf("PtlMDRelease() failed\n"); abort();
    } 

    if ( (retval = PtlEQFree( md.eq_handle ) ) != PTL_OK ) {
        printf("PtlEQFree() failed %d\n",retval); abort();
    }

    printf("%s():%d\n",__func__,__LINE__);
}

#if 0
    int i;
    unsigned char* start = md.start + event.remote_offset;
    for ( i = 0; i < event.mlength; i++ ) {
        unsigned int tmp = (i + event.remote_offset ) % 256;
        unsigned char c = start[ i ];
        if ( c !=  tmp ) {
            printf("%d != %d\n", tmp, c );
        }
    }
#endif



static void nid1( ptl_handle_ni_t ni_handle, ptl_process_t* id )
{
    int             retval;
    ptl_pt_index_t  pt_index;
    ptl_handle_me_t me_handle;
    ptl_handle_md_t md_handle;
    ptl_handle_eq_t eq_handle;
    ptl_handle_ct_t ct_handle;
    ptl_event_t     event;
    ptl_me_t        me;
    ptl_md_t        md;
    int*           buf = (int*) malloc( MSG_LENGTH );

    memset(buf,0,MSG_LENGTH);

    ptl_process_t target_id = *id;
    target_id.phys.nid = (id->phys.nid + 1 ) % 2;
    printf("%s():%d %d -> %d\n",__func__,__LINE__, 
                           id->phys.nid, target_id.phys.nid );

    if ( ( retval = PtlEQAlloc( ni_handle, 10, &eq_handle ) ) != PTL_OK ) {
        printf("PtlEQAlloc() failed %d\n",retval); abort();
    }

    if ( ( retval = PtlCTAlloc( ni_handle, &ct_handle ) ) != PTL_OK ) {
        printf("PtlCTAlloc() failed %d\n",retval); abort();
    } 

    if ( (retval = PtlPTAlloc( ni_handle, 
                    0, 
                    eq_handle,
                    PTL_INDEX,
                    &pt_index ) ) != PTL_OK ) {
        printf("PtlPTAlloc() failed %d\n",retval); abort();
    }

    md.start = buf;
    md.length = MSG_LENGTH;
    md.options = 0;
    md.ct_handle = PTL_CT_NONE;
    md.eq_handle = eq_handle;

    if ( PtlMDBind( ni_handle, &md, &md_handle ) != PTL_OK ) {
        printf("PtlMDBind() failed\n"); abort();
    } 

    if ( ( retval = PtlTriggeredGet( 
                        md_handle, 
                        0 + SHORT_MSG_LENGTH, // local_offset,
                        MSG_LENGTH - SHORT_MSG_LENGTH, // length
                        target_id, 
                        PTL_INDEX,
                        MATCH_BITS,
                        SHORT_MSG_LENGTH, // remote_offset
                        &md, // user_ptr
                        ct_handle,
                        1  // threshhold
                        ) ) != PTL_OK ) 
    {
        printf("PtlTriggerGet() failed\n"); abort();
    } 

    me.start = buf;
    me.length = SHORT_MSG_LENGTH;
    me.ct_handle = ct_handle;
    me.min_free = 0;
    me.options = PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL;
    me.match_id = target_id;
    me.match_bits = MATCH_BITS;
    me.ignore_bits = 0;

    if ( ( retval = PtlMEAppend( ni_handle,  
                        pt_index,
                        &me,
                        PTL_PRIORITY_LIST,
                        &me,
                        &me_handle) ) != PTL_OK ) {
        printf("PtlMEAppend() failed %d\n",retval); abort();
    }

    cnos_barrier();

    printf("wait for PTL_EVENT_PUT\n");
    if ( ( retval = PtlEQWait( eq_handle, &event ) ) != PTL_OK ) {
        printf("PtlEQWait() failed %d\n",retval); abort();
    }

    //printEvent(&event);
    assert( event.type == PTL_EVENT_PUT );
    assert( event.initiator.phys.nid == 0 );
    assert( event.initiator.phys.pid == id->phys.pid );
    assert( event.pt_index == pt_index );
    //assert( event.start == me.start + event.remote_offset );
    assert( event.user_ptr == &me );
    assert( event.mlength == me.length / OFFSET_DIV ); 
    assert( event.user_ptr == &me ); 
    assert( event.ni_fail_type == PTL_NI_OK );
    printf("got PTL_EVENT_PUT, remote_offset=%lu\n",
                                            event.remote_offset);

    int i;
    for ( i = 0; i < (signed) (MSG_LENGTH/sizeof(int)); i++ ) {
        if ( buf[i] != i ) {
            printf("%d %d\n",i,buf[i]);
        }
    }

    if ( ( retval = PtlMEUnlink( me_handle ) ) != PTL_OK ) {
        printf("PtlMEFree() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTFree( ni_handle, 
                    pt_index ) ) != PTL_OK ) {
        printf("PtlPTFree() failed %d\n",retval); abort();
    }

    if ( (retval = PtlEQFree( eq_handle ) ) != PTL_OK ) {
        printf("PtlEQFree() failed %d\n",retval); abort();
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

