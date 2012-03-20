/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>
#include <ptl_internal_netIf.h>
#include <ptl_internal_debug.h>

int PtlMDBind(ptl_handle_ni_t   ni_handle,
              const ptl_md_t*         md,
              ptl_handle_md_t*  md_handle)
{
    const ptl_internal_handle_converter_t ct = { md->ct_handle };
    const ptl_internal_handle_converter_t eq = { md->eq_handle };

    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_internal_handle_converter_t mdh = ni;

    PTL_DBG("\n");

    struct PtlAPI* api = GetPtlAPI( ni );

    mdh.s.selector = HANDLE_MD_CODE;

    ptl_md_t tmp_md = *md;
    tmp_md.ct_handle = md->ct_handle == PTL_CT_NONE ? -1 : ct.s.code;
    tmp_md.eq_handle = md->eq_handle == PTL_EQ_NONE ? -1 : eq.s.code;

    int retval = api->PtlMDBind( api, &tmp_md );
    if ( retval < 0 )  return -retval;

    mdh.s.code = retval; 

    *md_handle = mdh.a;
    
    return PTL_OK; 
}

int PtlMDRelease( ptl_handle_md_t md_handle )
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( md_handle, &ni_handle ); 

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t md = { md_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    PTL_DBG("\n");
    int retval = api->PtlMDRelease( api, md.s.code );
    if ( retval < 0 )  return -retval;

    return PTL_OK; 
}
