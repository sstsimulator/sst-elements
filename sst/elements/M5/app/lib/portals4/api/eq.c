/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
#include <portals4.h>
#include <ptl_internal_netIf.h>

const ptl_handle_eq_t PTL_EQ_NONE = (( 1 << 24 ) - 1); 

int PtlEQAlloc(ptl_handle_ni_t      ni_handle,
               ptl_size_t           count,
               ptl_handle_eq_t *    eq_handle)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_internal_handle_converter_t eqh = ni;

    struct PtlAPI* api = GetPtlAPI( ni );

    eqh.s.selector = HANDLE_EQ_CODE;

    int retval = api->PtlEQAlloc( api, count );
    if ( retval < 0 )  return -retval;

    eqh.s.code = retval;

    *eq_handle = eqh.a;

    return PTL_OK;
}

int PtlEQFree(ptl_handle_eq_t eq_handle)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( eq_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t eq = { eq_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQFree( api, eq.s.code );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlEQGet(ptl_handle_eq_t    eq_handle,
             ptl_event_t *      event)
{
    return PTL_OK;
}

int PtlEQWait(ptl_handle_eq_t   eq_handle,
              ptl_event_t *     event)
{
    ptl_handle_ni_t ni_handle;
    PtlNIHandle( eq_handle, &ni_handle );

    const ptl_internal_handle_converter_t ni = { ni_handle };
    const ptl_internal_handle_converter_t eq = { eq_handle };

    struct PtlAPI* api = GetPtlAPI( ni );

    int retval = api->PtlEQWait( api, eq.s.code, event );
    if ( retval < 0 )  return -retval;

    return PTL_OK;
}

int PtlEQPoll(ptl_handle_eq_t *     eq_handles,
              unsigned int          size,
              ptl_time_t            timeout,
              ptl_event_t *         event,
              int *                 which)
{
    return PTL_OK;
}
