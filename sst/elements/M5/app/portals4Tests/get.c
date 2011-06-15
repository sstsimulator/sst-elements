#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <portals4.h>

static void nid0( ptl_handle_ni_t, ptl_process_t* );
static void nid1( ptl_handle_ni_t, ptl_process_t* );

#define SYS_virt2phys 442

static inline unsigned long virt2phys( void* addr ) {
    return (unsigned long) syscall( SYS_virt2phys, addr );
}    

//#define MSG_SIZE 0x2000 
#define MSG_SIZE  100
#define PTL_INDEX 10
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
        nid0(ni_handle,&id);
    } else {
        nid1(ni_handle,&id);
    }

    if ( ( retval = PtlNIFini( ni_handle ) ) != PTL_OK ) { 
        printf("PtlNIFini() failed %d\n",retval); abort();
    }
    PtlFini();

    printf("goodbye\n");
    return 0;
}


static inline u_int realcc (void) {
  u_long cc;
  /* read the 64 bit process cycle counter into variable cc: */
  asm volatile("rpcc %0" : "=r"(cc) : : "memory");
  return cc;                    /* return the lower 32 bits */
}

static inline unsigned int virtcc (void) {
  u_long cc;
  asm volatile("rpcc %0" : "=r"(cc) : : "memory");
  return (cc + (cc<<32)) >> 32; /* add process offset and count */
}

void spin_sleep( int count )
{
    int stop = realcc() + count;
    
    while( (int) realcc() < stop ) {};
}
static void nid0( ptl_handle_ni_t ni_handle, ptl_process_t* id )
{
    ptl_handle_md_t md_handle;
    ptl_md_t md;
    ptl_process_t   target_id = *id;
    printf("%s():%d ni_handle=%#lx\n",__func__,__LINE__,ni_handle);

    md.start = malloc(MSG_SIZE);
    md.length = MSG_SIZE;
    md.options = PTL_MD_EVENT_CT_SEND; 
    md.eq_handle = PTL_EQ_NONE;


    if ( PtlCTAlloc( ni_handle, &md.ct_handle ) != PTL_OK ) {
        printf("PtlCTAlloc() failed\n"); abort();
    } 

    if ( PtlMDBind( ni_handle, &md, &md_handle ) != PTL_OK ) {
        printf("PtlMDBind() failed\n"); abort();
    } 
    target_id.phys.nid = 1;

    
    spin_sleep(100000000);
    if ( PtlGet( md_handle, 0, MSG_SIZE, target_id, PTL_INDEX,
                        match_bits, 0, NULL ) ) { 
        printf("PtlGet() failed\n"); abort();
    } 

    ptl_ct_event_t ct_event;
    if ( PtlCTWait( md.ct_handle, 1, &ct_event) != PTL_OK ) {
        printf("PtlCTWait() failed\n"); abort();
    } 

    int i;
    for ( i = 0; i < md.length; i++ ) {
        if ( ((unsigned char*)md.start)[i] != i % 256) {
            printf("%d != %d\n",i%256,((unsigned char*)md.start)[i]);
        }
    }

    printf("ct_event sucess %d failure %d\n",
                        ct_event.success, ct_event.failure);
    if ( PtlMDRelease( md_handle ) != PTL_OK ) {
        printf("PtlMDRelease() failed\n"); abort();
    } 
    if ( PtlCTFree( md.ct_handle ) != PTL_OK ) {
        printf("PtlCTFree() failed\n"); abort();
    } 

    printf("%s():%d\n",__func__,__LINE__);
}

static void nid1( ptl_handle_ni_t ni_handle, ptl_process_t* id )
{
    int retval;
    ptl_handle_me_t  me_handle;
    ptl_pt_index_t   pt_index;
    ptl_handle_eq_t  eq_handle;
    ptl_event_t event;
    ptl_me_t me;
    printf("%s():%d\n",__func__,__LINE__);

    if ( ( retval = PtlEQAlloc( ni_handle, 2, &eq_handle ) ) != PTL_OK ) {
        printf("PtlEQAlloc() failed %d\n",retval); abort();
    }

    if ( (retval = PtlPTAlloc( ni_handle, 
                    0, 
                    eq_handle,
                    10,
                    &pt_index ) ) != PTL_OK ) {
        printf("PtlPTAlloc() failed %d\n",retval); abort();
    }

    me.start = malloc(MSG_SIZE);
    me.length = MSG_SIZE;
    me.ct_handle = PTL_CT_NONE;
    me.min_free = 0;
    //me.ac = {}; 
    me.options = PTL_ME_OP_PUT;
    me.match_id.phys.nid = 0;
    me.match_id.phys.pid = 1;
    me.match_bits = match_bits;
    me.ignore_bits = 0;

    int i;
    for ( i = 0; i < me.length; i++ ) {
        ((unsigned char*)me.start)[i] = i;
    }

    if ( ( retval = PtlMEAppend( ni_handle,  
                        pt_index,
                        &me,
                        PTL_PRIORITY_LIST,
                        NULL,
                        &me_handle) ) != PTL_OK ) {
        printf("PtlMEAppend() failed %d\n",retval); abort();
    }

    if ( ( retval = PtlEQWait( eq_handle, &event ) ) != PTL_OK ) {
        printf("PtlEQWait() failed %d\n",retval); abort();
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
