/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#include <stdio.h>
#include <portals4.h>
#include <ptl_internal_handles.h>
#include <ptl_internal_netIf.h>
#include <ptl_internal_debug.h>

int PtlPTAlloc(ptl_handle_ni_t  ni_handle,
               unsigned int     options,
               ptl_handle_eq_t  eq_handle,
               ptl_pt_index_t   pt_index_req,
               ptl_pt_index_t*  pt_index)
{
    const ptl_internal_handle_converter_t eq = { eq_handle };
    const ptl_internal_handle_converter_t ni = { ni_handle };
    struct PtlAPI* api = GetPtlAPI( ni );

    PTL_DBG("\n");
    
    int handle = eq_handle == PTL_EQ_NONE ? -1 : eq.s.code;

    ptl_pt_index_t retval = 
            api->PtlPTAlloc( api, options, handle, pt_index_req );
    if ( retval < 0 ){
        return -retval;
    }
    *pt_index = retval;
    return PTL_OK;
}

int PtlPTFree(ptl_handle_ni_t   ni_handle,
              ptl_pt_index_t    pt_index)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };
    struct PtlAPI* api = GetPtlAPI( ni );

    PTL_DBG("\n");

    return api->PtlPTFree( api, pt_index );
}

int PtlPTDisable(ptl_handle_ni_t    ni_handle,
                 ptl_pt_index_t     pt_index)
{
    return PTL_FAIL;
}

int PtlPTEnable(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index)
{
    return PTL_FAIL;
}
