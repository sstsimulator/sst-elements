#include <ptl_internal_netIf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <foo.h>

static int ptlPTAlloc( void* data,
                        unsigned int    options,
                        ptl_handle_eq_t eq_handle,
                        ptl_pt_index_t  pt_index_req,
                        ptl_pt_index_t *pt_index)
{
    printf("%s() %p\n",__func__,data);
    return ((foo*)data)->ptAlloc( options, eq_handle, pt_index_req, pt_index );
}

static int ptlPTFree( void* data, ptl_pt_index_t pt_index)
{
    printf("%s() %p\n",__func__,data);
    return ((foo*)data)->ptFree( pt_index );
}

static _netIf_t* ifInit(
              unsigned int      options,
              ptl_pid_t         pid,
              ptl_ni_limits_t   *desired,
              ptl_ni_limits_t   *actual,
              ptl_size_t        map_size,
              ptl_process_t     *desired_mapping,
              ptl_process_t     *actual_mapping
    )
{
    printf("%s()\n",__func__);
    _netIf_t* netIf = (_netIf_t*) malloc( sizeof( *netIf ) );
    assert(netIf);
    netIf->data = new foo( options, pid, desired, actual, map_size,
                desired_mapping, actual_mapping ); 
    netIf->PtlPTAlloc = ptlPTAlloc;
    netIf->PtlPTFree = ptlPTFree;
    netIf->name = "PtlNic";

    return netIf;
}

static int ifFini( _netIf_t* netIf )
{
    printf("%s()\n",__func__);
    delete (foo*)(netIf->data);
    free( netIf );
    return 0;
}

static netIf_t netIf;

static __attribute__ ((constructor)) void init(void)
{
    printf("%s\n",__func__);
    netIf.init = ifInit;
    netIf.fini = ifFini;

    ptl_internal_register_netIf( "PtlNic", &netIf );
}

int ptlNic;
