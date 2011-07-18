#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <ptl_internal_netIf.h>
#include <ptl_internal_debug.h>

#include <ptlIF.h>
#include <ptlAPI.h>

namespace PtlNic {

static int ptlNIInit(
                struct ::PtlAPI*  obj,
                unsigned int      options,
                ptl_pid_t         pid,
                ptl_ni_limits_t   *desired,
                ptl_ni_limits_t   *actual,
                ptl_size_t        map_size,
                ptl_process_t     *desired_mapping,
                ptl_process_t     *actual_mapping
    )
{
    return ((PtlAPI*) obj->data)->ptlNIInit( options, pid, desired, actual,
                        map_size, desired_mapping, actual_mapping );
}


static int ptlNIFini( struct ::PtlAPI* obj )
{
    return ((PtlAPI*) obj->data)->ptlNIFini( );
}

static int ptlPTAlloc(  struct ::PtlAPI* obj, 
                        unsigned int    options,
                        ptl_handle_eq_t eq_handle,
                        ptl_pt_index_t  pt_index_req )
{
    return ((PtlAPI*) obj->data)->ptlPTAlloc( 
                        options, eq_handle, pt_index_req );
}

static int ptlPTFree( struct ::PtlAPI* obj, ptl_pt_index_t pt_index )
{
    return ((PtlAPI*) obj->data)->ptlPTFree( pt_index ); 
}

static int ptlMDBind( struct ::PtlAPI* obj, ptl_md_t* md )
{
    return ((PtlAPI*) obj->data)->ptlMDBind( md ); 
}

static int ptlMDRelease( struct ::PtlAPI* obj, ptl_handle_md_t md_handle)
{
    return ((PtlAPI*) obj->data)->ptlMDRelease( md_handle ); 
}

static int ptlMEAppend( struct ::PtlAPI* obj,
                ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void*               user_ptr )
{
    return ((PtlAPI*) obj->data)->ptlMEAppend( pt_index, me, 
                            ptl_list, user_ptr ); 
}

static int ptlMEUnlink( struct ::PtlAPI* obj, ptl_handle_me_t me_handle)
{
    return ((PtlAPI*) obj->data)->ptlMEUnlink( me_handle ); 
}

static int  ptlGetId( struct ::PtlAPI* obj, ptl_process_t* id ) 
{
    return ((PtlAPI*) obj->data)->ptlGetId( id ); 
}

static int  ptlCTAlloc( struct ::PtlAPI* obj ) 
{
    return ((PtlAPI*) obj->data)->ptlCTAlloc(); 
}

static int  ptlCTFree( struct ::PtlAPI* obj, ptl_handle_ct_t ct ) 
{
    return ((PtlAPI*) obj->data)->ptlCTFree( ct ); 
}

static int ptlCTWait( struct ::PtlAPI* obj,
                ptl_handle_ct_t   ct,
                ptl_size_t        test,
                ptl_ct_event_t *  event)
{
    return ((PtlAPI*) obj->data)->ptlCTWait( ct, test,event ); 
}

static int ptlPut( struct ::PtlAPI* obj,
            ptl_handle_md_t  md_handle,
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
    return ((PtlAPI*) obj->data)->ptlPut( md_handle,
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

static int ptlGet( struct ::PtlAPI* obj,
            ptl_handle_md_t  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void *           user_ptr )
{
    return ((PtlAPI*) obj->data)->ptlGet( md_handle,
                                local_offset,
                                length,
                                target_id,
                                pt_index,
                                match_bits,
                                remote_offset,
                                user_ptr);
}

static int ptlTrigGet( struct ::PtlAPI* obj,
            ptl_handle_md_t  md_handle,
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
    return ((PtlAPI*) obj->data)->ptlTrigGet( md_handle,
                                local_offset,
                                length,
                                target_id,
                                pt_index,
                                match_bits,
                                remote_offset,
                                user_ptr,
                                trig_ct_handle,
                                threshold );
}

static int ptlEQAlloc( struct ::PtlAPI* obj, ptl_size_t count )
{
    return ((PtlAPI*) obj->data)->ptlEQAlloc( count ); 
}

static int ptlEQFree( struct ::PtlAPI* obj, ptl_handle_eq_t eq_handle)
{
    return ((PtlAPI*) obj->data)->ptlEQFree( eq_handle ); 
}

static int ptlEQWait( struct ::PtlAPI* obj, ptl_handle_eq_t   eq_handle,
              ptl_event_t *     event)
{
    return ((PtlAPI*) obj->data)->ptlEQWait( eq_handle, event ); 
}

static int ptlEQGet( struct ::PtlAPI* obj, ptl_handle_eq_t   eq_handle,
              ptl_event_t *     event)
{
    return ((PtlAPI*) obj->data)->ptlEQGet( eq_handle, event ); 
}

static struct ::PtlAPI* initPtlIF( struct ::PtlIF* obj )
{
    struct ::PtlAPI*  ptlAPI = (struct ::PtlAPI*)malloc( sizeof(*ptlAPI) );
//    PTL_DBG("\n");
    ptlAPI->PtlNIInit   = ptlNIInit;
    ptlAPI->PtlNIFini   = ptlNIFini;
    ptlAPI->PtlPTAlloc  = ptlPTAlloc;
    ptlAPI->PtlPTFree   = ptlPTFree;
    ptlAPI->PtlMDBind   = ptlMDBind;
    ptlAPI->PtlMDRelease = ptlMDRelease;
    ptlAPI->PtlMEAppend = ptlMEAppend;
    ptlAPI->PtlMEUnlink = ptlMEUnlink;
    ptlAPI->PtlGetId    = ptlGetId; 
    ptlAPI->PtlCTAlloc  = ptlCTAlloc;
    ptlAPI->PtlCTFree   = ptlCTFree;
    ptlAPI->PtlCTWait   = ptlCTWait;
    ptlAPI->PtlPut      = ptlPut;
    ptlAPI->PtlGet      = ptlGet;
    ptlAPI->PtlTrigGet  = ptlTrigGet;
    ptlAPI->PtlEQAlloc  = ptlEQAlloc;
    ptlAPI->PtlEQFree   = ptlEQFree;
    ptlAPI->PtlEQWait   = ptlEQWait;
    ptlAPI->PtlEQGet    = ptlEQGet;
    ptlAPI->data = new PtlAPI( *(PtlIF*) obj->data );
    return ptlAPI;
}

static void finiPtlIF( struct ::PtlAPI* ptlAPI )
{
    delete (PtlAPI*) ptlAPI->data; 
    free( ptlAPI );
}

static struct ::PtlIF* initNetIF( )
{
    struct ::PtlIF*  ptlIF = (struct ::PtlIF*)malloc( sizeof(*ptlIF) );
    ptlIF->init = initPtlIF;
    ptlIF->fini = finiPtlIF;
    ptlIF->data = new PtlIF( /*jid*/ 1, getuid() );
    
    return ptlIF;
}

static void finiNetIF( struct ::PtlIF* ptlIF )
{
    delete (PtlIF*) ptlIF->data;
    free( ptlIF );
}

static struct NetIF netIf;


static __attribute__ ((constructor)) void init(void)
{
    netIf.init = initNetIF;
    netIf.fini = finiNetIF;

    ptl_internal_register_netIF( "PtlNic", &netIf );
}

}

int ptlNic;
