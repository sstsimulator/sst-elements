/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>
#include <ptl_internal_netIf.h>

int PtlCTAlloc(ptl_handle_ni_t      ni_handle,
               ptl_handle_ct_t *    ct_handle)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_internal_handle_converter_t cth = ni;

    struct PtlAPI* api = GetPtlAPI( ni );

    cth.s.selector = HANDLE_CT_CODE;

    int retval = api->PtlCTAlloc( api );
    if ( retval < 0 )  return -retval;

    cth.s.code = retval;

    *ct_handle = cth.a;

    return PTL_OK;        
}

int PtlCTFree(ptl_handle_ct_t ct_handle)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( ct_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t ct = { ct_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlCTFree( api, ct.s.code );
    if ( retval < 0 )  return -retval;

    return PTL_OK;        
}

int PtlCTGet(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t *   event)
{
    return PTL_FAIL;        
}

int PtlCTWait(ptl_handle_ct_t   ct_handle,
              ptl_size_t        test,
              ptl_ct_event_t *  event)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( ct_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t ct = { ct_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlCTWait( api, ct.s.code, test, event );
    if ( retval < 0 )  return -retval;

    return PTL_OK;        
}

int PtlCTPoll(const ptl_handle_ct_t * ct_handles,
              const ptl_size_t *      tests,
              unsigned int      size,
              ptl_time_t        timeout,
              ptl_ct_event_t *  event,
              unsigned int *    which)
{
    return PTL_FAIL;        
}

int PtlCTSet(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t     test)
{
    return PTL_FAIL;        
}

int PtlCTInc(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t     increment)
{
    return PTL_FAIL;        
}
