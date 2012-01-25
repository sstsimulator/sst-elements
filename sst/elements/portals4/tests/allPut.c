#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <portals4.h>
#include <m5rt.h>
#include <assert.h>

#define DBG( fmt, args... ) \
    fprintf(stderr, "%d:%s():%d: "fmt, myRank, __func__, __LINE__, ##args)


static void postRecv( ptl_handle_ni_t, ptl_process_t*, int numRank );
static void send( ptl_handle_ni_t, ptl_process_t*, int numRank );
static void waitRecv( ptl_handle_ni_t, ptl_process_t*, int numRank );

static void printEvent( ptl_event_t *event );

//#define ACK_REQ    PTL_ACK_REQ
#define ACK_REQ    PTL_NO_ACK_REQ

//#define MSG_SIZE 0x2000 
#define MSG_SIZE  512
#define PTL_INDEX 11
ptl_match_bits_t match_bits = 0xdeadbeef;

int myRank;

extern char **environ;
int main( int argc, char* argv[] )
{
    DBG("hello mike\n");

    ptl_handle_ni_t ni_handle;
    ptl_ni_limits_t  ni_ask_limits;
    ptl_ni_limits_t  ni_got_limits;

    ptl_process_t id;
    int retval;

    if ( ( retval = PtlInit() ) != PTL_OK ){
        DBG("PtlInit() failed %d\n", retval ); abort();
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
        DBG("PtlNIInit() failed %d\n", retval ); abort();
    }
//    DBG("max_pt_index=%d\n",ni_got_limits.max_pt_index);

    if ( ( retval = PtlGetId( ni_handle, &id ) ) != PTL_OK ) {
        DBG("PtlNIInit() failed %d\n",retval); abort();
    }

//    DBG("id.phys.nid=%#x id.phys.pid=%d\n",id.phys.nid,id.phys.pid);

    myRank = cnos_get_rank();//id.phys.nid;

    int numRanks = cnos_get_size();

    DBG("myRank=%d numRanks=%d\n",myRank,numRanks);

    cnos_nidpid_map_t* map;
    int foo = cnos_get_nidpid_map( &map );
    DBG("foo=%d\n",foo);
    int i;
    for ( i = 0; i < numRanks; i++ ) {
        DBG("rank=%d nid=%d pid=%d \n",i, map[i].nid,map[i].pid );
    }
    return 0;

    postRecv(ni_handle,&id,numRanks);
    cnos_barrier();
    send(ni_handle,&id,numRanks);
    waitRecv(ni_handle,&id,numRanks);

    if ( ( retval = PtlNIFini( ni_handle ) ) != PTL_OK ) { 
        DBG("PtlNIFini() failed %d\n",retval); abort();
    }
    PtlFini();

    DBG("goodbye\n");
    cnos_barrier();
    return 0;
}


static void send( ptl_handle_ni_t ni_handle, ptl_process_t* id, int numRanks)
{
    int retval;
    ptl_handle_md_t md_handle;
    ptl_md_t md;
    ptl_process_t   target_id = *id;
//    DBG("ni_handle=%#lx\n", (unsigned long) ni_handle);

    md.start = malloc(MSG_SIZE);
    md.length = MSG_SIZE;
    md.options = 0;
    md.eq_handle = PTL_EQ_NONE;
    md.ct_handle = PTL_CT_NONE;

    int i;
    for ( i = 0; i < md.length; i++ ) {
        ((unsigned char*)md.start)[i] = i;
    }

    if ( ( retval = PtlEQAlloc( ni_handle, 
                        numRanks*2 + 1, &md.eq_handle ) ) != PTL_OK ) {
        DBG("PtlEQAlloc() failed %d\n",retval); abort();
    }

    if ( PtlMDBind( ni_handle, &md, &md_handle ) != PTL_OK ) {
        DBG("PtlMDBind() failed\n"); abort();
    } 

    ptl_ack_req_t ack_req = ACK_REQ;

    for ( i = 0; i < numRanks; i++ ) {
        if ( i == myRank ) continue;
        ptl_hdr_data_t hdr_data = i;
        ptl_size_t offset = myRank * ( md.length/ numRanks );
        ptl_size_t length = md.length / numRanks;
        
   //     DBG("rank=%d offset=%lu length=%lu\n",i,offset,length);
        target_id.phys.nid = i;
        if ( PtlPut( md_handle, offset, length , ack_req, target_id, PTL_INDEX,
                    match_bits, offset, &md, hdr_data ) ) { 
            DBG("PtlPtl() failed\n"); abort();
        } 
    }

    for ( i = 0; i < numRanks; i++ ) {
        if ( i == myRank ) continue;
        ptl_event_t event;
        if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
            DBG("PtlEQWait() failed %d\n",retval); abort();
        }

        //printEvent( &event );
        assert( event.type == PTL_EVENT_SEND );
        assert( event.user_ptr == &md ); 
        assert( event.ni_fail_type == PTL_NI_OK );
//        DBG("got PTL_EVENT_SEND\n");

        if ( ack_req == PTL_ACK_REQ ) {
            if ( ( retval = PtlEQWait( md.eq_handle, &event ) ) != PTL_OK ) {
                DBG("PtlEQWait() failed %d\n",retval); abort();
            }
//            printEvent( &event );
            assert( event.type == PTL_EVENT_ACK );
            assert( event.mlength == md.length/numRanks );
            assert( event.user_ptr == &md ); 
            assert( event.ni_fail_type == PTL_NI_OK );
            //assert( event.remote_offset == i * md.length/numRanks  ); 
//            DBG("got PTL_EVENT_ACK offset=%lu\n", event.remote_offset );
        }
    }

    if ( PtlMDRelease( md_handle ) != PTL_OK ) {
        DBG("PtlMDRelease() failed\n"); abort();
    } 

    if ( (retval = PtlEQFree( md.eq_handle ) ) != PTL_OK ) {
            DBG("PtlEQFree() failed %d\n",retval); abort();
    }

    DBG("%s():%d\n",__func__,__LINE__);
}

ptl_handle_me_t  me_handle;
ptl_handle_eq_t  eq_handle;
ptl_pt_index_t   pt_index;
ptl_me_t me;

static void postRecv( ptl_handle_ni_t ni_handle, ptl_process_t* id, 
                                                        int numRanks)
{
    eq_handle = PTL_EQ_NONE;
    int retval;
//    DBG("%s():%d\n",__func__,__LINE__);

    me.ct_handle = PTL_CT_NONE;

    if ( ( retval = PtlEQAlloc( ni_handle, 
                            numRanks+1, &eq_handle ) ) != PTL_OK ) {
        DBG("PtlEQAlloc() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTAlloc( ni_handle, 
                    0, 
                    eq_handle,
                    PTL_INDEX,
                    &pt_index ) ) != PTL_OK ) {
        DBG("PtlPTAlloc() failed %d\n",retval); abort();
    }

    assert( PTL_INDEX == pt_index );

    me.start = malloc(MSG_SIZE);
    me.length = MSG_SIZE;
    me.min_free = 0;
    me.options = PTL_ME_OP_PUT;
    me.match_id.phys.nid = PTL_NID_ANY;
    me.match_id.phys.pid = 1;
    me.match_bits = match_bits;
    me.ignore_bits = 0;

    int i;
    for ( i = 0; i < me.length/numRanks; i++ ) {
        int j = me.length/numRanks * myRank + i;
        ((unsigned char*)me.start)[j] = j;
    }

    if ( ( retval = PtlMEAppend( ni_handle,  
                        pt_index,
                        &me,
                        PTL_PRIORITY_LIST,
                        &me, // user_ptr
                        &me_handle) ) != PTL_OK ) {
        DBG("PtlMEAppend() failed %d\n",retval); abort();
    }
}

static void waitRecv( ptl_handle_ni_t ni_handle, ptl_process_t* id, 
                                                        int numRanks)
{
    int i;
    int retval;
    for ( i = 0; i < numRanks; i++ )
    {
        if ( i == myRank ) continue;
        ptl_event_t event;
        if ( ( retval = PtlEQWait( eq_handle, &event ) ) != PTL_OK ) {
            DBG("PtlEQWait() failed %d\n",retval); abort();
        }
        //printEvent( &event );
        assert( event.type == PTL_EVENT_PUT );
        assert( event.initiator.phys.pid == 1 );
        assert( event.pt_index == pt_index );
        assert( event.user_ptr == &me );
        assert( event.ni_fail_type == PTL_NI_OK );
        assert( event.user_ptr == &me );

#if 0
        DBG("got PTL_EVENT_PUT, nid=%d hdr_data=%lu start=%p offset=%lu\n", 
                    event.initiator.phys.nid, event.hdr_data, 
                    event.start,
                    event.start-me.start);
#endif
    }

    for ( i = 0; i < me.length; i++ ) {
        unsigned int tmp = i  % 256;
        unsigned char c = ( (unsigned char*) me.start )[ i ]; 
        if ( c !=  tmp ) {
            DBG("%d != %d\n", tmp, c );
        }
    }

    if ( ( retval = PtlMEUnlink( me_handle ) ) != PTL_OK ) {
        DBG("PtlMEFree() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTFree( ni_handle, 
                    pt_index ) ) != PTL_OK ) {
        DBG("PtlPTFree() failed %d\n",retval); abort();
    }

    if ( (retval = PtlEQFree( eq_handle ) ) != PTL_OK ) {
        DBG("PtlEQFree() failed %d\n",retval); abort();
    }
}    

static void printEvent( ptl_event_t *event )
{
    DBG("type=%d\n",event->type);
    DBG("initiator.phys=%d:%d\n",event->initiator.phys.nid,
                                event->initiator.phys.pid);
    DBG("pt_index=%d\n",event->pt_index);
    DBG("uid=%d\n",event->uid);
    DBG("jid=%d\n",event->jid);
    DBG("match_bits=%#lx\n",event->match_bits);
    DBG("rlength=%lu\n",event->rlength);
    DBG("mlength=%lu\n",event->mlength);
    DBG("remote_offset=%lu\n",event->remote_offset);
    DBG("start=%p\n",event->start);
    DBG("user_ptr=%p\n",event->user_ptr);
    DBG("hdr_data=%#lx\n",event->hdr_data); 
    DBG("ni_fail_type=%d\n",event->ni_fail_type);
    DBG("atomic_operation=%d\n",event->atomic_operation);
    DBG("atomic_type=%d\n",event->atomic_type);
}
